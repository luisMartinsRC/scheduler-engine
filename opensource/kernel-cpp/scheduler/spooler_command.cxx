// $Id: spooler_command.cxx 15021 2011-08-24 16:49:02Z jz $        Joacim Zschimmer, Zschimmer GmbH, http://www.zschimmer.com
/*
    Hier ist implementiert

    Command_processor
*/


#include "spooler.h"
#include "../file/anyfile.h"
#include "../kram/licence.h"
#include "../zschimmer/z_sql.h"
#include "../zschimmer/z_gzip.h"

// Für temporäre Datei:
#include <sys/stat.h>               // S_IREAD, stat()
#include <fcntl.h>                  // O_RDONLY

#if defined SYSTEM_WIN
#   include <io.h>                  // open(), read() etc.
#   include <share.h>
#   include <direct.h>              // mkdir
#   include <windows.h>
#else
#   include <stdio.h>               // fileno
#   include <unistd.h>              // read(), write(), close()
#   include <signal.h>              // kill()
#endif

#include <sys/types.h>
#include <sys/timeb.h>
#include <sys/stat.h>

namespace sos {
namespace scheduler {

using namespace std;
using xml::Xml_writer;

//-------------------------------------------------------------------------------------------------

//#include "spooler_http_files.cxx"     // Generiert mit:  cd html && perl ../make/files_to_cxx.pl jz/*.html jz/*.js jz/*.xslt jz/*.css

//-------------------------------------------------------------------------------------------------

const string default_filename = "index.html";

//-------------------------------------------------------------------Log_categories_reset_operation

struct Log_categories_reset_operation : Async_operation
{
    Log_categories_reset_operation( Spooler* spooler ) : _spooler(spooler) {}

    bool async_finished_() const { return false; }

    string async_state_text_() const { return "Log_categories_reset_operation"; }

    bool async_continue_( Continue_flags )
    {
        static_log_categories.restore_from( _spooler->_original_log_categories );
        _spooler->log()->info( message_string( "SCHEDULER-710" ) );
        return true;
    }


    Spooler* const             _spooler;
};

//---------------------------------------------------------------Remote_task_close_command_response

struct Remote_task_close_command_response : File_buffered_command_response
{
                                Remote_task_close_command_response( Api_process*, Communication::Connection* );
                               ~Remote_task_close_command_response()                                {}

    // Async_operation
    virtual bool                async_finished_             () const                                { return _state == s_finished; }
    virtual string              async_state_text_           () const                                { return "Remote_task_close_command_response"; }
    virtual bool                async_continue_             ( Continue_flags );

    void                        close                       ();

  private:
    void                        write_file                  ( const string& what, const File_path& );

    enum State { s_initial, s_waiting, s_deleting_files, s_responding, s_finished };

    Fill_zero                  _zero_;
    Process_id                 _process_id;
    ptr<Api_process>           _process;
    ptr<Communication::Connection>  _connection;
    ptr<Async_operation>       _operation;
    State                      _state;
    double                     _trying_deleting_files_until;
    ptr<Remote_task_close_command_response> _hold_self;
};

//---------------------------Remote_task_close_command_response::Remote_task_close_command_response

Remote_task_close_command_response::Remote_task_close_command_response( Api_process* p, Communication::Connection* c )
: 
    _zero_(this+1), 
    _process(p), 
    _connection(c),
    _hold_self(c? NULL : this)
{
}

//--------------------------------------------------------Remote_task_close_command_response::close

void Remote_task_close_command_response::close()
{
    File_buffered_command_response::close();
}

//----------------------------------------------Remote_task_close_command_response::async_continue_

bool Remote_task_close_command_response::async_continue_( Continue_flags continue_flags )
{
    Z_DEBUG_ONLY( if( _operation )  assert( _operation->async_finished() ) );

    bool something_done = false;
    ptr<Remote_task_close_command_response> hold_me = this;

    switch( _state )
    {
        case s_initial:
        {
            _process_id = _process->process_id();
            _operation = _process->close__start();
            _operation->set_async_parent( this );       // Weckt uns, wenn _operation fertig ist
            _state = s_waiting;
        }

        case s_waiting:
        {
            if( _operation->async_finished() ) {
                _operation  = NULL;
                _process->close__end();

                _state = s_deleting_files;
                something_done = true;
            }
        }

        if( _state != s_deleting_files )  break;

        case s_deleting_files:
        {
            bool ok;
            if (Local_api_process* local_api_process = dynamic_cast<Local_api_process*>(+_process)) {
                ok = local_api_process->try_delete_files( _process->log() );
                if (!ok) {
                    // Das könnte mit dem Code in Task (spooler_task.cxx) zusammengefasst werden, als eigene Async_operation
                    // Ebenso (aber synchron) mit Remote_module_instance_server::try_delete_files()

                    double now = double_from_gmtime();

                    if( !_trying_deleting_files_until )  
                    {
                        string paths = join( ", ", local_api_process->undeleted_files() );
                        if( _process->log() )  _process->log()->debug( message_string( "SCHEDULER-876", paths ) );  // Nur beim ersten Mal
                    }

                    if( _trying_deleting_files_until  &&  _trying_deleting_files_until < now )   // Nach Fristablauf geben wir auf
                    {
                        string paths = join( ", ", local_api_process->undeleted_files() );
                        if( _process->log() )  _process->log()->info( message_string( "SCHEDULER-878", paths ) );
                        //_job->log()->warn( message_string( "SCHEDULER-878", paths ) );
                        ok = true;
                    }
                    else
                    {
                        if( !_trying_deleting_files_until )  _trying_deleting_files_until = now + delete_temporary_files_delay.as_double();
                        set_async_next_gmtime( min( now + delete_temporary_files_retry.as_double(), _trying_deleting_files_until ) );
                    }
                }
            } else {
                ok = true;
            }

            if( ok )
                _state = s_responding;

            something_done = true;
        }

        if( _state != s_responding )  break;

        case s_responding: {    // XML-Anwort 
            if (_connection) {
                begin_standard_response();
                _xml_writer.begin_element( "ok" );
                //int KEIN_STDOUT;
                //write_file( "stdout", _process->stdout_path() );
                //write_file( "stderr", _process->stderr_path() );
                _xml_writer.end_element( "ok" );
                _xml_writer.flush();
                end_standard_response();
                _connection->_operation_connection->unregister_task_process(_process_id);
                _state = s_finished;
                if( continue_flags )
                    _connection->async_wake();
            } else {
                _process->spooler()->unregister_api_process(_process_id);
                _hold_self = NULL;
                set_async_manager(NULL);
                _state = s_finished;
            }

            _process    = NULL;
            _connection = NULL;

            something_done = true;
        }

        default: ;
    }

    return something_done;
}

//---------------------------------------------------Remote_task_close_command_response::write_file

void Remote_task_close_command_response::write_file( const string& name, const File_path& path )
{
    string text;

    try
    {
        text = string_from_file( path );
        //for( int i = 0; i < text.length(); i++ )  if( text[i] == '\0' )  text[i] = ' ';
    }
    catch( exception& x ) { text = S() << "ERROR: " << x.what(); }

    _xml_writer.begin_element( "file" );
    _xml_writer.set_attribute( "name", name );
    _xml_writer.set_attribute( "encoding", "hex" );
    _xml_writer.write( lcase_hex( (const Byte*)text.data(), text.length() ) );
    _xml_writer.end_element( "file" );
}

//------------------------------------------------------------------------------------dom_append_nl

void dom_append_nl( const xml::Element_ptr& )
{
    //indent ersetzt diese Newlines.    element.appendChild( element.ownerDocument().createTextNode( "\n" ) );
}

//-----------------------------------------------------------------------------create_error_element

xml::Element_ptr create_error_element( const xml::Document_ptr& document, const Xc_copy& x, time_t gm_time )
{
    xml::Element_ptr e = document.createElement( "ERROR" );

    if( gm_time )  e.setAttribute( "time", xml_of_time_t(gm_time));

    if( !empty( x->name() )          )  e.setAttribute( "class" , x->name()          );

    e.setAttribute( "code", x->code() );
    e.setAttribute( "text", remove_password( x->what() ) );
    
    if( !empty( x->_pos.filename() ) )  e.setAttribute( "source", x->_pos.filename() );
    if( x->_pos._line >= 0           )  e.setAttribute( "line"  , as_string( x->_pos._line + 1 ) );
    if( x->_pos._col  >= 0           )  e.setAttribute( "col"   , as_string( x->_pos._col + 1  ) );

    return e;
}

//-----------------------------------------------------------------------------create_error_element

xml::Element_ptr create_error_element( const xml::Document_ptr& document, const zschimmer::Xc& x, time_t gm_time )
{
    Xc xc ( "" );
    xc.set_code( x.code().c_str() );
    xc.set_what( x.what() );

    Xc_copy xc_copy ( xc );
    xc_copy.set_time( (double)gm_time );

    return create_error_element( document, xc_copy );
}

//-----------------------------------------------------------------------------append_error_element

void append_error_element( const xml::Element_ptr& element, const Xc_copy& x )
{
    element.appendChild( create_error_element( element.ownerDocument(), x, (time_t)x.time() ) );
}


//--------------------------------------------------------------------------------xc_from_dom_error

Xc_copy xc_from_dom_error( const xml::Element_ptr& element )
{
    Xc x ( "" );

    x.set_code( element.getAttribute( "code" ).c_str() );
    x.set_what( element.getAttribute( "text" ) );

    return x;
}

//------------------------------------------------------------------------------how_what::Show_what

Show_what::Show_what( Show_what_enum what ) 
: 
    _zero_(this+1), 
    _what(what), 
    _max_orders(INT_MAX),
    _max_task_history(10),
    _folder_path( root_path )
{
}

//-------------------------------------------------------------------------Show_what::set_dom

void Show_what::set_dom( const Scheduler& scheduler, const xml::Element_ptr& element )
{

    string max_orders = element.getAttribute( "max_orders" );
    if( max_orders != "" )  _max_orders = as_int( max_orders );

    string max_order_history = element.getAttribute( "max_order_history" );
    if( max_order_history != "" )  _max_order_history = as_int( max_order_history );

    string max_task_history = element.getAttribute( "max_task_history" );
    if( max_task_history != "" )  _max_task_history = as_int( max_task_history );

    _folder_path = Absolute_path( root_path, element.getAttribute( "path", _folder_path ) );

    set_what( element.getAttribute( "what" ) );
    set_subsystems( scheduler, element.getAttribute( "subsystems" ) );

    _max_order_history = element.int_getAttribute( "max_order_history", _max_order_history );

}

//--------------------------------------------------------------------------------Show_what::set_what
void Show_what::set_what(const string& what) {

    const char* p = what.c_str();  // Bsp: "all"  "orders,description"  "task_queue,orders,description,"
    while( *p )
    {
        while( *p == ' ' )  p++;

        if( string_equals_prefix_then_skip( &p, "none"             ) )  _what = show_standard;       // Setzt Flags zurück! (Provisorisch, solange jobs,tasks default ist)
        else
        if( string_equals_prefix_then_skip( &p, "all!"             ) )  _what = _what | show_all;
        else
        if( string_equals_prefix_then_skip( &p, "all"              ) )  _what = _what | show_all_;
        else
        if( string_equals_prefix_then_skip( &p, "task_queue"       ) )  _what = _what | show_task_queue;
        else
        if( string_equals_prefix_then_skip( &p, "orders"           ) )  _what = _what | show_orders;
        else
        if( string_equals_prefix_then_skip( &p, "job_chains"       ) )  _what = _what | show_job_chains;
        else
        if( string_equals_prefix_then_skip( &p, "job_chain_orders" ) )  _what = _what | show_job_chain_orders;
        else
        if( string_equals_prefix_then_skip( &p, "job_orders"       ) )  _what = _what | show_job_orders;
        else
        if( string_equals_prefix_then_skip( &p, "description"      ) )  _what = _what | show_description;
        else
        if( string_equals_prefix_then_skip( &p, "log"              ) )  _what = _what | show_log;
        else
        if( string_equals_prefix_then_skip( &p, "task_history"     ) )  _what = _what | show_task_history;
        else
        if( string_equals_prefix_then_skip( &p, "order_history"    ) )  _max_order_history = 20;
        else
        if( string_equals_prefix_then_skip( &p, "remote_schedulers") )  _what = _what | show_remote_schedulers;
        else
        if( string_equals_prefix_then_skip( &p, "schedules"        ) )  _what = _what | show_schedules;
        else
        if( string_equals_prefix_then_skip( &p, "run_time"         ) )  _what = _what | show_schedule;
        else
        if( string_equals_prefix_then_skip( &p, "job_chain_jobs"   ) )  _what = _what | show_job_chain_jobs;
        else
        if( string_equals_prefix_then_skip( &p, "jobs"             ) )  _what = _what | show_jobs;
        else
        if( string_equals_prefix_then_skip( &p, "tasks"            ) )  _what = _what | show_tasks;
        else
        if( string_equals_prefix_then_skip( &p, "job_commands"     ) )  _what = _what | show_job_commands;
        else
        if( string_equals_prefix_then_skip( &p, "blacklist"        ) )  _what = _what | show_blacklist;
        else
        if( string_equals_prefix_then_skip( &p, "order_source_files") )  _what = _what | show_order_source_files;
        else
        if( string_equals_prefix_then_skip( &p, "payload"          ) )  _what = _what | show_payload;
        else
        if( string_equals_prefix_then_skip( &p, "job_params"       ) )  _what = _what | show_job_params;
        else
        if( string_equals_prefix_then_skip( &p, "cluster"          ) )  _what = _what | show_cluster;
        else
        if( string_equals_prefix_then_skip( &p, "operations"       ) )  _what = _what | show_operations;
        else
        if( string_equals_prefix_then_skip( &p, "folders"          ) )  _what = _what | show_folders;
        else
        if( string_equals_prefix_then_skip( &p, "no_subfolders"    ) )  _what = _what | show_no_subfolders;
        else
        if( string_equals_prefix_then_skip( &p, "source"           ) )  _what = _what | show_source;
        else
        if( string_equals_prefix_then_skip( &p, "statistics"       ) )  _what = _what | show_statistics;        // JS-507
        //else
        //if( string_equals_prefix_then_skip( &p, "subfolders"       ) )  _what = _what | show_subfolders;
        else
        if( string_equals_prefix_then_skip( &p, "standard"         ) )  ;
        else
            z::throw_xc( "SCHEDULER-164", what );
        if( *p != ','  &&  *p != ' '  &&  *p != '\0' )  z::throw_xc( "SCHEDULER-164", what );

        while( *p == ' '  ||  *p == ',' )  p++;

        if( *p == 0 )  break;
    }
}

//--------------------------------------------------------------------------------Show_what::set_subsystems
void Show_what::set_subsystems(const Scheduler& scheduler, const string& subsystems_string) {
    
    vector<string> subsystems = vector_split(" +",subsystems_string);
    Z_FOR_EACH(vector<string>, subsystems, it)
    {
        string subsystem_name = *it;
        Subsystem* s = scheduler._subsystem_register.get(subsystem_name); 
        _subsystem_set.insert(s);
    }

}

//-------------------------------------------------------------Command_processor::Command_processor

Command_processor::Command_processor( Spooler* spooler, Security::Level security_level, const Host& client_host, Communication::Operation* cp) : 
    _zero_(this+1),
    _spooler(spooler),
    _communication_operation( cp ),
    _client_host(client_host),
    _validate(true),
    _security_level( security_level )
{
    _variable_set_map[ variable_set_name_for_substitution ] = _spooler->_environment;
    _spooler->_executing_command = true;

    begin_answer();
}

//------------------------------------------------------------Command_processor::~Command_processor

Command_processor::~Command_processor()
{
    _spooler->_executing_command = false;
}

//----------------------------------------------------------------Command_processor::execute_config

xml::Element_ptr Command_processor::execute_config( const xml::Element_ptr& config_element )
{
    if( _security_level < Security::seclev_all )  z::throw_xc( "SCHEDULER-121" );

    if( !config_element.nodeName_is( "config" ) )  z::throw_xc( "SCHEDULER-113", config_element.nodeName() );

    string spooler_id = config_element.getAttribute( "spooler_id" );
    if( spooler_id.empty()  ||  spooler_id == _spooler->id()  ||  _spooler->_manual )
    {
        if( _load_base_config_immediately )  _spooler->load_config( config_element, _source_filename, true );
                                       else  _spooler->cmd_load_config( config_element, _source_filename );
    }

    return _answer.createElement( "ok" );
}

//-------------------------------------------------------------Command_processor::execute_show_jobs

xml::Element_ptr Command_processor::execute_show_jobs( const Show_what& show )
{
    if( _security_level < Security::seclev_info )  z::throw_xc( "SCHEDULER-121" );

    return _spooler->root_folder()->job_folder()->dom_element( _answer, show );
}

//--------------------------------------------------------------Command_processor::execute_job_why

xml::Element_ptr Command_processor::execute_job_why( const xml::Element_ptr& element )
{
    if( _security_level < Security::seclev_info )  z::throw_xc( "SCHEDULER-121" );
    return _spooler->job_subsystem()->job( Absolute_path( root_path, element.getAttribute( "job" ) ) ) -> why_dom_element( _answer );
}

//----------------------------------------------------------Command_processor::execute_show_threads
/*
xml::Element_ptr Command_processor::execute_show_threads( const Show_what& show )
{
    if( _security_level < Security::seclev_info )  z::throw_xc( "SCHEDULER-121" );

    return _spooler->threads_as_xml( _answer, show );
}
*/
//--------------------------------------------------Command_processor::execute_show_process_classes

xml::Element_ptr Command_processor::execute_show_process_classes( const Show_what& show )
{
    if( _security_level < Security::seclev_info )  z::throw_xc( "SCHEDULER-121" );

    return _spooler->root_folder()->process_class_folder()->dom_element( _answer, show );
}

//---------------------------------------------------------Command_processor::execute_scheduler_log

xml::Element_ptr Command_processor::execute_scheduler_log( const xml::Element_ptr& element, const Show_what& )
{
    xml::Element_ptr result;

    if( _security_level < Security::seclev_info )  z::throw_xc( "SCHEDULER-121" );

    if( element.nodeName_is( "scheduler_log.log_categories.reset" ) )
    {
        if( int delay = element.int_getAttribute( "delay", 0 ) )
        {
            ptr<Log_categories_reset_operation> op = Z_NEW( Log_categories_reset_operation( _spooler ) );
            op->set_async_delay( delay );
            _spooler->_log_categories_reset_operation = +op;
            op->set_async_manager( _spooler->_connection_manager );
        }
        else
        {
            static_log_categories.restore_from( _spooler->_original_log_categories );
            _spooler->_log_categories_reset_operation = NULL;
        }

        result = _answer.createElement( "ok" );
    }
    else
    if( element.nodeName_is( "scheduler_log.log_categories.show" ) )
    {
        result = _answer.createElement( "log_categories" );
        result.setAttribute( "categories", static_log_categories.to_string() );

        if( _spooler->_log_categories_reset_operation )  
            result.setAttribute( "reset_at", Time(_spooler->_log_categories_reset_operation->async_next_gmtime()).xml_value());

        Z_DEBUG_ONLY( result.setAttribute( "debug", static_log_categories.debug_string() ) );


        // Einstellungen aus static_log_categories übernehmen

        Log_categories::Map map = static_log_categories.map_copy();

        Z_FOR_EACH( Log_categories::Map, map, e )
        {
            string path = e->first;

            xml::Element_ptr cat_element = _answer.createElement( "log_category" );

            cat_element.setAttribute( "path"        , path == ""? "all" : e->first );
            cat_element.setAttribute( "value"       , e->second._value );

            if( e->second._used_counter )
            cat_element.setAttribute( "used_counter", e->second._used_counter );

            if( e->second._denied_counter )
            cat_element.setAttribute( "denied_counter", e->second._denied_counter );

            if( e->second._type == Log_categories::Entry::e_implicit )  cat_element.setAttribute( "mode", "implicit" );
            else
            if( e->second._type == Log_categories::Entry::e_explicit )  cat_element.setAttribute( "mode", "explicit" );

            if( e->second._children_too                              )  cat_element.setAttribute( "children_too", "yes" );

            #ifdef Z_DEBUG
              //if( e->second._children_too_derived                  )  cat_element.setAttribute( "children_too_derived", "yes" );
                if( e->second._has_default                           )  cat_element.setAttribute( "has_default", "yes" );
                cat_element.setAttribute( "default", e->second._default_value? "yes" : "no" );
            #endif            

            result.appendChild( cat_element );
        }


        // Einstellung der Dokumentation übernehmen

        xml::Document_ptr doc = xml::Document_ptr::from_xml_string(java_resource_as_string("com/sos/scheduler/enginedoc/common/log_categories.xml"));
        xml::Element_ptr  doc_log_categories_element = doc.select_element_strict( "/log_categories" );

        execute_scheduler_log__append( doc_log_categories_element, "", result );
    }
    else
    if( element.nodeName_is( "scheduler_log.log_categories.set" ) )
    {
        z::static_log_categories.set( element.getAttribute_mandatory( "category" ), element.bool_getAttribute( "value", true ) );
        result = _answer.createElement( "ok" );
    }
    else 
        z::throw_xc( Z_FUNCTION, element.nodeName() );

    return result;
}

//-------------------------------------------------Command_processor::execute_scheduler_log__append

void Command_processor::execute_scheduler_log__append( const xml::Element_ptr& doc_log_categories_element, const string& prefix, const xml::Element_ptr& result )
{
    DOM_FOR_EACH_ELEMENT( doc_log_categories_element, doc_cat_element )
    {
        if( doc_cat_element.nodeName_is( "log_category" ) )
        {
            string name = doc_cat_element.getAttribute_mandatory( "name" );
            if( name != "*" )
            {
                string path = prefix + name;

                xml::Element_ptr cat_element = result.select_node( "log_category [ @path=" + quoted_string( path ) + " ]" );
                if( !cat_element )
                {
                    cat_element = _answer.createElement( "log_category" );
                    cat_element.setAttribute( "path" , path );
                    cat_element.setAttribute( "value", static_log_categories.is_set( path, true ) );
                    result.appendChild( cat_element );
                }

                cat_element.setAttribute_optional( "title", doc_cat_element.getAttribute( "title" ) );

                execute_scheduler_log__append( doc_cat_element, path + ".", result );
            }
        }
    }
}

//---------------------------------------------------------------Command_processor::execute_licence

xml::Element_ptr Command_processor::execute_licence( const xml::Element_ptr& element, const Show_what& )
{
    if( element.nodeName_is( "licence.use" ) )
    {
        sos_static_ptr()->_licence->read_key( element.getAttribute_mandatory( "key" ) );
    }
    else 
        z::throw_xc( Z_FUNCTION, element.nodeName() );

    return _answer.createElement( "ok" );
}

//-------------------------------------------------------------Command_processor::execute_subsystem

/**
* \brief JS-507: Informationen zu den Subsystemen in XML-Struktur ausgeben
* \detail 
* Diese Methode liefert Informationen zu den Subsystemen in einer XML-Struktur.
*
* \version 2.1.1 - 2010-05-28
*
* \param element - XML-Kommando (<show_subsystem .../>)
* \param show_what - Attribut @what in Datenstruktur aufgelöst.
* \return XML-Element mit den Informationen zu den Subsystemen
*/
xml::Element_ptr Command_processor::execute_subsystem( const xml::Element_ptr& element, const Show_what& show_what )
{
    //TODO In eigene Klasse auslagern, z.B. Subsystem_subsystem. Oder Meta_subsystem.
    //Meta_subsystem::dom_element()
    //Meta_subsystem::register

    if( element.nodeName_is( "subsystem.show" ) )
    {
        xml::Element_ptr result = _answer.createElement( "subsystems" );
        //TODO Subsysteme aus einem Register lesen
        result.appendChild_if( _spooler->job_subsystem()->dom_element( _answer, show_what ) );
        result.appendChild_if( _spooler->task_subsystem()->dom_element( _answer, show_what ) );
        result.appendChild_if( _spooler->order_subsystem()->dom_element( _answer, show_what ) );
        result.appendChild_if( _spooler->standing_order_subsystem()->dom_element( _answer, show_what ) );
        result.appendChild_if( _spooler->schedule_subsystem()->dom_element( _answer, show_what ) );
        result.appendChild_if( _spooler->process_class_subsystem()->dom_element( _answer, show_what ) );;
        result.appendChild_if( _spooler->folder_subsystem()->dom_element( _answer, show_what ) );
        result.appendChild_if( _spooler->lock_subsystem()->dom_element( _answer, show_what ) );

        return result;
    }
    else 
        z::throw_xc( Z_FUNCTION, element.nodeName() );
}

//------------------------------------------------------------Command_processor::execute_show_state

xml::Element_ptr Command_processor::execute_show_state( const xml::Element_ptr& element, const Show_what& show_ )
{
    if( _security_level < Security::seclev_info )  z::throw_xc( "SCHEDULER-121" ); 

    Show_what show = show_;
    if( show.is_set( show_all_ ) )  show |= Show_what_enum( show_task_queue | show_description | show_remote_schedulers );

    if( element.nodeName_is( "s" ) )  show |= show_job_chains | show_job_chain_orders | show_operations | show_folders | show_remote_schedulers | show_schedules;


    return _spooler->state_dom_element( _answer, show );
}

//---------------------------------------------------------------Command_processor::get_id_and_prev

void Command_processor::get_id_and_next( const xml::Element_ptr& element, int* id, int* next )
{
    *id = element.uint_getAttribute( "id", -1 );

    string prev_str = element.getAttribute( "prev" );

    *next = prev_str == ""   ? ( *id == -1? -10 : 0 ) :
            prev_str == "all"? -INT_MAX 
                             : -as_int(prev_str);

    string next_str = element.getAttribute( "next" );
    if( next_str != "" )  *next = as_uint(next_str);

    const int max_n = 1000;
    if( abs(*next) > max_n )  *next = sgn(*next) * max_n,  _spooler->log()->warn( message_string( "SCHEDULER-285", max_n ) );
}

//---------------------------------------------------------Command_processor::execute_show_calendar

xml::Element_ptr Command_processor::execute_show_calendar( const xml::Element_ptr& element, const Show_what& show_what )
{
    if( _security_level < Security::seclev_info )  z::throw_xc( "SCHEDULER-121" );

    Show_calendar_options options;

    options._from   = Time::now();
    options._before = Time::never;
    options._limit  = element.int_getAttribute( "limit", 100 );

    if( element.hasAttribute( "from"   ) )  options._from   = Time::of_date_time( element.getAttribute( "from"  ), _spooler->_time_zone_name );
    if( element.hasAttribute( "before" ) )  options._before = Time::of_date_time( element.getAttribute( "before" ), _spooler->_time_zone_name );
                                      else  options._before = options._from.midnight() + Duration(7*24*3600 + 1);   // Default: eine Woche




    xml::Element_ptr calendar_element = _answer.createElement( "calendar" );

    if( show_what.is_set( show_jobs )  &&  options._count < options._limit )
        _spooler->job_subsystem()->append_calendar_dom_elements( calendar_element, &options );

    if( show_what.is_set( show_orders )  &&  options._count < options._limit )
        _spooler->order_subsystem()->append_calendar_dom_elements( calendar_element, &options );

    return calendar_element;
}

//----------------------------------------------------------Command_processor::execute_show_history

xml::Element_ptr Command_processor::execute_show_history( const xml::Element_ptr& element, const Show_what& show_ )
{
    if( _security_level < Security::seclev_info )  z::throw_xc( "SCHEDULER-121" );

    Show_what show = show_;
    if( show.is_set( show_all_ ) )  show |= show_log;

    int id, next;
    get_id_and_next( element, &id, &next );
    
    ptr<Job> job = _spooler->job_subsystem()->job( Absolute_path( root_path, element.getAttribute( "job" ) ) );

    return job->read_history( _answer, id, next, show );
}

//----------------------------------------------------Command_processor::execute_show_order_history
/*
xml::Element_ptr Command_processor::execute_show_order_history( const xml::Element_ptr& element, const Show_what& show )
{
    if( _security_level < Security::seclev_info )  z::throw_xc( "SCHEDULER-121" );

    if( show.is_set( show_all_ ) )  show = Show_what_enum( show | show_log );

    int id, next;
    get_id_and_prev( element, &id, &next );
    
    Sos_ptr<Job_chain> job_chain = _spooler->order_subsystem()->job_chain( element.getAttribute( "job_chain" ) );

    return job_chain->read_order_history( _answer, id, next, show );
}
*/
//-----------------------------------------------------Command_processor::execute_modify_hot_folder

xml::Element_ptr Command_processor::execute_modify_hot_folder( const xml::Element_ptr& element )
{
    if( _security_level < Security::seclev_no_add )  z::throw_xc( "SCHEDULER-121" );

    Absolute_path folder_path = element.hasAttribute( "folder" )? Absolute_path( root_path, element.getAttribute( "folder" ) )
                                                                : root_path;

    _spooler->folder_subsystem()->write_configuration_file_xml( folder_path, element.first_child_element() );

    return _answer.createElement( "ok" );
}

//--------------------------------------------------------Command_processor::execute_modify_spooler

xml::Element_ptr Command_processor::execute_modify_spooler( const xml::Element_ptr& element )
{
    if( _security_level < Security::seclev_no_add )  z::throw_xc( "SCHEDULER-121" );

    int timeout = element.int_getAttribute( "timeout", 999999999 );

    string cmd = element.getAttribute( "cmd" );
  //if( !cmd.empty() )
    {
        if( cmd == "pause"                 )  _spooler->cmd_pause();
        else
        if( cmd == "continue"              )  _spooler->cmd_continue();
        else
      //if( cmd == "stop"                  )  _spooler->cmd_stop();
      //else
        if( cmd == "reload"                )  _spooler->cmd_reload();
        else
        if( cmd == "terminate"             )  _spooler->cmd_terminate( false, timeout );
        else
        if( cmd == "terminate_and_restart" )  _spooler->cmd_terminate_and_restart( timeout );
        else
        if( cmd == "let_run_terminate_and_restart" )  _spooler->cmd_let_run_terminate_and_restart();
        else
        if( cmd == "abort_immediately"             )  _spooler->abort_immediately(); 
        else
        if( cmd == "abort_immediately_and_restart" )  _spooler->abort_immediately( true );
        else
            z::throw_xc( "SCHEDULER-105", cmd );
    }
    
    return _answer.createElement( "ok" );
}

//----------------------------------------------------------Command_processor::execute_settings_set

xml::Element_ptr Command_processor::execute_setting_set(const xml::Element_ptr& element)
{
    string name = element.getAttribute("name");
    string value = element.getAttribute("value");
    ptr<Com_variable_set> v = new Com_variable_set();
    v->set_var(name, value);
    _spooler->modify_settings(*v);
    return _answer.createElement( "ok" );
}

//-------------------------------------------------------------Command_processor::execute_terminate

xml::Element_ptr Command_processor::execute_terminate( const xml::Element_ptr& element )
{
    if( _security_level < Security::seclev_no_add )  z::throw_xc( "SCHEDULER-121" );

    bool   restart        = element.bool_getAttribute( "restart"                     , false );
    bool   all_schedulers = element.bool_getAttribute( "all_schedulers"              , false );
    int    timeout        = element. int_getAttribute( "timeout"                     , INT_MAX );
    string member_id      = element.     getAttribute( "cluster_member_id"           );
    bool   delete_dead    = element.bool_getAttribute( "delete_dead_entry"           , false );
    string continue_excl  = element.bool_getAttribute( "continue_exclusive_operation" )? cluster::continue_exclusive_any 
                                                                                       : "non_backup";
    if( member_id == ""  ||  member_id == _spooler->cluster_member_id() )
    {
        _spooler->cmd_terminate( restart, timeout, continue_excl, all_schedulers );
    }
    else
    {
        if( !_spooler->_cluster )  z::throw_xc( Z_FUNCTION, "no cluster" );

        if( delete_dead )
        {
            _spooler->_cluster->delete_dead_scheduler_record( member_id );
        }
        else
        {
            S cmd;
            cmd << "<terminate";
            if( timeout < INT_MAX )  cmd << " timeout='" << timeout << "'";
            if( restart )  cmd << " restart='yes'";
            cmd << "/>";

            _spooler->_cluster->set_command_for_scheduler( (Transaction*)NULL, cmd, member_id );
        }
    }

    return _answer.createElement( "ok" );
}

//--------------------------------------------------------------Command_processor::execute_show_job

xml::Element_ptr Command_processor::execute_show_job( const xml::Element_ptr& element, const Show_what& show_ )
{
    if( _security_level < Security::seclev_info )  z::throw_xc( "SCHEDULER-121" );

    Show_what show = show_;
    if( show.is_set( show_all_ ) )  show |= show_description | show_task_queue | show_orders;

    Job_chain*    job_chain      = NULL;
    Absolute_path job_chain_path = Absolute_path( root_path, element.getAttribute( "job_chain" ) );
    
    if( job_chain_path != "" )  job_chain = _spooler->order_subsystem()->job_chain( job_chain_path );
    
    return _spooler->job_subsystem()->job( Absolute_path( root_path, element.getAttribute( "job" ) ) ) -> dom_element( _answer, show, job_chain );
}

//------------------------------------------------------------Command_processor::execute_modify_job

xml::Element_ptr Command_processor::execute_modify_job( const xml::Element_ptr& element )
{
    if( _security_level < Security::seclev_no_add )  z::throw_xc( "SCHEDULER-121" );
    _spooler->assert_is_activated( Z_FUNCTION );

    Absolute_path job_path = Absolute_path( root_path, element.getAttribute( "job" ) );
    string        cmd_name =                     element.getAttribute( "cmd" );

    Job::State_cmd cmd = cmd_name.empty()? Job::sc_none 
                                         : Job::as_state_cmd( cmd_name );

    Job* job = _spooler->job_subsystem()->job( job_path );


    DOM_FOR_EACH_ELEMENT( element, e )
    {
        if( e.nodeName_is( "run_time" ) )  { job->set_schedule_dom(e);  break; }
    }


    if( cmd )  job->set_state_cmd( cmd );
    
    return _answer.createElement( "ok" );
}

//----------------------------------------------------------Command_processor::execute_show_cluster

xml::Element_ptr Command_processor::execute_show_cluster( const xml::Element_ptr&, const Show_what& show )
{
    if( _security_level < Security::seclev_info )  z::throw_xc( "SCHEDULER-121" );

    xml::Element_ptr result = _answer.createElement( "cluster" );

    if( _spooler->_cluster )  result.appendChild( _spooler->_cluster->dom_element( _answer, show ) );

    result.appendChild( _spooler->_supervisor->dom_element( _answer, show ) );
    
    return result;
}

//-------------------------------------------------------------Command_processor::execute_show_task

xml::Element_ptr Command_processor::execute_show_task( const xml::Element_ptr& element, const Show_what& show )
{
    if( _security_level < Security::seclev_info )  z::throw_xc( "SCHEDULER-121" );

    int task_id = element.int_getAttribute( "id" );

    ptr<Task> task = _spooler->get_task_or_null( task_id );
    if( task )
    {
        return task->dom_element( _answer, show );
    }
    else
    {
        return _spooler->db()->read_task( _answer, task_id, show );
    }
}

//---------------------------------------------------------Command_processor::execute_check_folders

xml::Element_ptr Command_processor::execute_check_folders( const xml::Element_ptr& )
{
    // Für die HTML-Oberfläche und Wecksignal vom Supervisor


    if( _security_level < Security::seclev_info )  z::throw_xc( "SCHEDULER-121" );
    _spooler->assert_is_activated( Z_FUNCTION );

    if( _spooler->_supervisor_client )  _spooler->_supervisor_client->start_update_configuration();
    _spooler->folder_subsystem()->handle_folders();
    
    return _answer.createElement( "ok" );
}

//-------------------------------------------------------------Command_processor::execute_kill_task

xml::Element_ptr Command_processor::execute_kill_task( const xml::Element_ptr& element )
{
    if( _security_level < Security::seclev_no_add )  z::throw_xc( "SCHEDULER-121" );
    _spooler->assert_is_activated( Z_FUNCTION );

    int    id          = element. int_getAttribute( "id" );
    string job_path    = element.     getAttribute( "job" );              // Hilfsweise
    bool   immediately = element.bool_getAttribute( "immediately", false );
    

    _spooler->job_subsystem()->job( Absolute_path( root_path, job_path ) )->kill_task( id, immediately );
    
    return _answer.createElement( "ok" );
}

//-------------------------------------------------------------Command_processor::execute_start_job

xml::Element_ptr Command_processor::execute_start_job( const xml::Element_ptr& element )
{
    if( _security_level < Security::seclev_no_add )  z::throw_xc( "SCHEDULER-121" );
    _spooler->assert_is_activated( Z_FUNCTION );

    string job_path        = element.getAttribute( "job"   );
    string task_name       = element.getAttribute( "name"  );
    string after_str       = element.getAttribute( "after" );
    string at_str          = element.getAttribute( "at"    );
    string web_service_name= element.getAttribute( "web_service" );
    bool   force           = element.bool_getAttribute( "force", true ); 

    Time start_at;

    if( at_str == ""       )  at_str = "now";
    if( at_str == "period" )  start_at = Time(0);                               // start="period" => start_at = 0 (sobald eine Periode es zulässt)
                        else  start_at = Time::of_date_time_with_now( at_str, _spooler->_time_zone_name );         // "now+..." möglich

    if( !after_str.empty() )  start_at = Time::now() + Duration( as_int( after_str ) );     // Entweder at= oder after=


    ptr<Com_variable_set> params      = new Com_variable_set;
    ptr<Com_variable_set> environment = new Com_variable_set;

    DOM_FOR_EACH_ELEMENT( element, e )
    {
        if( e.nodeName_is( "params"      ) )   params->set_dom( e, &_variable_set_map );
        else
        if( e.nodeName_is( "environment" ) )   environment = new Com_variable_set,  environment->set_dom( e, NULL, "variable" );
    }

    ptr<Task> task = _spooler->job_subsystem()->job(Absolute_path(root_path, job_path))->start_task(params, environment, start_at, force, task_name, web_service_name);

    xml::Element_ptr result = _answer.createElement( "ok" ); 
    if (task)  result.appendChild( task->dom_element( _answer, Show_what() ) );
    return result;
}

//------------------------------------Command_processor::execute_remote_scheduler_start_remote_task

xml::Element_ptr Command_processor::execute_remote_scheduler_start_remote_task( const xml::Element_ptr& start_task_element )
{
    Z_LOGI2("Z-REMOTE-118", Z_FUNCTION << " " << start_task_element.xml_string() << "\n");
    if (!_spooler->_remote_commands_allowed_for_licence) z::throw_xc("SCHEDULER-717");
    if( _security_level < Security::seclev_all )  z::throw_xc( "SCHEDULER-121" );
    _spooler->assert_is_activated( Z_FUNCTION );

    int  tcp_port = start_task_element.int_getAttribute( "tcp_port" );
    string kind   = start_task_element.    getAttribute( "kind" );


    Z_LOG2("Z-REMOTE-118", Z_FUNCTION << " new Process\n");
    Api_process_configuration api_process_configuration;
    api_process_configuration._controller_address = Host_and_port(client_host(), tcp_port);
    api_process_configuration._is_thread = kind == "process";
    api_process_configuration._log_stdout_and_stderr = true;     // Prozess oder Thread soll stdout und stderr selbst über COM/TCP protokollieren
    api_process_configuration._java_options = start_task_element.getAttribute("java_options");
    api_process_configuration._java_classpath = start_task_element.getAttribute("java_classpath");
    ptr<Api_process> process = Api_process::new_process(_spooler, (Prefix_log*)NULL, api_process_configuration);

    Z_LOG2("Z-REMOTE-118", Z_FUNCTION << " process->start()\n");
    process->start();

    if (_communication_operation) {  // Old plain TCP
        Z_LOG2("Z-REMOTE-118", Z_FUNCTION << " register_task_process()\n");
        _communication_operation->_operation_connection->register_task_process( process );
    } else {
        _spooler->register_api_process(process);
    }
    
    if (_log) _log->info(message_string("SCHEDULER-848", process->pid(), api_process_configuration._controller_address.as_string()));

    xml::Element_ptr result = _answer.createElement( "process" ); 
    result.setAttribute( "process_id", process->process_id() );
    if( process->pid() )  result.setAttribute( "pid", process->pid() );
    Z_LOG2("Z-REMOTE-118", Z_FUNCTION << " result=" << result.xml_string() << "\n");
    return result;
}

//------------------------------------Command_processor::execute_remote_scheduler_remote_task_close

xml::Element_ptr Command_processor::execute_remote_scheduler_remote_task_close( const xml::Element_ptr& close_element )
{
    if (!_spooler->_remote_commands_allowed_for_licence) z::throw_xc("SCHEDULER-717");
    if( _security_level < Security::seclev_all )  z::throw_xc( "SCHEDULER-121" );
    _spooler->assert_is_activated( Z_FUNCTION );

    int  process_id = close_element. int_getAttribute( "process_id" );
    bool kill       = close_element.bool_getAttribute( "kill", false );

    Api_process* process = _communication_operation?  // Old plain TCP
        _communication_operation->_operation_connection->get_task_process( process_id )
        : _spooler->task_process(process_id);

    if( kill )  process->kill();

    ptr<Remote_task_close_command_response> response = Z_NEW(Remote_task_close_command_response(process, _communication_operation ? _communication_operation->_connection : NULL));
    response->set_async_manager( _spooler->_connection_manager );
    response->async_continue();
    if (_communication_operation) {  // Old plain TCP
        _response = response;
        return xml::Element_ptr();
    } else
        return _answer.createElement("ok");
}

//--------------------------------------------------------------Command_processor::execute_add_jobs

xml::Element_ptr Command_processor::execute_add_jobs( const xml::Element_ptr& add_jobs_element )
{
    if( _security_level < Security::seclev_all )  z::throw_xc( "SCHEDULER-121" );

    _spooler->cmd_add_jobs( add_jobs_element );

    return _answer.createElement( "ok" );
}

//-------------------------------------------------------------------Command_processor::execute_job

xml::Element_ptr Command_processor::execute_job( const xml::Element_ptr& job_element )
{
    if( _security_level < Security::seclev_all )  z::throw_xc( "SCHEDULER-121" );

    _spooler->cmd_job( job_element );

    return _answer.createElement( "ok" );
}

//-------------------------------------------------------------Command_processor::execute_job_chain

xml::Element_ptr Command_processor::execute_job_chain( const xml::Element_ptr& job_chain_element )
{
    if( _security_level < Security::seclev_all )  z::throw_xc( "SCHEDULER-121" );

    _spooler->root_folder()->job_chain_folder()->add_or_replace_file_based_xml( job_chain_element );


    //// Siehe auch Spooler::set_dom()
    //ptr<Job_chain> job_chain = new Job_chain( _spooler );
    //job_chain->set_folder_path( root_path );
    //job_chain->set_name( job_chain_element.getAttribute( "name" ) );
    //job_chain->set_dom( job_chain_element );

    //job_chain->initialize();
    //_spooler->root_folder()->job_chain_folder()->add_job_chain( job_chain );
    //job_chain->activate();

    return _answer.createElement( "ok" );
}

//-------------------------------------------------------Command_processor::execute_show_job_chains

xml::Element_ptr Command_processor::execute_show_job_chains( const xml::Element_ptr&, const Show_what& show_ )
{
    if( _security_level < Security::seclev_info )  z::throw_xc( "SCHEDULER-121" );

    Show_what show = show_;
    if( show.is_set( show_all_   ) )  show |= show._what | show_description | show_orders;
    if( show.is_set( show_orders ) )  show |= show_job_chain_orders;

    return _spooler->root_folder()->job_chain_folder()->dom_element( _answer, show | show_job_chains | show_job_chain_jobs );
}

//--------------------------------------------------------Command_processor::execute_show_job_chain

xml::Element_ptr Command_processor::execute_show_job_chain( const xml::Element_ptr& show_job_chain_element, const Show_what& show_ )
{
    if( _security_level < Security::seclev_info )  z::throw_xc( "SCHEDULER-121" );

    Show_what show = show_;
    if( show.is_set( show_all_   ) )  show |= show._what | show_description | show_orders;
    if( show.is_set( show_orders ) )  show |= show_job_chain_orders;

    Absolute_path job_chain_path = Absolute_path( root_path, show_job_chain_element.getAttribute( "job_chain" ) );

    return _spooler->order_subsystem()->job_chain( job_chain_path )->dom_element( _answer, show );
}

//------------------------------------------------------------Command_processor::execute_show_order

xml::Element_ptr Command_processor::execute_show_order( const xml::Element_ptr& show_order_element, const Show_what& show_ )
{
    if( _security_level < Security::seclev_info )  z::throw_xc( "SCHEDULER-121" );

    Show_what show = show_;
    if( show.is_set( show_all_ ) )  show = Show_what( show_standard );

    Absolute_path job_chain_path = Absolute_path( root_path, show_order_element.getAttribute( "job_chain" ) );
    Order::Id     id             = show_order_element.getAttribute( "order"      );
    string        history_id     = show_order_element.getAttribute( "history_id" );
    string        id_string      = string_from_variant( id );
    ptr<Order>    order;
    string        log;

    if( history_id == "" )
    {
        Job_chain* job_chain = _spooler->order_subsystem()->job_chain( job_chain_path );
        
        order = job_chain->order_or_null( id );

        if( !order  &&  job_chain->is_distributed() ) 
            order = _spooler->order_subsystem()->try_load_distributed_order_from_database( (Transaction*)NULL, job_chain_path, id, Order_subsystem::lo_allow_occupied);
    }

    if( !order  &&  _spooler->db()->opened() )
    {
        Read_transaction ta ( _spooler->db() );
    
        if( history_id == "" )
        {
            Any_file sel = ta.open_result_set(
                           " select max(`history_id`) as history_id_max "
                           "  from " + _spooler->db()->_order_history_tablename +
                           "  where `spooler_id`=" + sql::quoted( _spooler->id_for_db() ) + 
                            " and `job_chain`="   + sql::quoted( job_chain_path.without_slash() ) +
                            " and `order_id`="    + sql::quoted( id_string ),
                            Z_FUNCTION );

            if( !sel.eof() )  history_id = sel.get_record().as_string( "history_id_max" );
        }

        if( history_id != "" )  
        {
            S select_sql;
            select_sql <<  "select `order_id`, `start_time`, `title`, `state`, `state_text`"
                           "  from " << _spooler->db()->_order_history_tablename <<
                           "  where `history_id`=" << history_id;
            if( id_string != "" )  select_sql << " and `order_id`=" << sql::quoted( id_string ); 

            Any_file sel = ta.open_result_set( S() << select_sql, Z_FUNCTION );

            if( !sel.eof() ) 
            {
                Record record = sel.get_record();

                //order = Z_NEW( Order( _spooler, sel.get_record() );
                order = _spooler->standing_order_subsystem()->new_order();
                order->set_id        ( record.as_string( "order_id"   ) );
                order->set_state     ( record.as_string( "state"      ) );
                order->set_state_text( record.as_string( "state_text" ) );
                order->set_title     ( record.as_string( "title"      ) );
              //order->set_priority  ( record.as_int   ( "priority"   ) );
                sel.close();

                if( show.is_set( show_log ) )
                {
                    log = file_as_string( S() << "-binary " GZIP_AUTO << _spooler->db()->db_name() << " -table=" + _spooler->db()->_order_history_tablename << " -blob=log" 
                                             " where `history_id`=" << history_id );
                }

                /* Payload steht nicht in der Historie
                if( show & show_payload )
                {
                    string payload = file_as_string( GZIP_AUTO + _spooler->db()->db_name() + " -table=" + db()->_order_history_tablename + " -clob=\"PAYLOAD\"" 
                                                     " where \"HISTORY_ID\"=" + history_id );
                    if( payload != "" )  order->set_payload( payload );
                }
                */
            }
        }
    }

    if( !order )  z::throw_xc( "SCHEDULER-162", id_string, job_chain_path );
    return order->dom_element( _answer, show, log != ""? &log : NULL );
}

//-------------------------------------------------------------Command_processor::execute_add_order

xml::Element_ptr Command_processor::execute_add_order( const xml::Element_ptr& add_order_element )
{
    if( _security_level < Security::seclev_all )  z::throw_xc( "SCHEDULER-121" );
    _spooler->assert_is_activated( Z_FUNCTION );

    //string job_name = add_order_element.getAttribute( "job" );

    ptr<Order> order = _spooler->standing_order_subsystem()->new_order();
    order->set_dom( add_order_element, &_variable_set_map );


    //if( job_name == "" )
    //{
        bool       replace   = add_order_element.bool_getAttribute( "replace", true );
        Job_chain* job_chain = _spooler->order_subsystem()->job_chain( Absolute_path( root_path, add_order_element.getAttribute( "job_chain" ) ) );

        if( replace )  order->place_or_replace_in_job_chain( job_chain );
                 else  order->place_in_job_chain( job_chain );
    //}
    //else 
    //{
    //    order->add_to_job( job_name );
    //}


    xml::Element_ptr result = _answer.createElement( "ok" ); 
    result.appendChild( order->dom_element( _answer, Show_what() ) );
    return result;
}

//-----------------------------------------xml::Element_ptr Command_processor::execute_modify_order

xml::Element_ptr Command_processor::execute_modify_order( const xml::Element_ptr& modify_order_element )
{
    if( _security_level < Security::seclev_no_add )  z::throw_xc( "SCHEDULER-121" );
    _spooler->assert_is_activated( Z_FUNCTION );

    Absolute_path job_chain_path = Absolute_path( root_path, modify_order_element.getAttribute( "job_chain" ) );
    Order::Id     id             = modify_order_element.getAttribute( "order"     );
    string        priority       = modify_order_element.getAttribute( "priority"  );
    string        state          = modify_order_element.getAttribute( "state"     );
    string        at             = modify_order_element.getAttribute( "at"        );

    ptr<Job_chain> job_chain = _spooler->order_subsystem()->job_chain( job_chain_path );
    ptr<Order> order = job_chain->is_distributed()? job_chain->order_or_null( id ) 
                                                  : job_chain->order( id );
    if (!order  &&  job_chain->is_distributed()) {
        string occupying_cluster_member_id;
        try {
            // FIXME Order_subsystem::lo_lock JS-1218 <modify_order> on distributed order does not lock order against concurrent modification
            order = _spooler->order_subsystem()->load_distributed_order_from_database((Transaction*)NULL, job_chain_path, id, (Order_subsystem::Load_order_flags)0, &occupying_cluster_member_id);  // Exception, wenn von einem Scheduler belegt
        } catch (const Xc& x) {
            if (string(x.code()) == "SCHEDULER-379") {  // Order is occupied? (in cluster)
                _spooler->_cluster->post_command_to_cluster_member(modify_order_element, occupying_cluster_member_id); 
            } 
            throw;  // Fehler trotzdem melden
        }
    }
    assert(order);
    if (modify_order_element.getAttribute("action") == "reset") {   // Außerhalb der Transaktion, weil move_to_other_nested_job_chain() wegen remove_from_job_chain() eigene Transaktionen öffnet.
        order->reset();
    }
    if (xml::Element_ptr run_time_element = modify_order_element.select_node("run_time")) {
        order->set_schedule((File_based*)NULL, run_time_element);
    }
    if (xml::Element_ptr params_element = modify_order_element.select_node("params")) {
        ptr<Com_variable_set> params = new Com_variable_set;
        params->set_dom(params_element);
        order->params()->merge(params);
    }
    if (xml::Element_ptr xml_payload_element = modify_order_element.select_node("xml_payload")) {
        order->set_payload_xml(xml_payload_element.first_child_element());
    }
    if (priority != "") {
        order->set_priority(as_int(priority));
    }
    if (state != "") {
        order->assert_no_task(Z_FUNCTION);
        order->set_state(state);
    }
    if (modify_order_element.hasAttribute("end_state")) {
        order->set_end_state(modify_order_element.getAttribute("end_state"));
    }
    if (at != "") {
        order->set_at(Time::of_date_time_with_now(at, _spooler->_time_zone_name));
    }
    if (modify_order_element.hasAttribute("setback")) {
        if (modify_order_element.bool_getAttribute("setback")) {
            z::throw_xc("SCHEDULER-351", modify_order_element.getAttribute("setback"));
            //order->setback();
        } else {
            order->assert_no_task(Z_FUNCTION);
            order->clear_setback(true);        // order->_setback_count belassen
        }
    }
    if (modify_order_element.hasAttribute("suspended")) {
        order->set_suspended(modify_order_element.bool_getAttribute("suspended"));
    }
    if (modify_order_element.hasAttribute("title")) {
        order->set_title(modify_order_element.getAttribute("title"));
    }
    if (job_chain && job_chain->orders_are_recoverable()) {
        order->persist();
        for (Retry_transaction ta(_spooler->db()); ta.enter_loop(); ta++) try {
            if (order->finished() && !order->is_file_based() && !order->is_on_blacklist()) {
                order->remove_from_job_chain(Order::jc_remove_from_job_chain_stack, &ta);
                order->close();
            } else {
                order->db_update(Order::update_anyway, &ta);
            }
            ta.commit(Z_FUNCTION);
        }
        catch (exception& x) { ta.reopen_database_after_error(zschimmer::Xc("SCHEDULER-360", _spooler->db()->_orders_tablename, x), Z_FUNCTION); }
    }
    return _answer.createElement( "ok" );
}

//----------------------------------------------------------Command_processor::execute_remove_order

xml::Element_ptr Command_processor::execute_remove_order( const xml::Element_ptr& modify_order_element )
{
    if( _security_level < Security::seclev_no_add )  z::throw_xc( "SCHEDULER-121" );
    _spooler->assert_is_activated( Z_FUNCTION );

    Absolute_path job_chain_path = Absolute_path( root_path, modify_order_element.getAttribute( "job_chain" ) );
    Order::Id     id             = modify_order_element.getAttribute( "order"     );

    ptr<Job_chain> job_chain = _spooler->order_subsystem()->job_chain( job_chain_path );
    ptr<Order>     order     = job_chain->is_distributed()? job_chain->order_or_null( id ) 
                                                          : job_chain->order( id );

    if( order )
    {
        order->remove( File_based::rm_base_file_too );
    }
    else
    {
        assert( job_chain->is_distributed() );
        bool ok = false;

        for( Retry_transaction ta ( _spooler->db() ); ta.enter_loop(); ta++ ) try
        {
            sql::Delete_stmt delete_stmt ( _spooler->db()->database_descriptor(), _spooler->db()->_orders_tablename );

            delete_stmt.add_where( _spooler->order_subsystem()->order_db_where_condition( job_chain_path, id.as_string() ) );
          //delete_stmt.and_where_condition( "occupying_cluster_member_id", sql::null_value );
            
            ta.execute( delete_stmt, Z_FUNCTION );
            ok = ta.record_count() == 1;
            
            //if( ta.record_count() == 0 )
            //{
            //    // Sollte Exception auslösen: nicht da oder belegt
            //    _spooler->order_subsystem()->load_distributed_order_from_database( job_chain_path, id );
            //    
            //    // Der Auftrag ist gerade freigegeben oder hinzugefügt worden
            //    delete_stmt.remove_where_condition( "occupying_cluster_member_id" );
            //    ta.execute_single( delete_stmt, Z_FUNCTION ); 
            //}

            ta.commit( Z_FUNCTION );
        }
        catch( exception& x ) { ta.reopen_database_after_error( zschimmer::Xc( "SCHEDULER-360", _spooler->db()->_orders_tablename, x ), Z_FUNCTION ); }
        if (!ok) z::throw_xc( "SCHEDULER-162", id.as_string() );
    }

    return _answer.createElement( "ok" );
}

//------------------------------------------------------Command_processor::execute_remove_job_chain

xml::Element_ptr Command_processor::execute_remove_job_chain( const xml::Element_ptr& modify_order_element )
{
    if( _security_level < Security::seclev_no_add )  z::throw_xc( "SCHEDULER-121" );
    _spooler->assert_is_activated( Z_FUNCTION );

    Absolute_path job_chain_path = Absolute_path( root_path, modify_order_element.getAttribute( "job_chain" ) );

    _spooler->order_subsystem()->job_chain( job_chain_path )->remove( File_based::rm_base_file_too );

    return _answer.createElement( "ok" );
}

//---------------------------------------------Command_processor::execute_register_remote_scheduler

xml::Element_ptr Command_processor::execute_register_remote_scheduler( const xml::Element_ptr& register_remote_scheduler_element )
{
    if( !_communication_operation )  z::throw_xc( "SCHEDULER-222", register_remote_scheduler_element.nodeName() );

    if( _security_level < Security::seclev_no_add )  z::throw_xc( "SCHEDULER-121" );

    _spooler->_supervisor->execute_register_remote_scheduler( register_remote_scheduler_element, _communication_operation );

    xml::Element_ptr e = _answer.createElement( "ok" );
    e.setAttribute("recommended", "supervisor.configuration.fetch");   // <supervisor.remote_scheduler.configuration.fetch_updated_files> ist veraltet. <supervisor.configuration.fetch> ist neu
    return e;
}

//-------------------------------------------------------Command_processor::execute_service_request

xml::Element_ptr Command_processor::execute_service_request( const xml::Element_ptr& service_request_element )
{
    if( _security_level < Security::seclev_no_add )  z::throw_xc( "SCHEDULER-121" );
    _spooler->assert_is_activated( Z_FUNCTION );

    ptr<Order> order = _spooler->standing_order_subsystem()->new_order();

    order->set_state( Web_service::forwarding_job_chain_forward_state );
    order->set_payload(Variant(service_request_element.xml_string()));
    order->place_in_job_chain( _spooler->root_folder()->job_chain_folder()->job_chain( Web_service::forwarding_job_chain_name ) );
    
    return _answer.createElement( "ok" );
}

//-------------------------------------------------------Command_processor::execute_service_request

xml::Element_ptr Command_processor::execute_get_events( const xml::Element_ptr& )
{
    ptr<Get_events_command_response> response = Z_NEW( Get_events_command_response( _spooler->_scheduler_event_manager ) );
    _response = response;

    _spooler->_scheduler_event_manager->add_get_events_command_response( response );

    _response->write( "<events>\n" );

    return xml::Element_ptr();    // Antwort wird asynchron übergeben
}

//----------------------------------------------------------Command_processor::execute_job_chain_command

xml::Element_ptr Command_processor::execute_job_chain_command( const xml::Element_ptr& element, const Show_what& show_ )
{
    if( _security_level < Security::seclev_all )  z::throw_xc( "SCHEDULER-121" );
    return _spooler->order_subsystem()->job_chain( Absolute_path( root_path, element.getAttribute( "job_chain" ) ) )->execute_xml( this, element, show_ );
}

//----------------------------------------------------------Command_processor::execute_job_chain_node_command

xml::Element_ptr Command_processor::execute_job_chain_node_command( const xml::Element_ptr& element, const Show_what& show_ )
{
    if( _security_level < Security::seclev_all )  z::throw_xc( "SCHEDULER-121" );
    return _spooler->order_subsystem()->job_chain( Absolute_path( root_path, element.getAttribute( "job_chain" ) ) ) ->
                    node_from_state( element.getAttribute( "state" ) )->execute_xml( this, element, show_ );
}

//-----------------------------------------et_events_command_response::~Get_events_command_response

Get_events_command_response::~Get_events_command_response()
{
    _scheduler_event_manager->remove_get_events_command_response( this );
}

//---------------------------------------------------------------Get_events_command_response::close

void Get_events_command_response::close()
{
    if( !_closed )
    {
        _closed = true;

        write( "</events>" );
        //Z_DEBUG_ONLY( int NULL_BYTE_ANHAENGEN );
    }
}

//---------------------------------------------------------Get_events_command_response::write_event

void Get_events_command_response::write_event( const Scheduler_event& event )
{
    write( event.xml_bytes() );

    if( _append_0_byte )  write( io::Char_sequence( "\0", 1 ) );       // 0-Byte anhängen
                    else  write( "\n" );
}

//---------------------------------------------------------------Command_processor::execute_command

xml::Element_ptr Command_processor::execute_command( const xml::Element_ptr& element )
{
    xml::Element_ptr result;

    if( _log ) 
    {
        Message_string m ( "SCHEDULER-965" );
        //m.set_max_insertion_length( INT_MAX );
        m.insert(1, element.xml_string());
        _log->info( m );
    }

    Show_what show = show_jobs | show_tasks;        // Zur Kompatibilität. Besser: <show_state what="jobs,tasks"/>
    show.set_dom(*_spooler, element);

    string element_name = element.nodeName();
    
    if( string_begins_with( element_name, "job_chain_node." ) )
    {
       result = execute_job_chain_node_command(element,show);
    }
    else
    if( string_begins_with( element_name, "job_chain." ) )
    {
       result = execute_job_chain_command(element,show);
    }
    else
    if( element_name == "lock"  ||  string_begins_with( element_name, "lock." ) )
    {
        result = _spooler->lock_subsystem()->execute_xml( this, element, show );
    }
    else
    if( element_name == "process_class"  ||  string_begins_with( element_name, "process_class." ) )
    {
        result = _spooler->process_class_subsystem()->execute_xml( this, element, show );
    }
    else
    if( string_begins_with( element_name, "schedule." ) )
    {
        result = _spooler->schedule_subsystem()->execute_xml( this, element, show );
    }
    else
    if( string_begins_with( element_name, "scheduler_log." ) )
    {
        result = execute_scheduler_log( element, show );
    }
    else
    if( string_begins_with( element_name, "licence." ) )
    {
        result = execute_licence( element, show );
    }
    else

    // JS-507: Informationen zu den Subsystemen
    if( string_begins_with( element_name, "subsystem." ) )
    {
        result = execute_subsystem( element, show );
    }

    else
    if( string_begins_with( element_name, "supervisor." ) )  _response = _spooler->_supervisor->execute_xml( element, this );
    else
    if( element.nodeName_is( "show_state"       ) 
     || element.nodeName_is( "s"                ) )  result = execute_show_state( element, show );  
    else
    if( element.nodeName_is( "show_calendar"    ) )  result = execute_show_calendar( element, show );
    else
    if( element.nodeName_is( "show_history"     ) )  result = execute_show_history( element, show );
    else
    if( element.nodeName_is( "modify_hot_folder" ) )  result = execute_modify_hot_folder( element );
    else
    if( element.nodeName_is( "modify_spooler"   ) )  result = execute_modify_spooler( element );
    else
    if( element.nodeName_is( "terminate"        ) )  result = execute_terminate( element );
    else
    if( element.nodeName_is( "modify_job"       ) )  result = execute_modify_job( element );
    else
    if( element.nodeName_is( "show_job"         ) )  result = execute_show_job( element, show );
    else
    if( element.nodeName_is( "show_jobs"        ) )  result = execute_show_jobs( show );
    else
    if( element.nodeName_is( "start_job"        ) )  result = execute_start_job( element );
    else
    if( element.nodeName_is( "remote_scheduler.start_remote_task" ) )  result = execute_remote_scheduler_start_remote_task( element );
    else
    if( element.nodeName_is( "remote_scheduler.remote_task.close" ) )  result = execute_remote_scheduler_remote_task_close( element );
    else
    if( element.nodeName_is( "show_cluster"     ) )  result = execute_show_cluster( element, show );
    else
    if( element.nodeName_is( "show_task"        ) )  result = execute_show_task( element, show );
    else
    if( element.nodeName_is( "check_folders"    ) )  result = execute_check_folders( element );
    else
    if( element.nodeName_is( "kill_task"        ) )  result = execute_kill_task( element );
    else
    if( element.nodeName_is( "add_jobs"         ) )  result = execute_add_jobs( element );
    else
    if( element.nodeName_is( "job"              ) )  result = execute_job( element );
    else
    if( element.nodeName_is( "job_chain"        ) )  result = execute_job_chain( element );
    else
    if( element.nodeName_is( "config"           ) )  result = execute_config( element );
    else
    if( element.nodeName_is( "show_job_chains"  ) )  result = execute_show_job_chains( element, show );
    else
    if( element.nodeName_is( "show_job_chain"   ) )  result = execute_show_job_chain( element, show );
    else
    if( element.nodeName_is( "show_order"       ) )  result = execute_show_order( element, show );
    else
    if( element.nodeName_is( "add_order"        ) ||
        element.nodeName_is( "order"            ) )  result = execute_add_order( element );
    else
    if( element.nodeName_is( "modify_order"     ) )  result = execute_modify_order( element );
  //else
  //if( element.nodeName_is( "show_order_history" ) )  result = execute_show_order_history( element, show );
    else
    if( element.nodeName_is( "param"            ) ) { _spooler->_variables->set_variable( element );  result = _answer.createElement( "ok" ); }
    else
    if( element.nodeName_is( "param.get"        ) ) { Com_variable* v = _spooler->_variables->variable_or_null( element.getAttribute( "name" ) );
                                                      result = v? v->dom_element( _answer, "param" ) 
                                                                : _answer.createElement( "ok" ); }
    else
    if( element.nodeName_is( "params"           ) ) { _spooler->_variables->set_dom( element );  result = _answer.createElement( "ok" ); }
    else
    if( element.nodeName_is( "params.get"       ) ) { result = _spooler->_variables->dom_element( _answer, "params", "param" ); }
    else
    if( element.nodeName_is( "register_remote_scheduler" ) )  result = execute_register_remote_scheduler( element );
    else
    if( element.nodeName_is( "remove_order"     ) )  result = execute_remove_order( element );
    else
    if( element.nodeName_is( "remove_job_chain" ) )  result = execute_remove_job_chain( element );
    else
    if( element.nodeName_is( "service_request"  ) )  result = execute_service_request( element );
    else
    if( element.nodeName_is( "setting.set"  ) )  result = execute_setting_set( element );
    else
    if( element.nodeName_is( "events.get" ) )  result = execute_get_events( element );   // Nicht offiziell, nur Test
    else
    if( element.nodeName_is( "job.why"          ) )  result = execute_job_why( element );
    else
        result = execute_command_in_java(element);

    return result? _answer.documentElement().firstChild().appendChild(result)
        : result;
}

//-------------------------------------------------------Command_processor::execute_command_in_java

xml::Element_ptr Command_processor::execute_command_in_java(const xml::Element_ptr& element) {
    string result = _spooler->schedulerJ().javaExecuteXml(element.xml_string());;
    if (result == "UNKNOWN_COMMAND")  z::throw_xc("SCHEDULER-105", element.nodeName());   //Provisorisch, bis Java ordentliche Scheduler-Exceptions liefert
    return result == ""? xml::Element_ptr() : _answer.importNode(xml::Document_ptr::from_xml_string(result).documentElement());
}

//------------------------------------------------------------------------------------xml_as_string

string xml_as_string( const xml::Document_ptr& document)
{
    string result;

    try 
    {
        result = document.xml_string();
        //if( indent_string != "" && result.find('\r') == string::npos)  result = replace_regex( result, "\n", "\r\n" );      // Für Windows-telnet
    }
    catch( const exception&  ) { return "<?xml version=\"1.0\"?><ERROR/>"; }
    catch( const _com_error& ) { return "<?xml version=\"1.0\"?><ERROR/>"; }

    return result;
}

//-------------------------------------------------------------------Command_processor::execute_http

void Command_processor::execute_http( http::Request* http_request, http::Response* http_response, Http_file_directory* http_file_directory )
{
    string          path                    = http_request->url_path();
    string          response_body;
    string          response_content_type;
    string const    show_log_request        = "/show_log?";

    try
    {
        if( _security_level < Security::seclev_info )  z::throw_xc( "SCHEDULER-121" );  // JS-486, hier keine Prüfung mehr

        if( path.find( ".." ) != string::npos )  z::throw_xc( "SCHEDULER-214", path );
//        if( path.find( ":" )  != string::npos )  z::throw_xc( "SCHEDULER-214", path );    // JS-748: timestamps in the command use the colon

        if( http_request->http_method() == "GET" )
        {
            if( string_begins_with( path, "/<" ) )   // direct XML command, e.g. <show_state/>, ASCII
            {
                string xml = path.substr( 1 );
                if (!string_begins_with(xml, "<show_") && !string_begins_with(xml, "<s ") && !string_begins_with(xml, "<s/")) throw http::Http_exception(http::status_403_forbidden, "");
                http_response->set_header( "Cache-Control", "no-cache" );
                response_body = execute_xml_string( xml, "  " );  // JS-486, the security level will be checked here, too (with error message)
                response_content_type = "text/xml; charset=" + string_encoding;
            }
            else
            if( string_ends_with( path, "?" ) )
            {
                string base_url = http_request->parameter("base_url");
                http_response->set_header( "Cache-Control", "no-cache" );

                if( string_ends_with( path, show_log_request ) )
                {
                    ptr<Prefix_log> log;

                    if( http_request->has_parameter( "job"   ) )
                    {
                        log = _spooler->job_subsystem()->job( Absolute_path( root_path, http_request->parameter( "job" ) ) )->_log;
                    }
                    else
                    if( http_request->has_parameter( "task"  ) )
                    {
                        int       task_id = as_int( http_request->parameter( "task" ) );
                        ptr<Task> task    = _spooler->get_task_or_null( task_id );

                        if( task )
                            log = task->log();
                        else
                        {
                            xml::Element_ptr task_element = _spooler->db()->read_task( _answer, task_id, show_log );
                            S title;  title << "Task " << task_id;

                            DOM_FOR_EACH_ELEMENT( task_element, e )
                            {
                                if( e.nodeName_is( "log" ) )
                                {
                                    //TODO Log wird im Speicher gehalten! Besser: In Datei schreiben, vielleicht sogar Task und Log anlegen
                                    http_response->set_chunk_reader( Z_NEW( http::Html_chunk_reader( Z_NEW( http::String_chunk_reader( e.text(), "text/plain; charset=" + string_encoding) ), base_url, title ) ) );
                                    return;
                                }
                            }

                            http_response->set_chunk_reader( Z_NEW( http::Html_chunk_reader( Z_NEW( http::String_chunk_reader( "Das Protokoll ist nicht lesbar." ) ), base_url, title ) ) );
                            return;
                        }
                    }
                    else
                    if( http_request->has_parameter( "order" ) )
                    {
                        Absolute_path job_chain_path = Absolute_path( root_path, http_request->parameter( "job_chain" ) );
                        string        order_id       = http_request->parameter( "order" );
                        string        history_id     = http_request->parameter( "history_id" );
                        

                        if( history_id == "" )
                        {
                            Job_chain* job_chain = _spooler->order_subsystem()->job_chain( job_chain_path );
                            ptr<Order> order     = job_chain->order_or_null( order_id );

                            if( !order  &&  job_chain->is_distributed() ) 
                            {
                                order = _spooler->order_subsystem()->try_load_distributed_order_from_database((Transaction*)NULL, job_chain_path, order_id, Order_subsystem::lo_allow_occupied);

                                if( order )
                                {
                                    //TODO Log wird im Speicher gehalten! Besser: In Datei schreiben
                                    http_response->set_chunk_reader( Z_NEW( http::Html_chunk_reader( Z_NEW( http::String_chunk_reader( order->log()->as_string(), "text/plain; charset=" + string_encoding)), base_url, order->log()->title() ) ) );
                                    return;
                                }
                            }

                            if( order )
                            {
                                log = order->_log;
                            }
                        }

                        if( !log )
                        {
                            Read_transaction ta ( _spooler->db() );

                            if( history_id == "" )
                            {
                                S select_sql;
                                select_sql << "select max( `history_id` ) as history_id_max "
                                               "  from " + _spooler->db()->_order_history_tablename +
                                               "  where `spooler_id`=" << sql::quoted( _spooler->id_for_db() ) + 
                                                 " and `job_chain`="   << sql::quoted( job_chain_path.without_slash() ) +
                                                 " and `order_id`="    << sql::quoted( order_id );
                                if( order_id != "" )  select_sql << " and `order_id`=" << sql::quoted( order_id );

                                Any_file sel = ta.open_result_set( select_sql, Z_FUNCTION );

                                if( !sel.eof() )
                                {
                                    history_id = sel.get_record().as_string( "history_id_max" );
                                }
                            }

                            if( history_id != "" )
                            {
                                string log_text = file_as_string( "-binary " GZIP_AUTO + _spooler->db()->db_name() + " -table=" + _spooler->db()->_order_history_tablename + " -blob=\"LOG\"" 
                                                                  " where `history_id`=" + history_id );
                                string title = "Auftrag " + order_id;
                                //TODO Log wird im Speicher gehalten! Besser: In Datei schreiben, vielleicht sogar Order und Log anlegen
                                http_response->set_chunk_reader( Z_NEW( http::Html_chunk_reader( Z_NEW( http::String_chunk_reader( log_text ) ), base_url, title ) ) );
                                return;
                            }

                            throw http::Http_exception( http::status_404_bad_request, "No order log" );
                        }
                    }

                    // Aufruf ohne Parameter, d.h. show_log?
                    else
                    {
                        log = _spooler->_log;  // Hauptprotokoll
                    }

                    if( log )
                    {
                        http_response->set_chunk_reader( Z_NEW( http::Html_chunk_reader( Z_NEW( http::Log_chunk_reader( log ) ), base_url, log->title() ) ) );
                        return;
                    }
                }
                else
                if( string_ends_with( path, "/job_description?" ) )
                {
                    Job* job = _spooler->job_subsystem()->job( Absolute_path( root_path, http_request->parameter( "job" ) ) );
                    
                    string description = job->description();
                    if (description == "") throw http::Http_exception( http::status_404_bad_request, "Der Job hat keine Beschreibung" );

                    if (http_request->header("accept").find("text/html") != string::npos) {
                        response_content_type = "text/html";
                        response_body = "<html><head><title>Scheduler-Job " + job->name() + "</title>";
                        response_body += "<style type='text/css'> @import 'scheduler.css'; @import 'custom.css';</style>";
                        response_body += "<body id='job_description'>";
                        response_body += description;
                        response_body += "</body></html>";
                    } else {
                        response_content_type = "text/plain";
                        response_body = description;
                    }
                }
                else
                if( string_ends_with( path, "/show_config?" ) )
                {
                    if( _spooler->_config_document )  response_body = _spooler->_config_document.xml_bytes(string_encoding);

                    response_content_type = "text/xml";
                }
                else
                    throw http::Http_exception( http::status_404_bad_request, "Ungueltiger URL-Pfad: " + path );
            }
            else
            {
                if( filename_of_path( path ).find( '.' ) == string::npos )      // Kein Punkt: Es muss ein Verzeichnis sein!
                {
                    if( !string_ends_with( path, "/" )  &&  isalnum( (uchar)*path.rbegin() ) )  // '?' am Ende führt zum erneuten GET mit demselben Pfad
                    {
                        // (Man könnte hier noch prüfen, ob's wirklich ein Verzeichnis ist.)
                        // Der Browser soll dem Verzeichnisnamen einen Schräger anhängen und das als Basisadresse für weitere Anfragen verwenden.
                        // http://localhost:6310/jz ==> http://localhost:6310/jz/, http://localhost:6310/jz/details.html
                        // Ohne diesen Mechanismus würde http://localhost:6310/details.html, also das Oberverzeichnis gelesen

                        path += "/";
                        http_response->set_status( http::status_301_moved_permanently );
                        http_response->set_header( "Location", "http://" + http_request->header( "host" ) + path );
                        return;
                    }

                    path += default_filename;
                }


                string filename;

                if( http_file_directory )
                {
                    filename = http_file_directory->file_path_from_url_path( path );
                }
                else
                {
                    if( _spooler->_http_server->directory().empty() )  z::throw_xc( "SCHEDULER-212" );
                    filename = File_path( _spooler->_http_server->directory(), path );
                }
/*
                struct stat st;
                memset( &st, 0, sizeof st );
                int err = stat( filename.c_str(), &st );
                if( !err  &&  stat.st_mode & S_IFDIR )
*/

                string extension = extension_of_path( filename );
             
                if( extension == "html"  
                 || extension == "htm"  )  response_content_type = "text/html";
                else
                if( extension == "xml"  )  response_content_type = "text/xml";
                else
                if( extension == "xsl"  )  response_content_type = "text/xml";  // wie xslt?      "text/xsl";
                else
                if( extension == "xslt" )  response_content_type = "text/xml";  // Firefox und Netscape verlangen text/xml!      "text/xslt";
                else
                if( extension == "xsd"  )  response_content_type = "text/xml";
                else
                if( extension == "js"   )  response_content_type = "text/javascript";
                else
                if( extension == "css"  )  response_content_type = "text/css";
                else
                if( extension == "ico"  )  response_content_type = "image/x-ico";
              //else
              //if( extension == "jar"  )  response_content_type = "application/x-java-archive";

                Mapped_file file ( filename, "r" );
                response_body = file.as_string();
            }
        }
        else
        if( http_request->http_method() == "POST" )
        {
            response_body = execute_xml_string( http_request->body(), "  " );   // Anders als bei Jetty ist der C++-Body ist eine Bytefolge, kein String. Die Bytefolge sollte ISO-8859-1-codiert sein, damit das funktioniert.
            response_content_type = "text/xml; charset=" + string_encoding;
        }
        else
            throw http::Http_exception( http::status_405_method_not_allowed );


        if( response_body.empty() )
        {
            response_body = execute_xml_string( "<show_state what=\"all,orders\"/>", "  " );
            response_content_type = "text/xml; charset=" + string_encoding;
        }

        http_response->set_chunk_reader( Z_NEW( http::String_chunk_reader( response_body, response_content_type ) ) );
    }
    catch (http::Http_exception& x) {
        _spooler->log()->debug( message_string( "SCHEDULER-311", http_request->http_method() + " " + path, x ) );
        http_response->set_status( x._status_code, x.what() );
        http_response->set_ready();
    }
    catch( const exception& x ) {
        _spooler->log()->debug( message_string( "SCHEDULER-311", http_request->http_method() + " " + path, x ) );
        http_response->set_status( http::status_404_bad_request, x.what() );
        http_response->set_ready();
        //throw http::Http_exception( http::status_404_bad_request, x.what() );  
    }
}

//---------------------------------------------------Command_processor::response_execute_xml_string

ptr<Command_response> Command_processor::response_execute( const string& xml_text_par, bool is_bytes, const string& indent_string )
{
    try 
    {
        _error = NULL;

        string xml_text = xml_text_par;
        if( strchr( xml_text.c_str(), '<' ) == NULL )  xml_text = "<" + xml_text + "/>";

        try {
            execute_2(dom_from_xml(xml_text, is_bytes));
        } catch (const _com_error& com_error) { throw_com_error(com_error, "DOM/XML"); }

        if( !_answer.documentElement().firstChild().hasChildNodes()  &&  !_response )  z::throw_xc( "SCHEDULER-353" );
    }
    catch( const Xc& x        ) { append_error_to_answer( x );  if( _log ) _log->error( x.what() ); }
    catch( const exception& x ) { append_error_to_answer( x );  if( _log ) _log->error( x.what() ); }

    ptr<Command_response> result = _response;
    if( !result )
    {
        //_spooler->signal("execute_xml");    // Sonst schläft der Scheduler unter SchedulerTest (Java) weiter, wenn executeXml() nach Start aufgerufen wird.
        ptr<Synchronous_command_response> r = Z_NEW( Synchronous_command_response( _answer.xml_bytes(string_encoding, indent_string != "") ) );
        result = +r;
    }
    
    return +result;
}

//------------------------------------------------------------Command_processor::execute_xml_string

string Command_processor::execute_xml_string( const string& xml_text_par, const string& indent_string )
{
    return response_execute_xml_string( xml_text_par, indent_string )->complete_text();
}

//--------------------------------------------------------------Command_processor::execute_xml_bytes

string Command_processor::execute_xml_bytes(const string& xml_text_par, const string& indent_string)
{
  return response_execute_xml_bytes(xml_text_par, indent_string)->complete_text();
}

//------------------------------------------------------------------------Command_processor::execute

xml::Document_ptr Command_processor::execute( const xml::Document_ptr& command_document )
{
    try 
    {
        _error = NULL;
        execute_2( command_document ); 
    }
    catch( const Xc& x        ) { append_error_to_answer( x );  if( _log ) _log->error( x.what() ); }
    catch( const exception& x ) { append_error_to_answer( x );  if( _log ) _log->error( x.what() ); }

    //if( !_spooler->check_is_active() )  _spooler->abort_immediately( Z_FUNCTION );

    return _answer;
}

//-----------------------------------------------------------Command_processor::execute_config_file

void Command_processor::execute_config_file( const string& filename )
{
    try
    {
        _source_filename = filename;

        string content = string_from_file( filename );

        Z_LOGI2( "scheduler", Z_FUNCTION << "\n" << filename << ":\n" << content << "\n" );

        xml::Document_ptr dom_document = dom_from_xml( content, true );
        dom_document.select_node_strict( "/spooler/config" );

        _dont_log_command = true;
        execute_2( dom_document );
    }
    catch( zschimmer::Xc& x )
    {
        x.append_text( "in configuration file " + filename );
        throw;
    }
}

//------------------------------------------------------------Command_processor::dom_from_xml

xml::Document_ptr Command_processor::dom_from_xml( const string& xml_text, bool is_bytes )
{
    Z_LOGI2( "scheduler.xml", "Reading XML document ...\n" );

    xml::Document_ptr command_doc;
    command_doc.create();

    try {
        if (is_bytes) command_doc.load_xml_bytes( xml_text );
                 else command_doc.load_xml_string(xml_text);
    } catch (exception& x) {
        _spooler->log()->error(x.what());       // Log ist möglicherweise noch nicht geöffnet
        throw;
    }

    Z_LOG2( "scheduler.xml", "XML document has been read\n" );

    return command_doc;
}

//----------------------------------------------------------------------Command_processor::execute_2

void Command_processor::execute_2( const xml::Document_ptr& command_doc )
{
    try 
    {
        if( !_dont_log_command )  {
            string line = replace_regex(command_doc.xml_string(), "\\?\\>\n", "?>", 1);
            string nl = !line.empty() && *line.rbegin() == '\n' ? "" : "\n";
            Z_LOG2( "scheduler", "Execute " << line << nl);
        }
        if( _spooler->_validate_xml  &&  _validate )  
        {
            _spooler->_schema.validate( command_doc );
        }

        execute_2( command_doc.documentElement() );
    }
    catch( const _com_error& com_error ) { throw_com_error( com_error, "DOM/XML" ); }

    // Eigentlich nur für einige möglicherweise langlaufende <show_xxx>-Kommandos nötig, z.B. <show_state>, <show_history> (mit Datenbank)
    if( !_spooler->check_is_active() )  _spooler->cmd_terminate_after_error( Z_FUNCTION, command_doc.xml_string());
}

//---------------------------------------------------------------------Command_processor::execute_2

void Command_processor::execute_2( const xml::Element_ptr& element )
{
    xml::Element_ptr e = element;

    if( e.nodeName_is( "spooler" ) ) 
    {
        xml::Node_ptr n = e.firstChild(); 
        while( n  &&  n.nodeType() != xml::ELEMENT_NODE )  n = n.nextSibling();
        e = n;
    }

    if( e )
    {
        if( e.nodeName_is( "commands" )  ||  e.nodeName_is( "command" )  ||  e.nodeName_is( "cluster_member_command" ) )
        {
            execute_commands( e );
        }
        else
        {
            xml::Element_ptr response_element = execute_command( e );  
            
            if( !response_element  &&  !_response )  z::throw_xc( "SCHEDULER-353", e.nodeName() );
        }
        
        if( e != element )  // In einer Verschachtelung von <spooler>?
        {
            xml::Node_ptr n = e.nextSibling(); 
            while( n  &&  n.nodeType() != xml::ELEMENT_NODE )  n = n.nextSibling();
            e = n;
            if( e )  z::throw_xc( "SCHEDULER-319", e.nodeName() ); 
        }
    }
}

//--------------------------------------------------------------Command_processor::execute_commands

void Command_processor::execute_commands( const xml::Element_ptr& commands_element )
{
    DOM_FOR_EACH_ELEMENT( commands_element, node )
    {
        xml::Element_ptr response_element = execute_command( node );
        if( !response_element )  z::throw_xc( "SCHEDULER-353", node.nodeName() );
    }
}

//------------------------------------------------------------------Command_processor::begin_answer

void Command_processor::begin_answer()
{
    if( !_answer )
    {
        _answer.create();
        _answer.appendChild( _answer.createElement( "spooler" ) );

        xml::Element_ptr answer_element = _answer.documentElement().appendChild( _answer.createElement( "answer" ) );
        answer_element.setAttribute( "time", Time::now().xml_value() );
    }
}

//--------------------------------------------------------Command_processor::append_error_to_answer

void Command_processor::append_error_to_answer( const exception& x )
{
    _error = x;
    if( _answer  &&  _answer.documentElement()  &&  _answer.documentElement().firstChild() ) 
        append_error_element( _answer.documentElement().firstChild(), x );
}

//--------------------------------------------------------Command_processor::append_error_to_answer

void Command_processor::append_error_to_answer( const Xc& x )
{
    _error = x;
    if( _answer  &&  _answer.documentElement()  &&  _answer.documentElement().firstChild() ) 
        append_error_element( _answer.documentElement().firstChild(), x );
}

//---------------------------------------------------------------Command_response::Command_response

//Command_response::Command_response()
//{
//}

//--------------------------------------------------------------------------Command_response::close

//void Command_response::close()
//{
//    Xml_response::close();
//}

//--------------------------------------------------------Command_response::begin_standard_response

void Command_response::begin_standard_response()
{
    _xml_writer.set_encoding( string_encoding );
    _xml_writer.write_prolog();

    write( "<spooler><answer time=\"" );
    write( Time::now().xml_value() );
    write( "\">" );
}

//----------------------------------------------------------Command_response::end_standard_response

void Command_response::end_standard_response()
{
    write( "</answer></spooler>" );
    write( io::Char_sequence( "\0", 1 ) );  // Null-Byte terminiert die XML-Antwort
}

//-----------------------------------File_buffered_command_response::File_buffered_command_response

File_buffered_command_response::File_buffered_command_response()
: 
    _zero_(this+1), 
    _buffer_size(recommended_response_block_size)
{
    _buffer.reserve( _buffer_size );
}

//----------------------------------File_buffered_command_response::~File_buffered_command_response
    
File_buffered_command_response::~File_buffered_command_response()
{
}

//------------------------------------------------------------File_buffered_command_response::close
                                                                                                
void File_buffered_command_response::close()
{
    if( _state == s_ready  &&  _buffer == "" )  _state = s_finished;
    _close = true;
}

//------------------------------------------------------------File_buffered_command_response::flush

void File_buffered_command_response::flush()
{
    if( _state == s_ready  &&  _buffer == "" )  _state = s_finished;
    _close = true;
}

//--------------------------------------------------File_buffered_command_response::async_continue_

bool File_buffered_command_response::async_continue_( Continue_flags )
{
    z::throw_xc( Z_FUNCTION );
    return false;
}

//------------------------------------------------------------File_buffered_command_response::write

void File_buffered_command_response::write( const io::Char_sequence& seq )
{
    if( _close )
    {
        Z_LOG( "*** " << Z_FUNCTION << "  closed: " << seq << "\n" );
        return;
    }


    if( seq.length() == 0 )  return;

    if( _state != s_congested )
    {
        if( _buffer.length() == 0  ||  _buffer.length() + seq.length() <= _buffer_size )
        {
            _buffer.append( seq.ptr(), seq.length() );
            signal_new_data();  // Mit Senden beginnen
        }
        else
        {
            _state = s_congested;
        }
    }

    if( _state == s_congested )
    {
        if( !_congestion_file.opened() )  
        {
            _congestion_file.open_temporary( File::open_unlink );
            _congestion_file.print( _buffer );
            _congestion_file_write_position += _buffer.length();
            _buffer = "";
        }
        
        if( _last_seek_for_read )
        {
            _congestion_file.seek( _congestion_file_write_position );
            _last_seek_for_read = false;
        }

        _congestion_file.print( seq );
        
        _congestion_file_write_position += seq.length();
    }
}

//----------------------------------------------------File_buffered_command_response::complete_text

string File_buffered_command_response::complete_text() 
{
    if (_state == s_congested)
        return _congestion_file.read_all();
    else
        return _buffer;
}

//---------------------------------------------------------File_buffered_command_response::get_part

string File_buffered_command_response::get_part()
{
    string result;

    switch( _state )
    {
        case s_ready:
        {
            if( _buffer == "" ) 
            {
                // Leeren String zurückgeben bedeutet, dass noch keine neuen Daten da sind
            }
            else
            {
                result = _buffer;
                _buffer = "";
            }

            break;
        }

        case s_congested:
        {
            if( !_last_seek_for_read )
            {
                _congestion_file.seek( _congestion_file_read_position );
                _last_seek_for_read = true;
            }

            result = _congestion_file.read_string( _buffer_size );

            _congestion_file_read_position += result.length();
            
            if( _congestion_file_read_position == _congestion_file_write_position )
            {
                _congestion_file.seek( 0 );
                _congestion_file.truncate( 0 );
                _congestion_file_read_position = 0;
                _congestion_file_write_position = 0;

                _state = s_ready;
            }

            break;
        }

        case s_finished:
            break;

        default:
            z::throw_xc( Z_FUNCTION, _state );
    }

    if( _close  &&  _state == s_ready  &&  _buffer == "" )  _state = s_finished;

    return result;
}

//-------------------------------------------------------------------------------------------------

} //namespace scheduler
} //namespace sos
