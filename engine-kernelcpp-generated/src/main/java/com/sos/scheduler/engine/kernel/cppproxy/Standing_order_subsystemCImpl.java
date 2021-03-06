// *** Generated by com.sos.scheduler.engine.cplusplus.generator ***

package com.sos.scheduler.engine.kernel.cppproxy;

@javax.annotation.Generated("C++/Java-Generator - SOS GmbH Berlin")
@SuppressWarnings({"unchecked", "rawtypes"})
final class Standing_order_subsystemCImpl
extends com.sos.scheduler.engine.cplusplus.runtime.CppProxyImpl<com.sos.scheduler.engine.cplusplus.runtime.Sister>
implements com.sos.scheduler.engine.kernel.cppproxy.Standing_order_subsystemC {

    // <editor-fold defaultstate="collapsed" desc="Generated code - DO NOT EDIT">

    private Standing_order_subsystemCImpl(com.sos.scheduler.engine.cplusplus.runtime.Sister context) { // Nur für JNI zugänglich
        requireContextIsNull(context);
    }

    @Override public String[] file_based_paths(boolean p0) {
        com.sos.scheduler.engine.cplusplus.runtime.CppProxy.threadLock.lock();
        try {
            String[] result = file_based_paths__native(cppReference(), p0);
            checkIsNotReleased(String[].class, result);
            return result;
        }
        catch (Exception x) { throw com.sos.scheduler.engine.cplusplus.runtime.CppProxies.propagateCppException(x, this); }
        finally {
            com.sos.scheduler.engine.cplusplus.runtime.CppProxy.threadLock.unlock();
        }
    }

    private static native String[] file_based_paths__native(long cppReference, boolean p0);


    @Override public boolean is_empty() {
        com.sos.scheduler.engine.cplusplus.runtime.CppProxy.threadLock.lock();
        try {
            return is_empty__native(cppReference());
        }
        catch (Exception x) { throw com.sos.scheduler.engine.cplusplus.runtime.CppProxies.propagateCppException(x, this); }
        finally {
            com.sos.scheduler.engine.cplusplus.runtime.CppProxy.threadLock.unlock();
        }
    }

    private static native boolean is_empty__native(long cppReference);


    @Override public com.sos.scheduler.engine.kernel.cppproxy.OrderC java_active_file_based(java.lang.String p0) {
        com.sos.scheduler.engine.cplusplus.runtime.CppProxy.threadLock.lock();
        try {
            com.sos.scheduler.engine.kernel.cppproxy.OrderC result = java_active_file_based__native(cppReference(), p0);
            checkIsNotReleased(com.sos.scheduler.engine.kernel.cppproxy.OrderC.class, result);
            return result;
        }
        catch (Exception x) { throw com.sos.scheduler.engine.cplusplus.runtime.CppProxies.propagateCppException(x, this); }
        finally {
            com.sos.scheduler.engine.cplusplus.runtime.CppProxy.threadLock.unlock();
        }
    }

    private static native com.sos.scheduler.engine.kernel.cppproxy.OrderC java_active_file_based__native(long cppReference, java.lang.String p0);


    @Override public com.sos.scheduler.engine.kernel.cppproxy.OrderC java_file_based(java.lang.String p0) {
        com.sos.scheduler.engine.cplusplus.runtime.CppProxy.threadLock.lock();
        try {
            com.sos.scheduler.engine.kernel.cppproxy.OrderC result = java_file_based__native(cppReference(), p0);
            checkIsNotReleased(com.sos.scheduler.engine.kernel.cppproxy.OrderC.class, result);
            return result;
        }
        catch (Exception x) { throw com.sos.scheduler.engine.cplusplus.runtime.CppProxies.propagateCppException(x, this); }
        finally {
            com.sos.scheduler.engine.cplusplus.runtime.CppProxy.threadLock.unlock();
        }
    }

    private static native com.sos.scheduler.engine.kernel.cppproxy.OrderC java_file_based__native(long cppReference, java.lang.String p0);


    @Override public com.sos.scheduler.engine.kernel.cppproxy.OrderC java_file_based_or_null(java.lang.String p0) {
        com.sos.scheduler.engine.cplusplus.runtime.CppProxy.threadLock.lock();
        try {
            com.sos.scheduler.engine.kernel.cppproxy.OrderC result = java_file_based_or_null__native(cppReference(), p0);
            checkIsNotReleased(com.sos.scheduler.engine.kernel.cppproxy.OrderC.class, result);
            return result;
        }
        catch (Exception x) { throw com.sos.scheduler.engine.cplusplus.runtime.CppProxies.propagateCppException(x, this); }
        finally {
            com.sos.scheduler.engine.cplusplus.runtime.CppProxy.threadLock.unlock();
        }
    }

    private static native com.sos.scheduler.engine.kernel.cppproxy.OrderC java_file_based_or_null__native(long cppReference, java.lang.String p0);


    @Override public java.util.ArrayList java_file_baseds() {
        com.sos.scheduler.engine.cplusplus.runtime.CppProxy.threadLock.lock();
        try {
            java.util.ArrayList result = java_file_baseds__native(cppReference());
            checkIsNotReleased(java.util.ArrayList.class, result);
            return result;
        }
        catch (Exception x) { throw com.sos.scheduler.engine.cplusplus.runtime.CppProxies.propagateCppException(x, this); }
        finally {
            com.sos.scheduler.engine.cplusplus.runtime.CppProxy.threadLock.unlock();
        }
    }

    private static native java.util.ArrayList java_file_baseds__native(long cppReference);


    // </editor-fold>
}
