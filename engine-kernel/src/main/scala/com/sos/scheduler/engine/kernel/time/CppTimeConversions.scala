package com.sos.scheduler.engine.kernel.time

import com.sos.scheduler.engine.data.configuration.SchedulerDataConstants.eternalCppMillis
import java.time.Instant

object CppTimeConversions {

  def eternalCppMillisToNoneInstant(millis: Long): Option[Instant] = {
    require(millis > 0, s"Timestamp from C++ is negative: $millis")
    Some(millis) filter { _ != eternalCppMillis } map Instant.ofEpochMilli
  }

  def zeroCppMillisToNoneInstant(millis: Long): Option[Instant] =
    Some(millis) filter { _ != 0 } map Instant.ofEpochMilli
}
