package com.sos.scheduler.engine.tests.jira.js1207

import com.sos.scheduler.engine.data.jobchain.JobChainPath
import com.sos.scheduler.engine.data.order.{OrderFinishedEvent, OrderId, OrderStepEndedEvent, OrderStepStartedEvent}
import com.sos.scheduler.engine.data.xmlcommands.OrderCommand
import com.sos.scheduler.engine.test.SchedulerTestUtils._
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import com.sos.scheduler.engine.tests.jira.js1207.JS1207IT._
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.collection.mutable
import scala.concurrent.duration.DurationInt
import scala.concurrent.{Await, Promise}

/**
 * JS-1198 und JS-1207 max_orders in nested jobchains.
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class JS1207IT extends FreeSpec with ScalaSchedulerTest {

  "JS-1198 limited inner jobchains" in {
    runOrders(UnlimitedOuterJobChainPath, n = 7) shouldEqual Map(
      UnlimitedOuterJobChainPath → 6,
      AInnerJobChainPath → 3,
      BInnerJobChainPath → 2,
      CInnerJobChainPath → 1
    )
  }

  "JS-1207 limited outer and inner jobchains" in {
    runOrders(LimitedOuterJobChainPath, n = 7) shouldEqual Map(
      LimitedOuterJobChainPath → 2,
      AInnerJobChainPath → 2,
      BInnerJobChainPath → 2,
      CInnerJobChainPath → 1
    )
  }

  /**
   * @return Map with maxima of simulateneously running orders per jobchain
   */
  private def runOrders(outerJobchainPath: JobChainPath, n: Int): Map[JobChainPath, Int] = {
    val promise = Promise[Unit]()
    var promisedFinishedOrderCount = n
    val counters = mutable.Map[JobChainPath, Statistic]() ++ (jobchainPaths map { o ⇒ o → new Statistic })
    eventBus.on[OrderStepStartedEvent] { case e ⇒
      counters(outerJobchainPath).onStepStarted()
      counters(e.orderKey.jobChainPath).onStepStarted()
    }
    eventBus.on[OrderStepEndedEvent] { case e ⇒
      counters(e.orderKey.jobChainPath).onStepEnded()
      counters(outerJobchainPath).onStepEnded()
    }
    eventBus.on[OrderFinishedEvent] { case _ ⇒
      promisedFinishedOrderCount -= 1
      if (promisedFinishedOrderCount == 0) promise.success(())
    }
    for (i ← 1 to n) scheduler executeXml OrderCommand(outerJobchainPath orderKey OrderId(s"TEST-ORDER-$i"))
    //v1.8 awaitSuccess(promise.future)
    Await.result(promise.future, 60.seconds)
    for ((jobchainPath, statistics) ← counters) withClue(s"$jobchainPath: ") { statistics.active shouldEqual 0 }
    counters.toMap collect { case (path, statistic) if statistic.maximum != 0 ⇒ path → statistic.maximum }
  }

  private def eventBus = controller.getEventBus
}

private object JS1207IT {
  private val UnlimitedOuterJobChainPath = JobChainPath("/test-outer-unlimited")
  private val LimitedOuterJobChainPath = JobChainPath("/test-outer-limited")
  private val AInnerJobChainPath = JobChainPath("/test-inner-a")
  private val BInnerJobChainPath = JobChainPath("/test-inner-b")
  private val CInnerJobChainPath = JobChainPath("/test-inner-c")
  private val jobchainPaths = List(UnlimitedOuterJobChainPath, LimitedOuterJobChainPath, AInnerJobChainPath, BInnerJobChainPath, CInnerJobChainPath)

  private class Statistic {
    var active: Int = 0
    var maximum: Int = 0

    def onStepStarted(): Unit = {
      active += 1
      if (maximum < active) maximum = active
    }

    def onStepEnded(): Unit = {
      active -= 1
    }
  }
}
