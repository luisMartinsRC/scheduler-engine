package com.sos.scheduler.engine.agent.common

import com.sos.scheduler.engine.agent.common.BeanPropertyReader._
import com.sos.scheduler.engine.common.scalautil.ScalaUtils._
import scala.reflect.ClassTag

/**
 * A mapper providing the bean properties of 
 * @author Joacim Zschimmer
 */
final class BeanPropertyReader[A](clas: Class[_ <: A], nameToConverter: NameToConverter) {
  private val methods = (
      for (d ← java.beans.Introspector.getBeanInfo(clas).getPropertyDescriptors if nameToConverter isDefinedAt d.getName;
           m ← Option(d.getReadMethod) if m.getParameterCount == 0)
      yield d.getName → m
    ).toMap

  def toMap[B <: A](bean: B): Map[String, Any] =
    for ((name, method) ← methods;
         converter ← nameToConverter.lift(name);
         value ← converter.lift(method.invoke(bean)))
    yield name → value
}

object BeanPropertyReader {
  type NameToConverter = PartialFunction[String, PartialFunction[Any, Any]]
  type ConditionalConverter = PartialFunction[Any, Any]

  val Keep: ConditionalConverter = { case o ⇒ o }

  def apply[A : ClassTag](nameToConverter: NameToConverter) = new BeanPropertyReader(implicitClass[A], nameToConverter)

  def toMap[A : ClassTag](bean: A)(nameToConverter: NameToConverter) = new BeanPropertyReader[A](bean.getClass, nameToConverter).toMap(bean)
}
