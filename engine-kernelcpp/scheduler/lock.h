// $Id: lock.h 13707 2008-10-17 10:41:41Z jz $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com 

#ifndef __SCHEDULER_LOCK_H
#define __SCHEDULER_LOCK_H

namespace sos {
namespace scheduler {
namespace lock {

struct Holder;
struct Lock_folder;
struct Requestor;
struct Use;

//---------------------------------------------------------------------------------------------Lock

struct Lock : idispatch_implementation< Lock, spooler_com::Ilock>, 
              file_based< Lock, Lock_folder, Lock_subsystem >, 
              javabridge::has_proxy<Lock>,
              Non_cloneable
{
    enum Lock_mode
    { 
        lk_exclusive     = 0,   // Index für _waiting_queues
        lk_non_exclusive = 1 
    };


                                Lock                        ( Lock_subsystem*, const string& name = "" );
                               ~Lock                        ();


    // Abstract_scheduler_object

    void                        close                       ();
  //string                      obj_name                    () const;

    jobject                     java_sister                 ()                                      { return javabridge::has_proxy<Lock>::java_sister(); }

    // file_based<>

    STDMETHODIMP_(ULONG)        AddRef                      ()                                      { return Idispatch_implementation::AddRef(); }
    STDMETHODIMP_(ULONG)        Release                     ()                                      { return Idispatch_implementation::Release(); }


    bool                        on_initialize               (); 
    bool                        on_load                     (); 
    bool                        on_activate                 ();

    void                        on_prepare_to_remove        ();
    bool                        can_be_removed_now          ();
    zschimmer::Xc               remove_error                ();

    bool                        can_be_replaced_now         ();
    void                        prepare_to_replace          ();
    Lock*                       on_replace_now              ();


    // Ilock

    STDMETHODIMP            get_Java_class_name             ( BSTR* result )                        { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name             ()                                      { return (char*)"sos.spooler.Lock"; }
    STDMETHODIMP            put_Name                        ( BSTR );     
    STDMETHODIMP            get_Name                        ( BSTR* result )                        { return String_to_bstr( name(), result ); }
    STDMETHODIMP            put_Max_non_exclusive           ( int );
    STDMETHODIMP            get_Max_non_exclusive           ( int* result )                         { *result = _config._max_non_exclusive;  return S_OK; }
    STDMETHODIMP                Remove                      ();


    //

    Lock_folder*                lock_folder                 () const                                { return typed_folder(); }


    void                    set_dom                         ( const xml::Element_ptr& );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );
  //void                        execute_xml                 ( const xml::Element_ptr&, const Show_what& );

    void                    set_max_non_exclusive           ( int );
    void                        register_lock_use           ( Use* lock_use )                       { _use_set.insert( lock_use ); }
    void                        unregister_lock_use         ( Use* lock_use )                       { _use_set.erase( lock_use ); }
    bool                        require_lock_for            ( Holder*, Use* );                      // false, falls Holder die Sperre mit einem anderen Use schon hält
    bool                        release_lock_for            ( Holder*, Use* );                      // false, falls Holder die Sperre mit einem anderen Use weiterhin hält
    void                        notify_waiting_requestor    ();
  //bool                        is_held_by                  ( Holder*, Lock_mode );
    bool                        is_held_by                  ( Holder*, Use* );
    int                         enqueue_lock_use            ( Use* );
    void                        dequeue_lock_use            ( Use* );
    int                         count_non_exclusive_holders () const;
    bool                        its_my_turn                 ( const Use*, Holder* ) const;
    bool                        is_free_for                 ( const Use*, Holder* ) const;
    bool                        is_free                     () const;
    string                      string_from_holders         () const;
    string                      string_from_uses            () const;


  private:
    Fill_zero                  _zero_;

    struct Configuration
    {
        int                    _max_non_exclusive;
    };

    Configuration              _config;


    Lock_mode                  _lock_mode;                  // Nur gültig, wenn !_holder_set.empty()
    State                      _state;

    typedef stdext::hash_set<Use*>             Use_set;
    typedef stdext::hash_map<Holder*,Use_set>  Holder_map;  
    Holder_map                 _holder_map;                 // Derselbe Holder kann dieselbe Sperre mehrmals halten, je mit einem Use

    typedef list<Use*>          Use_list;
    vector<Use_list>           _waiting_queues;             // Index: Lock_mode, eine Liste für lk_non_exclusive und eine für lk_exclusive

    Use_set                    _use_set;


    static Class_descriptor     class_descriptor;
    static const Com_method     _methods[];
};

//--------------------------------------------------------------------------------------Lock_folder

struct Lock_folder : typed_folder< Lock >
{
                                Lock_folder                 ( Folder* );
                               ~Lock_folder                 ();


  //void                        set_dom                     ( const xml::Element_ptr& );
  //void                        execute_xml_lock            ( const xml::Element_ptr& );
    void                        add_lock                    ( Lock* lock )                          { add_file_based( lock ); }
    void                        remove_lock                 ( Lock* lock )                          { remove_file_based( lock ); }
    Lock*                       lock                        ( const string& name )                  { return file_based( name ); }
    Lock*                       lock_or_null                ( const string& name )                  { return file_based_or_null( name ); }
  //xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );
    xml::Element_ptr            new_dom_element             ( const xml::Document_ptr& doc, const Show_what& ) { return doc.createElement( "locks" ); }
};

//----------------------------------------------------------------------------------------------Use
// Verbindet Lock mit Requestor

struct Use : Object, 
             Dependant,
             Abstract_scheduler_object, 
             Non_cloneable
{
                                Use                         ( Requestor*, const Absolute_path& lock_path = Absolute_path(), Lock::Lock_mode = Lock::lk_exclusive );
                               ~Use                         ();

    void                        close                       ();
    void                        initialize                  ();
    void                        load                        ();

    void                    set_dom                         ( const xml::Element_ptr& );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );

    void                    set_folder_path                 ( const Absolute_path& p )              { _folder_path = p; }


    // Dependant:
    bool                        on_requisite_loaded         ( File_based* );
    Prefix_log*                 log                         ()                                      { return Abstract_scheduler_object::log(); }

    Lock*                       lock                        () const;
    Lock*                       lock_or_null                () const;
    Requestor*                  requestor                   () const                                { return _requestor; }
    Absolute_path               lock_path                   () const                                { return _lock_path; }
    string                      lock_normalized_path        () const;
    Lock::Lock_mode             lock_mode                   () const                                { return _lock_mode; }

    string                      obj_name                    () const;

  private:
    Fill_zero                  _zero_;
    Absolute_path              _folder_path;
    Absolute_path              _lock_path;
    Lock::Lock_mode            _lock_mode;
    Requestor* const           _requestor;
};

//----------------------------------------------------------------------------------------Requestor

struct Requestor : Object, 
                   Abstract_scheduler_object, 
                   Non_cloneable
{
                                Requestor                   ( Abstract_scheduler_object* );
                               ~Requestor                   ();

    void                        close                       ();

    void                    set_dom                         ( const xml::Element_ptr& );            // Für <lock.use>, Use
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& );

    void                    set_folder_path                 ( const Absolute_path& p )              { _folder_path = p; }
    Absolute_path               folder_path                 () const                                { return _folder_path; }
    
    Use*                        add_lock_use                ( const Absolute_path& lock_path, Lock::Lock_mode );
    Use*                        lock_use_or_null            ( const Absolute_path& lock_path, Lock::Lock_mode );

    void                        initialize                  ();
    void                        load                        ();

    bool                        is_enqueued                 () const                                { return _is_enqueued; }
    bool                        locks_are_known             () const;
    bool                        locks_are_available_for_holder( Holder* ) const;
    virtual bool                locks_are_available         () const                                { return locks_are_available_for_holder( (Holder*)NULL ); }
    bool                        enqueue_lock_requests       ( Holder* );
    void                        dequeue_lock_requests       ( Log_level = log_debug3 );
    Abstract_scheduler_object*           object                      () const                                { return _object; }

  //void                        on_new_lock                 ( Lock* );
    virtual void                on_locks_are_available      ()                                      = 0;
  //virtual void                on_removing_lock            ( lock::Lock* )                         = 0;

    string                      obj_name                    () const;

  private:
    Fill_zero                  _zero_;
    Abstract_scheduler_object*          _object;
    bool                       _is_enqueued;
    Absolute_path              _folder_path;

  public:
    typedef list< ptr<Use> >    Use_list;
    Use_list                   _use_list;
}; 

//-------------------------------------------------------------------------------------------Holder
    
struct Holder : Object, Abstract_scheduler_object, Non_cloneable
{
                                Holder                      ( Abstract_scheduler_object* );
    virtual                    ~Holder                      ();

    void                        close                       ();

  //const Requestor*            static_requestor            ()                                      { return *_requestor_list.begin(); }
    void                        add_requestor               ( const Requestor* );
    void                        remove_requestor            ( const Requestor* );
    bool                        is_known_requestor          ( const Requestor* );
  //bool                        is_holding_requestor        ( const Requestor* );
    void                        hold_locks                  ( const Requestor* );
    void                        hold_lock                   ( Use* );
    bool                        try_hold                    ( Use* );
    void                        release_locks               ();
    void                        release_locks               ( const Requestor* );
    bool                        is_holding_all_of           ( const Requestor* );                   // Alle Sperren haltend?
    bool                        is_holding_none_of          ( const Requestor* );
    Abstract_scheduler_object*           object                      () const                                { return _object; }

    string                      obj_name                    () const;

  private:
    Fill_zero                  _zero_;
  //bool                       _is_holding;
    set<const Requestor*>      _requestor_set;              // 1) <lock.use>  2) Per API für die Task  3) Per API für spooler_process()
  //set<const Requestor*>      _holding_requestor_set;      // Die Requestor, deren Sperren gehalten werden (belegt sind)

  protected:
    Abstract_scheduler_object*          _object;                     // Task
};

//-----------------------------------------------------------------------------------Lock_subsystem

struct Lock_subsystem : idispatch_implementation< Lock_subsystem, spooler_com::Ilocks>, 
                        file_based_subsystem< Lock >,
                        javabridge::has_proxy<Lock_subsystem>
{
                                Lock_subsystem              ( Scheduler* );

    // Subsystem

    void                        close                       ();
    bool                        subsystem_initialize        ();
    bool                        subsystem_load              ();
    bool                        subsystem_activate          ();

    jobject                     java_sister                 ()                                      { return javabridge::has_proxy<Lock_subsystem>::java_sister(); }


    // File_based_subsystem

    string                      object_type_name            () const                                { return "Lock"; }
    string                      filename_extension          () const                                { return ".lock.xml"; }
    string                      xml_element_name            () const                                { return "lock"; }
    string                      xml_elements_name           () const                                { return "locks"; }
  //string                      normalized_name             ( const string& name ) const            { return name; }
    ptr<Lock>                   new_file_based              (const string& source);
    xml::Element_ptr            new_file_baseds_dom_element ( const xml::Document_ptr& doc, const Show_what& ) { return doc.createElement( "locks" ); }


    ptr<Lock_folder>            new_lock_folder             ( Folder* folder )                      { return Z_NEW( Lock_folder( folder ) ); }

    // Ilocks

    STDMETHODIMP            get_Java_class_name             ( BSTR* result )                        { return String_to_bstr( const_java_class_name(), result ); }
    STDMETHODIMP_(char*)  const_java_class_name             ()                                      { return (char*)"sos.spooler.Locks"; }
    STDMETHODIMP            get_Lock                        ( BSTR, spooler_com::Ilock** );
    STDMETHODIMP            get_Lock_or_null                ( BSTR, spooler_com::Ilock** );
    STDMETHODIMP                Create_lock                 ( spooler_com::Ilock** );
    STDMETHODIMP                Add_lock                    ( spooler_com::Ilock* );



    Lock*                       lock                        ( const Absolute_path& path ) const     { return file_based( path ); }
    Lock*                       lock_or_null                ( const Absolute_path& path ) const     { return file_based_or_null( path ); }

  //xml::Element_ptr            execute_xml                 ( Command_processor*, const xml::Element_ptr&, const Show_what& );

  private:
    static Class_descriptor     class_descriptor;
    static const Com_method     _methods[];
};

//-------------------------------------------------------------------------------------------------

} //namespace lock
} //namespace scheduler
} //namespace sos

#endif
