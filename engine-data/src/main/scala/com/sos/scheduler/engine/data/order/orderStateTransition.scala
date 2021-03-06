package com.sos.scheduler.engine.data.order

import com.sos.scheduler.engine.data.job.ReturnCode

/**
 * @author Joacim Zschimmer
 */
sealed trait OrderStateTransition

object OrderStateTransition {

  def ofCppInternalValue(internalValue: Long) = internalValue match {
    case Long.MaxValue ⇒
      KeepOrderStateTransition
    case i ⇒
      assert(i.toInt == i, s"OrderStateTransition($i)")
      ProceedingOrderStateTransition(ReturnCode(i.toInt))
  }
}

/**
 * Order proceeds to another jobchain node.
 */
trait ProceedingOrderStateTransition extends OrderStateTransition {
  def returnCode: ReturnCode
}

object ProceedingOrderStateTransition {
  def apply(returnCode: ReturnCode) = returnCode match {
    case ReturnCode.Success ⇒ SuccessOrderStateTransition
    case rc ⇒ ErrorOrderStateTransition(rc)
  }

  def unapply(o: ProceedingOrderStateTransition) = Some(o.returnCode)
}

/**
 * Order proceeds to another jobchain node, used by attribute "next_state".
 */
case object SuccessOrderStateTransition extends ProceedingOrderStateTransition {
  def returnCode = ReturnCode.Success
}

/**
 * Order proceeds to another jobchain node, used by attribute "error_state".
 */
final case class ErrorOrderStateTransition(returnCode: ReturnCode) extends ProceedingOrderStateTransition

object ErrorOrderStateTransition {
  def Standard = ErrorOrderStateTransition(ReturnCode.StandardFailure)
}

/**
 * Order step could not been completed and order stays in same jobchain node.
 */
case object KeepOrderStateTransition extends OrderStateTransition
