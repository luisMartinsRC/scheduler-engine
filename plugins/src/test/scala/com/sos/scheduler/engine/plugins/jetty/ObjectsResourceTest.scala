package com.sos.scheduler.engine.plugins.jetty

import java.io.{BufferedReader, Reader}
import java.net.URI
import javax.ws.rs.core.MediaType._
import com.sos.scheduler.engine.test.scala.{CheckedBeforeAll, ScalaSchedulerTest}
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import com.sun.jersey.api.client.Client
import org.apache.log4j.Logger
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner

/** JS-795: Einbau von Jetty in den JobScheduler. */
@RunWith(classOf[JUnitRunner])
final class ObjectsResourceTest extends ScalaSchedulerTest with CheckedBeforeAll {
  import ObjectsResourceTest._

  private val client = Client.create()

  override protected def checkedBeforeAll(configMap: Map[String, Any]) {
    controller.activateScheduler()
    super.checkedBeforeAll(configMap)
  }

  private val objectsResource = client.resource(contextUri+"/objects")

  test("Read a task log") {
    new Thread("LogReader") {
      start()
      override def run() {
        logReader(objectsResource.path("a.job/log").accept(TEXT_PLAIN_TYPE).get(classOf[Reader]))
        logger.info("---------------------");
      }
    }
    Thread.sleep(2000)
    scheduler.executeXml(<start_job job='/a'/>)
  }

//  test("Read a job log") {
//    logReader(objectsResource.path("a.job/log").accept(TEXT_PLAIN_TYPE).get(classOf[Reader]))
//  }
}

object ObjectsResourceTest {
  private val logger = Logger.getLogger(classOf[ObjectsResourceTest])
  private val jettyPortNumber = 44440
  private val contextUri = new URI("http://localhost:"+ jettyPortNumber + JettyPlugin.contextPath)

  private def logReader(reader: Reader) {
    val r = new BufferedReader(reader)
    while (true) {
      val line = r.readLine();
      if (line == null) return
      logger.info(line)
    }
  }
}