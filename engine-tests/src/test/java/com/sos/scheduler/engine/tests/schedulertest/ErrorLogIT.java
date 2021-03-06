package com.sos.scheduler.engine.tests.schedulertest;

import com.sos.scheduler.engine.kernel.log.PrefixLog;
import com.sos.scheduler.engine.test.SchedulerTest;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import static org.junit.Assert.fail;

/** Testet {@link com.sos.scheduler.engine.test.SchedulerTest} */
public final class ErrorLogIT extends SchedulerTest {
    private static final Logger logger = LoggerFactory.getLogger(ErrorLogIT.class);

    @Test public void infoLogLineShouldNotThrowException() {
        controller().activateScheduler();
        instance(PrefixLog.class).info("TEST-INFO");
        controller().close();
    }

    @Test public void errorLogLineShouldThrowException() {
        controller().activateScheduler();
        instance(PrefixLog.class).error("TEST-ERROR");
        boolean ok = false;
        try {
            scheduler().executeCallQueue();
            controller().close();
        } catch (Exception x) {
            logger.debug("Expected: " + x);
            ok = true;
        }
        if (!ok) fail("Missing Exception for error log line");
    }
}
