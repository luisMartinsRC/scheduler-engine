package com.sos.scheduler.engine.agent.fileordersource

import com.sos.scheduler.engine.agent.data.commands._
import com.sos.scheduler.engine.agent.data.responses.FileOrderSourceContent
import com.sos.scheduler.engine.agent.fileordersource.RequestFileOrderSourceContentExecutorTest._
import com.sos.scheduler.engine.common.scalautil.Closers.implicits._
import com.sos.scheduler.engine.common.scalautil.Closers.withCloser
import com.sos.scheduler.engine.common.scalautil.FileUtils.implicits._
import com.sos.scheduler.engine.common.scalautil.FileUtils.touchAndDeleteWithCloser
import com.sos.scheduler.engine.common.scalautil.Futures._
import com.sos.scheduler.engine.common.time.ScalaTime._
import java.nio.file.Files.{createTempDirectory, delete, setLastModifiedTime}
import java.nio.file.Paths
import java.nio.file.attribute.FileTime
import java.time.ZoneOffset.UTC
import java.time.{Duration, ZonedDateTime}
import org.junit.runner.RunWith
import org.scalatest.FreeSpec
import org.scalatest.concurrent.Futures
import org.scalatest.junit.JUnitRunner
import scala.util.matching.Regex

/**
 * @author Joacim Zschimmer
 */
@RunWith(classOf[JUnitRunner])
final class RequestFileOrderSourceContentExecutorTest extends FreeSpec with Futures {

  "RequestFileOrderSourceContent" in {
    withCloser { implicit closer ⇒
      val dir = createTempDirectory("agent-") withCloser delete
      val expectedResponse = FileOrderSourceContent(List(
        FileOrderSourceContent.Entry((dir / s"$MatchingString-1").toString, Timestamp.toEpochMilli),
        FileOrderSourceContent.Entry((dir / s"$MatchingString-3").toString, (Timestamp + 2.s).toEpochMilli),
        FileOrderSourceContent.Entry((dir / s"$MatchingString-2").toString, (Timestamp + 4.s).toEpochMilli)))
      for (entry ← expectedResponse.files) {
        val path = Paths.get(entry.path)
        touchAndDeleteWithCloser(path)
        setLastModifiedTime(path, FileTime.fromMillis(entry.lastModifiedTime))
      }
      val knownFile = dir / s"$MatchingString-known"
      touchAndDeleteWithCloser(knownFile)
      touchAndDeleteWithCloser(dir / "ignored-file")

      val future = RequestFileOrderSourceContentExecutor.apply(RequestFileOrderSourceContent(
        directory = dir.toString,
        regex = Regex.quote(MatchingString),
        duration = Duration.ofMillis(Long.MaxValue),
        knownFiles = Set(knownFile.toString)))
      val response = awaitResult(future, 10.s)
      assert(response == expectedResponse)
    }
  }

  "RequestFileOrderSourceContent with directory watching" in {
    withCloser { implicit closer ⇒
      val dir = createTempDirectory("agent-") withCloser delete
      val file = dir / s"$MatchingString-TEST"
      val knownFile = dir / s"$MatchingString-known"
      val expectedResponse = FileOrderSourceContent(List(FileOrderSourceContent.Entry(file.toString, Timestamp.toEpochMilli)))
      val future = RequestFileOrderSourceContentExecutor.apply(RequestFileOrderSourceContent(
        directory = dir.toString,
        regex = Regex.quote(MatchingString),
        duration = Duration.ofMillis(Long.MaxValue),
        knownFiles = Set(knownFile.toString)))
      sleep(2.s)
      assert(!future.isCompleted)
      touchAndDeleteWithCloser(knownFile)
      touchAndDeleteWithCloser(dir / "ignored-file")
      sleep(1.s)
      assert(!future.isCompleted)
      touchAndDeleteWithCloser(file)
      setLastModifiedTime(file, FileTime.from(Timestamp))
      awaitResult(future, 5.s)
      val response = awaitResult(future, 10.s)
      assert(response == expectedResponse)
    }
  }
}

private object RequestFileOrderSourceContentExecutorTest{
  private val Timestamp = ZonedDateTime.of(2015, 5, 1, 12, 0, 0, 0, UTC).toInstant
  private val MatchingString = "MATCHING"
}
