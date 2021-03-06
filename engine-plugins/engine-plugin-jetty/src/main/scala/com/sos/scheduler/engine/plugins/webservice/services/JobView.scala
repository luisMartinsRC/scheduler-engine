package com.sos.scheduler.engine.plugins.webservice.services

import com.sos.scheduler.engine.data.job.JobPath
import java.net.URI
import javax.ws.rs.core.UriBuilder

case class JobView(descriptionUri: URI, configurationUri: URI, logSnapshotUri: URI)

object JobView {
  def apply(path: JobPath, jobUri: URI) = {
    def newUri() = UriBuilder.fromUri(jobUri)

    new JobView(
      descriptionUri = newUri().segment("description").build(),
      configurationUri = newUri().segment("configuration").build(),
      logSnapshotUri = newUri().segment("log").build()
    )
  }
}