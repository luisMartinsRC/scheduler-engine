package com.sos.scheduler.engine.plugins.jetty

import com.sun.jersey.api.client.{Client, ClientResponse}
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import java.net.URI
import java.util.zip.GZIPInputStream
import javax.ws.rs.core.MediaType._
import javax.ws.rs.core.Response.Status.UNAUTHORIZED
import org.apache.log4j.Logger
import org.junit.runner.RunWith
import org.scalatest.junit.JUnitRunner
import org.scalatest.matchers.ShouldMatchers._
import org.joda.time.Duration

import JettyPluginTest._
import com.sun.jersey.api.client.filter.{ClientFilter, GZIPContentEncodingFilter}

@RunWith(classOf[JUnitRunner])
final class CppServletTest extends ScalaSchedulerTest {
  import CppServletTest._

  override protected def checkedBeforeAll(configMap: Map[String, Any]) {
    controller.activateScheduler()
    super.checkedBeforeAll(configMap)
  }

  private val readTimeout = new Duration(30*1000)
  private val uri = contextUri

  for (testConf <- TestConf(newClient(), withGzip = false) :: Nil) {
                   //TestConf(newClient(new GZIPContentEncodingFilter(false)), withGzip = true) :: Nil) {

    val client = testConf.client

    test("Kommando über POST "+testConf) {
      val result = stringFromResponse(client.resource(uri).`type`(TEXT_XML_TYPE).accept(TEXT_XML_TYPE).post(classOf[ClientResponse], "<show_state/>"))
      result should include ("<state")
    }

    test("Kommando über POST ohne Authentifizierung "+testConf) {
      val x = intercept[com.sun.jersey.api.client.UniformInterfaceException] {
        Client.create().resource(contextUri).`type`(TEXT_XML_TYPE).accept(TEXT_XML_TYPE).post(classOf[String], "<show_state/>")
      }
      x.getResponse.getStatus should equal(UNAUTHORIZED.getStatusCode)
    }

    test("Kommando über GET "+testConf) {
      val result = stringFromResponse(client.resource(uri).path("<show_state/>").accept(TEXT_XML_TYPE).get(classOf[ClientResponse]))
      result should include ("<state")
    }

    test("show_log?task=... "+testConf) {
      scheduler.executeXml(<order job_chain={jobChainPath} id={orderId}/>)
      Thread.sleep(500)  //TODO TaskStartedEvent
      val result = stringFromResponse(client.resource(uri).path("show_log").queryParam("task", "1").accept(TEXT_HTML_TYPE).get(classOf[ClientResponse]))
      result should include("SCHEDULER-918  state=closed")
      result should include("SCHEDULER-962")  // "Protocol ends in ..."
      result.trim should endWith("</html>")
    }

    def stringFromResponse(r: ClientResponse) = {
      assert(r.getEntityInputStream.isInstanceOf[GZIPInputStream] == testConf.withGzip,
        (if (testConf.withGzip) "" else " No") +"gzip compressed data expected")
      r.getEntity(classOf[String])
    }
  }

  private def newClient(filters: ClientFilter*) = {
    val result = newAuthentifyingClient(timeout = readTimeout)
    for (f <- filters) result.addFilter(f)
    result
  }
}

object CppServletTest {
  private val logger = Logger.getLogger(classOf[CppServletTest])
  private val jettyPortNumber = 44440
  private val contextUri = new URI("http://localhost:"+ jettyPortNumber + JettyPluginConfiguration.cppPrefixPath)
  private val jobChainPath = "a"
  private val orderId = "1"

  private case class TestConf(client: Client, withGzip: Boolean) {
    override def toString = if (withGzip) " mit gzip" else ""
  }
}