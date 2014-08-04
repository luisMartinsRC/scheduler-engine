// $Id: spooler_task.h 14660 2011-06-23 12:04:10Z jz $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

#ifndef __SPOOLER_TASK_H
#define __SPOOLER_TASK_H


namespace sos {
namespace scheduler {


struct Task_lock_requestor;

namespace job {
    struct Task_closed_call;
    struct Task_starting_completed_call;
    struct Task_opening_completed_call;
    struct Task_step_completed_call;
    struct Try_next_step_call;
    struct Task_end_completed_call;
    struct Next_spooler_process_call;
    struct Next_order_step_call;
    struct Task_idle_timeout_call;
    struct Task_end_with_period_call;
    struct Task_locks_are_available_call;
    struct Task_check_for_order_call;
    struct Task_on_success_completed_call;
    struct Task_on_error_completed_call;
    struct Task_exit_completed_call;
    struct Task_release_completed_call;
    struct Task_wait_for_subprocesses_completed_call;
    struct Task_ended_completed_call;
    DEFINE_SIMPLE_CALL(Task, Task_process_ended_call);
    struct Remote_task_running_call;
    struct End_task_call;
    struct Warn_longer_than_call;
    struct Task_timeout_call;
    struct Subprocess_timeout_call;
    struct Try_deleting_files_call;
    struct Killing_task_call;
}

//--------------------------------------------------------------------------------------Start_cause

enum Start_cause
{
    cause_none                  = 0,    // Noch nicht gestartet
    cause_period_once           = 1,    // <schedule once="yes">
    cause_period_single         = 2,    // <schedule single_start="yes">
    cause_period_repeat         = 3,    // <schedule repeat="..">
    cause_queue                 = 5,    // <start_job at="">
    cause_queue_at              = 6,    // <start_job at="..">
    cause_directory             = 7,    // start_when_directory_changed
    cause_signal                = 8,
    cause_delay_after_error     = 9,
    cause_order                 = 10,
    cause_wake                  = 11,   // sc_wake oder sc_wake_when_in_period
    cause_min_tasks             = 12
};

string                          start_cause_name            ( Start_cause );

//----------------------------------------------------------------------------------------------Task

struct Task : Object, 
              Scheduler_object,
              javabridge::has_proxy<Task>
{
    enum State
    {
        s_none,

        s_loading,
        s_waiting_for_process,  // Prozess aus Prozessklasse wählen, evtl. warten, bis ein Prozess verfügbar ist.
        s_starting,             // load, spooler_init, spooler_open
                                // Bis hier gilt Task::starting() == true
        s_opening,              // spooler_open()
        s_opening_waiting_for_locks, // spooler_open() usw.: wegen try_hold_lock() erneut aufrufen
        s_running,              // Läuft (wenn _in_step, dann in step__start() und step__end() muss gerufen werden)
        s_running_delayed,      // spooler_task.delay_spooler_process gesetzt
        s_running_waiting_for_locks, // spooler_process(): wegen try_hold_lock() erneut aufrufen
        s_running_waiting_for_order,
        s_running_process,      // Läuft in einem externen Prozess, auf dessen Ende nur gewartet wird
        s_running_remote_process,   // Prozess, der über remote Scheduler läuft (über Remote_module_instance_proxy)

        s_suspended,            // Angehalten
                                // Ab hier gilt Task::ending() == true
        s_ending,               // spooler_close,  
        s_ending_waiting_for_subprocesses, // nach spooler_close, Ende der Subprozesse abwarten
        s_on_success,           // spooler_on_success
        s_on_error,             // spooler_on_error
        s_exit,                 // spooler_exit
        s_release,              // Release()
        s_killing,
        s_ended,                // 
        s_deleting_files,
        s_closed,               // close() gerufen, Object Task ist nicht mehr zu gebrauchen
        s__max
    };


    enum Call_state
    {
        c_none,
        c_begin,
        c_step,
        c_end,
    };


    enum Lock_level 
    {
        lock_level_task_api,        // Task.try_hold_lock() in spooler_open() oder spooler_init(), gilt für die Task
        lock_level_process_api,     // Task.try_hold_lock() in spooler_process(), gilt für nur für spooler_process()
        lock_level__max = lock_level_process_api
    };


    struct Registered_pid : z::Object, Non_cloneable
    {
                                Registered_pid              ( Task*, int pid, const Time& timeout, bool wait, bool ignore_error, bool ignore_signal, 
                                                              bool is_process_group, const string& title );
                               ~Registered_pid              ()                                      { close(); }

        void                    close                       ();
        void                    try_kill                    ();


        Spooler*               _spooler;
        Task* const            _task;
        int                    _pid;
        Time                   _timeout_at;
        bool                   _wait;                       // Auf Ende der Task warten und Exitcode und Signal auswerten (-> Task-Error)
        bool                   _ignore_error;
        bool                   _ignore_signal;
        bool                   _is_process_group;           // kill auf die ganze Prozessgruppe (Unix)
        string                 _title;                      // Kann die Kommandozeile sein
        bool                   _killed;
    };


    typedef stdext::hash_map< int, ptr<Registered_pid> >  Registered_pids;



                                Task                        ( Standard_job* );
                               ~Task                        ();

    // Scheduler_object::
    void                        write_element_attributes    ( const xml::Element_ptr& element ) const;


    int                         id                          () const                                { return _id; }

    enum End_mode { end_none = 0, end_normal, end_nice, end_kill_immediately };
    void                        cmd_end                     ( End_mode = end_normal );
    void                        cmd_nice_end                ( Job* for_job = NULL );

    void                        close                       ();
    void                        job_close                   ();                                     // Setzt _job = NULL
    void                        move_to_new_job             ( Standard_job* new_job )               { assert( _state == s_none );  _job = new_job; }
    void                    set_dom                         ( const xml::Element_ptr& );
    xml::Element_ptr            dom_element                 ( const xml::Document_ptr&, const Show_what& ) const;
    xml::Document_ptr           dom                         ( const Show_what& ) const;

    State                       state                       () const                                { return _state; }
    void                        init                        ();
    bool                        do_something                ();

    Standard_job*               job                         ();
    Time                        calculated_start_time       ( const Time& now );
    void                        on_remote_task_running      ();
    void                        on_call                     (const job::Task_starting_completed_call&);
    void                        on_call                     (const job::Task_opening_completed_call&);
    void                        on_call                     (const job::Task_step_completed_call& );
    void                        on_call                     (const job::Next_spooler_process_call& );
    void                        on_call                     (const job::Try_next_step_call& );
    void                        on_call                     (const job::Next_order_step_call& );
    void                        on_call                     (const job::Task_end_completed_call&);
    void                        on_call                     (const job::Task_idle_timeout_call&);
    void                        on_call                     (const job::Remote_task_running_call&);
    void                        on_call                     (const job::Task_end_with_period_call&);
    void                        on_call                     (const job::Task_locks_are_available_call&);
    void                        on_call                     (const job::Task_check_for_order_call&);
    void                        on_call                     (const job::Task_on_success_completed_call&);
    void                        on_call                     (const job::Task_on_error_completed_call&);
    void                        on_call                     (const job::Task_exit_completed_call&);
    void                        on_call                     (const job::Task_release_completed_call&);
    void                        on_call                     (const job::Task_process_ended_call&);
    void                        on_call                     (const job::Task_ended_completed_call&);
    void                        on_call                     (const job::Task_wait_for_subprocesses_completed_call&);
    void                        on_call                     (const job::End_task_call&);
    void                        on_call                     (const job::Warn_longer_than_call&);
    void                        on_call                     (const job::Task_timeout_call&);
    void                        on_call                     (const job::Subprocess_timeout_call&);
    void                        on_call                     (const job::Try_deleting_files_call&);
    void                        on_call                     (const job::Killing_task_call&);

    Task_subsystem*             thread                      ()                                      { return _thread; }
    string                      name                        () const                                { return obj_name(); }
    virtual string              obj_name                    () const                                { return _obj_name; }

    string                      state_name                  () const                                { return state_name( state() ); }
    static string               state_name                  ( State );
    State                       state                       ()                                      { return _state; }
    bool                        starting                    ()                                      { return _state > s_none  &&  _state <= s_starting; }
    bool                        ending                      ()                                      { return _end  ||  _state >= s_ending; }
    bool                        is_idle                     ()                                      { return _state == s_running_waiting_for_order  &&  !_end; }

    bool                        running_state_reached       () const                                { return _running_state_reached; }
    Time                        last_process_start_time     ()                                      { return _last_process_start_time; }
    Time                        ending_since                ()                                      { return _ending_since; }

    void                        merge_params                ( const Com_variable_set* p )           { _params->merge( p ); }
    ptr<Com_variable_set>       params                      ()                                      { return _params; }

    void                        merge_environment           ( const Com_variable_set* e )           { _environment->merge( e ); }
    Com_variable_set*           environment_or_null         () const                                { return _environment; }

    bool                        has_error                   ()                                      { return _error != NULL; }
    void                    set_error_xc_only               ( const zschimmer::Xc& );
    void                    set_error_xc_only               ( const Xc& );
    void                    set_error_xc_only_base          ( const Xc& );

    void                    set_exit_code                   ( int exit_code )                       { _exit_code = exit_code; }
    int                         exit_code                   ()                                      { return _exit_code; }

    void                    set_web_service                 ( Web_service* );
    void                    set_web_service                 ( const string& name );
    Web_service*                web_service                 () const;
    Web_service*                web_service_or_null         () const                                { return _web_service; }

    Order*                      fetch_and_occupy_order      ( const Time& now, const string& cause );
    void                        postprocess_order           ( Order::Order_state_transition, bool due_to_exception = false );

    void                        add_pid                     ( int pid, const Duration& timeout = Duration::eternal);
    void                        remove_pid                  ( int pid );
    void                        add_subprocess              ( int pid, const Duration& timeout, bool ignore_error, bool ignore_signal, bool is_process_group, const string& title );
    void                        set_subprocess_timeout      ();
    bool                        check_subprocess_timeout    ( const Time& now );
    bool                        shall_wait_for_registered_pid();
    string                      trigger_files               () const                                { return _trigger_files; }
    Order*                      order                       ()                                      { return _order; }
    pid_t                       pid                         () const                                { return _module_instance? _module_instance->pid() : 0; }
    Xc_copy                     error                       ()                                      { return _error; }
    bool                        force                       () const                                { return _force_start; }
    const Time&                 at                          () const                                { return _start_at; }
    const string                log_string                  () const                                { return log()->as_string(); }
    const File_path             stdout_path                 () const                                { return _module_instance->stdout_path(); }
    const File_path             stderr_path                 () const                                { return _module_instance->stderr_path(); }    

  protected:
    friend struct               Stdout_reader;
    friend struct               Task_lock_requestor;

    void                        detach_order_after_error    ();
    void                        detach_order                ();

    void                        finish                      ();
    void                        fetch_order_parameters_from_process();
    void                        set_state_texts_from_stdout ( const string& );
    void                        process_on_exit_commands    ();
    bool                        load                        ();
    Async_operation*            begin__start                ();
    bool                        step__end                   ();
    void                        count_step                  ();
    string                      remote_process_step__end    ();
    bool                        operation__end              ();
    void                        close_operation             ();

    void                        set_mail_defaults           ();
    void                        trigger_event               ( Scheduler_event* );
    void                        send_collected_log          ();

    void                        set_cause                   ( Start_cause );
    Start_cause                 cause                       () const                                { return _cause; }

    void                        set_history_field           ( const string& name, const Variant& value );
    bool                        has_parameters              ();
    xml::Document_ptr           parameters_as_dom           ()                                      { return _params->dom( Com_variable_set::xml_element_name(), "variable" ); }


    void                        wake_when_longer_than       ();
    bool                        check_timeout               ( const Time& now );
    void                        check_if_shorter_than       ( const Time& now );
    bool                        check_if_longer_than        ( const Time& now );
    bool                        try_kill                    ();
    
    bool                        wait_until_terminated       ( double wait_time = time::never_double );
    void                        set_delay_spooler_process   (const Duration&);

    bool                        try_hold_lock               ( const Path& lock_path, lock::Lock::Lock_mode = lock::Lock::lk_exclusive );
    void                        delay_until_locks_available ();
    void                        on_locks_are_available      ( Task_lock_requestor* );
    Lock_level                  current_lock_level          ();

    void                        set_state                   ( State );

    void                        set_error_xc                ( const Xc& );
    void                        set_error_xc                ( const z::Xc& );
    void                        set_error                   ( const Xc& x )                         { set_error_xc( x ); }
    void                        set_error                   ( const z::Xc& x )                      { set_error_xc( x ); }
    void                        set_error                   ( const exception& );
    void                        set_error                   ( const _com_error& );
    void                        reset_error                 ()                                      { _error = NULL,  _non_connection_reset_error = NULL,  _is_connection_reset_error = false,  _log->reset_highest_level(); }
  
    friend struct               Standard_job;
    friend struct               Standard_job::Task_queue;
    friend struct               com_objects::Com_task;
    friend struct               database::Task_history;

    bool                        do_kill                     ();
    bool                        do_load                     ();
    bool                        do_begin__end               ();
    Async_operation*            do_end__start               ();
    void                        do_end__end                 ();
    Async_operation*            do_step__start              ();
    Variant                     do_step__end                ();
    Async_operation*            do_call__start              ( const string& method );
    bool                        do_call__end                ();
    Async_operation*            do_release__start           ();
    void                        do_release__end             ();

    Async_operation*            do_close__start             ();
    void                        do_close__end               ();

    bool                        loaded                      ()                                      { return _module_instance && _module_instance->loaded(); }
    virtual bool                has_step_count              ()                                      { return true; }

 private:
    void                        set_enqueued_state          ();
    void                        set_state_direct            ( State );
    Host_and_port               read_remote_scheduler_parameter();

    void register_task_timeout_call();
    void unregister_task_timeout_call();

 protected:
    Fill_zero                  _zero_;

    Z_DEBUG_ONLY( string       _job_name; )
    int                        _id;
    State                      _state;
    State                      _enqueued_state;
    string                     _obj_name;

    Standard_job*              _job;
    Task_subsystem*            _thread;
    Task_history               _history;
    typed_call_register<Task>  _call_register;
    ptr<Async_operation>       _sync_operation;

    Thread_semaphore           _terminated_events_lock;
    vector<Event*>             _terminated_events;

    Start_cause                _cause;
    double                     _cpu_time;
    int                        _step_count;
    int                        _exit_code;

    bool                       _let_run;                    // Task zuende laufen lassen, nicht bei _job._period.end() beenden
    bool                       _begin_called;
    End_mode                   _end;
    bool                       _scheduler_815_logged;
    bool                       _closed;
    int                        _delayed_after_error_task_id;

    Time                       _enqueue_time;
    bool                       _force_start;                // Auch um _start_at starten, wenn gerade keine <run_time>-Periode vorliegt
    Time                       _start_at;                   // Zu diesem Zeitpunkt (oder danach) starten. 
    Time                       _running_since;
    Time                       _ending_since;
    Time                       _last_process_start_time;
    Time                       _last_operation_time;
    Time                       _next_spooler_process;
    Duration                   _timeout;                    // Frist für eine Operation (oder INT_MAX)
    Time                       _last_warn_if_longer_operation_time;
    Time                       _idle_since;
    Time                       _idle_timeout_at;
    Time                       _subprocess_timeout;
    Duration                   _warn_if_longer_than;
    Duration                   _warn_if_shorter_than;
    Time                       _trying_deleting_files_until;

    bool                       _killed;                     // Task abgebrochen (nach do_kill/timeout)
    bool                       _kill_tried;
    bool                       _module_instance_async_error;    // SCHEDULER-202
    bool                       _is_in_database;             // Datensatz für diese Task ist in der Datenbank
    bool                       _running_state_reached;      // Zustand s_running... erreicht
    bool                       _is_first_job_delay_after_error;
    bool                       _is_last_job_delay_after_error;
    bool                       _move_order_to_error_state;
    bool                       _delay_until_locks_available;
    Order::Order_state_transition _order_state_transition;

    ptr<Async_operation>       _operation;
    ptr<Com_variable_set>      _params;
    ptr<Com_variable_set>      _environment;
    Variant                    _result;
    string                     _name;
    ptr<Order>                 _order;
    ptr<Order>                 _order_for_task_end;         // Wird später als _order auf NULL gesetzt, damit im Fehlerfall XSLT-eMail mit <order> verschickt wird. Und für <commands on_exit_code=""/>
    string                     _changed_directories;        // Durch Semikolon getrennt
    string                     _trigger_files;              // Durch Semikolon getrennt

    Registered_pids            _registered_pids;            // Für add_pid() und remove_pid(). kill_task immediately_yes soll auch diese Prozesse abbrechen.
    Subprocess_register        _subprocess_register;        // Fall Task im Scheduler-Prozess läuft
    Call_state                 _call_state;
    Xc_copy                    _error;
    Xc_copy                    _non_connection_reset_error;  // Der Fehler vor einem Verbindungsfehler wird hier aufgehoben, falls er wegen ignore_signals=".." wiederhergestellt werden muss
    bool                       _is_connection_reset_error;

    ptr<Module_instance>       _module_instance;            // Nur für Module_task. Hier, damit wir nicht immer wieder casten müssen.
    ptr<Web_service>           _web_service;
    vector< ptr<lock::Requestor> > _lock_requestors;        // Nur für log_level_task_api und log_level_process_api
    ptr<lock::Holder>          _lock_holder;

    ptr<File_logger>           _file_logger;                // Übernimmt kontinuierlich stdout und stderr ins Protokoll
};

//----------------------------------------------------------------------------------------Task_list

typedef list< ptr<Task> >   Task_list;
typedef stdext::hash_set< ptr<Task> >   Task_set;

#define FOR_EACH_TASK( ITERATOR, TASK )  FOR_EACH( Task_set, _task_set, ITERATOR )  if( Task* TASK = *ITERATOR )
#define FOR_EACH_TASK_CONST( ITERATOR, TASK )  FOR_EACH_CONST( Task_set, _task_set, ITERATOR )  if( Task* TASK = *ITERATOR )

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos

#endif
