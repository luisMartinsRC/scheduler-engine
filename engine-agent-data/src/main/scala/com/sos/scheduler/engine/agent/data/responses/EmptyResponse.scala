package com.sos.scheduler.engine.agent.data.responses

/**
 * @author Joacim Zschimmer
 */
case object EmptyResponse extends Response {

  def toXmlElem = <ok/>
}
