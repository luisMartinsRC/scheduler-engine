package com.sos.scheduler.engine.tests.jira.js616

import JS616IT._
import com.sos.scheduler.engine.common.time.ScalaJoda._
import com.sos.scheduler.engine.data.order.{OrderKey, OrderFinishedEvent}
import com.sos.scheduler.engine.test.scala.ScalaSchedulerTest
import com.sos.scheduler.engine.test.scala.SchedulerTestImplicits._
import org.joda.time.DateTimeZone
import org.joda.time.Instant.now
import org.joda.time.format.ISODateTimeFormat
import org.junit.Ignore
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.junit.JUnitRunner
import com.sos.scheduler.engine.common.scalautil.AutoClosing.autoClosing

@Ignore("Test versagt, weil Fehler nicht behoben ist")   // TODO Test versagt, weil Fehler nicht behoben ist
@RunWith(classOf[JUnitRunner])
final class JS616IT extends FunSuite with ScalaSchedulerTest {
  test("JS-616 Bug fix: Order Job does not start when having a single start schedule") {
    autoClosing(controller.newEventPipe()) { eventPipe =>
      val t = now + 3.s
      scheduler executeXml
        <job name="test-shell">
          <script language="shell">exit 0</script>
          <run_time>
            <at at={ISODateTimeFormat.dateHourMinuteSecond.withZone(DateTimeZone.getDefault).print(t)}/>
          </run_time>
        </job>
      eventPipe.nextWithCondition[OrderFinishedEvent] { _.orderKey == shellOrderKey }
    }
  }
}


private object JS616IT {
  private val shellOrderKey = OrderKey.of("/test-shell", "1")
}