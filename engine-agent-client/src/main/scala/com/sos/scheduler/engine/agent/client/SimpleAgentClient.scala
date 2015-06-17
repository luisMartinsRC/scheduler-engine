package com.sos.scheduler.engine.agent.client

import akka.actor.ActorSystem
import com.sos.scheduler.engine.common.commandline.CommandLineArguments
import com.sos.scheduler.engine.common.scalautil.Closers.implicits._
import com.sos.scheduler.engine.common.scalautil.Closers.withCloser

/**
 * Simple client for JobScheduler Agent.
 * <p>
 * Should be closed after use, to close all remaining HTTP connections.
 *
 * @author Joacim Zschimmer
 */
final class SimpleAgentClient private(protected[client] val agentUri: String) extends AgentClient with AutoCloseable {

  protected val actorSystem = ActorSystem("SimpleAgentClient")

  def close() = actorSystem.shutdown()
}

object SimpleAgentClient {
  def apply(agentUri: String) = new SimpleAgentClient(agentUri)

  def main(args: Array[String]): Unit = {
    val arguments = CommandLineArguments(args)
    val agentUrl = arguments.string("-url=")
    val command = arguments.string("-command=")
    arguments.requireNoMoreArguments()

    withCloser{ implicit closer ⇒
      val client = SimpleAgentClient(agentUrl).closeWithCloser
      val response = client.executeJsonCommandSynchronously(command)
      println(response)
    }

  }
}
