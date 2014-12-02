package com.sos.scheduler.engine.tests.jira.js1003

import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.data.jobchain.{JobChainNodeAction, JobChainPath}
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.data.xmlcommands.ModifyOrderCommand.Action
import com.sos.scheduler.engine.data.xmlcommands.{ModifyOrderCommand, OrderCommand}
import com.sos.scheduler.engine.kernel.order.OrderSubsystem
import com.sos.scheduler.engine.test.EventPipe
import com.sos.scheduler.engine.test.SchedulerTestUtils.order
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1003.JS1003IT._
import org.joda.time.{DateTime, Instant}
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JS1003IT extends FreeSpec with ScalaSchedulerTest {

  private lazy val orderSubsystem = instance[OrderSubsystem]
  private lazy val jobChain = orderSubsystem.jobChain(TestJobChainPath)

  "(prepare job chain)" in {
    jobChain.node(State100).action = JobChainNodeAction.nextState
    jobChain.node(State300).action = JobChainNodeAction.stop
  }

  "Resetting scheduled standing order should should wait" in {
    val orderKey = StandingOrderKey
    resetOrderShouldWait(orderKey, scheduledAt = Some(ScheduledStart)) {
      scheduler executeXml ModifyOrderCommand(orderKey, suspended = Some(false), at = Some(ModifyOrderCommand.NowAt))
    }
  }

  "Reset scheduled non-standing order should wait" in {
    val orderKey = TestJobChainPath orderKey "Scheduled"
    resetOrderShouldWait(orderKey, scheduledAt = Some(ScheduledStart)) {
      val runtime = <run_time><at at="2030-12-31 12:00"/></run_time>
      scheduler executeXml OrderCommand(orderKey, xmlChildren = runtime)
      scheduler executeXml ModifyOrderCommand(orderKey, at = Some(ModifyOrderCommand.NowAt))
    }
  }

  "Reset non-standing order should restart should wait" in {
    val orderKey = TestJobChainPath orderKey "Non-scheduled"
    resetOrderShouldWait(orderKey, scheduledAt = None) {
      scheduler executeXml OrderCommand(orderKey)
    }
  }

  "Reset repeating non-standing order should repeat immediately" in {
    val orderKey = TestJobChainPath orderKey "Repeating"
    autoClosing(controller.newEventPipe()) { implicit eventPipe ⇒
      val runtime = <run_time><period repeat="01:00:00"/></run_time>
      scheduler executeXml OrderCommand(orderKey, xmlChildren = runtime)
      checkBehaviourUntilReset(orderKey)
      eventPipe.nextKeyed[OrderStateChangedEvent](orderKey).previousState shouldBe State200
    }
  }

  private def resetOrderShouldWait(orderKey: OrderKey, scheduledAt: Option[Instant])(startOrder: ⇒ Unit): Unit = {
    autoClosing(controller.newEventPipe()) { implicit eventPipe ⇒
      startOrder
      checkBehaviourUntilReset(orderKey)
      order(orderKey).nextInstantOption shouldEqual scheduledAt
      order(orderKey).state shouldBe State200
      Thread.sleep(2000)
      order(orderKey).state shouldBe State200
    }
  }

  private def checkBehaviourUntilReset(orderKey: OrderKey)(implicit eventPipe: EventPipe): Unit = {
    //Wird übersprungen: eventPipe.nextKeyed[OrderStateChangedEvent](orderKey).previousState shouldBe State100
    eventPipe.nextKeyed[OrderStateChangedEvent](orderKey).previousState shouldBe State200
    def order = orderSubsystem.order(orderKey)
    order.state shouldBe State300
    scheduler executeXml ModifyOrderCommand(orderKey, suspended = Some(true))
    order shouldBe 'suspended
    scheduler executeXml ModifyOrderCommand(orderKey, action = Some(Action.reset))
    order should not be 'suspended
    eventPipe.nextKeyed[OrderStateChangedEvent](orderKey).previousState shouldBe State300
  }
}

private object JS1003IT {
  private val ScheduledStart = new DateTime(2030, 12, 31, 12, 0).toInstant
  private val TestJobChainPath = JobChainPath("/test")
  private val State100 = OrderState("100")
  private val State200 = OrderState("200")
  private val State300 = OrderState("300")
  private val StandingOrderKey = TestJobChainPath orderKey "1"
}
