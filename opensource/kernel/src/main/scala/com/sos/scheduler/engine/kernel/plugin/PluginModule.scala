package com.sos.scheduler.engine.kernel.plugin

import com.google.inject.TypeLiteral
import com.sos.scheduler.engine.kernel.configuration.ScalaAbstractModule
import scala.collection.immutable

final class PluginModule(pluginConfigurations: immutable.Seq[PluginConfiguration])
extends ScalaAbstractModule  {

  def configure() {
    bind(new TypeLiteral[immutable.Seq[PluginConfiguration]] {}) toInstance pluginConfigurations
    pluginConfigurations flatMap { _.guiceModuleOption } foreach install
    bindInstance(pluginConfigurations)
  }
}

object PluginModule {
  def apply(xml: String) = new PluginModule(PluginConfiguration.readXml(xml))
}