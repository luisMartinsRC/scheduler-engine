package com.sos.scheduler.engine.kernel.processclass.common

import com.sos.scheduler.engine.common.async.FutureCompletion.functionToFutureTimedCall
import com.sos.scheduler.engine.common.async.{CallQueue, TimedCall}
import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.scalautil.ScalaUtils.withToString
import com.sos.scheduler.engine.common.time.ScalaTime._
import com.sos.scheduler.engine.kernel.processclass.common.FailableSelector._
import java.time.{Duration, Instant}
import scala.concurrent.{Future, Promise}
import scala.util.control.NonFatal
import scala.util.{Failure, Success, Try}

/**
 * @author Joacim Zschimmer
 */
class FailableSelector[Failable, Result](
  failables: FailableCollection[Failable],
  callbacks: Callbacks[Failable, Result],
  callQueue: CallQueue) {

  import callQueue.implicits.executionContext

  @volatile private[this] var timedCall: TimedCall[Unit] = null
  @volatile private[this] var selected: Option[Failable] = None
  @volatile private[this] var cancelled = false
  private[this] val promise = Promise[(Failable, Result)]()

  final def start(): Future[(Failable, Result)] = {
    if (timedCall != null) throw new IllegalStateException("Single start only")
    def loopUntilConnected(): Unit = {
      val (delay, failable) = failables.nextDelayAndEntity()
      if (delay > 0.s) {
        callbacks.onDelay(delay, failable)
      }
      val t = functionToFutureTimedCall[Unit](now + delay, withToString(toString) {
        selected = Some(failable)
        catchInFuture { callbacks.apply(failable) } onComplete {
          case Success(Success(result)) ⇒
            failables.clearFailure(failable)
            promise.success(failable → result)
          case x if cancelled ⇒
            logger.debug(s"$x")
            promise.failure(new CancelledException)
          case Success(Failure(throwable)) ⇒ // Tolerated failure
            failables.setFailure(failable, throwable)
            loopUntilConnected()
          case x@Failure(_: TimedCall.CancelledException) ⇒
            logger.debug(s"$x")
            promise.failure(new CancelledException)
          case x@Failure(throwable) ⇒ // Failure lets abort FailableSelector
            failables.setFailure(failable, throwable)
            promise.failure(throwable)
        }
      })
      callQueue.add(t)
      timedCall = t
      t.future onFailure {
        case throwable: TimedCall.CancelledException ⇒ promise.tryFailure(new CancelledException)
        case throwable ⇒ promise.tryFailure(throwable)
      }
    }
    loopUntilConnected()
    promise.future
  }

  final def cancel(): Unit = {
    cancelled = true
    for (o ← Option(timedCall)) {
      callQueue.tryCancel(o)
    }
  }

  final def future = promise.future

  final def isCancelled = cancelled

  final override def toString = s"FailableSelector $selectedString"

  final def selectedString: String = s"${selected getOrElse "(none)"}" + (if (cancelled) " cancelled" else "")

  protected def now = Instant.now()
}

object FailableSelector {
  private val logger = Logger(getClass)

  trait Callbacks[Failable, Result] {
    /**
     * @return Future resulting in<br/>
     *         Success(Success(x)) => a selected,<br/>
     *         Success(Failure(t)) => a inaccessible,<br/>
     *         or Failure(t) => failure, abort
     */
    def apply(o: Failable): Future[Try[Result]]
    def onDelay(delay: Duration, a: Failable): Unit
  }

  private def catchInFuture[Result](body: ⇒ Future[Result]): Future[Result] =
    try body
    catch {
      case NonFatal(t) ⇒ Promise[Result]().failure(t).future
    }

  final class CancelledException private[FailableSelector] extends RuntimeException
}
