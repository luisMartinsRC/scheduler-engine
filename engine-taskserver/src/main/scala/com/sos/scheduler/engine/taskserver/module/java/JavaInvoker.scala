package com.sos.scheduler.engine.taskserver.module.java

import com.sos.scheduler.engine.minicom.idispatch.IDispatch.implicits._
import com.sos.scheduler.engine.minicom.idispatch.{IDispatch, Invocable, InvocableIDispatch}
import com.sos.scheduler.engine.minicom.remoting.IDispatchInvoker
import com.sos.scheduler.engine.taskserver.module.java.JavaInvoker._
import scala.runtime.BoxedUnit.UNIT

/**
 * @author Joacim Zschimmer
 */
private[java] class JavaInvoker(val iDispatch: IDispatch) extends IDispatchInvoker {

  private[java] def call(richName: String, arguments: Array[AnyRef]) =
    (richName.head match {
      case '<' ⇒
        val dispId = iDispatch.getIdOfName(richName.tail)
        iDispatch.invokeGet(dispId, arguments)
      case '>' ⇒
        require(arguments.nonEmpty, s"Missing arguments to set property $richName")
        val dispId = iDispatch.getIdOfName(richName.tail)
        iDispatch.invokePut(dispId, arguments = arguments take arguments.size - 1, value = arguments.last)
      case _ ⇒
        val dispId = iDispatch.getIdOfName(richName)
        iDispatch.invokeMethod(dispId, arguments)
    })
    .asInstanceOf[AnyRef] match {
      case result: IDispatch ⇒ toSosSpoolerIdispatch(result)
      case UNIT ⇒ ""  // Unit results from COM VT_EMPTY, which is expected to be an empty string
      case o ⇒ o
    }
}

private[java] object JavaInvoker {
  private[java] def apply(o: Invocable) = new JavaInvoker(InvocableIDispatch(o))

  private def toSosSpoolerIdispatch(iDispatch: IDispatch): sos.spooler.Idispatch = {
    val className = iDispatch.invokeGet("java_class_name").asInstanceOf[String]
    val constructor = Class.forName(className).getDeclaredConstructor(classOf[sos.spooler.Invoker])
    constructor.newInstance(new JavaInvoker(iDispatch)).asInstanceOf[sos.spooler.Idispatch]
  }
}
