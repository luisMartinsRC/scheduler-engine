package com.sos.scheduler.engine.agent.client

import com.sos.scheduler.engine.tunnel.data.TunnelId
import spray.http.Uri
import spray.http.Uri.Path

/**
 * URIs of the JobScheduler Agent.
 *
 * @author Joacim Zschimmer
 */
final class AgentUris private(agentUri: String) {

  val command: String =
    uriString(Path("command"))

  def overview: String =
    uriString(Path("overview"))

  def fileStatus(filePath: String): String =
    (withPath(Path("fileStatus")) withQuery ("file" → filePath)).toString()

  object tunnelHandler {
    def overview: String =
      withPath(Path("tunnels")).toString()

    def items: String =
      withPath(Path("tunnels") / "item").toString()

    def item(id: TunnelId): String =
      withPath(Path("tunnels") / "item" / id.string).toString()
  }

  def apply(relativeUri: String): String =
    uriString(Path(relativeUri stripPrefix "/"))

  private def uriString(path: Path) = withPath(path).toString

  def withPath(path: Path) = Uri(agentUri) withPath Path(s"/jobscheduler/agent/${path.toString stripPrefix "/"}")

  override def toString = agentUri
}

object AgentUris {
  def apply(agentUri: String) = new AgentUris(agentUri stripSuffix "/")
}
