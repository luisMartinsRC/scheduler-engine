package com.sos.scheduler.engine.tests.jira.js498;


import com.google.common.io.Files;
import com.sos.scheduler.engine.data.order.OrderFinishedEvent;
import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.kernel.scheduler.SchedulerException;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.CommandBuilder;
import org.apache.log4j.Logger;
import org.junit.Test;

import java.io.File;
import java.io.IOException;
import java.nio.charset.Charset;
import java.util.HashMap;
import java.util.List;

import static org.junit.Assert.assertTrue;

/**
 * This is a test for scripting with the Rhino engine. The jobchain started executes
 * - a job with all knowing API methods (including the monitors).
 * - a job with a range of api calls for the JobScheduler api.
 *
 * At the end of the test the resultfile will checked for the estimated functions calls.
 *
 * <div class="sos_branding">
 *   <p>(c) 2011 SOS GmbH - Berlin (<a style='color:darkblue' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 *
 * @author ss
 * @version 1.0 - 16.12.2011 13:39:41
 *
 */
public class JS498OrderTest extends SchedulerTest {
    
    private static final Logger logger = Logger.getLogger(JS498OrderTest.class);


    private static final String jobchain = "chain_rhino";
	private final CommandBuilder util = new CommandBuilder();

	private HashMap<String,String> resultMap;

    @Test
    public void test() throws InterruptedException, IOException {
        //controller().activateScheduler("-e","-ignore-process-classes","-log-level=info","-log=" + logFile);
        controller().activateScheduler("-log-level=info");
        File resultFile = prepareResultFile();
        controller().scheduler().executeXml(util.addOrder(jobchain).getCommand());
        controller().waitForTermination(shortTimeout);
        resultMap = getResultMap(resultFile);
        testObjects();
        testFunctions();
    }

    private File prepareResultFile() {
        String resultFileName = scheduler().getConfiguration().localConfigurationDirectory().getAbsolutePath() + "/resultfile.txt";
        File resultFile = new File(resultFileName);
        resultFile.delete();
        return resultFile;
    }

    private HashMap<String,String> getResultMap(File resultFile) throws IOException {
        HashMap<String,String> result = new HashMap<String, String>();
        List<String> lines = Files.readLines(resultFile, Charset.defaultCharset());
        for(String line : lines) {
            String[] arr = line.split("=");
            if (arr.length != 2)
                throw new SchedulerException("line in resultfile '" + resultFile + "' is not valid: " + line);
            result.put(arr[0],arr[1]);
        }
        return result;
    }
	
	@EventHandler
	public void handleOrderEnd(OrderFinishedEvent e) throws IOException {
        controller().terminateScheduler();
	}

    // result of job rhino_objects
    public void testObjects() throws IOException {
        assertObject("spooler.variables.count", "2");
        assertObject("spooler_task.order.job_chain.name","chain_rhino");
        assertObject("spooler_task.params.names","taskparam1;taskparam2");
        assertObject("spooler_job.order_queue.length","1");
        assertObject("spooler_task.order.id","chain_rhino");
    }

    // result of job rhino_objects
    public void testObjectsJob() throws IOException {
        assertObject("spooler.variables.count", "2");
        assertObject("spooler_task.params.names", "taskparam1;taskparam2");
    }

    // result of job rhino_functions
    public void testFunctions() throws IOException {
        assertFunction("spooler_init");
        assertFunction("spooler_open");
        assertFunction("spooler_process");
        assertFunction("spooler_close");
        assertFunction("spooler_on_success");
        assertFunction("spooler_exit");
        assertFunction("spooler_task_before");
        assertFunction("spooler_task_after");
        assertFunction("spooler_process_before");
        assertFunction("spooler_process_after");
    }

	/**
	 * checks if an estimated funtion was called.
	 * @param function
	 */
	private void assertFunction(String function) {
		assertObject(function,"1");		// any funtion should be called exactly one time
	}
	
	/**
	 * checks if an estimated object was given
	 */
	private void assertObject(String varname, String expected) {
		String value = resultMap.get(varname);
		assertTrue(varname + " is not set in scheduler variables", value != null);
		assertTrue(value + " is not valid - " + expected + " expected", value.equals(expected));
	}

}