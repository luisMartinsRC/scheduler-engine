package com.sos.scheduler.engine.tests.jira.js795

import javax.ws.rs.core.MediaType
import javax.ws.rs._
import javax.inject.Inject
import scala.collection.JavaConversions._
import com.sos.scheduler.engine.kernel.Scheduler

@Path("objects")
class ObjectsResource @Inject()(scheduler: Scheduler) {
  @GET
  @Path("jobs/{path:.+}/")
  @Produces(Array(MediaType.TEXT_XML))
  def get() =
    <scheduler>
      {scheduler.getJobSubsystem.getVisibleNames map { name => <job name={name}/> }}
    </scheduler>

//  @GET
//  @Path("{path:.+}.{type:[a-z_]+}/log")
//  @Produces(Array(MediaType.TEXT_XML))
//  def get(@DefaultValue("false") @QueryParam("snapshort") snapshot: Boolean) = {
//
//  }
}