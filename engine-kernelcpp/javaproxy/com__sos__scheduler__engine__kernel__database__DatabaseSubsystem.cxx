// *** Generated by com.sos.scheduler.engine.cplusplus.generator ***

#include "_precompiled.h"

#include "com__sos__scheduler__engine__kernel__database__DatabaseSubsystem.h"
#include "java__lang__Object.h"
#include "java__lang__String.h"

namespace javaproxy { namespace com { namespace sos { namespace scheduler { namespace engine { namespace kernel { namespace database { 

struct DatabaseSubsystem__class : ::zschimmer::javabridge::Class
{
    DatabaseSubsystem__class(const string& class_name);
   ~DatabaseSubsystem__class();


    static const ::zschimmer::javabridge::class_factory< DatabaseSubsystem__class > class_factory;
};

const ::zschimmer::javabridge::class_factory< DatabaseSubsystem__class > DatabaseSubsystem__class::class_factory ("com.sos.scheduler.engine.kernel.database.DatabaseSubsystem");

DatabaseSubsystem__class::DatabaseSubsystem__class(const string& class_name) :
    ::zschimmer::javabridge::Class(class_name)
{}

DatabaseSubsystem__class::~DatabaseSubsystem__class() {}




DatabaseSubsystem::DatabaseSubsystem(jobject jo) { if (jo) assign_(jo); }

DatabaseSubsystem::DatabaseSubsystem(const DatabaseSubsystem& o) { assign_(o.get_jobject()); }

#ifdef Z_HAS_MOVE_CONSTRUCTOR
    DatabaseSubsystem::DatabaseSubsystem(DatabaseSubsystem&& o) { set_jobject(o.get_jobject());  o.set_jobject(NULL); }
#endif

DatabaseSubsystem::~DatabaseSubsystem() { assign_(NULL); }





::zschimmer::javabridge::Class* DatabaseSubsystem::java_object_class_() const { return _class.get(); }

::zschimmer::javabridge::Class* DatabaseSubsystem::java_class_() { return DatabaseSubsystem__class::class_factory.clas(); }


void DatabaseSubsystem::Lazy_class::initialize() const {
    _value = DatabaseSubsystem__class::class_factory.clas();
}


}}}}}}}
