package com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.expected

import com.sos.scheduler.engine.data.order.OrderState
import com.sos.scheduler.engine.tests.order.monitor.spoolerprocessafter.setting.Setting

abstract sealed class OrderStateExpectation {
  def isValid(o: OrderState): Boolean

  override def toString = getClass.getSimpleName
}

case object InitialState extends OrderStateExpectation {
  def isValid(o: OrderState) = o.asString startsWith Setting.initialOrderStatePrefix
}

case object SuccessState extends OrderStateExpectation {
  def isValid(o: OrderState) = o == new OrderState("SUCCESS")
}

case object ErrorState extends OrderStateExpectation {
  def isValid(o: OrderState) = o == new OrderState("ERROR")
}
