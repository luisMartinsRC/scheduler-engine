package com.sos.scheduler.engine.kernel.processclass.agent

import com.sos.scheduler.engine.client.agent.{ApiProcessConfiguration, HttpRemoteProcess, HttpRemoteProcessStarter}
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.kernel.async.{CppCall, SchedulerThreadCallQueue}
import com.sos.scheduler.engine.kernel.processclass.agent.CppHttpRemoteApiProcessClient._
import com.sos.scheduler.engine.kernel.processclass.common.{FailableCollection, FailableSelector}
import javax.inject.Inject
import org.joda.time.Duration
import scala.concurrent.Future
import scala.util.{Failure, Success}

/**
 * @author Joacim Zschimmer
 */
@ForCpp
final class CppHttpRemoteApiProcessClient private(
  apiProcessConfiguration: ApiProcessConfiguration, schedulerApiTcpPort: Int, warningCall: CppCall, resultCall: CppCall,
  starter: HttpRemoteProcessStarter, callQueue: SchedulerThreadCallQueue)
extends AutoCloseable {

  import callQueue.implicits.executionContext

  private object callbacks extends FailableSelector.Callbacks[Agent, HttpRemoteProcess] {
    def apply(agent: Agent) = {
      val future = starter.startRemoteTask(schedulerApiTcpPort, apiProcessConfiguration, remoteUri = agent.address)
      future map Success.apply recoverWith {
        case e: spray.can.Http.ConnectionAttemptFailedException ⇒
          warningCall.call(e)
          Future { Failure(e) }
      }
    }

    def onDelay(delay: Duration, agent: Agent) = warningCall.call(null)
  }
  private[this] var agentSelector: AgentSelector = null
  private[this] var waitStopped = false
  private var remoteTaskClosed = false

  def close(): Unit = {
    if (agentSelector != null) {
      agentSelector.cancel()
    }
  }

  def startRemoteTask(failableAgents: FailableAgents): Unit = {
    agentSelector = startNewAgentSelector(failableAgents)
  }

  def changeFailableAgents(failableAgents: FailableAgents): Unit = {
    val a = agentSelector
    agentSelector.future onFailure { case t ⇒
      logger.debug(s"$t")
      agentSelector = startNewAgentSelector(failableAgents)
    }
    a.cancel()
  }

  private def startNewAgentSelector(failableAgents: FailableAgents) = {
    val result = new AgentSelector(failableAgents, callbacks, callQueue)
    result.start() onComplete {
      case Success((agent, httpRemoteProcess)) ⇒
        logger.debug(s"Process on agent $agent started: $httpRemoteProcess")
        resultCall.call(Success(()))
      case f @ Failure(throwable) if waitStopped ⇒
        logger.debug(s"Waiting for agent has been stopped: $throwable")
        resultCall.call(f)
      case f @ Failure(_: FailableSelector.CancelledException) ⇒
        logger.debug(s"$f")
      case x @ Failure(throwable) ⇒
        if (result.isCancelled) {
          logger.debug(s"$x")
        } else {
          logger.debug(s"Process on $result could not be started: $throwable", throwable)
          resultCall.call(Failure(throwable))
        }
    }
    result
  }

  def stopTaskWaitingForAgent(): Unit = {
    waitStopped = true
    if (agentSelector != null) {
      agentSelector.cancel()
    }
  }

  @ForCpp
  def closeRemoteTask(kill: Boolean): Unit =
    if (!remoteTaskClosed) {
      killOrCloseRemoteTask(kill = kill, killOnlySignal = None)
    }

  @ForCpp
  def killRemoteTask(killOnlySignal: Int): Boolean = killOrCloseRemoteTask(kill = true, killOnlySignal = Some(killOnlySignal))

  def killOrCloseRemoteTask(kill: Boolean, killOnlySignal: Option[Int]): Boolean = {
    // TODO Race condition? Wenn das <start>-Kommando gerade rübergeschickt wird, startet der Prozess und verbindet sich
    // mit einem com_remote-Port. Der aber ist vielleicht schon von der nächsten Task, die gerade gestartet wird.
    // Wird die dann gestört? Wenigstens ist es wenig wahrscheinlich.
    agentSelector match {
      case null ⇒ false
      case _ ⇒
        agentSelector.cancel()
        agentSelector.future onSuccess {
          case (agent, httpRemoteProcess) ⇒
            killOnlySignal match {
              case Some(signal) ⇒
                httpRemoteProcess.killRemoteTask(unixSignal = signal) onFailure {
                  case t ⇒ logger.error(s"Process '$httpRemoteProcess' on agent '$agentSelector' could not be signalled or closed : $t")
                }
              case None ⇒ httpRemoteProcess.closeRemoteTask(kill = kill) onFailure {
                case t ⇒ logger.error(s"Process '$httpRemoteProcess' on agent '$agentSelector' could not be signalled or closed : $t")
              }
              remoteTaskClosed = true  // Do not execute remote_scheduler.remote_task.close twice!
            }
            // C++ will keine Bestätigung
        }
        true
    }
  }

  @ForCpp
  override def toString = if (agentSelector != null) agentSelector.selectedString else "CppHttpRemoteApiProcessClient (not started)"
}

object CppHttpRemoteApiProcessClient {
  private type AgentSelector = FailableSelector[Agent, HttpRemoteProcess]
  private type FailableAgents = FailableCollection[Agent]

  private val logger = Logger(getClass)

  final class Factory @Inject private(httpRemoteProcessStarter: HttpRemoteProcessStarter, callQueue: SchedulerThreadCallQueue) {
    def apply(apiProcessConfiguration: ApiProcessConfiguration, schedulerApiTcpPort: Int, warningCall: CppCall, resultCall: CppCall) =
      new CppHttpRemoteApiProcessClient(
        apiProcessConfiguration,
        schedulerApiTcpPort,
        warningCall = warningCall,
        resultCall = resultCall,
        httpRemoteProcessStarter,
        callQueue)
  }
}
