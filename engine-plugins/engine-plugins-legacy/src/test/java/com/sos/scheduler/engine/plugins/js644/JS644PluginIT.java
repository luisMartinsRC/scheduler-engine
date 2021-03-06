package com.sos.scheduler.engine.plugins.js644;

import com.sos.scheduler.engine.common.sync.Gate;
import com.sos.scheduler.engine.data.filebased.FileBasedActivatedEvent;
import com.sos.scheduler.engine.data.job.JobPath;
import com.sos.scheduler.engine.data.jobchain.JobChainPath;
import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.job.Job;
import com.sos.scheduler.engine.kernel.job.JobSubsystem;
import com.sos.scheduler.engine.kernel.order.jobchain.JobChain;
import com.sos.scheduler.engine.test.SchedulerTest;
import org.junit.Test;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;

import static com.sos.scheduler.engine.plugins.js644.JS644PluginIT.M.jobActivated;
import static com.sos.scheduler.engine.plugins.js644.JS644PluginIT.M.jobchainActivated;
import static org.hamcrest.MatcherAssert.assertThat;

public final class JS644PluginIT extends SchedulerTest {
    private static final JobPath jobPath = new JobPath("/a");
    private static final JobChainPath jobChainPath = new JobChainPath("/A");

    enum M { jobActivated, jobchainActivated }
    private final Gate<M> gate = new Gate<M>();
    private volatile boolean schedulerIsActive = false;

    @Test public void test() throws Exception {
        controller().activateScheduler();
        schedulerIsActive = true;
        modifyJobFile();
        gate.expect(jobActivated, TestTimeout);
        gate.expect(jobchainActivated, TestTimeout);
    }

    private void modifyJobFile() throws IOException {
        File jobFile = fileBasedFile(instance(JobSubsystem.class).job(jobPath));
        assertThat(jobFile + " does not exist", jobFile.exists());
        try (OutputStream out = new FileOutputStream(jobFile, true)) {
            out.write(' ');
        }
    }

    @HotEventHandler public void handleEvent(FileBasedActivatedEvent e, Job job) throws InterruptedException {
        if (schedulerIsActive && job.path().equals(jobPath))
            gate.put(jobActivated);
    }

    @HotEventHandler public void handleEvent(FileBasedActivatedEvent e, JobChain jobChain) throws InterruptedException {
        if (schedulerIsActive && jobChain.path().equals(jobChainPath))
            gate.put(jobchainActivated);
    }

    private File fileBasedFile(Job o) {
        return new File(controller().environment().liveDirectory(), o.getPath().string() + ".job.xml");
    }
}
