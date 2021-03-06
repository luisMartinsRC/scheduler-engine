package com.sos.scheduler.engine.agent.configuration

import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.junit.JUnitRunner

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class AgentConfigurationTest extends FreeSpec {

  "Empty argument list is rejected" in {
    intercept[NoSuchElementException] { AgentConfiguration(Nil) }
  }

  "-http-port=" in {
    assert(AgentConfiguration(List("-http-port=1234")).httpPort == 1234)
    intercept[IllegalArgumentException] { AgentConfiguration(List("-http-port=65536")) }
    intercept[IllegalArgumentException] { AgentConfiguration(httpPort = 65536) }
  }
}
