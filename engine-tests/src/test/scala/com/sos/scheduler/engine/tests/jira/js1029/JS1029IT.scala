package com.sos.scheduler.engine.tests.jira.js1029

import com.sos.scheduler.engine.common.system.Files.removeFile
import com.sos.scheduler.engine.common.system.OperatingSystem.isUnix
import com.sos.scheduler.engine.data.event.Event
import com.sos.scheduler.engine.data.job.{JobPath, TaskEndedEvent, TaskId, TaskStartedEvent}
import com.sos.scheduler.engine.eventbus.HotEventHandler
import com.sos.scheduler.engine.kernel.job.Task
import com.sos.scheduler.engine.test.configuration.TestConfiguration
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1029.JS1029IT._
import java.io.File
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JS1029IT extends FunSuite with ScalaSchedulerTest {

  override protected lazy val testConfiguration = TestConfiguration(
    testClass = getClass,
    terminateOnError = false)

  if (isUnix) {  // Nur unter Unix kann eine offene Datei gelöscht werden
    test("JobScheduler should handle removed stdout file") {
      val eventPipe = controller.newEventPipe()
      scheduler executeXml <start_job job={testJobPath.string}/>
      val MyTaskStartedEvent(taskId, stdoutFile) = eventPipe.nextAny[MyTaskStartedEvent]
      try {
        removeFile(stdoutFile)
        killTask(taskId)
        eventPipe.nextKeyed[TaskEndedEvent](taskId)
      }
      finally
        killTask(taskId)
    }
  }

  private def killTask(taskId: TaskId): Unit = {
    scheduler executeXml <kill_task job={testJobPath.string} id={taskId.string} immediately="yes"/>
  }

  @HotEventHandler def handleEvent(e: TaskStartedEvent, task: Task): Unit = {
    controller.eventBus publishCold MyTaskStartedEvent(e.taskId, task.stdoutFile)
  }
}

private object JS1029IT {
  private val testJobPath = JobPath("/test")

  private case class MyTaskStartedEvent(taskId: TaskId, stdoutFile: File) extends Event
}
