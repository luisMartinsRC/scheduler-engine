package com.sos.scheduler.engine.tests.jira.js806

import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.common.scalautil.xmls.ScalaXmls.implicits._
import com.sos.scheduler.engine.data.filebased.FileBasedActivatedEvent
import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.log.{InfoLogEvent, LogEvent}
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.data.order._
import com.sos.scheduler.engine.data.xmlcommands.ModifyOrderCommand
import com.sos.scheduler.engine.kernel.variable.VariableSet
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js806.JS806IT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

/** JS-806 Orders in setback can not be changed. */
@RunWith(classOf[JUnitRunner])
final class JS806IT extends FreeSpec with ScalaSchedulerTest {

  private lazy val liveDirectory = controller.environment.liveDirectory
  private lazy val variableSet = instance[VariableSet]

  "Change of order configuration file while order is set back should be effective when order has been reset" in {
    val myOrderKey = SetbackJobChainPath orderKey "A"
    autoClosing(controller.newEventPipe()) { eventPipe ⇒
      variableSet("TestJob.setback") = true.toString
      order(myOrderKey).title shouldEqual OriginalTitle
      scheduler executeXml ModifyOrderCommand.startNow(myOrderKey)
      eventPipe.nextKeyed[OrderSetBackEvent](myOrderKey).state shouldEqual OrderState("200")
      eventPipe.nextKeyed[OrderStepEndedEvent](myOrderKey)

      myOrderKey.file(liveDirectory).xml = <order title={ChangedTitle}><run_time/></order>
      scheduler executeXml ModifyOrderCommand(myOrderKey, action = Some(ModifyOrderCommand.Action.reset))

      eventPipe.nextKeyed[FileBasedActivatedEvent](myOrderKey)
      order(myOrderKey).title shouldEqual ChangedTitle
    }
  }

  "Change of order configuration file while order is suspended should be effective when order has been reset" in {
    val jobChainPath = SuspendingJobChainPath
    val myOrderKey = jobChainPath orderKey "A"
    autoClosing(controller.newEventPipe()) { eventPipe ⇒
      order(myOrderKey).title shouldEqual OriginalTitle
      scheduler executeXml <job_chain_node.modify job_chain={jobChainPath.string} state="200" action="stop"/>
      scheduler executeXml ModifyOrderCommand.startNow(myOrderKey)
      eventPipe.nextKeyed[OrderStepEndedEvent](myOrderKey)
      eventPipe.nextKeyed[OrderStateChangedEvent](myOrderKey).stateChange shouldEqual OrderState("100") -> OrderState("200")
      myOrderKey.file(liveDirectory).xml = <order title={ChangedTitle}><run_time/></order>
      scheduler executeXml ModifyOrderCommand(myOrderKey, suspended = Some(true))
      eventPipe.nextWithCondition[InfoLogEvent] { _.codeOption == Some(MessageCode("SCHEDULER-991")) }    // "Order has been suspended"
      scheduler executeXml ModifyOrderCommand(myOrderKey, action = Some(ModifyOrderCommand.Action.reset))
      eventPipe.nextWithCondition[InfoLogEvent] { _.codeOption == Some(MessageCode("SCHEDULER-992")) }    // "Order ist not longer suspended"
      eventPipe.nextKeyed[FileBasedActivatedEvent](myOrderKey)
      order(myOrderKey).title shouldEqual ChangedTitle
      scheduler executeXml <job_chain_node.modify job_chain={jobChainPath.string} state="200" action="process"/>
    }
  }

  "Change of order configuration file while order is running should be effective when order is finished" in {
    val jobChainPath = StoppingJobChainPath
    val myOrderKey = jobChainPath orderKey "A"
    autoClosing(controller.newEventPipe()) { eventPipe ⇒
      order(myOrderKey).title shouldEqual OriginalTitle
      scheduler executeXml <job_chain_node.modify job_chain={jobChainPath.string} state="200" action="stop"/>
      scheduler executeXml ModifyOrderCommand.startNow(myOrderKey)
      eventPipe.nextKeyed[OrderStepEndedEvent](myOrderKey)
      myOrderKey.file(liveDirectory).xml = <order title={ChangedTitle}><run_time/></order>
      eventPipe.nextWithCondition[LogEvent] { _.codeOption == Some(MessageCode("SCHEDULER-892")) }   // This Standing_order is going to be replaced due to changed configuration file ...
      scheduler executeXml <job_chain_node.modify job_chain={jobChainPath.string} state="200" action="process"/>
      eventPipe.nextKeyed[OrderFinishedEvent](myOrderKey)
      eventPipe.nextKeyed[FileBasedActivatedEvent](myOrderKey)
      order(myOrderKey).title shouldEqual ChangedTitle
    }
  }

  "Change of order configuration file while order is set back should be effective when order is finished" in {
    // Wie vorheriger Test, aber komplizierter mit setback
    val myOrderKey = SetbackJobChainPath orderKey "B"
    autoClosing(controller.newEventPipe()) { eventPipe ⇒
      variableSet("TestJob.setback") = true.toString
      order(myOrderKey).title shouldEqual OriginalTitle
      scheduler executeXml ModifyOrderCommand.startNow(myOrderKey)
      eventPipe.nextKeyed[OrderSetBackEvent](myOrderKey).state shouldEqual OrderState("200")
      eventPipe.nextKeyed[OrderStepEndedEvent](myOrderKey)
      myOrderKey.file(liveDirectory).xml = <order title={ChangedTitle}><run_time/></order>
      variableSet("TestJob.setback") = false.toString
      eventPipe.nextKeyed[OrderFinishedEvent](myOrderKey)
      eventPipe.nextKeyed[FileBasedActivatedEvent](myOrderKey)
      order(myOrderKey).title shouldEqual ChangedTitle
    }
  }
}

private object JS806IT {
  private val SetbackJobChainPath = JobChainPath("/test-setback")
  private val StoppingJobChainPath = JobChainPath("/test-stop")
  private val SuspendingJobChainPath = JobChainPath("/test-suspend")
  private val OriginalTitle = "TITLE"
  private val ChangedTitle = "CHANGED"
}
