// $Id: spooler_order_file.cxx 14125 2010-11-04 12:32:04Z jz $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com

/*
    VERBESSERUNG:

    request_order() durch ein Abonnement ersetzen: 
    Job oder Task kann Aufträge abonnieren und das Abonnement auch wieder abbestellen.
    Dann wird das Verzeichnis außerhalb der <schedule/> nicht überwacht.

    Wir brauchen ein Verzeichnis der Abonnementen (struct Job/Task : Order_source_abonnent)
*/


#include "spooler.h"

using stdext::hash_set;
using stdext::hash_map;

namespace sos {
namespace scheduler {
namespace order {

using namespace job_chain;

//--------------------------------------------------------------------------------------------const

const string                    scheduler_file_path_variable_name         = "scheduler_file_path";
const Absolute_path             file_order_sink_job_path                  ( "/scheduler_file_order_sink" );
const int                       delay_after_error_default                 = INT_MAX;
const Duration                  file_order_sink_job_idle_timeout_default  = Duration(60);
const int                       directory_file_order_source_max_default   = 100;      // Nicht zuviele Aufträge, sonst wird der Scheduler langsam (in remove_order?)
const int                       max_tries                                 = 2;        // Nach Fehler machen wie sofort einen zweiten Versuch
const bool                      alert_when_directory_missing_default      = true;

#ifdef Z_WINDOWS
    const int   directory_file_order_source_repeat_default  = 60;
#else
    const int   directory_file_order_source_repeat_default  = 10;
#endif

//-------------------------------------------------------------------------------------------static

//Directory_file_order_source::Class_descriptor    Directory_file_order_source::class_descriptor ( &typelib, "Spooler.Directory_file_order_source", Directory_file_order_source::_methods );

//----------------------------------------------------------------------Directory_file_order_source

struct Directory_file_order_source : Directory_file_order_source_interface
{
    enum State
    {
        s_none,
        s_order_requested
    };


    explicit                    Directory_file_order_source( Job_chain*, const xml::Element_ptr& );
                               ~Directory_file_order_source();

    // Async_operation:
    virtual Socket_event*       async_event             ()                                          { return &_notification_event; }
    virtual bool                async_continue_         ( Continue_flags );
    virtual bool                async_finished_         () const                                    { return false; }
    virtual string              async_state_text_       () const;

    // Order_source:
    void                        close                   ();
    xml::Element_ptr            dom_element             ( const xml::Document_ptr&, const Show_what& );
    void                        initialize              ();
    void                        activate                ();
    bool                        request_order           ( const string& cause );
    Order*                      fetch_and_occupy_order  (const Order::State&, Task* occupying_task, const Time& now, const string& cause);
    void                        withdraw_order_request  ();
    string                      obj_name                () const;


  private:
    void                        send_mail               ( Scheduler_event_type, const exception* );
    void                        start_or_continue_notification( bool was_notified );
    void                        close_notification      ();
    void                        read_directory          ( bool was_notified, const string& cause );
    void                        read_new_files_and_handle_deleted_files( const string& cause );
    bool                        read_new_files          ();
    bool                        clean_up_blacklisted_files();
    Duration                    delay_after_error       ();
    void                        clear_new_files         ();
    void                        read_known_orders       ( String_set* known_orders );

    Fill_zero                  _zero_;
    File_path                  _path;
    string                     _regex_string;
    Regex                      _regex;
    Duration                   _delay_after_error;
    Duration                   _repeat;
    bool                       _expecting_request_order;
    Xc_copy                    _directory_error;
    bool                       _send_recovered_mail;
    Event                      _notification_event;             // Nur Windows
    Time                       _notification_event_time;        // Wann wir zuletzt die Benachrichtigung bestellt haben
    int                        _max_orders;
    bool                       _alert_when_directory_missing;

    vector< ptr<zschimmer::file::File_info> > _new_files;
    int                        _new_files_index;
    int                        _new_files_count;        // _new_files.size() ohne NULL-Einträge
    Time                       _new_files_time;


    struct Bad_entry : Object
    {
                                Bad_entry               ( const File_path& p, const exception& x )  : _file_path(p), _error(x) {}

        File_path              _file_path;
        Xc_copy                _error;
    };

    typedef stdext::hash_map< string, ptr<Bad_entry> >  Bad_map;
    Bad_map                    _bad_map;

    bool                       _are_blacklisted_orders_cleaned_up;
};

//------------------------------------------------------------------File_order_sink_module_instance

struct File_order_sink_module_instance : Internal_module_instance
{
    File_order_sink_module_instance( Module* m ) 
    : 
        Internal_module_instance( m ) 
    {
    }



    bool spooler_process()
    {
        bool   result = false;
        Order* order  = _task->order();

        if( !order )  return false;         // Fehler

        File_path path = string_from_variant( order->param( scheduler_file_path_variable_name ) );
        if( path == "" )
        {
            _log->warn( message_string( "SCHEDULER-343", order->obj_name() ) );
            result = false;
        }
        else
        {
            Sink_node* sink_node = Sink_node::cast( order->job_chain_node() );
            if( !sink_node )  assert(0), z::throw_xc( Z_FUNCTION );

            if( !path.file_exists() )
            {
                _log->warn( message_string( "SCHEDULER-339", path ) );
                result = false;
            }
            else
            {
                try
                {
                    if( sink_node->file_order_sink_move_to() != "" )
                    {
                        order->log()->info( message_string( "SCHEDULER-980", path, sink_node->file_order_sink_move_to() ) );

                        path.move_to( sink_node->file_order_sink_move_to() );

                        result = true;
                    }
                    else
                    if( sink_node->file_order_sink_remove() )
                    {
                        order->log()->info( message_string( "SCHEDULER-979", path ) );

                        path.unlink();

                        result = true;
                    }
                    else
                        assert(0), z::throw_xc( Z_FUNCTION );
                }
                catch( exception& x )
                {
                    order->log()->warn( x.what() );     // Nicht error(), damit der Job nicht stoppt
                }

                if( result == false )  order->set_on_blacklist();
            }
        }

        order->set_end_state_reached();

        return result;
    }
};

//---------------------------------------------------------------------------File_order_sink_module

struct File_order_sink_module : Internal_module
{
    File_order_sink_module( Spooler* spooler )
    :
        Internal_module( spooler )
    {
    }


    ptr<Module_instance> create_instance_impl(const string& remote_scheduler)
    { 
        ptr<File_order_sink_module_instance> result = Z_NEW( File_order_sink_module_instance( this ) );  
        return +result;
    }
};

//------------------------------------------------------------------------------File_order_sink_job

struct File_order_sink_job : Internal_job
{
    File_order_sink_job( Scheduler* scheduler )
    :
        Internal_job( scheduler, file_order_sink_job_path.without_slash(), +Z_NEW( File_order_sink_module( scheduler ) ) )
    {
    }
};

//-----------------------------------------------------------------------------init_file_order_sink

void init_file_order_sink( Scheduler* scheduler )
{
    ptr<File_order_sink_job> file_order_sink_job = Z_NEW( File_order_sink_job( scheduler ) );

    file_order_sink_job->set_visible( visible_no );
    file_order_sink_job->set_order_controlled();
    file_order_sink_job->set_idle_timeout( file_order_sink_job_idle_timeout_default );

    scheduler->root_folder()->job_folder()->add_job( +file_order_sink_job );

    // Der Scheduler führt Tasks des Jobs scheduler_file_order_sink in jedem Scheduler-Schritt aus,
    // damit sich die Aufträge nicht stauen (Der interne Job läuft nicht in einem eigenen Prozess)
    // Siehe Task_subsystem::step().
}

//------------------------------------------------------------------new_directory_file_order_source

ptr<Directory_file_order_source_interface> new_directory_file_order_source( Job_chain* job_chain, const xml::Element_ptr& element )
{
    ptr<Directory_file_order_source> result = Z_NEW( Directory_file_order_source( job_chain, element ) );
    return +result;
}

//-----------------------------------------Directory_file_order_source::Directory_file_order_source

Directory_file_order_source::Directory_file_order_source( Job_chain* job_chain, const xml::Element_ptr& element )
:
    //Idispatch_implementation( &class_descriptor ),
    Directory_file_order_source_interface( job_chain, type_directory_file_order_source ),
    _zero_(this+1),
    _delay_after_error(delay_after_error_default),
    _repeat(directory_file_order_source_repeat_default),
    _max_orders(directory_file_order_source_max_default),
    _alert_when_directory_missing(alert_when_directory_missing_default)
{
    _path = subst_env( element.getAttribute( "directory" ) );

    _regex_string = element.getAttribute( "regex" );
    if( _regex_string != "" )
    {
        _regex.compile( _regex_string );
    }

    _delay_after_error = Duration(element.int_getAttribute( "delay_after_error", int_cast(_delay_after_error.seconds())));

    if( element.getAttribute( "repeat" ) == "no" )  _repeat = Duration::eternal;
                                              else  _repeat = Duration(element.int_getAttribute( "repeat", int_cast(_repeat.seconds())));

    _max_orders = element.int_getAttribute( "max", _max_orders );
    _next_state = normalized_state( element.getAttribute( "next_state", _next_state.as_string() ) );
    _alert_when_directory_missing = element.bool_getAttribute( "alert_when_directory_missing", _alert_when_directory_missing );
}

//----------------------------------------Directory_file_order_source::~Directory_file_order_source
    
Directory_file_order_source::~Directory_file_order_source()
{
    //if( _spooler->_connection_manager )  _spooler->_connection_manager->remove_operation( this );

    close();
}

//---------------------------------------------------------------Directory_file_order_source::close

void Directory_file_order_source::close()
{
    close_notification();

    if( _next_node ) 
    {
        _next_node->unregister_order_source( this );
        _next_node = NULL;
    }

    _job_chain = NULL;   // close() wird von ~Job_chain gerufen, also kann Job_chain ungültig sein
}

//-------------------------------------------------------------xml::Element_ptr Node::xml

xml::Element_ptr Directory_file_order_source::dom_element( const xml::Document_ptr& document, const Show_what& show )
{
    xml::Element_ptr element = document.createElement( "file_order_source" );

                                             element.setAttribute         ( "directory" , _path );
                                             element.setAttribute_optional( "regex"     , _regex_string );
        if( _notification_event._signaled )  element.setAttribute         ( "signaled"  , "yes" );
        if( _max_orders < INT_MAX )          element.setAttribute         ( "max"       , _max_orders );
        if( !_next_state.is_missing() )      element.setAttribute         ( "next_state", debug_string_from_variant( _next_state ) );

        if (!delay_after_error().is_eternal())  element.setAttribute( "delay_after_error", delay_after_error().seconds() );
        if (!_repeat.is_eternal())              element.setAttribute( "repeat"           , _repeat.seconds());
        element.setAttribute( "alert_when_directory_missing", _alert_when_directory_missing );
        
        if( _directory_error )  append_error_element( element, _directory_error );

        if( _new_files_index < _new_files.size() )
        {
            xml::Element_ptr files_element = document.createElement( "files" );
            files_element.setAttribute( "snapshot_time", _new_files_time.xml_value() );
            files_element.setAttribute( "count"        , _new_files_count );

            if( show.is_set( show_order_source_files ) )
            {
                for( int i = _new_files_index, j = show._max_orders; i < _new_files.size() && j > 0; i++, j-- )
                {
                    zschimmer::file::File_info* f = _new_files[ i ];
                    if( f )
                    {
                        xml::Element_ptr file_element = document.createElement( "file" );
                        file_element.setAttribute( "last_write_time", xml_of_time_t(f->last_write_time()));
                        file_element.setAttribute( "path"           , f->path() );

                        files_element.appendChild( file_element );
                    }
                }
            }

            element.appendChild( files_element );
        }

        if( _bad_map.size() > 0 )
        {
            xml::Element_ptr bad_files_element = document.createElement( "bad_files" );
            bad_files_element.setAttribute( "count", (int)_bad_map.size() );

            if( show.is_set( show_order_source_files ) )
            {
                Z_FOR_EACH( Bad_map, _bad_map, it )
                {
                    Bad_entry* bad_entry = it->second;

                    xml::Element_ptr file_element = document.createElement( "file" );
                    file_element.setAttribute( "path", bad_entry->_file_path );
                    append_error_element( file_element, bad_entry->_error );

                    bad_files_element.appendChild( file_element );
                }
            }

            element.appendChild( bad_files_element );
        }

    return element;
}

//---------------------------------------------------Directory_file_order_source::async_state_text_

string Directory_file_order_source::async_state_text_() const
{ 
    S result;
    
    result << "Directory_file_order_source(\"" << _path << "\"";
    if( _regex_string != "" )  result << ",\"" << _regex_string << "\"";
    if( _notification_event.signaled_flag() )  result << ",signaled!";
    result << ")";

    return result;
}

//--------------------------------------Directory_file_order_source::start_or_continue_notification
#ifdef Z_WINDOWS

void Directory_file_order_source::start_or_continue_notification( bool was_notified )
{
    // Windows XP:
    // Ein überwachtes lokales Verzeichnis kann entfernt (rd), aber nicht angelegt (mkdir) werden. Der Name ist gesperrt.
    // Aber ein überwachtes Verzeichnis im Netzwerk kann entfernt und wieder angelegt werden, 
    // ohne dass die Überwachung das mitbekommt. Sie signaliert keine Veränderung im neuen Verzeichnis, ist also nutzlos.
    // Deshalb erneuern wir die Verzeichnisüberwachung, wenn seit _repeat Sekunde kein Signal gekommen ist.

    try {
        if( !_notification_event.handle()  ||  Time::now() >= _notification_event_time + _repeat )
        {
            _notification_event_time = Time::now();

            Z_LOG2( "scheduler.file_order", "FindFirstChangeNotification( \"" << _path.path() << "\", FALSE, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME );\n" );
            HANDLE h = FindFirstChangeNotification( _path.path().c_str(), FALSE, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME );
            if (h == INVALID_HANDLE_VALUE) {
                if (_alert_when_directory_missing)
                    z::throw_mswin( "FindFirstChangeNotification", _path.path() );
            } else {
                if( _notification_event.handle() )      // Signal retten. Eigentlich überflüssig, weil wir hiernach sowieso das Verzeichnis lesen
                {
                    _notification_event.wait( 0 );
                    if( _notification_event.signaled() )      
                    {
                        _notification_event.set_signaled();     
                        Z_LOG2( "scheduler.file_order", Z_FUNCTION << " Old directory watchers signal has been transferd to new watcher.\n" );
                    }

                    close_notification();
                }

                _notification_event.set_handle( h );
                _notification_event.set_name( "FindFirstChangeNotification " + _path );
        
                add_to_event_manager( _spooler->_connection_manager );
            }
        }
        else
        if( was_notified )
        {
            _notification_event_time = Time::now();

            Z_LOG2( "scheduler.file_order", "FindNextChangeNotification(\"" << _path << "\")\n" );
            BOOL ok = FindNextChangeNotification( _notification_event.handle() );
            if( !ok )  throw_mswin_error( "FindNextChangeNotification" );
        }
    } 
    catch (exception &x) {
        log()->debug(message_string("SCHEDULER-724", _path, x.what()));
    }
}

#endif
//--------------------------------------------------Directory_file_order_source::close_notification

void Directory_file_order_source::close_notification()
{
#   ifdef Z_WINDOWS
        if( _notification_event.handle() )
        {
            remove_from_event_manager();
            set_async_manager( _spooler->_connection_manager );   // remove_from_event_manager() für set_async_next_gmtime() rückgängig machen

            Z_LOG2( "scheduler.file_order", "FindCloseChangeNotification(\"" << _path << "\")\n" );
            FindCloseChangeNotification( _notification_event.handle() );
            _notification_event._handle = NULL;   // set_handle() ruft CloseHandle(), das wäre nicht gut
        }
#   endif
}

//----------------------------------------------------------Directory_file_order_source::initialize

void Directory_file_order_source::initialize()
{
    Order_source::initialize();     // Setzt _next_order_queue
    assert( _next_node );

    _next_node->register_order_source( this );
}

//-----------------------------------------------------------Directory_file_order_source::activaate

void Directory_file_order_source::activate()
{
    Job* next_job = NULL;

    if( Job_node* job_node = Job_node::try_cast(_next_node) )
    {
        if( job_node->normalized_job_path() == file_order_sink_job_path )  z::throw_xc( "SCHEDULER-342", _job_chain->obj_name() );
        next_job = job_node->job_or_null();
    }

    _expecting_request_order = true;            // Auf request_order() warten
    set_async_next_gmtime( double_time_max );
    //set_async_next_gmtime( (time_t)0 );     // Am Start das Verzeichnis auslesen

    set_async_manager( _spooler->_connection_manager );


    if( next_job  &&  next_job->state() > Job::s_not_initialized )  
        next_job->on_order_possibly_available();     // Der Job bestellt den nächsten Auftrag (falls in einer Periode)
}

//-------------------------------------------------------Directory_file_order_source::request_order

bool Directory_file_order_source::request_order( const string& cause )
{
    bool result = _new_files_index < _new_files.size();

    if( !result )
    {
        if( _expecting_request_order 
         || async_next_gmtime_reached() )       // 2007-01-09 nicht länger: Das, weil die Jobs bei jeder Gelegenheit do_something() durchlaufen, auch wenn nichts anliegt (z.B. bei TCP-Verkehr)
        {
            Z_LOG2( "scheduler.file_order", Z_FUNCTION << " cause=" << cause << "\n" );

            async_wake();   // Veranlasst Aufruf von async_continue_()

            _expecting_request_order = false;
        }
        else
        {
            //Z_LOG2( "scheduler.file_order", Z_FUNCTION << " cause=" << cause << ", !async_next_gmtime_reached()\n" );
        }
    }

    return result;
}

//----------------------------------------------Directory_file_order_source::withdraw_order_request

void Directory_file_order_source::withdraw_order_request()
{
    Z_LOGI2( "zschimmer", Z_FUNCTION << " " << _path << "\n" );

    _expecting_request_order = true;
    set_async_next_gmtime( double_time_max );
}

//------------------------------------------------------Directory_file_order_source::read_directory

void Directory_file_order_source::read_directory( bool was_notified, const string& )
{
    for( int try_index = 1;; try_index++ )           // Nach einem Fehler machen wir einen zweiten Versuch, bevor wir eine eMail schicken
    {
        try
        {
#           ifdef Z_WINDOWS
                // Verzeichnisüberwachung starten oder fortsetzen,
                // bevor die Dateinamen gelesen werden, damit Änderungen während oder kurz nach dem Lesen bemerkt werden.
                // Das kann ein Ereignis zu viel geben. Aber besser als eins zu wenig.

                start_or_continue_notification( was_notified );
#           endif


            if( _new_files_index == _new_files.size() )     // Noch Dateien im Puffer
            {
                //read_new_files_and_handle_deleted_files( cause );
                read_new_files();
                clean_up_blacklisted_files();
                while( _new_files_index < _new_files.size()  &&  _new_files[ _new_files_index ] == NULL )  _new_files_index++;
            }

            if( _directory_error )
            {
                log()->info( message_string( "SCHEDULER-984", _path ) );

                if( _alert_when_directory_missing && _send_recovered_mail )
                {
                    _send_recovered_mail = false;
                    send_mail( evt_file_order_source_recovered, NULL );
                }

                _directory_error = NULL;
            }
        }
        catch( exception& x )
        {
            clear_new_files();

            if( _directory_error )
            {
                log()->debug( x.what() );      // Nur beim ersten Mal eine Warnung
            }
            else
            {
                if ( _alert_when_directory_missing )
                {
                    log()->warn( x.what() ); 

                    if( _spooler->_mail_on_error )
                    {
                        _send_recovered_mail = true; 
                        send_mail( evt_file_order_source_error, &x );
                    }
                }
                else
                {
                    log()->info( x.what() );
                }
            }

            _directory_error = x;

            close_notification();  // Schließen, sonst kann ein entferntes Verzeichnis nicht wieder angelegt werden (Windows blockiert den Namen)
        }

        break;
    }

    //_expecting_request_order = false;
}

//----------------------------------------------Directory_file_order_source::fetch_and_occupy_order

Order* Directory_file_order_source::fetch_and_occupy_order(const Order::State& fetching_state, Task* occupying_task, const Time& now, const string& cause)
{
    Order* result = NULL;

    String_set  known_orders;
    bool        known_orders_has_been_read = false;


    while( !result  &&  _new_files_index < _new_files.size() )
    {
        if( zschimmer::file::File_info* new_file = _new_files[ _new_files_index ] )
        {
            File_path  path = new_file->path();
            ptr<Order> order;
            bool       was_in_job_chain = false;

            try
            {
                if( path.file_exists()  &&  !_job_chain->order_or_null( path ) )
                {
                    if( !known_orders_has_been_read )   // Eine Optimierung: Damit try_place_in_job_chain() nicht bei jeder Datei feststellen muss, dass es bereits einen Auftrag in der Datenbank gibt.
                    {
                        if( _job_chain->is_distributed() )  read_known_orders( &known_orders );
                        known_orders_has_been_read = true;
                    }

                    order = _spooler->standing_order_subsystem()->new_order();

                    order->set_file_path( path );
                    order->set_state(fetching_state);

                    string date = Time( new_file->last_write_time(), Time::is_utc).as_string(_spooler->_time_zone_name, time::without_ms );


                    bool ok = true;

                    if( ok )  ok = known_orders.find( order->string_id() ) == known_orders.end();     // Auftrag ist noch nicht bekannt?

                    if( ok )
                    {
                        was_in_job_chain = order->try_place_in_job_chain( _job_chain );
                        ok &= was_in_job_chain;
                        // !ok ==> Auftrag ist bereits vorhanden
                    }

                    if( ok  &&  order->is_distributed() ) 
                    {
                        ok = order->db_occupy_for_processing();

                        if( !path.file_exists() )
                        {
                            // Ein anderer Scheduler hat vielleicht den Dateiauftrag blitzschnell erledigt
                            order->db_release_occupation(); 
                            ok = false;
                        }
                    }

                    if( ok )  order->occupy_for_task( occupying_task, now );

                    if( ok  &&  order->is_distributed() )  _job_chain->add_order( order );

                    if( ok )
                    {
                        result = order;
                        log()->info( message_string( "SCHEDULER-983", order->obj_name(), "written at " + date ) );
                    }

                    if( !ok )  
                    {
                        if( !was_in_job_chain )  order->remove_from_job_chain();
                        order->close();
                        //order->close( was_in_job_chain? Order::cls_dont_remove_from_job_chain : Order::cls_remove_from_job_chain );
                        order = NULL;
                    }
                }

                _new_files[ _new_files_index ] = NULL;
                --_new_files_count;

                if( _bad_map.find( path ) != _bad_map.end() )   // Zurzeit nicht denkbar, weil es nur zu lange Pfade betrifft
                {
                    _bad_map.erase( path );
                    log()->info( message_string( "SCHEDULER-347", path ) );
                }

            }
            catch( exception& x )   // Möglich bei für Order.id zu langem Pfad
            {
                if( _bad_map.find( path ) == _bad_map.end() )
                {
                    log()->error( x.what() );
                    z::Xc xx ( "SCHEDULER-346", path );
                    log()->warn( xx.what() );
                    _bad_map[ path ] = Z_NEW( Bad_entry( path, x ) );

                    xx.append_text( x.what() );
                    send_mail( evt_file_order_error, &xx );
                }

                if( order ) 
                {
                    if( !was_in_job_chain )  order->remove_from_job_chain();
                    order->close();
                    //order->close( was_in_job_chain? Order::cls_dont_remove_from_job_chain : Order::cls_remove_from_job_chain );
                }
            }
        }

        _new_files_index++;
    }

    return result;
}

//---------------------------------------------------Directory_file_order_source::read_known_orders

void Directory_file_order_source::read_known_orders( String_set* known_orders )
{
    S select_sql;
    select_sql << "select `id`  from " << db()->_orders_tablename
               << "  where " << _job_chain->db_where_condition();

    for( Retry_transaction ta ( _spooler->db() ); ta.enter_loop(); ta++ ) try
    {
        known_orders->clear();

        Any_file result_set = ta.open_result_set( select_sql, Z_FUNCTION );
        while( !result_set.eof() )
        {
            Record record = result_set.get_record();
            known_orders->insert( record.as_string( 0 ) );
        }
    }
    catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", db()->_orders_tablename, x ), Z_FUNCTION ); }
}

//-----------------------------------------------------Directory_file_order_source::clear_new_files

void Directory_file_order_source::clear_new_files()
{
    _new_files.clear();
    _new_files.reserve( 1000 );
    _new_files_index = 0;
    _new_files_count = 0;

    _are_blacklisted_orders_cleaned_up = false;
}

//------------------------------------------------------Directory_file_order_source::read_new_files

bool Directory_file_order_source::read_new_files()
{
    clear_new_files();
    _new_files_time  = Time::now();

    Z_LOG2( "scheduler.file_order", Z_FUNCTION << "  " << _path << "\n" );
    bool is_first_file = true;

    for( Directory_watcher::Directory_reader dir ( _path, _regex_string == ""? NULL : &_regex );; )
    {
        if( _spooler->_cluster )  _spooler->_cluster->do_a_heart_beat_when_needed( Z_FUNCTION );    // PROVISORISCH FÜR LANGE VERZEICHNISSE AUF ENTFERNTEM RECHNER, macht bei Bedarf einen Herzschlag

        //Z_LOG2( "scheduler.file_order", Z_FUNCTION << "  " << _path << "  " << _new_files.size() << " Dateinamen gelesen\n" );

        ptr<zschimmer::file::File_info> file_info = dir.get();
        if( !file_info )  break;

        bool file_still_exists = file_info->try_call_stat();       // last_write_time füllen für sort, quick_last_write_less()
        if( file_still_exists )
        {
            _new_files.push_back( file_info );
            _new_files_count++;
        }

        if( is_first_file ) 
        {
            is_first_file = false;
            Z_LOG2( "scheduler.file_order", Z_FUNCTION << "  " << file_info->path() << ", erste Datei\n" );
        }
    }

    Z_LOG2( "scheduler.file_order", Z_FUNCTION << "  " << _path << "  " << _new_files.size() << " filenames has been read\n" );

    sort( _new_files.begin(), _new_files.end(), zschimmer::file::File_info::quick_last_write_less );

    return !_new_files.empty();
}

//------------------------------------------Directory_file_order_source::clean_up_blacklisted_files

bool Directory_file_order_source::clean_up_blacklisted_files()
{
    bool result = false;

    if( !_are_blacklisted_orders_cleaned_up )
    {
        hash_set<string> blacklisted_files;


        try
        {
            blacklisted_files = _job_chain->db_get_blacklisted_order_id_set( _path, _regex );
        }
        catch( exception& x )  { _log->error( S() << x.what() << ", in " << Z_FUNCTION << ", db_get_blacklisted_order_id_set()\n" ); }


        if( !blacklisted_files.empty() )
        {
            hash_set<string> removed_blacklisted_files = blacklisted_files;

            for( int i = 0; i < _new_files.size(); i++ )
            {
                if( zschimmer::file::File_info* new_file = _new_files[ i ] )
                    removed_blacklisted_files.erase( new_file->path() );
            }


            Z_FOR_EACH( hash_set<string>, removed_blacklisted_files, it ) 
            {
                string path = *it;

                try
                {
                    if( ptr<Order> order = _job_chain->order_or_null( path ) )  // Kein Datenbankzugriff 
                    {
                        order->log()->info( message_string( "SCHEDULER-981" ) );   // "File has been removed"
                        order->remove_from_job_chain();   
                        order->close();
                    }
                    else
                    if( _job_chain->is_distributed() )
                    {
                        Transaction ta ( _spooler->db() ); 

                        ptr<Order> order = order_subsystem()->try_load_distributed_order_from_database( &ta, _job_chain->path(), path, Order_subsystem::lo_blacklisted_lock );
                        if( order )
                        {
                            order->log()->info( message_string( "SCHEDULER-981" ) );   // "File has been removed"
                            order->db_delete( Order::update_not_occupied, &ta );
                        }

                        ta.commit( Z_FUNCTION );
                    }
                }
                catch( exception& x )
                {
                    _log->error( S() << x.what() << ", in " << Z_FUNCTION << ", " << path << "\n" );
                }
            }
        }

        _are_blacklisted_orders_cleaned_up = true;
    }

    return result;
}

//-----------------------------------------------------------Directory_file_order_source::send_mail

void Directory_file_order_source::send_mail( Scheduler_event_type event_code, const exception* x )
{
    try
    {                                   
        switch( event_code )
        {
            case evt_file_order_source_error:
            {
                assert( x );

                Scheduler_event scheduler_event ( event_code, log_error, this );

                scheduler_event.set_message( x->what() );
                scheduler_event.set_error( *x );
                scheduler_event.mail()->set_from_name( _spooler->name() + ", " + _job_chain->obj_name() );    // "Scheduler host:port -id=xxx Job chain ..."
                scheduler_event.mail()->set_subject( string("ERROR ") + x->what() );

                S body;
                body << Sos_optional_date_time::now().as_string() << "\n";
                body << "\n";
                body << _job_chain->obj_name() << "\n";
                body << "Scheduler -id=" << _spooler->id() << "  host=" << _spooler->_complete_hostname << "\n";
                body << "\n";
                body << "<file_order_source directory=\"" << _path << "\"/> doesn't work because of following error:\n";
                body << x->what() << "\n";
                body << "\n";

                if( !delay_after_error().is_eternal() )
                {
                    body << "Retrying every " << delay_after_error() << " seconds.\n";
                    body << "You will be notified when the directory is accessible again\n";
                }

                scheduler_event.mail()->set_body( body );
                scheduler_event.send_mail( _spooler->_mail_defaults );

                break;
            }

            case evt_file_order_source_recovered:
            {
                string msg = message_string( "SCHEDULER-984", _path );
                Scheduler_event scheduler_event ( event_code, log_info, this );

                scheduler_event.set_message( msg );
                scheduler_event.mail()->set_from_name( _spooler->name() + ", " + _job_chain->obj_name() );    // "Scheduler host:port -id=xxx Job chain ..."
                scheduler_event.mail()->set_subject( msg );

                S body;
                body << Sos_optional_date_time::now().as_string() << "\n\n" << _job_chain->obj_name() << "\n";
                body << "Scheduler -id=" << _spooler->id() << "  host=" << _spooler->_complete_hostname << "\n\n";
                body << msg << "\n";

                scheduler_event.mail()->set_body( body );
                scheduler_event.send_mail( _spooler->_mail_defaults );

                break;
            }

            case evt_file_order_error:
            {
                string msg = x->what();
                Scheduler_event scheduler_event ( event_code, log_info, this );

                scheduler_event.set_message( msg );
                scheduler_event.set_error( *x );
                scheduler_event.mail()->set_from_name( _spooler->name() + ", " + _job_chain->obj_name() );    // "Scheduler host:port -id=xxx Job chain ..."
                scheduler_event.mail()->set_subject( msg );

                S body;
                body << Sos_optional_date_time::now().as_string() << "\n\n" << _job_chain->obj_name() << "\n";
                body << "Scheduler -id=" << _spooler->id() << "  host=" << _spooler->_complete_hostname << "\n\n";
                body << msg << "\n";

                scheduler_event.mail()->set_body( body );
                scheduler_event.send_mail( _spooler->_mail_defaults );

                break;
            }

            default:
                assert(0), z::throw_xc( Z_FUNCTION );
        }
    }
    catch( const exception& x )  { log()->warn( x.what() ); }
}

//-----------------------------------------------------Directory_file_order_source::async_continue_

bool Directory_file_order_source::async_continue_( Async_operation::Continue_flags flags )
{
    bool    was_notified = _notification_event.signaled_flag();

    string  cause = was_notified                    ? "Notification" :
                    flags & cont_next_gmtime_reached? "Wartezeit abgelaufen"   // Das Flag ist doch immer gesetzt, oder?
                                                    : Z_FUNCTION;

    _notification_event.reset();

    read_directory( was_notified, cause );

    if (_new_files_index < _new_files.size()) {
        _job_chain->tip_for_new_order(_next_state);
    }

    int delay = int_cast(_directory_error        ? delay_after_error().seconds() :
                         _expecting_request_order? INT_MAX                 // Nächstes request_order() abwarten
                                                 : _repeat.seconds());     // Unter Unix funktioniert's _nur_ durch wiederkehrendes Nachsehen
    set_async_delay( max( 1, delay ) );     // Falls ein Spaßvogel es geschafft hat, repeat="0" anzugeben
    //Z_LOG2( "scheduler.file_order", Z_FUNCTION  << " set_async_delay(" << delay << ")  _expecting_request_order=" << _expecting_request_order << 
    //          "   async_next_gmtime" << Time( async_next_gmtime() ).as_string() << "GMT \n" );

    return true;
}

//---------------------------------------------------Directory_file_order_source::delay_after_error

Duration Directory_file_order_source::delay_after_error()
{
    return !_delay_after_error.is_eternal()? _delay_after_error : _repeat;
}

//------------------------------------------------------------Directory_file_order_source::obj_name

string Directory_file_order_source::obj_name() const
{
    S result;

    result << "Directory_file_order_source(\"" << _path << "\",\"" << _regex_string << "\")";

    return result;
}

//-------------------------------------------------------------------------------------------------

} //namespace order
} //namespace spoooler
} //namespace sos