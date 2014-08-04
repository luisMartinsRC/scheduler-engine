package com.sos.scheduler.engine.tests.jira.js1174

class Job3 extends sos.spooler.Job_impl {

  private val sleeps = 0
  private var processSteps = 3;

  override def spooler_init() = {
    spooler_log.info("")
    spooler_log.info("- 1 ----------------------------- SPOOLER_INIT -----------------------------")
    spooler_log.info("sleep " + sleeps + "s ...")
    Thread.sleep(sleeps*1000)
    true
  }

  override def spooler_open() = {
    spooler_log.info("")
    spooler_log.info("- 2 ----------------------------- SPOOLER_OPEN -----------------------------")
    spooler_log.info("sleep " + sleeps + "s ...")
    Thread.sleep(sleeps*1000)
    true
  }

  override def spooler_process() = {
    spooler_log.info("")
    spooler_log.info("- 3 ----------------------------- SPOOLER_PROCESS -----------------------------")
    spooler_log.info("sleep " + sleeps + "s ...")
    Thread.sleep(sleeps*1000)
    false
  }

  override def spooler_close() = {
    spooler_log.info("")
    spooler_log.info("- 4 ----------------------------- SPOOLER_CLOSE -----------------------------")
    spooler_log.info("sleep " + sleeps + "s ...")
    Thread.sleep(sleeps*1000)
  }

  override def spooler_on_success() = {
    spooler_log.info("")
    spooler_log.info("- 5 ----------------------------- SPOOLER_ON_SUCCESS -----------------------------")
    spooler_log.info("sleep " + sleeps + "s ...")
    Thread.sleep(sleeps*1000)
  }

  override def spooler_on_error() = {
    spooler_log.info("")
    spooler_log.info("- 5 ----------------------------- SPOOLER_ON_ERROR -----------------------------")
    spooler_log.info("sleep " + sleeps + "s ...")
    Thread.sleep(sleeps*1000)
  }

  override def spooler_exit() = {
    spooler_log.info("")
    spooler_log.info("- 6 ----------------------------- SPOOLER_EXIT -----------------------------")
    spooler_log.info("sleep " + sleeps + "s ...")
    Thread.sleep(sleeps*1000)
  }
}
