package com.sos.scheduler.engine.plugins.jetty.log

import java.io.{BufferedReader, Reader}
import javax.ws.rs.core.MediaType._
import com.sun.jersey.api.client.WebResource
import com.sos.scheduler.engine.plugins.jetty.JettyPlugin
import com.sos.scheduler.engine.plugins.jetty.util.JettyPluginTests.{javaContextUri, newAuthResource}
import com.sos.scheduler.engine.test.scala.{CheckedBeforeAll, ScalaSchedulerTest}
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import org.apache.log4j.Logger
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner

//TODO Wechsel der Datei bei Log.start_new_file() und instance_number berücksichtigen
//TODO Datei selbst löschen, wenn Servlet länger lebt als Prefix_log?

@RunWith(classOf[JUnitRunner])
final class LogServletTest extends ScalaSchedulerTest with CheckedBeforeAll {
  import LogServletTest._

  override val configurationPackage = classOf[JettyPlugin].getPackage
  private lazy val objectsResource = newAuthResource(javaContextUri(injector) + "/objects")

  test("Read a task log") {
    startLogThread(objectsResource.path("job.log").queryParam("job", "a"))
    for (i <- 1 to 3) {
      Thread.sleep(1000)
      scheduler.executeXml(<start_job job='/a'/>)
    }
    Thread.sleep(100)
  }

  test("Read a job snapshot log") {
    logReader(objectsResource.path("job/log.snapshot").queryParam("job", "a").accept(TEXT_PLAIN_TYPE).get(classOf[Reader]))
  }

  // Fehler SCHEDULER-291  Error when removing protocol file: ERRNO-13  Permission denied
  ignore("Read an order log") {
    scheduler.executeXml(<order job_chain='/a' id='1'/>)
    startLogThread(objectsResource.path("order.log").queryParam("job_chain", "a").queryParam("order", "1"))
    Thread.sleep(1000)
  }

  private def startLogThread(r: WebResource) {
    new Thread("LogReader " + r) {
      override def run() {
        logReader(r.accept(TEXT_PLAIN_TYPE).get(classOf[Reader]))
        logger.info("---------------------")
      }
    }.start()
  }
}

object LogServletTest {
  private val logger = Logger.getLogger(classOf[LogServletTest])

  private def logReader(reader: Reader) {
    val r = new BufferedReader(reader)
    while (true) {
      val line = r.readLine();
      if (line == null) return
      logger.info(line)
    }
  }
}