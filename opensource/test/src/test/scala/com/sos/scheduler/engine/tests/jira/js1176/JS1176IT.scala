package com.sos.scheduler.engine.tests.jira.js1176

import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.HasCloser.implicits._
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.scalautil.SideEffect._
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.common.utils.FreeTcpPortFinder._
import com.sos.scheduler.engine.data.job.JobPath
import com.sos.scheduler.engine.data.log.WarningLogEvent
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.kernel.async.SchedulerThreadCallQueue
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1176.JS1176IT._
import com.sun.jersey.api.client.Client
import javax.ws.rs.core.MediaType.TEXT_XML_TYPE
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner
import scala.concurrent.ExecutionContext.Implicits.global
import scala.concurrent.{Await, Future, Promise}

/**
 * JS-1176 BUG: If you change a job configuration while the database connection get lost then the JobScheduler can crash.
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1176IT extends FreeSpec with ScalaSchedulerTest {

  private val httpPort = findRandomFreeTcpPort()
  private val databaseServer = new RestartableH2DatabaseServer(testEnvironment.databaseDirectory).registerCloseable
  override protected lazy val testConfiguration = TestConfiguration(getClass,
    mainArguments = List(s"-tcp-port=$httpPort"),
    database = Some(databaseServer.databaseConfiguration))


  override protected def onBeforeSchedulerActivation(): Unit = {
    databaseServer.start()
  }

  private implicit def implicitCallQueue: SchedulerThreadCallQueue = instance[SchedulerThreadCallQueue]

  "JS-1176" in {
    controller.toleratingErrorCodes(Set(MessageCode("SCHEDULER-303"))) {
      autoClosing(controller.newEventPipe()) { eventPipe ⇒
        val waitingPromise = Promise[Unit]()
        controller.getEventBus.onHot[WarningLogEvent] {
          case e if e.codeOption == Some(MessageCode("SCHEDULER-958")) ⇒ // "Waiting 20 seconds before reopening the database"
            waitingPromise.trySuccess(())
        }
        databaseServer.stop()
        testEnvironment.fileFromPath(TestJobPath).append(" ")
        Await.result(waitingPromise.future, ConfigurationDirectoryWatchPeriod + shortTimeout)
        val future = Future {
          Thread.sleep(5000)
          logger.debug("Starting database")
          databaseServer.start()
          logger.debug("Database is running")
        }
        assert(!future.isCompleted)
        logger.debug("Before commands")
        val webClient = Client.create() sideEffect { o ⇒ onClose { o.destroy() } }
        val webResource = webClient.resource(s"http://127.0.0.1:$httpPort")
        webResource.`type`(TEXT_XML_TYPE).post(<check_folders/>.toString())  //<commands><subsystem.show what="statistics"/><show_state subsystems="lock schedule process_class folder" what="folders cluster remote_schedulers schedules"/></commands>
        //scheduler executeXml <commands><show_state max_task_history="0" path="/" subsystems="job folder" what="folders no_subfolders "/><show_state max_task_history="0" path="/test" subsystems="job folder" what="folders no_subfolders "/></commands>
        //scheduler executeXml <show_job job={TestJobPath.string} what="run_time job_params task_history" max_task_history="2"/>
        logger.debug("Commands executed")
        Await.result(future, LostDatabaseRetryTimeout + shortTimeout)
      }
    }
  }
}

private object JS1176IT {
  private val logger = Logger(getClass)
  private val TestJobPath = JobPath("/test")
  private val LostDatabaseRetryTimeout = 60.s
  private val ConfigurationDirectoryWatchPeriod = 60.s
}

