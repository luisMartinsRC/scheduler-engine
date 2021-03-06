package com.sos.scheduler.engine.common.time

import com.sos.scheduler.engine.common.scalautil.Logger
import com.sos.scheduler.engine.common.time.ScalaTime._
import java.lang.System.nanoTime
import java.time.Duration

/**
 * @author Joacim Zschimmer
 */
final class Stopwatch {
  private val start = nanoTime()

  def elapsedMs: Long = duration.toMillis

  def duration: Duration = Duration.ofNanos(nanoTime() - start)

  override def toString = duration.pretty
}

object Stopwatch {
  private val logger = Logger(getClass)

  def measureTime(n: Int, itemName: String)(body: ⇒ Unit): Result = {
    body  // Warm-up
    val start = nanoTime()
    for (_ ← 1 to n) body
    val duration = Duration.ofNanos(nanoTime() - start)
    val r = Result(n, itemName, duration)
    logger.info(r.toString)
    r
  }

  final case class Result(n: Int, itemName: String, totalDuration: Duration) {
    val singleDuration = totalDuration / n
    override def toString = s"${singleDuration.pretty}/$itemName (${totalDuration.pretty}/$n)"
  }
}
