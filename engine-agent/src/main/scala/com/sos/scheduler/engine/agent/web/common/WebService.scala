package com.sos.scheduler.engine.agent.web.common

import spray.routing.Route

/**
 * @author Joacim Zschimmer
 */
trait WebService {
  def route: Route
}
