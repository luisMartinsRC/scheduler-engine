// *** Generated by com.sos.scheduler.engine.cplusplus.generator ***

#include "spooler.h"
#include "../zschimmer/java.h"
#include "../zschimmer/Has_proxy.h"
#include "../zschimmer/javaproxy.h"
#include "../zschimmer/lazy.h"

using namespace ::zschimmer;
using namespace ::zschimmer::javabridge;

namespace zschimmer { namespace javabridge { 

    template<> const class_factory<Proxy_class> has_proxy< ::sos::scheduler::order::Order >::proxy_class_factory("com.sos.scheduler.engine.kernel.cppproxy.OrderCImpl");

}}

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { 

static jstring JNICALL calculate_1db_1distributed_1next_1time(JNIEnv* jenv, jobject, jlong cppReference)
{
    Env env = jenv;
    try {
        ::sos::scheduler::order::Order* o_ = has_proxy< ::sos::scheduler::order::Order >::of_cpp_reference(cppReference,"::sos::scheduler::order::Order::calculate_db_distributed_next_time()");
        return env.jstring_from_string(o_->calculate_db_distributed_next_time());
    }
    catch(const exception& x) {
        env.set_java_exception(x);
        return jstring();
    }
}

}}}}}}}

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { 

static jstring JNICALL database_1runtime_1xml(JNIEnv* jenv, jobject, jlong cppReference)
{
    Env env = jenv;
    try {
        ::sos::scheduler::order::Order* o_ = has_proxy< ::sos::scheduler::order::Order >::of_cpp_reference(cppReference,"::sos::scheduler::order::Order::database_runtime_xml()");
        return env.jstring_from_string(o_->database_runtime_xml());
    }
    catch(const exception& x) {
        env.set_java_exception(x);
        return jstring();
    }
}

}}}}}}}

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { 

static jstring JNICALL database_1xml(JNIEnv* jenv, jobject, jlong cppReference)
{
    Env env = jenv;
    try {
        ::sos::scheduler::order::Order* o_ = has_proxy< ::sos::scheduler::order::Order >::of_cpp_reference(cppReference,"::sos::scheduler::order::Order::database_xml()");
        return env.jstring_from_string(o_->database_xml());
    }
    catch(const exception& x) {
        env.set_java_exception(x);
        return jstring();
    }
}

}}}}}}}

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { 

static jstring JNICALL end_1state_1string(JNIEnv* jenv, jobject, jlong cppReference)
{
    Env env = jenv;
    try {
        ::sos::scheduler::order::Order* o_ = has_proxy< ::sos::scheduler::order::Order >::of_cpp_reference(cppReference,"::sos::scheduler::order::Order::end_state_string()");
        return env.jstring_from_string(o_->end_state_string());
    }
    catch(const exception& x) {
        env.set_java_exception(x);
        return jstring();
    }
}

}}}}}}}

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { 

static jstring JNICALL file(JNIEnv* jenv, jobject, jlong cppReference)
{
    Env env = jenv;
    try {
        ::sos::scheduler::order::Order* o_ = has_proxy< ::sos::scheduler::order::Order >::of_cpp_reference(cppReference,"::sos::scheduler::order::Order::file()");
        return env.jstring_from_string(o_->file());
    }
    catch(const exception& x) {
        env.set_java_exception(x);
        return jstring();
    }
}

}}}}}}}

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { 

static jstring JNICALL file_1based_1state_1name(JNIEnv* jenv, jobject, jlong cppReference)
{
    Env env = jenv;
    try {
        ::sos::scheduler::order::Order* o_ = has_proxy< ::sos::scheduler::order::Order >::of_cpp_reference(cppReference,"::sos::scheduler::order::Order::file_based_state_name()");
        return env.jstring_from_string(o_->file_based_state_name());
    }
    catch(const exception& x) {
        env.set_java_exception(x);
        return jstring();
    }
}

}}}}}}}

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { 

static jlong JNICALL file_1modification_1time_1t(JNIEnv* jenv, jobject, jlong cppReference)
{
    Env env = jenv;
    try {
        ::sos::scheduler::order::Order* o_ = has_proxy< ::sos::scheduler::order::Order >::of_cpp_reference(cppReference,"::sos::scheduler::order::Order::file_modification_time_t()");
        return (o_->file_modification_time_t());
    }
    catch(const exception& x) {
        env.set_java_exception(x);
        return jlong();
    }
}

}}}}}}}

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { 

static jstring JNICALL file_1path(JNIEnv* jenv, jobject, jlong cppReference)
{
    Env env = jenv;
    try {
        ::sos::scheduler::order::Order* o_ = has_proxy< ::sos::scheduler::order::Order >::of_cpp_reference(cppReference,"::sos::scheduler::order::Order::file_path()");
        return env.jstring_from_string(o_->file_path());
    }
    catch(const exception& x) {
        env.set_java_exception(x);
        return jstring();
    }
}

}}}}}}}

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { 

static jboolean JNICALL has_1base_1file(JNIEnv* jenv, jobject, jlong cppReference)
{
    Env env = jenv;
    try {
        ::sos::scheduler::order::Order* o_ = has_proxy< ::sos::scheduler::order::Order >::of_cpp_reference(cppReference,"::sos::scheduler::order::Order::has_base_file()");
        return (o_->has_base_file());
    }
    catch(const exception& x) {
        env.set_java_exception(x);
        return jboolean();
    }
}

}}}}}}}

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { 

static jstring JNICALL initial_1state_1string(JNIEnv* jenv, jobject, jlong cppReference)
{
    Env env = jenv;
    try {
        ::sos::scheduler::order::Order* o_ = has_proxy< ::sos::scheduler::order::Order >::of_cpp_reference(cppReference,"::sos::scheduler::order::Order::initial_state_string()");
        return env.jstring_from_string(o_->initial_state_string());
    }
    catch(const exception& x) {
        env.set_java_exception(x);
        return jstring();
    }
}

}}}}}}}

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { 

static jboolean JNICALL is_1file_1based_1reread(JNIEnv* jenv, jobject, jlong cppReference)
{
    Env env = jenv;
    try {
        ::sos::scheduler::order::Order* o_ = has_proxy< ::sos::scheduler::order::Order >::of_cpp_reference(cppReference,"::sos::scheduler::order::Order::is_file_based_reread()");
        return (o_->is_file_based_reread());
    }
    catch(const exception& x) {
        env.set_java_exception(x);
        return jboolean();
    }
}

}}}}}}}

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { 

static jboolean JNICALL is_1on_1blacklist(JNIEnv* jenv, jobject, jlong cppReference)
{
    Env env = jenv;
    try {
        ::sos::scheduler::order::Order* o_ = has_proxy< ::sos::scheduler::order::Order >::of_cpp_reference(cppReference,"::sos::scheduler::order::Order::is_on_blacklist()");
        return (o_->is_on_blacklist());
    }
    catch(const exception& x) {
        env.set_java_exception(x);
        return jboolean();
    }
}

}}}}}}}

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { 

static jboolean JNICALL is_1to_1be_1removed(JNIEnv* jenv, jobject, jlong cppReference)
{
    Env env = jenv;
    try {
        ::sos::scheduler::order::Order* o_ = has_proxy< ::sos::scheduler::order::Order >::of_cpp_reference(cppReference,"::sos::scheduler::order::Order::is_to_be_removed()");
        return (o_->is_to_be_removed());
    }
    catch(const exception& x) {
        env.set_java_exception(x);
        return jboolean();
    }
}

}}}}}}}

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { 

static jboolean JNICALL is_1visible(JNIEnv* jenv, jobject, jlong cppReference)
{
    Env env = jenv;
    try {
        ::sos::scheduler::order::Order* o_ = has_proxy< ::sos::scheduler::order::Order >::of_cpp_reference(cppReference,"::sos::scheduler::order::Order::is_visible()");
        return (o_->is_visible());
    }
    catch(const exception& x) {
        env.set_java_exception(x);
        return jboolean();
    }
}

}}}}}}}

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { 

static void JNICALL java_1remove(JNIEnv* jenv, jobject, jlong cppReference)
{
    Env env = jenv;
    try {
        ::sos::scheduler::order::Order* o_ = has_proxy< ::sos::scheduler::order::Order >::of_cpp_reference(cppReference,"::sos::scheduler::order::Order::java_remove()");
        (o_->java_remove());
    }
    catch(const exception& x) {
        env.set_java_exception(x);
    }
}

}}}}}}}

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { 

static jobject JNICALL job_1chain(JNIEnv* jenv, jobject, jlong cppReference)
{
    Env env = jenv;
    try {
        ::sos::scheduler::order::Order* o_ = has_proxy< ::sos::scheduler::order::Order >::of_cpp_reference(cppReference,"::sos::scheduler::order::Order::job_chain()");
        return Has_proxy::jobject_of(o_->job_chain());
    }
    catch(const exception& x) {
        env.set_java_exception(x);
        return jobject();
    }
}

}}}}}}}

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { 

static jstring JNICALL job_1chain_1path_1string(JNIEnv* jenv, jobject, jlong cppReference)
{
    Env env = jenv;
    try {
        ::sos::scheduler::order::Order* o_ = has_proxy< ::sos::scheduler::order::Order >::of_cpp_reference(cppReference,"::sos::scheduler::order::Order::job_chain_path_string()");
        return env.jstring_from_string(o_->job_chain_path_string());
    }
    catch(const exception& x) {
        env.set_java_exception(x);
        return jstring();
    }
}

}}}}}}}

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { 

static jobject JNICALL log(JNIEnv* jenv, jobject, jlong cppReference)
{
    Env env = jenv;
    try {
        ::sos::scheduler::order::Order* o_ = has_proxy< ::sos::scheduler::order::Order >::of_cpp_reference(cppReference,"::sos::scheduler::order::Order::log()");
        return Has_proxy::jobject_of(o_->log());
    }
    catch(const exception& x) {
        env.set_java_exception(x);
        return jobject();
    }
}

}}}}}}}

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { 

static jstring JNICALL name(JNIEnv* jenv, jobject, jlong cppReference)
{
    Env env = jenv;
    try {
        ::sos::scheduler::order::Order* o_ = has_proxy< ::sos::scheduler::order::Order >::of_cpp_reference(cppReference,"::sos::scheduler::order::Order::name()");
        return env.jstring_from_string(o_->name());
    }
    catch(const exception& x) {
        env.set_java_exception(x);
        return jstring();
    }
}

}}}}}}}

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { 

static jlong JNICALL next_1time_1millis(JNIEnv* jenv, jobject, jlong cppReference)
{
    Env env = jenv;
    try {
        ::sos::scheduler::order::Order* o_ = has_proxy< ::sos::scheduler::order::Order >::of_cpp_reference(cppReference,"::sos::scheduler::order::Order::next_time_millis()");
        return (o_->next_time_millis());
    }
    catch(const exception& x) {
        env.set_java_exception(x);
        return jlong();
    }
}

}}}}}}}

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { 

static jobject JNICALL params(JNIEnv* jenv, jobject, jlong cppReference)
{
    Env env = jenv;
    try {
        ::sos::scheduler::order::Order* o_ = has_proxy< ::sos::scheduler::order::Order >::of_cpp_reference(cppReference,"::sos::scheduler::order::Order::params()");
        return Has_proxy::jobject_of(o_->params());
    }
    catch(const exception& x) {
        env.set_java_exception(x);
        return jobject();
    }
}

}}}}}}}

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { 

static jstring JNICALL path(JNIEnv* jenv, jobject, jlong cppReference)
{
    Env env = jenv;
    try {
        ::sos::scheduler::order::Order* o_ = has_proxy< ::sos::scheduler::order::Order >::of_cpp_reference(cppReference,"::sos::scheduler::order::Order::path()");
        return env.jstring_from_string(o_->path());
    }
    catch(const exception& x) {
        env.set_java_exception(x);
        return jstring();
    }
}

}}}}}}}

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { 

static jint JNICALL priority(JNIEnv* jenv, jobject, jlong cppReference)
{
    Env env = jenv;
    try {
        ::sos::scheduler::order::Order* o_ = has_proxy< ::sos::scheduler::order::Order >::of_cpp_reference(cppReference,"::sos::scheduler::order::Order::priority()");
        return (o_->priority());
    }
    catch(const exception& x) {
        env.set_java_exception(x);
        return jint();
    }
}

}}}}}}}

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { 

static void JNICALL set_1end_1state__Ljava_lang_String_2(JNIEnv* jenv, jobject, jlong cppReference, jstring p0)
{
    Env env = jenv;
    try {
        ::sos::scheduler::order::Order* o_ = has_proxy< ::sos::scheduler::order::Order >::of_cpp_reference(cppReference,"::sos::scheduler::order::Order::set_end_state()");
        (o_->set_end_state(env.string_from_jstring(p0)));
    }
    catch(const exception& x) {
        env.set_java_exception(x);
    }
}

}}}}}}}

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { 

static void JNICALL set_1force_1file_1reread(JNIEnv* jenv, jobject, jlong cppReference)
{
    Env env = jenv;
    try {
        ::sos::scheduler::order::Order* o_ = has_proxy< ::sos::scheduler::order::Order >::of_cpp_reference(cppReference,"::sos::scheduler::order::Order::set_force_file_reread()");
        (o_->set_force_file_reread());
    }
    catch(const exception& x) {
        env.set_java_exception(x);
    }
}

}}}}}}}

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { 

static void JNICALL set_1id__Ljava_lang_String_2(JNIEnv* jenv, jobject, jlong cppReference, jstring p0)
{
    Env env = jenv;
    try {
        ::sos::scheduler::order::Order* o_ = has_proxy< ::sos::scheduler::order::Order >::of_cpp_reference(cppReference,"::sos::scheduler::order::Order::set_id()");
        (o_->set_id(env.string_from_jstring(p0)));
    }
    catch(const exception& x) {
        env.set_java_exception(x);
    }
}

}}}}}}}

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { 

static void JNICALL set_1on_1blacklist(JNIEnv* jenv, jobject, jlong cppReference)
{
    Env env = jenv;
    try {
        ::sos::scheduler::order::Order* o_ = has_proxy< ::sos::scheduler::order::Order >::of_cpp_reference(cppReference,"::sos::scheduler::order::Order::set_on_blacklist()");
        (o_->set_on_blacklist());
    }
    catch(const exception& x) {
        env.set_java_exception(x);
    }
}

}}}}}}}

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { 

static void JNICALL set_1priority__I(JNIEnv* jenv, jobject, jlong cppReference, jint p0)
{
    Env env = jenv;
    try {
        ::sos::scheduler::order::Order* o_ = has_proxy< ::sos::scheduler::order::Order >::of_cpp_reference(cppReference,"::sos::scheduler::order::Order::set_priority()");
        (o_->set_priority(p0));
    }
    catch(const exception& x) {
        env.set_java_exception(x);
    }
}

}}}}}}}

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { 

static void JNICALL set_1suspended__Z(JNIEnv* jenv, jobject, jlong cppReference, jboolean p0)
{
    Env env = jenv;
    try {
        ::sos::scheduler::order::Order* o_ = has_proxy< ::sos::scheduler::order::Order >::of_cpp_reference(cppReference,"::sos::scheduler::order::Order::set_suspended()");
        (o_->set_suspended(p0 != 0));
    }
    catch(const exception& x) {
        env.set_java_exception(x);
    }
}

}}}}}}}

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { 

static void JNICALL set_1title__Ljava_lang_String_2(JNIEnv* jenv, jobject, jlong cppReference, jstring p0)
{
    Env env = jenv;
    try {
        ::sos::scheduler::order::Order* o_ = has_proxy< ::sos::scheduler::order::Order >::of_cpp_reference(cppReference,"::sos::scheduler::order::Order::set_title()");
        (o_->set_title(env.string_from_jstring(p0)));
    }
    catch(const exception& x) {
        env.set_java_exception(x);
    }
}

}}}}}}}

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { 

static jobject JNICALL source_1xml_1bytes(JNIEnv* jenv, jobject, jlong cppReference)
{
    Env env = jenv;
    try {
        ::sos::scheduler::order::Order* o_ = has_proxy< ::sos::scheduler::order::Order >::of_cpp_reference(cppReference,"::sos::scheduler::order::Order::source_xml_bytes()");
        return java_byte_array_from_c(o_->source_xml_bytes());
    }
    catch(const exception& x) {
        env.set_java_exception(x);
        return jobject();
    }
}

}}}}}}}

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { 

static jstring JNICALL string_1id(JNIEnv* jenv, jobject, jlong cppReference)
{
    Env env = jenv;
    try {
        ::sos::scheduler::order::Order* o_ = has_proxy< ::sos::scheduler::order::Order >::of_cpp_reference(cppReference,"::sos::scheduler::order::Order::string_id()");
        return env.jstring_from_string(o_->string_id());
    }
    catch(const exception& x) {
        env.set_java_exception(x);
        return jstring();
    }
}

}}}}}}}

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { 

static jstring JNICALL string_1payload(JNIEnv* jenv, jobject, jlong cppReference)
{
    Env env = jenv;
    try {
        ::sos::scheduler::order::Order* o_ = has_proxy< ::sos::scheduler::order::Order >::of_cpp_reference(cppReference,"::sos::scheduler::order::Order::string_payload()");
        return env.jstring_from_string(o_->string_payload());
    }
    catch(const exception& x) {
        env.set_java_exception(x);
        return jstring();
    }
}

}}}}}}}

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { 

static jstring JNICALL string_1state(JNIEnv* jenv, jobject, jlong cppReference)
{
    Env env = jenv;
    try {
        ::sos::scheduler::order::Order* o_ = has_proxy< ::sos::scheduler::order::Order >::of_cpp_reference(cppReference,"::sos::scheduler::order::Order::string_state()");
        return env.jstring_from_string(o_->string_state());
    }
    catch(const exception& x) {
        env.set_java_exception(x);
        return jstring();
    }
}

}}}}}}}

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { 

static jboolean JNICALL suspended(JNIEnv* jenv, jobject, jlong cppReference)
{
    Env env = jenv;
    try {
        ::sos::scheduler::order::Order* o_ = has_proxy< ::sos::scheduler::order::Order >::of_cpp_reference(cppReference,"::sos::scheduler::order::Order::suspended()");
        return (o_->suspended());
    }
    catch(const exception& x) {
        env.set_java_exception(x);
        return jboolean();
    }
}

}}}}}}}

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace cppproxy { 

static jstring JNICALL title(JNIEnv* jenv, jobject, jlong cppReference)
{
    Env env = jenv;
    try {
        ::sos::scheduler::order::Order* o_ = has_proxy< ::sos::scheduler::order::Order >::of_cpp_reference(cppReference,"::sos::scheduler::order::Order::title()");
        return env.jstring_from_string(o_->title());
    }
    catch(const exception& x) {
        env.set_java_exception(x);
        return jstring();
    }
}

}}}}}}}

const static JNINativeMethod native_methods[] = {
    { (char*)"calculate_db_distributed_next_time__native", (char*)"(J)Ljava/lang/String;", (void*)::javaproxy::com::sos::scheduler::engine::kernel::cppproxy::calculate_1db_1distributed_1next_1time },
    { (char*)"database_runtime_xml__native", (char*)"(J)Ljava/lang/String;", (void*)::javaproxy::com::sos::scheduler::engine::kernel::cppproxy::database_1runtime_1xml },
    { (char*)"database_xml__native", (char*)"(J)Ljava/lang/String;", (void*)::javaproxy::com::sos::scheduler::engine::kernel::cppproxy::database_1xml },
    { (char*)"end_state_string__native", (char*)"(J)Ljava/lang/String;", (void*)::javaproxy::com::sos::scheduler::engine::kernel::cppproxy::end_1state_1string },
    { (char*)"file__native", (char*)"(J)Ljava/lang/String;", (void*)::javaproxy::com::sos::scheduler::engine::kernel::cppproxy::file },
    { (char*)"file_based_state_name__native", (char*)"(J)Ljava/lang/String;", (void*)::javaproxy::com::sos::scheduler::engine::kernel::cppproxy::file_1based_1state_1name },
    { (char*)"file_modification_time_t__native", (char*)"(J)J", (void*)::javaproxy::com::sos::scheduler::engine::kernel::cppproxy::file_1modification_1time_1t },
    { (char*)"file_path__native", (char*)"(J)Ljava/lang/String;", (void*)::javaproxy::com::sos::scheduler::engine::kernel::cppproxy::file_1path },
    { (char*)"has_base_file__native", (char*)"(J)Z", (void*)::javaproxy::com::sos::scheduler::engine::kernel::cppproxy::has_1base_1file },
    { (char*)"initial_state_string__native", (char*)"(J)Ljava/lang/String;", (void*)::javaproxy::com::sos::scheduler::engine::kernel::cppproxy::initial_1state_1string },
    { (char*)"is_file_based_reread__native", (char*)"(J)Z", (void*)::javaproxy::com::sos::scheduler::engine::kernel::cppproxy::is_1file_1based_1reread },
    { (char*)"is_on_blacklist__native", (char*)"(J)Z", (void*)::javaproxy::com::sos::scheduler::engine::kernel::cppproxy::is_1on_1blacklist },
    { (char*)"is_to_be_removed__native", (char*)"(J)Z", (void*)::javaproxy::com::sos::scheduler::engine::kernel::cppproxy::is_1to_1be_1removed },
    { (char*)"is_visible__native", (char*)"(J)Z", (void*)::javaproxy::com::sos::scheduler::engine::kernel::cppproxy::is_1visible },
    { (char*)"java_remove__native", (char*)"(J)V", (void*)::javaproxy::com::sos::scheduler::engine::kernel::cppproxy::java_1remove },
    { (char*)"job_chain__native", (char*)"(J)Lcom/sos/scheduler/engine/kernel/cppproxy/Job_chainC;", (void*)::javaproxy::com::sos::scheduler::engine::kernel::cppproxy::job_1chain },
    { (char*)"job_chain_path_string__native", (char*)"(J)Ljava/lang/String;", (void*)::javaproxy::com::sos::scheduler::engine::kernel::cppproxy::job_1chain_1path_1string },
    { (char*)"log__native", (char*)"(J)Lcom/sos/scheduler/engine/kernel/cppproxy/Prefix_logC;", (void*)::javaproxy::com::sos::scheduler::engine::kernel::cppproxy::log },
    { (char*)"name__native", (char*)"(J)Ljava/lang/String;", (void*)::javaproxy::com::sos::scheduler::engine::kernel::cppproxy::name },
    { (char*)"next_time_millis__native", (char*)"(J)J", (void*)::javaproxy::com::sos::scheduler::engine::kernel::cppproxy::next_1time_1millis },
    { (char*)"params__native", (char*)"(J)Lcom/sos/scheduler/engine/kernel/cppproxy/Variable_setC;", (void*)::javaproxy::com::sos::scheduler::engine::kernel::cppproxy::params },
    { (char*)"path__native", (char*)"(J)Ljava/lang/String;", (void*)::javaproxy::com::sos::scheduler::engine::kernel::cppproxy::path },
    { (char*)"priority__native", (char*)"(J)I", (void*)::javaproxy::com::sos::scheduler::engine::kernel::cppproxy::priority },
    { (char*)"set_end_state__native", (char*)"(JLjava/lang/String;)V", (void*)::javaproxy::com::sos::scheduler::engine::kernel::cppproxy::set_1end_1state__Ljava_lang_String_2 },
    { (char*)"set_force_file_reread__native", (char*)"(J)V", (void*)::javaproxy::com::sos::scheduler::engine::kernel::cppproxy::set_1force_1file_1reread },
    { (char*)"set_id__native", (char*)"(JLjava/lang/String;)V", (void*)::javaproxy::com::sos::scheduler::engine::kernel::cppproxy::set_1id__Ljava_lang_String_2 },
    { (char*)"set_on_blacklist__native", (char*)"(J)V", (void*)::javaproxy::com::sos::scheduler::engine::kernel::cppproxy::set_1on_1blacklist },
    { (char*)"set_priority__native", (char*)"(JI)V", (void*)::javaproxy::com::sos::scheduler::engine::kernel::cppproxy::set_1priority__I },
    { (char*)"set_suspended__native", (char*)"(JZ)V", (void*)::javaproxy::com::sos::scheduler::engine::kernel::cppproxy::set_1suspended__Z },
    { (char*)"set_title__native", (char*)"(JLjava/lang/String;)V", (void*)::javaproxy::com::sos::scheduler::engine::kernel::cppproxy::set_1title__Ljava_lang_String_2 },
    { (char*)"source_xml_bytes__native", (char*)"(J)[B", (void*)::javaproxy::com::sos::scheduler::engine::kernel::cppproxy::source_1xml_1bytes },
    { (char*)"string_id__native", (char*)"(J)Ljava/lang/String;", (void*)::javaproxy::com::sos::scheduler::engine::kernel::cppproxy::string_1id },
    { (char*)"string_payload__native", (char*)"(J)Ljava/lang/String;", (void*)::javaproxy::com::sos::scheduler::engine::kernel::cppproxy::string_1payload },
    { (char*)"string_state__native", (char*)"(J)Ljava/lang/String;", (void*)::javaproxy::com::sos::scheduler::engine::kernel::cppproxy::string_1state },
    { (char*)"suspended__native", (char*)"(J)Z", (void*)::javaproxy::com::sos::scheduler::engine::kernel::cppproxy::suspended },
    { (char*)"title__native", (char*)"(J)Ljava/lang/String;", (void*)::javaproxy::com::sos::scheduler::engine::kernel::cppproxy::title }
};

namespace zschimmer { namespace javabridge { 

    template<> void has_proxy< ::sos::scheduler::order::Order >::register_cpp_proxy_class_in_java() {
        Env env;
        Class* cls = has_proxy< ::sos::scheduler::order::Order >::proxy_class_factory.clas();
        int ret = env->RegisterNatives(*cls, native_methods, sizeof native_methods / sizeof native_methods[0]);
        if (ret < 0)  env.throw_java("RegisterNatives", "com.sos.scheduler.engine.kernel.cppproxy.OrderCImpl");
    }

}}
