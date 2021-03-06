package com.sos.scheduler.engine.agent.configuration.inject

import akka.actor.{ActorRefFactory, ActorSystem}
import com.google.common.io.Closer
import com.google.inject.{Injector, Provides}
import com.sos.scheduler.engine.agent.configuration.AgentConfiguration
import com.sos.scheduler.engine.agent.configuration.Akkas.newActorSystem
import com.sos.scheduler.engine.agent.web.common.WebService
import com.sos.scheduler.engine.common.guice.ScalaAbstractModule
import scala.collection.immutable

/**
 * @author Joacim Zschimmer
 */
final class AgentModule(agentConfiguration: AgentConfiguration) extends ScalaAbstractModule {

  private val closer = Closer.create()

  protected def configure() = {
    bindInstance[Closer](closer)
    bindInstance[AgentConfiguration](agentConfiguration)
    provideSingleton[ActorSystem] { newActorSystem("JobScheduler-Agent")(closer) }
  }

  @Provides
  private def webServices(injector: Injector): immutable.Seq[WebService] =
    agentConfiguration.webServiceClasses map { o ⇒ injector.getInstance(o) }

  @Provides
  private def actorRefFactory(o: ActorSystem): ActorRefFactory = o
}
