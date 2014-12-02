package com.sos.scheduler.engine.client.command

import akka.actor.ActorSystem
import com.sos.scheduler.engine.common.xml.XmlUtils.loadXml
import com.sos.scheduler.engine.cplusplus.runtime.annotation.ForCpp
import com.sos.scheduler.engine.kernel.async.{CppCall, SchedulerThreadCallQueue}
import javax.inject.{Inject, Singleton}
import org.w3c.dom.Document
import scala.concurrent.ExecutionContext
import scala.util.Try

/**
 * @author Joacim Zschimmer
 */
@ForCpp
@Singleton
final class CppHttpSchedulerCommandClient @Inject private(
  callQueue: SchedulerThreadCallQueue,
  actorSystem: ActorSystem,
  commandClient: HttpSchedulerCommandClient)
  (implicit executionContext: ExecutionContext) {

  @ForCpp
  def postXml(uri: String, xmlBytes: Array[Byte], resultCall: CppCall): Unit = {
    commandClient.uncheckedExecuteXml(uri, xmlBytes).onComplete { o: Try[String] ⇒
      val documentTry = o map loadXml
      callQueue {
        resultCall.call(documentTry: Try[Document])
      }
    }
  }
}
