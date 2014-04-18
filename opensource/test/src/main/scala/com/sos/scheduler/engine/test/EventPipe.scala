package com.sos.scheduler.engine.test

import EventPipe._
import _root_.scala.annotation.tailrec
import _root_.scala.reflect.ClassTag
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.data.event.{KeyedEvent, Event}
import com.sos.scheduler.engine.eventbus._
import com.sos.scheduler.engine.main.event.TerminatedEvent
import java.util.concurrent.LinkedBlockingQueue
import java.util.concurrent.TimeUnit
import org.joda.time.Instant.now
import org.joda.time.{ReadableDuration, Duration}

final class EventPipe(eventBus: EventBus, defaultTimeout: Duration)
extends EventHandlerAnnotated with AutoCloseable {

  private val queue = new LinkedBlockingQueue[Event]

  def close() {
    eventBus unregisterAnnotated this
  }

  @EventHandler def add(e: Event) {
    queue.add(e)
  }

  def nextAny[E <: Event : ClassTag]: E =
    nextEvent[E](defaultTimeout, matchEveryEvent)

  def nextKeyed[E <: KeyedEvent : ClassTag](key: E#Key, timeout: Duration = defaultTimeout): E =
    nextEvent[E](timeout, { _.key == key })

  def nextWithCondition[E <: Event : ClassTag](condition: E ⇒ Boolean): E =
    nextEvent[E](defaultTimeout, condition)

  def nextWithTimeoutAndCondition[E <: Event : ClassTag](timeout: Duration)(condition: E ⇒ Boolean): E =
    nextEvent[E](timeout, condition)

  private def nextEvent[E <: Event](timeout: Duration, predicate: E ⇒ Boolean)(implicit classTag: ClassTag[E]): E =
    nextEvent2(timeout, predicate, classTag.runtimeClass.asInstanceOf[Class[E]])

  private def nextEvent2[E <: Event](timeout: Duration, predicate: E ⇒ Boolean, expectedEventClass: Class[E]): E = {
    val until = now() + timeout

    @tailrec def waitForEvent(): E = {
      def expectedName = expectedEventClass.getSimpleName
      tryPoll(until - now()) match {
        case None ⇒ throw new TimeoutException(s"Expected Event '$expectedName' has not arrived within ${timeout.pretty}")
        case Some(e: TerminatedEvent) ⇒ sys.error(s"Expected event '$expectedName' has not arrived before ${classOf[TerminatedEvent].getName} has arrived")
        case Some(e: Event) if (expectedEventClass isAssignableFrom e.getClass) && evalPredicateIfDefined(predicate, e.asInstanceOf[E]) ⇒ e.asInstanceOf[E]
        case _ ⇒ waitForEvent()
      }
    }

    waitForEvent()
  }

  def poll(t: ReadableDuration): Event =
    tryPoll(t) getOrElse sys.error(s"Event has not arrived within ${t.pretty}")

  def tryPoll(t: ReadableDuration): Option[Event] =
    Option(queue.poll(t.getMillis, TimeUnit.MILLISECONDS))
}

object EventPipe {
  private val matchEveryEvent = (e: Event) ⇒ true
  private val matchNoEvent = (e: Event) ⇒ false

  class TimeoutException(override val getMessage: String) extends RuntimeException

  private def evalPredicateIfDefined[E <: Event](predicate: E ⇒ Boolean, event: E) =
    predicate match {
      case pf: PartialFunction[E, Boolean] ⇒ pf.applyOrElse(event, matchNoEvent)
      case _ ⇒ predicate(event)
    }
}
