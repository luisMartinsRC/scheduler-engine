//package com.sos.scheduler.engine.tests.order.test1
//
//import com.sos.scheduler.engine.kernel.order.OrderState
//import com.sos.scheduler.engine.kernel.order.OrderStateChangedEvent
//import com.sos.scheduler.engine.kernel.util.Time
//import org.junit._
//import com.sos.scheduler.engine.test.{ScalaEventThread, ScalaSchedulerTest}
//
//// Wie die Java-Klasse Example2Text. scheduler.xml siehe Java-Verzeichnis, dasselbe Paket.
//class Example2ScalaTest extends ScalaSchedulerTest {
//    private val eventTimeout = Time.of(3)
//
//    @Test def test() {
//        controller.subscribeEvents(new MyEventThread)
//        controller.runScheduler(shortTimeout)
//    }
//
//    class MyEventThread extends ScalaEventThread(controller) {
//        filter { case e: OrderStateChangedEvent => e.getKey.getId == "id.1" }
//
//        @Override protected def runEventThread() {
//            expect(new OrderState("state.2"))
//            expect(new OrderState("state.end"))
//            Thread.sleep(1000)
//        }
//
//        def expect(state: OrderState) {
//            expectEvent(eventTimeout) { case e: OrderStateChangedEvent => e.getOrder.getState == state }
//        }
//    }
//}