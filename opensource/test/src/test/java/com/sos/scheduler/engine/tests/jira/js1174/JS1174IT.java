package com.sos.scheduler.engine.tests.jira.js1174;

import com.sos.scheduler.engine.data.job.TaskEndedEvent;
import com.sos.scheduler.engine.data.order.OrderFinishedEvent;
import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.CommandBuilder;
import org.joda.time.Duration;
import org.junit.Test;

public final class JS1174IT extends SchedulerTest {

    private int countEndedTasks = 0;
//    private int countOrderFinished = 0;
    private static final Duration timeout = Duration.standardSeconds(60);

    @Test
    public void test() {
        CommandBuilder util = new CommandBuilder();
        controller().activateScheduler();
//        controller().scheduler().executeXml("<add_order job_chain='chain1' id='ID1'/>");
//        controller().scheduler().executeXml("<add_order job_chain='chain1' id='ID2'/>");
        controller().scheduler().executeXml(util.startJobImmediately("job2").getCommand());

//        controller().waitForTermination(shortTimeout);
        controller().waitForTermination(timeout);
    }

    @EventHandler
    public void handleEvent(TaskEndedEvent e) {
        countEndedTasks++;
        if (countEndedTasks == 1) {
            controller().terminateScheduler();
        }
    }

//    @EventHandler
//    public void handleEvent(OrderFinishedEvent e) {
//        countOrderFinished++;
//        if (countOrderFinished == 2) {
//            controller().terminateScheduler();
//        }
//    }
}
