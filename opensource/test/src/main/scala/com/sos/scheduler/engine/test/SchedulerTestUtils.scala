package com.sos.scheduler.engine.test

import com.sos.scheduler.engine.data.folder.{JobChainPath, JobPath}
import com.sos.scheduler.engine.data.job.TaskClosedEvent
import com.sos.scheduler.engine.data.order.OrderKey
import com.sos.scheduler.engine.kernel.job.{Job, JobSubsystem}
import com.sos.scheduler.engine.kernel.order.jobchain.JobChain
import com.sos.scheduler.engine.kernel.order.{OrderSubsystem, Order}
import com.sos.scheduler.engine.kernel.scheduler.HasInjector
import com.sos.scheduler.engine.test.EventBusTestFutures.implicits._
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._

object SchedulerTestUtils {

  def job(jobPath: JobPath)(implicit hasInjector: HasInjector): Job =
    hasInjector.injector.getInstance(classOf[JobSubsystem]).job(jobPath)

  def jobChain(jobChainPath: JobChainPath)(implicit hasInjector: HasInjector): JobChain =
    hasInjector.injector.getInstance(classOf[OrderSubsystem]).jobChain(jobChainPath)

  def order(key: OrderKey)(implicit hasInjector: HasInjector): Order =
    hasInjector.injector.getInstance(classOf[OrderSubsystem]).order(key)

  def runJobAndWaitForEnd(jobPath: JobPath)(implicit controller: TestSchedulerController, timeout: TestTimeout) {
    controller.getEventBus.awaitingEvent[TaskClosedEvent](predicate = _.jobPath == jobPath) {
      controller.scheduler executeXml <start_job job={jobPath.string}/>
    }
  }
}