package com.sos.scheduler.engine.tests.jira.js1174

class Job1 extends sos.spooler.Job_impl {

  private val sleeps = 3

  override def spooler_init() = {
    spooler_log.info("")
    spooler_log.info("- 1 ----------------------------- SPOOLER_INIT -----------------------------")
    spooler_log.info(" spooler_task.order().id(): " + spooler_task.order().id())
    spooler_log.info("sleep " + sleeps + "s ...")
    Thread.sleep(sleeps*1000)
    true
  }

  override def spooler_open() = {
    spooler_log.info("")
    spooler_log.info("- 2 ----------------------------- SPOOLER_OPEN -----------------------------")
    spooler_log.info(" spooler_task.order().id(): " + spooler_task.order().id())
    spooler_log.info("sleep " + sleeps + "s ...")
    Thread.sleep(sleeps*1000)
    true
  }

  override def spooler_process() = {
    spooler_log.info("")
    spooler_log.info("- 3 ----------------------------- SPOOLER_PROCESS -----------------------------")
    spooler_log.info(" spooler_task.order().id(): " + spooler_task.order().id())
    spooler_log.info("sleep " + sleeps + "s ...")
    Thread.sleep(sleeps*1000)

    /*
    spooler_log.info("TestJob Start -----------------------------")

    spooler_log.info("sleep...")
    Thread.sleep(1*1000)
    spooler_log.info("sleep...")
    Thread.sleep(1*1000)
    spooler_log.info("sleep...")
    Thread.sleep(1*1000)
    spooler_log.info("sleep...")
    Thread.sleep(1*1000)
    spooler_log.info("sleep...")
    Thread.sleep(1*1000)

    spooler_log.info("TestJob End -------------------------------")
    */

    true
  }

  override def spooler_close() = {
    spooler_log.info("")
    spooler_log.info("- 4 ----------------------------- SPOOLER_CLOSE -----------------------------")
    val t = spooler_task
    spooler_log.info(" spooler_task.id(): " + t.id())
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
    val t = spooler_task
    spooler_log.info(" spooler_task.id(): " + t.id())
    spooler_log.info("sleep " + sleeps + "s ...")
    Thread.sleep(sleeps*1000)
  }
}
