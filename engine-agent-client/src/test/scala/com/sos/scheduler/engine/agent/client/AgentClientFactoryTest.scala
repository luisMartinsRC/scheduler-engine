package com.sos.scheduler.engine.agent.client

import akka.actor.ActorSystem
import akka.util.Timeout
import com.google.common.io.Closer
import com.google.common.io.Files._
import com.google.inject.Guice
import com.sos.scheduler.engine.agent.Agent
import com.sos.scheduler.engine.agent.client.AgentClientFactory._
import com.sos.scheduler.engine.agent.data.commands.RequestFileOrderSourceContent
import com.sos.scheduler.engine.agent.data.responses.FileOrderSourceContent
import com.sos.scheduler.engine.common.guice.GuiceImplicits.RichInjector
import com.sos.scheduler.engine.common.guice.ScalaAbstractModule
import com.sos.scheduler.engine.common.scalautil.Closers.implicits._
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.FileUtils.touchAndDeleteWithCloser
import com.sos.scheduler.engine.common.time.ScalaTime._
import java.nio.file.Files._
import java.nio.file.attribute.FileTime
import java.time.Instant
import java.util.concurrent.TimeUnit.MILLISECONDS
import org.junit.runner.RunWith
import org.scalatest.concurrent.ScalaFutures
import org.scalatest.junit.JUnitRunner
import org.scalatest.{BeforeAndAfterAll, FreeSpec}
import scala.concurrent.Await
import scala.concurrent.duration._
import scala.util.matching.Regex

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class AgentClientFactoryTest extends FreeSpec with ScalaFutures with BeforeAndAfterAll {

  override implicit val patienceConfig = PatienceConfig(timeout = 10.seconds)
  private implicit val closer = Closer.create()
  private lazy val agent = Agent.forTest().closeWithCloser
  private val injector = Guice.createInjector(new ScalaAbstractModule {
    def configure() = bindInstance[ActorSystem] { ActorSystem() }
  })
  private lazy val client = injector.instance[AgentClientFactory].apply(agentUri = agent.localUri)

  override def beforeAll() = Await.result(agent.start(), 10.seconds)

  override def afterAll() = {
    closer.close()
    super.afterAll()
  }

  "commandMillisToRequestTimeout" in {
    val upperBound = RequestFileOrderSourceContent.MaxDuration  // The upper bound depends on Akka tick length (Int.MaxValue ticks, a tick can be as short as 1ms)
    for (millis ← List[Long](0, 1, upperBound.toMillis)) {
      assert(commandMillisToRequestTimeout(millis) == Timeout(RequestTimeout.toMillis + millis, MILLISECONDS))
    }
  }

  "readFiles" in {
    val dir = createTempDirectory("agent-") withCloser delete
    val knownFile = dir / "x-known"
    val instant = Instant.parse("2015-01-01T12:00:00Z")
    val expectedFiles = List(
      (dir / "x-1", instant),
      (dir / "prefix-x-3", instant + 2.s),
      (dir / "x-2", instant + 4.s))
    val expectedResult = FileOrderSourceContent(expectedFiles map { case (file, t) ⇒ FileOrderSourceContent.Entry(file.toString, t.toEpochMilli) })
    val ignoredFiles = List(
      (knownFile, instant),
      (dir / "ignore-4", instant))
    for ((file, t) ← expectedFiles ++ ignoredFiles) {
      touchAndDeleteWithCloser(file)
      setLastModifiedTime(file, FileTime.from(t))
    }
    val regex = "x-"
    assert(new Regex(regex).findFirstIn(knownFile.toString).isDefined)
    val command = RequestFileOrderSourceContent(
      directory = dir.toString,
      regex = regex,
      durationMillis = RequestFileOrderSourceContent.MaxDuration.toMillis,
      knownFiles = Set(knownFile.toString))
    whenReady(client.executeCommand(command)) { o ⇒
      assert(o == expectedResult)
    }
  }

  "fileExists" in {
    val file = createTempFile("sos", ".tmp")
    closer.onClose { deleteIfExists(file) }
    for (i ← 1 to 3) {   // Check no-cache
      touch(file)
      whenReady(client.fileExists(file.toString)) { exists ⇒ assert(exists) }
      delete(file)
      whenReady(client.fileExists(file.toString)) { exists ⇒ assert(!exists) }
    }
  }

  "Invalid URI" in {
    val client = injector.instance[AgentClientFactory].apply(agentUri = "INVALID-URI")
    val e = intercept[RuntimeException] { Await.result(client.fileExists("FILE"), 10.seconds) }
    assert(e.toString contains "INVALID-URI")
  }
}
