package com.sos.scheduler.engine.tests.jira.js976

import com.sos.scheduler.engine.test.configuration.{DefaultDatabaseConfiguration, HostwareDatabaseConfiguration, TestConfiguration}
import com.sos.scheduler.engine.test.scalatest.ScalaSchedulerTest
import org.junit.runner.RunWith
import org.scalatest.FunSuite
import org.scalatest.junit.JUnitRunner

/** Test setzt eine laufende MySQL-Datenbank mit wait_timeout=1 voraus.
  *
  * Erforderliche MySQL-Kommandos:
  *   create database scheduler;
  *   create user 'scheduler'@'localhost';
  *   grant ...;
  */
@RunWith(classOf[JUnitRunner])
final class JS976IT extends FunSuite with ScalaSchedulerTest {

  private val withMySQL = System.getProperty("JS-976") != null   // Test mit MySQL nur, wenn die System-Property gesetzt ist

  override lazy val testConfiguration =
    TestConfiguration(
      testClass = getClass,
      database = Some(
        if (withMySQL) HostwareDatabaseConfiguration("jdbc -class=com.mysql.jdbc.Driver -user=scheduler jdbc:mysql://127.0.0.1/scheduler")
        else DefaultDatabaseConfiguration()))

  if (withMySQL)
    test("JS-976") {
      //sleep(3.s)
      scheduler executeXml <job_chain_node.modify job_chain="test" state="100" action="process"/>   // Datenbank hat sich Zustand vom letzten Testlauf gemerkt
      scheduler executeXml <job_chain_node.modify job_chain="test" state="100" action="stop"/>
    }
}
