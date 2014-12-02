package com.sos.scheduler.engine.tests.scheduler.job.login

import com.sos.scheduler.engine.data.job.{JobPath, TaskEndedEvent}
import com.sos.scheduler.engine.kernel.variable.VariableSet
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import com.sos.scheduler.engine.tests.scheduler.job.login.JobLoginIT._
import java.util.regex.Pattern
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.Matchers._
import org.scalatest.junit.JUnitRunner
import scala.util.matching.Regex

@RunWith(classOf[JUnitRunner])
final class JobLoginIT extends FunSuite with ScalaSchedulerTest {

  private val noLoginJobPath = JobPath("/testNoLogin")
  private val loginJobPath = JobPath("/testLogin")

  test("Job without <login>") {
    val expectedPropertyMap = (propertyNames map { name => name -> System.getProperty(name) }).toMap
    runJob(noLoginJobPath) should equal (expectedPropertyMap)
  }

  ignore("Job with <login>") {
    val properties = runJob(loginJobPath)
    properties("user.name") should equal ("-user name-")
    properties("user.dir") should equal ("-working directory-")
    properties("user.home") should equal ("-home directory-")
  }

  private def runJob(jobPath: JobPath) = {
    val eventPipe = controller.newEventPipe()
    startJob(jobPath)
    eventPipe.nextWithCondition[TaskEndedEvent] { _.jobPath == jobPath }
    jobPropertyMap(jobPath)
  }

  private def startJob(jobPath: JobPath): Unit = {
    scheduler executeXml <start_job job={jobPath.string}/>
  }

  private def jobPropertyMap(jobPath: JobPath) = {
    val namePrefix = jobPath.name +"."
    val NamePattern = new Regex(Pattern.quote(namePrefix) +"(.*)")
    val result = instance[VariableSet] collect { case (NamePattern(propertyName), v) => propertyName -> v }
    result.toMap
  }
}

object JobLoginIT {
  val propertyNames = List("user.name", "user.home", "user.dir")
}
