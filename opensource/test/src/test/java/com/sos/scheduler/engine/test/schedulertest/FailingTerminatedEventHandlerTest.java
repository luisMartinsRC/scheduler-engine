package com.sos.scheduler.engine.test.schedulertest;

import org.junit.Test;

import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.main.event.TerminatedEvent;
import com.sos.scheduler.engine.test.SchedulerTest;

/** Testet {@link com.sos.scheduler.engine.test.SchedulerTest} */
public final class FailingTerminatedEventHandlerTest extends SchedulerTest {
    @Test(expected=MyError.class) public void quickTest() {
        controller().startScheduler();
        controller().close();
    }

    @HotEventHandler public void handleEvent(TerminatedEvent e) {
        throw new MyError();
    }
}