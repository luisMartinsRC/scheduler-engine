package com.sos.scheduler.engine.data.scheduler

import com.sos.scheduler.engine.data.base.{HasKey, IsString}
import com.sos.scheduler.engine.data.scheduler.VariablePersistentState.{StringValue, _}

final case class VariablePersistentState(name: String, value: IntOrStringValue)
extends HasKey[String] {

  def key = name

  def int = value.asInstanceOf[IntValue].int
  def string = value.asInstanceOf[StringValue].string
}

object VariablePersistentState {
  sealed trait IntOrStringValue

  final case class IntValue(int: Int) extends IntOrStringValue

  final case class StringValue(string: String) extends IntOrStringValue with IsString
}
