/*
 * <div class="sos_branding">
 *   <p>(c) 2012 SOS GmbH - Berlin (<a style='color:silver' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 */

package com.sos.scheduler.engine.tests.jira.js498;


import com.sos.scheduler.engine.eventbus.EventHandler;
import com.sos.scheduler.engine.eventbus.HotEventHandler;
import com.sos.scheduler.engine.kernel.job.events.TaskEndedEvent;
import com.sos.scheduler.engine.kernel.order.OrderFinishedEvent;
import com.sos.scheduler.engine.kernel.variable.VariableSet;
import com.sos.scheduler.engine.test.SchedulerTest;
import com.sos.scheduler.engine.test.util.CommandBuilder;
import com.sos.scheduler.engine.test.util.FileUtils;
import org.junit.Ignore;
import org.junit.Test;

import java.io.File;
import java.io.IOException;

import static org.junit.Assert.assertTrue;

/**
 * This is a test for scripting with the Rhino engine. The jobchain chain_scripting executes a job with all knwowing API
 * methods (including the monitors). At the end of the test the resultfile will checked for the estimated functions calls.
 * 
 * <div class="sos_branding">
 *   <p>(c) 2011 SOS GmbH - Berlin (<a style='color:darkblue' href='http://www.sos-berlin.com'>http://www.sos-berlin.com</a>)</p>
 * </div>
 *  
 * @author ss
 * @version 1.0 - 16.12.2011 13:39:41
 *
 */
public class JS498AdapterTest extends SchedulerTest {

	private final String jobName = "script_adapter";
	private final CommandBuilder util = new CommandBuilder();
	private VariableSet resultSet;

	@Ignore
	public void testFunctions() throws InterruptedException, IOException {
        File resultFile = FileUtils.getResourceFile(this.getClass(), "scheduler.log");
		controller().activateScheduler("-e","-log-level=info","-log=" + resultFile);
		// controller().scheduler().executeXml( util.startJobImmediately(jobName).getCommand() );
		controller().scheduler().executeXml( util.addOrder(jobName).addParam("script_language","dummy").getCommand() );
		controller().waitForTermination(shortTimeout);
//		testAssertions();
	}
	
	@EventHandler
	public void handleOrderEnd(TaskEndedEvent e) throws IOException {
//		resultSet = scheduler().getVariables();
		controller().terminateScheduler();
	}
	
	public void testAssertions() throws IOException {
		assertObject("spooler.variables.count","2");
		assertObject("spooler_task.order.job_chain.name","chain_rhino");
		assertObject("spooler_task.params.names","taskparam1;taskparam2");
		assertObject("spooler_job.order_queue.length","1");
		assertObject("spooler_task.order.id","chain_rhino");
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
		String value = resultSet.get(varname);
		assertTrue(varname + " is not set in scheduler variables", value != null);
		assertTrue(value + " is not valid - " + expected + " expected", value.equals(expected));
	}

}