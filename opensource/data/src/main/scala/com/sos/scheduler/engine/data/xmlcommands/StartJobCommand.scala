package com.sos.scheduler.engine.data.xmlcommands

import com.sos.scheduler.engine.data.folder.JobPath

final case class StartJobCommand(jobPath: JobPath) extends XmlCommand {
  def xmlElem = <start_job job={jobPath.string}/>
}