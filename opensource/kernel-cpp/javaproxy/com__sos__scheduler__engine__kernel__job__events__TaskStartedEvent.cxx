// *** Generated by com.sos.scheduler.engine.cplusplus.generator ***

#include "_precompiled.h"

#include "com__sos__scheduler__engine__kernel__job__events__TaskStartedEvent.h"
#include "com__sos__scheduler__engine__kernel__job__UnmodifiableTask.h"
#include "com__sos__scheduler__engine__kernel__job__events__UnmodifiableTaskEvent.h"
#include "java__lang__String.h"

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace job { namespace events { 

struct TaskStartedEvent__class : ::zschimmer::javabridge::Class
{
    TaskStartedEvent__class(const string& class_name);
   ~TaskStartedEvent__class();

    ::zschimmer::javabridge::Method const __constructor__Lcom_sos_scheduler_engine_kernel_job_UnmodifiableTask_2__method;

    static const ::zschimmer::javabridge::class_factory< TaskStartedEvent__class > class_factory;
};

const ::zschimmer::javabridge::class_factory< TaskStartedEvent__class > TaskStartedEvent__class::class_factory ("com.sos.scheduler.engine.kernel.job.events.TaskStartedEvent");

TaskStartedEvent__class::TaskStartedEvent__class(const string& class_name) :
    ::zschimmer::javabridge::Class(class_name)
    ,__constructor__Lcom_sos_scheduler_engine_kernel_job_UnmodifiableTask_2__method(this, "<init>", "(Lcom/sos/scheduler/engine/kernel/job/UnmodifiableTask;)V"){}

TaskStartedEvent__class::~TaskStartedEvent__class() {}



TaskStartedEvent TaskStartedEvent::new_instance(const ::zschimmer::javabridge::proxy_jobject< ::javaproxy::com::sos::scheduler::engine::kernel::job::UnmodifiableTask >& p0) {
    TaskStartedEvent result;
    result.java_object_allocate_();
    ::zschimmer::javabridge::raw_parameter_list<1> parameter_list;
    parameter_list._jvalues[0].l = p0.get_jobject();
    TaskStartedEvent__class* cls = result._class.get();
    cls->__constructor__Lcom_sos_scheduler_engine_kernel_job_UnmodifiableTask_2__method.call(result.get_jobject(), parameter_list);
    return result;
}


TaskStartedEvent::TaskStartedEvent(jobject jo) { if (jo) assign_(jo); }

TaskStartedEvent::TaskStartedEvent(const TaskStartedEvent& o) { assign_(o.get_jobject()); }

#ifdef Z_HAS_MOVE_CONSTRUCTOR
    TaskStartedEvent::TaskStartedEvent(TaskStartedEvent&& o) { set_jobject(o.get_jobject());  o.set_jobject(NULL); }
#endif

TaskStartedEvent::~TaskStartedEvent() { assign_(NULL); }





::zschimmer::javabridge::Class* TaskStartedEvent::java_object_class_() { return _class.get(); }

::zschimmer::javabridge::Class* TaskStartedEvent::java_class_() { return TaskStartedEvent__class::class_factory.clas(); }


void TaskStartedEvent::Lazy_class::initialize() {
    _value = TaskStartedEvent__class::class_factory.clas();
}


}}}}}}}}