package com.sos.scheduler.engine.kernel.plugin

import com.sos.scheduler.engine.common.scalautil.HasCloser

/**
 * @author Joacim Zschimmer
 */
trait Plugin extends HasCloser {

  private var _isPrepared = false
  private var _isActive = false

  onClose {
    _isActive = false
    _isPrepared = false
  }

  final def prepare(): Unit = {
    onPrepare()
    _isPrepared = true
  }

  def onPrepare(): Unit = {}

  final def activate(): Unit = {
    if (!_isPrepared)  throw new IllegalStateException
    onActivate()
    _isActive = true
  }

  def onActivate(): Unit = {}

  def xmlState: String = ""

  final def isPrepared = _isPrepared

  final def isActive = _isActive
}
