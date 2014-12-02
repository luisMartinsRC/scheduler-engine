package com.sos.scheduler.engine.tests.jira.js1082

import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing
import com.sos.scheduler.engine.data.job.{JobPath, TaskId}
import com.sos.scheduler.engine.data.message.MessageCode
import com.sos.scheduler.engine.kernel.persistence.hibernate.RichEntityManager.toRichEntityManager
import com.sos.scheduler.engine.kernel.scheduler.SchedulerConstants.taskIdOffset
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException
import com.sos.scheduler.engine.persistence.entities.TaskHistoryEntity
import com.sos.scheduler.engine.test.SchedulerTestUtils.runJobAndWaitForEnd
import com.sos.scheduler.engine.test.TestSchedulerController
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.jira.js1082.JS1082IT._
import javax.persistence.EntityManagerFactory
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner

@RunWith(classOf[JUnitRunner])
final class JS1082IT extends FreeSpec with ScalaSchedulerTest {

  "While waiting for reconnect with database, JobScheduler should reject command start-job" in {
    runJobAndWaitForEnd(testJobPath) shouldEqual TaskId(taskIdOffset)
    withDatabaseError {
      interceptDatabaseError {
        runJobAndWaitForEnd(testJobPath)
      }
    }
    runJobAndWaitForEnd(testJobPath) shouldEqual TaskId(taskIdOffset + 1)
    autoClosing(instance[EntityManagerFactory].createEntityManager()) { entityManager ⇒
      val taskHistoryEntities = entityManager.fetchSeq[TaskHistoryEntity]("select t from TaskHistoryEntity t order by t.id")
      taskHistoryEntities map { o ⇒ o.id -> o.jobPath } shouldEqual List(
        taskIdOffset - 1 -> "(Spooler)",
        taskIdOffset -> testJobPath.withoutStartingSlash,
        taskIdOffset + 1 -> testJobPath.withoutStartingSlash)
    }
  }

  private def withDatabaseError(body: ⇒ Unit)(implicit controller: TestSchedulerController): Unit = {
    autoClosing(controller.newJDBCConnection()) { connection ⇒
      val statement = connection.createStatement()
      val renamedTableName = s"$variablesTableName-renamed"
      statement.execute(s"""alter table "$variablesTableName" rename to "$renamedTableName" """)
      statement.execute("""commit""")
      body
      statement.execute(s"""alter table "$renamedTableName" rename to "$variablesTableName" """)
      statement.execute("""commit""")
    }
  }

  private def interceptDatabaseError(body: ⇒ Unit): Unit = {
    controller.toleratingErrorCodes(Set(MessageCode("SCHEDULER-303"), MessageCode("SCHEDULER-304"))) {
      val e = intercept[SchedulerException](body)
      s"${e.getMessage} " should startWith ("SCHEDULER-304 ")
      e.getMessage should include (variablesTableName)
    }
  }
}

private object JS1082IT {
  private val variablesTableName = "SCHEDULER_VARIABLES"
  private val testJobPath = JobPath("/test")
}
