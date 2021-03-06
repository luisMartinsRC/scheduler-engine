package com.sos.scheduler.engine.minicom.remoting.serial

import com.sos.scheduler.engine.minicom.idispatch.Invocable
import com.sos.scheduler.engine.minicom.remoting.serial.variantArrayFlags._
import com.sos.scheduler.engine.minicom.remoting.serial.variantTypes._
import com.sos.scheduler.engine.minicom.types.HRESULT.DISP_E_BADVARTYPE
import com.sos.scheduler.engine.minicom.types.{COMException, VariantArray}
import org.scalactic.Requirements._

/**
 * @author Joacim Zschimmer
 */
private[remoting] trait VariantDeserializer extends BaseDeserializer {

  def readVariant(): Any = {
    val vt = readInt32()
    vt match {
      case _ if (vt & VT_ARRAY) != 0 ⇒ readVariantArray()
      case VT_UNKNOWN | VT_DISPATCH ⇒ readInvocableOrNull()  // To make any sense, VT_UNKNOWN should denote here an IDispatch
      case _ ⇒ readSimpleVariant(vt)
    }
  }

  private def readVariantArray(): VariantArray = {
    val dimensions = readInt16()
    require(dimensions == 1)
    val features = readInt16()
    require((features & ~(FADF_FIXEDSIZE | FADF_HAVEVARTYPE | FADF_BSTR | FADF_VARIANT)) == 0, f"Only FADF_FIXEDSIZE, FADF_HAVEVARTYPE and FADF_VARIANT are accepted, not fFeature=$features%04x")
    // Nicht bei com.cxx (Unix): require((features & FADF_HAVEVARTYPE) != 0, f"FADF_HAVEVARTYPE is required, not fFeature=$features%04x")
    val count = readInt32()
    val lowerBound = readInt32()
    require(lowerBound == 0)
    readInt32() match {
//      case VT_UI1 ⇒
//        require((features & FADF_HAVEVARTYPE) != 0, f"VT_UI1 requires FADF_HAVEVARTYPE, not fFeature=$features%04x")
//      case VT_BSTR ⇒
//        require((features & FADF_BSTR) != 0, f"VT_BSTR requires FADF_HAVEVARTYPE | FADF_VARIANT, not fFeature=$features%04x")
      case VT_VARIANT ⇒
        require((features & FADF_VARIANT) != 0, f"VT_VARIANT requires FADF_HAVEVARTYPE | FADF_VARIANT, not fFeature=$features%04x")
        VariantArray(Vector.fill(count) { readVariant() })
      case o ⇒ throw new COMException(DISP_E_BADVARTYPE, f"Unsupported Array Variant VT=$o%x")
    }
  }

  private def readSimpleVariant(vt: Int): Any =
    vt match {
      case VT_EMPTY ⇒ ()
      //case VT_NULL ⇒
      //case VT_I2 ⇒
      case VT_I4 | VT_INT ⇒ readInt32()
      //case VT_R4 ⇒
      //case VT_R8 ⇒
      //case VT_CY ⇒
      //case VT_DATE ⇒
      case VT_BSTR ⇒ readString()
      //case VT_ERROR ⇒
      case VT_BOOL ⇒ readBoolean()
      //case VT_DECIMAL ⇒
      //case VT_I1 ⇒
      //case VT_UI1 ⇒
      //case VT_UI2 ⇒
      //case VT_UI4 ⇒ IntVariant(readInt32())
      case VT_I8 ⇒ readInt64()
      //case VT_UI8 ⇒
      //case VT_INT ⇒
      //case VT_UINT ⇒
      case o ⇒ throw new COMException(DISP_E_BADVARTYPE, f"Unsupported Variant VT=$o%x")
    }

  protected def readInvocableOrNull(): Invocable = throw new UnsupportedOperationException("readInvocableOption is not implemented")  // Method is overridden
}
