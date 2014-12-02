package com.sos.scheduler.engine.tests.scheduler.job.manyjobs

import com.sos.scheduler.engine.common.time.ScalaJoda.{DurationRichInt, sleep}
import com.sos.scheduler.engine.common.time.Stopwatch
import com.sos.scheduler.engine.data.filebased.FileBasedActivatedEvent
import com.sos.scheduler.engine.data.job.{JobPath, TaskStartedEvent}
import com.sos.scheduler.engine.eventbus.EventHandler
import com.sos.scheduler.engine.test.binary.CppBinariesDebugMode
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.test.util.time.WaitForCondition.waitForCondition
import com.sos.scheduler.engine.tests.scheduler.job.manyjobs.ManyJobsIT._
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.junit.JUnitRunner
import scala.collection.mutable
import scala.math._
import scala.sys.error
import scala.util.Try

@RunWith(classOf[JUnitRunner])
final class ManyJobsIT extends FunSuite with ScalaSchedulerTest {

  override lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    binariesDebugMode = if (n > 0) Some(CppBinariesDebugMode.release) else None)

  private var activatedJobCount = 0
  private var taskCount = 0
  private lazy val jobs = 1 to n map JobDefinition
  private lazy val jobStatistics = mutable.HashMap[JobPath, JobStatistics]() ++ (jobs map { j => j.path -> JobStatistics() })

  if (n > 0) {
    test(s"Adding $n jobs") {
      val stopwatch = new Stopwatch
      for (j <- jobs) scheduler executeXml j.xmlElem
      waitForCondition(timeout=n*100.ms, step=100.ms) { activatedJobCount == n }
      System.err.println(s"${jobs.size} added in $stopwatch (${1000f * jobs.size / stopwatch.elapsedMs} jobs/s)")
    }

    test(s"Running 1 task/job/s in ${duration.getMillis}ms") {
      if (n == 0) pending
      else {
        scheduler executeXmls (
              jobs map { o =>
                <modify_job job={o.path.string} cmd="enable"/>
                <modify_job job={o.path.string} cmd="unstop"/>
              })
        sleep(duration)
        controller.eventBus.dispatchEvents()
        System.err.println(s"$taskCount tasks in ${duration.getStandardSeconds}s, ${1000f * taskCount / duration.getMillis} tasks/s")
      }
    }
  }

  @EventHandler def handle(e: FileBasedActivatedEvent): Unit = {
    Some(e.typedPath) collect { case p: JobPath if jobStatistics.keySet contains p => activatedJobCount += 1 }
  }

  @EventHandler def handle(e: TaskStartedEvent): Unit = {
    taskCount += 1
    jobStatistics(e.jobPath).taskCount += 1
  }
}

private object ManyJobsIT {
  private val propertyName = "ManyJobsIT"
  private val minimumTasks = 10
  private val duration = 30.s

  private lazy val n = {
    val v = System.getProperty(propertyName, "0")
    Try(v.toInt) getOrElse error(s"System property $propertyName=$v not a number") match {
      case 0 => 0
      case o => max(o, minimumTasks)
    }
  }
  private val tasksPerSecond = 40.0

  case class JobDefinition(name: Int) {
    def path = JobPath(s"/a-$name")

    def xmlElem =
      <job name={path.name} enabled="false">
        <script language="shell">exit 0</script>
        <run_time repeat="1"/>
      </job>
  }

  case class JobStatistics(var taskCount: Int = 0)
}

