package com.sos.scheduler.engine.client.agent

import akka.actor.ActorSystem
import com.sos.scheduler.engine.client.command.SchedulerCommandClient
import com.sos.scheduler.engine.tunnel.client.{TcpToHttpBridge, WebTunnelClient}
import java.net.InetSocketAddress

/**
 * A remote process started by [[HttpRemoteProcessStarter]], with API calls tunnelled via HTTP and Agent.
 *
 * @author Joacim Zschimmer
 */
private[agent] final class TunnelledHttpRemoteProcess(
  actorSystem: ActorSystem,
  protected val classicClient: SchedulerCommandClient,
  protected val processDescriptor: ProcessDescriptor,
  schedulerApiTcpPort: Int,
  tunnelClient: WebTunnelClient)
extends HttpRemoteProcess {

  protected def executionContext = actorSystem.dispatcher

  private val tcpHttpBridge = new TcpToHttpBridge(
    actorSystem,
    connectTo = new InetSocketAddress("127.0.0.1", schedulerApiTcpPort),
    tunnelToken = processDescriptor.tunnelTokenOption getOrElse { throw new NoSuchElementException(s"Missing TunnelToken") },
    tunnelClient = tunnelClient)

  def start() = tcpHttpBridge.start()

  def close() = tcpHttpBridge.close()
}
