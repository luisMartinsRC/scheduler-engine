package com.sos.scheduler.engine.tests.jira.js1399

import com.google.common.io.Files.touch
import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.{OrderFinishedEvent, OrderTouchedEvent}
import com.sos.scheduler.engine.data.processclass.ProcessClassPath
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits.RichEventBus
import com.sos.scheduler.engine.test.ImplicitTimeout
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.agent.AgentWithSchedulerTest
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1399.JS1399IT._
import java.net.{InetSocketAddress, ServerSocket}
import java.nio.file.Path
import java.time.Duration
import java.util.concurrent.TimeoutException
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1399IT extends FreeSpec with ScalaSchedulerTest with AgentWithSchedulerTest {

  private lazy val directory = testEnvironment.newFileOrderSourceDirectory()
  private lazy val matchingFile = directory / "X-MATCHING-FILE"
  private lazy val orderKey = TestJobChainPath orderKey matchingFile.toString

  "file_order_source waits for process_class" in {
    writeConfigurationFile(TestJobChainPath, newJobChainElem(directory, agentUri, 1.s))
    eventBus.awaitingKeyedEvent[OrderFinishedEvent](orderKey) {
      intercept[TimeoutException] {
        implicit val implicitTimeout = ImplicitTimeout(3.s)
        eventBus.awaitingKeyedEvent[OrderTouchedEvent](orderKey) {
          touch(matchingFile)
        }
      }
      writeConfigurationFile(TestProcessClassPath, <process_class remote_scheduler={agentUri}/>)
    }
  }

  "file_order_source stops when process_class is to be removed, and restarts when process_class is added" in {
    deleteConfigurationFile(TestProcessClassPath)
    eventBus.awaitingKeyedEvent[OrderFinishedEvent](orderKey) {
      intercept[TimeoutException] {
        implicit val implicitTimeout = ImplicitTimeout(3.s)
        eventBus.awaitingKeyedEvent[OrderTouchedEvent](orderKey) {
          touch(matchingFile)
        }
      }
      writeConfigurationFile(TestProcessClassPath, <process_class remote_scheduler={agentUri}/>)
    }
  }

  "file_order_source handles change of process_class" in {
    controller.toleratingErrorLogEvent(_.message contains "spray.can.Http$ConnectionException") {
      autoClosing(new ServerSocket()) { socket ⇒
        socket.bind(new InetSocketAddress("127.0.0.1", 0))
        val deadPort = socket.getLocalPort
        sleep(1.s)  // Wait a second to get a new file timestamp
        writeConfigurationFile(TestProcessClassPath, <process_class remote_scheduler={s"http://127.0.0.1:$deadPort"}/>)
        intercept[TimeoutException] {
          implicit val implicitTimeout = ImplicitTimeout(3.s)
          eventBus.awaitingKeyedEvent[OrderTouchedEvent](orderKey) {
            touch(matchingFile)
          }
        }
      }
      deleteConfigurationFile(TestJobChainPath)  // Close to get HTTP error while we tolerate it
    }
  }

  private def newJobChainElem(directory: Path, agentUri: String, repeat: Duration): xml.Elem =
    <job_chain file_watching_process_class={TestProcessClassPath.withoutStartingSlash}>
      <file_order_source directory={directory.toString} regex="MATCHING-" repeat={repeat.getSeconds.toString}/>
      <job_chain_node state="100" job="/test"/>
      <file_order_sink state="END" remove="true"/>
    </job_chain>
}

private object JS1399IT {
  private val TestJobChainPath = JobChainPath("/test")
  private val TestProcessClassPath = ProcessClassPath("/test")
}
