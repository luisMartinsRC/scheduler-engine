package com.sos.scheduler.engine.taskserver.module

import com.sos.scheduler.engine.common.scalautil.xmls.ScalaXMLEventReader
import com.sos.scheduler.engine.data.base.IsString

/**
 * @author Joacim Zschimmer
 */
final case class Script(string: String) extends IsString

object Script {
  def parseXmlString(xmlString: String): Script =
    ScalaXMLEventReader.parseString(xmlString) { eventReader ⇒
      import eventReader._
      val parts = parseElement("source") {
        parseEachRepeatingElement("source_part") {
          val lineNr = attributeMap.get("linenr")  // TODO Die Zeilennummer an die ScriptingEngine übergeben
          eatText()
        }
      }
      Script(parts mkString "")
    }
}
