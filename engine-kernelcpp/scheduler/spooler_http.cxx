// $Id: spooler_http.cxx 14154 2010-12-14 15:45:45Z ss $
/*
    Hier sind implementiert

    Parser
    Request
    Response
    Log_chunk_reader
    Html_chunk_reader
    String_chunk_reader

    Notdürftige Implementierung von HTTP 1.1
    Siehe http://www.w3.org/Protocols/rfc2616/rfc2616.html
*/


/*
    Mögliche Verbesserungen:

    HTTP 1.1 ist nur dürftig implementiert.
    Pipelining ist derzeit nicht möglich. Dazu muss ein signalisiertes recv während send() möglich sein --> Async_socket umarbeiten, getrenntes _state für recv() und send()
    "Transfer-Encoding: chunked" nicht, wenn nur ein oder kein Chunk da ist (z.B.String_chunk_reader)
    Mehrere Chunks mit einem send-scattered() senden
    _operation->begin() schon wenn Kopf empfangen. Antworten und Anfrage empfangen gleichzeitig!  Request->get_part()
    "Last-Modified:" der Datei senden

*/


#include "spooler.h"
#include "../zschimmer/charset.h"
#include "../zschimmer/regex_class.h"

using namespace std;

namespace sos {
namespace scheduler {
namespace http {

//-------------------------------------------------------------------------------------------static

stdext::hash_map<int,string>    http_status_messages;

//--------------------------------------------------------------------------------------------const

const string                    default_charset_name = "ISO-8859-1";
const size_t                    max_line_length      = 10000;
const char                      hex[]                = "0123456789abcdef";



struct Http_status_code_table 
{   
    Status_code                _code; 
    const char*                _text;
};


const Http_status_code_table http_status_code_table[]  =
{
    { status_200_ok                            , "OK" },
    { status_301_moved_permanently             , "Moved Permanently" },
    { status_401_permission_denied             , "Permission Denied" },
    { status_403_forbidden                     , "Forbidden" },
    { status_404_bad_request                   , "Bad Request" },
    { status_405_method_not_allowed            , "Method Not Allowed" },
    { status_500_internal_server_error         , "Internal Server Error" },
    { status_501_not_implemented               , "Not Implemented" },
    { status_504_gateway_timeout               , "Gateway Timeout" },
    { status_505_http_version_not_supported    , "HTTP Version Not Supported" },
    {}
};


const char allowed_html_characters[ 256 ] =
{
    // 0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f   
       0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0,  // 00   \t, \n, \r  (\r von XML nur geduldet, aber nicht gelesen)
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 10
       1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 20   &
       1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1,  // 30   < >
       1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 40
       1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 50
       1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 60
       1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,  // 70
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 80
       0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 90
       1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // a0
       1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // b0
       1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // c0
       1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // d0
       1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // e0
       1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1   // f0
};

//-------------------------------------------------------------------------------------------------

struct Http_server : Http_server_interface 
{
                                Http_server                 ( Scheduler* scheduler )                : Http_server_interface( scheduler, type_http_server ) {}

    // Subsystem
    void                        close                       ()                                      {}
    bool                        subsystem_initialize        ();
    bool                        subsystem_load              ();
  //bool                        subsystem_activate          ();
    string                      name                        () const                                { return "http_server"; }

    // Http_server_interface
    File_path                   directory                   () const                                { return _spooler->settings()->_html_dir; }
};

//-------------------------------------------------------------------------------------------Z_INIT

Z_INIT( scheduler_http )
{
    for( const Http_status_code_table* p = http_status_code_table; p->_code; p++ )
    {
        http_status_messages[ (int)p->_code ] = p->_text;
    }
}

//----------------------------------------------------------------------------------new_http_server

ptr<Http_server_interface> new_http_server( Scheduler* scheduler )
{
    ptr<Http_server> http_server = Z_NEW( Http_server( scheduler ) );
    return +http_server;
}

//--------------------------------------------------------------------------------------date_string

string date_string( time_t t )
{
    //                  0    '    1    '    2    '
    char time_text[] = "Sun Jan 01 00:00:00 1900 GMT";
    //                  ^^^^^^^^^^^^^^^^^^^^^^^^^ wird von asctime() überschrieben

#   ifdef Z_WINDOWS
        asctime_s( time_text, sizeof time_text, gmtime( &t ) );
#    else
        struct tm  tm;
        asctime_r( gmtime_r( &t, &tm ), time_text );
#   endif
    
    string s = time_text;
    string week_day = s.substr(0, 3);
    string day = s.substr(8, 2);
    string month = s.substr(4, 3);
    string year = s.substr(20, 4);
    string time = s.substr(11, 8);
    //RFC 7231: "Tue, 15 Nov 1994 08:12:31 GMT"
    return week_day + ", " + day + " " + month + " " + year + " " + time + " GMT"; 
}

//-----------------------------------------------------------------------get_content_type_parameter

string get_content_type_parameter( const string& content_type, const string& param_name )
{
    const char* p = content_type.c_str();
    
    while(1)
    {
        p = strchr( p, ';' );
        if( !p )  break;

        p++;
        while( isspace( (unsigned char)*p ) )  p++;

        const char* n = param_name.c_str();

        while( *p  &&  *n  &&  tolower( (unsigned char)*p ) == tolower( (unsigned char)*n ) )  p++, n++;
        if( *n == 0 &&  *p == '=' )
        {
            p++;
            string result;
            while( *p  &&  *p != ';' )  result += *p++;
            return rtrim( result );
        }
    }

    return "";
}

//--------------------------------------------------------------------------Http_server::initialize

bool Http_server::subsystem_initialize()
{
    _subsystem_state = subsys_initialized;
    return true;
}

//--------------------------------------------------------------------------------Http_server::load

bool Http_server::subsystem_load()
{
    return true;
}

//-------------------------------------------------------------------------------------Headers::get

string Headers::get( const string& name ) const
{
    Map::const_iterator it = _map.find( lcase( name ) );
    return it == _map.end()? "" : it->second._value;
}

//-------------------------------------------------------------------------------------Headers::set

void Headers::set( const string& name, const string& value )
{
    if( name == "" )  z::throw_xc( Z_FUNCTION, "Name is empty" );
    if( value.find( '\n' ) != string::npos )  z::throw_xc( Z_FUNCTION, "Value has multiple lines" );

    set_unchecked( name, value );
}

//---------------------------------------------------------------------------Headers::set_unchecked

void Headers::set_unchecked( const string& name, const string& value )
{
    string lname = lcase( name );

    if( value != "" )  _map[ lname ] = Entry( name, value );
                 else  _map.erase( lname );
}

//-----------------------------------------------------------------------------Headers::set_default

void Headers::set_default( const string& name, const string& value )
{
    if( value != "" )
    {
        string        lname = lcase( name );
        Map::iterator it    = _map.find( lname );

        if( it == _map.end() )  set( name, value );
    }
}

//------------------------------------------------------------------------Headers::set_content_type

void Headers::set_content_type( const string& content_type ) 
{
    string e = get( "Content-Type" );

    size_t s = e.find( ';' );
    if( s == string::npos )  s = e.length();

    if( s > 0  &&  e[ s ] == ' ' )  s--;

    set( "Content-Type", content_type + e.substr( s ) );
}

//----------------------------------------------------------------------------Headers::content_type

string Headers::content_type() const
{
    string result = get( "Content-Type" );

    size_t s = result.find( ';' );
    if( s == string::npos )  s = result.length();

    return rtrim( result.substr( 0, s ) );
}

//--------------------------------------------------------------Headers::set_content_type_parameter

void Headers::set_content_type_parameter( const string& name, const string& value )
{
    string result;
    string content_type       = get( "Content-Type" );
    string lcase_content_type = lcase( content_type );
    Regex  regex ( "; *" + lcase(name) + "=[^;]*(;|$)" );


    if( Regex_match m = regex.match( lcase_content_type ) )
    {
        result = content_type.substr( 0, m.offset() );
        if( value != "" )  result += "; " + name + "=" + value;
        result += content_type.substr( m.end() );
    }
    /*
    string result;
    string content_type       = get( "Content-Type" );
    string lcase_content_type = lcase( content_type );

    size_t pos = lcase_content_type.find( lcase( name ) + "=" );
    if( pos != string::npos )
    {
        int vor = pos;
        while( vor > 0  &&  content_type[ vor ] != ";" )  pos--;

        int nach = pos;
        while( nach < content_type.length()  && 
        result = content_type.substr( 0, vor ) + "; " + name + "=" + value + " " + content_type.substr( nach );
    */
    else
    {
        if( value != "" )  result = content_type + "; " + name + "=" + value;
                     else  result = content_type;
    }

    set( "Content-Type", result );
}

//-----------------------------------------------------------------------------------Headers::print

void Headers::print( String_stream* s ) const
{
    Z_FOR_EACH_CONST( Map, _map, h )
    {
        *s << h->second._name << ": " << h->second._value << "\r\n";
    }
}

//-----------------------------------------------------------------------------------Headers::print

void Headers::print( String_stream* s, const string& header_name ) const
{
    Map::const_iterator it = _map.find( lcase( header_name ) );
    if( it != _map.end() )  print( s, it );
}

//-----------------------------------------------------------------------------------Headers::print

void Headers::print( String_stream* s, const Headers::Map::const_iterator& it ) const
{
    if( it != _map.end() )
    {
        *s << it->second._name << ": " << it->second._value << "\r\n";
    }
}

//------------------------------------------------------------------------Headers::print_and_remove

void Headers::print_and_remove( String_stream* s, const string& header_name )
{
    Map::iterator it = _map.find( lcase( header_name ) );
    if( it != _map.end() )
    {
        print( s, it );
        _map.erase( it );
    }
}

//------------------------------------------------------------------------------Headers::put_Header

STDMETHODIMP Headers::put_Header( BSTR name, BSTR value )
{ 
    HRESULT hr = S_OK;
    
    try
    {
        set( string_from_bstr( name ), string_from_bstr( value ) );
    }
    catch( const exception&  x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }
    
    return hr;
}

//------------------------------------------------------------------------------Headers::get_Header

STDMETHODIMP Headers::get_Header( BSTR name, BSTR* result )
{ 
    HRESULT hr = S_OK;
    
    try
    {
        hr = String_to_bstr( get( string_from_bstr( name ) ), result ); 
    }
    catch( const exception&  x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }
    
    return hr;
}

//------------------------------------------------------------------------Headers::put_Content_type

STDMETHODIMP Headers::put_Content_type( BSTR value )
{
    HRESULT hr = S_OK;
    
    try
    {
        set_content_type( string_from_bstr( value ) );
    }
    catch( const exception&  x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }
    
    return hr;
}

//------------------------------------------------------------------------Headers::get_Content_type

STDMETHODIMP Headers::get_Content_type( BSTR* result )
{
    HRESULT hr = S_OK;
    
    try
    {
        hr = String_to_bstr( content_type(), result );
    }
    catch( const exception&  x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }
    
    return hr;
}

//------------------------------------------------------------------------Headers::put_Charset_name

STDMETHODIMP Headers::put_Charset_name( BSTR value )
{
    HRESULT hr = S_OK;
    
    try
    {
        set_content_type_parameter( "charset", string_from_bstr( value ) );
    }
    catch( const exception&  x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }
    
    return hr;
}

//------------------------------------------------------------------------Headers::get_Charset_name

STDMETHODIMP Headers::get_Charset_name( BSTR* result )
{
    HRESULT hr = S_OK;
    
    try
    {
        hr = String_to_bstr( charset_name(), result );
    }
    catch( const exception&  x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }
    
    return hr;
}

//-----------------------------------------------------------------------------------Parser::Parser
    
Parser::Parser( C_request* request )
:
    _zero_(this+1),
    _request( request )
{
    _text.reserve( 2000 );
}

//------------------------------------------------------------------------------------Parser::close
    
void Parser::close()
{
    _request = NULL;
}

//---------------------------------------------------------------------------------Parser::add_text
    
void Parser::add_text( const char* text, int len )
{
    _text.append( text, len );

    if( !_reading_body )
    {
        int end_size = 3;
        size_t header_end = _text.find( "\n\r\n" );  // Leerzeile?
        if( header_end == string::npos )  header_end = _text.find( "\n\n" ), end_size = 2;

        if( header_end != string::npos )
        {
            // Kopf gelesen

            _body_start = int_cast(header_end) + end_size;
            _reading_body = true;

            parse_header();
            string content_length = _request->_headers[ "content-length" ];
            if( !content_length.empty() )
            {
                _content_length = as_uint( content_length );
            }
        }
    }

    if( _reading_body  &&  _text.length() >= _body_start + _content_length )
    {
        size_t rest = _text.length() - ( _body_start + _content_length );
        if( rest > 0 )  
        {
            if( rest == 2  &&  string_ends_with( _text, "\r\n" ) )  {}  // Okay für Firefox
                                                              else  z::throw_xc( "SPOOLER-HTTP toomuchdata" );
        }

        _request->_body.assign( _text.data() + _body_start, _content_length ); 
    }
}

//------------------------------------------------------------------------------Parser::is_complete

bool Parser::is_complete()
{
    return _reading_body  &&  (    _text.length() == _body_start + _content_length
                                || _text.length() == _body_start + _content_length + 2 );  // Firefox hängt noch ein \r\n an
}

//-----------------------------------------------------------------------------Parser::parse_header

void Parser::parse_header()
{
    /*if( z::Log_ptr log = "http" )
    {
        int end = _text.find( '\n' );
        if( end == string::npos )  end = _text.length();   // Vorsichtshalber
                             else  end++;
        log << string( _text, end );
    }*/

    _next_char = _text.c_str();

    _request->_http_method = eat_word();
    _request->_path     = eat_path();
    _request->_protocol = eat_word();
    eat_line_end();

    while( next_char() > ' ' )
    {
        string name = eat_until( ":" );
                      eat( ":" );
        string value = eat_until( "" );
        _request->_headers.set_unchecked( name, value );
        eat_line_end();
    }

    eat_line_end();
}

//-------------------------------------------------------------------------------Parser::eat_spaces

void Parser::eat_spaces()
{ 
    while( *_next_char == ' ' )  _next_char++; 
}

//--------------------------------------------------------------------------------------Parser::eat

void Parser::eat( const char* what )
{
    const char* w = what;
    while( *w  &&  *_next_char == *w )  w++, _next_char++;
    if( *w != '\0' )  
    {
        if( what[0] == '\n' )  z::throw_xc( "SCHEDULER-213", "Line end" );
                         else  z::throw_xc( "SCHEDULER-213", what );
    }

    eat_spaces();
}

//-----------------------------------------------------------------------------Parser::eat_line_end

void Parser::eat_line_end()
{
    eat_spaces();

    if( *_next_char == '\r' )  _next_char++;
    eat( "\n" );
}

//---------------------------------------------------------------------------------Parser::eat_word

string Parser::eat_word()
{
    string word;
    while( *_next_char > ' ' )  word += *_next_char++;

    eat_spaces();
    return word;
}

//--------------------------------------------------------------------------------Parser::eat_until

string Parser::eat_until( const char* character_set )
{
    string word;
    while( *_next_char >= ' '  &&  strchr( character_set, *_next_char ) == NULL )  word += *_next_char++;

    eat_spaces();
    return rtrim( word );
}

//---------------------------------------------------------------------------------Parser::eat_path

string Parser::eat_path()
{
    eat_spaces();

    string word;
    string path;
    string parameter_name;
    enum State { in_path, in_parameter }  state = in_path;

    while(1)
    {
        if( state == in_path  &&  _next_char[0] == '?' )
        {
            path = word + _next_char[0];
            word = "";
            _next_char++;
            state = in_parameter;
        }
        else
        if( state == in_parameter  &&  _next_char[0] == '&'  ||  (Byte)_next_char[0] <= (Byte)' ' )
        {
            if( state == in_path )  path = word;
                              else  _request->_parameters[ parameter_name ] = word;
            state = in_parameter;
            if( (Byte)_next_char[0] <= (Byte)' ' )  break;
            word = "";
            parameter_name = "";
            _next_char++;
        }
        else
        if( _next_char[0] == '=' && state == in_parameter )
        {
            parameter_name = word;
            word = "";
            _next_char++;
        }
        else
        if( _next_char[0] == '%'  &&  _next_char[1] != '\0'  &&  _next_char[2] != '\0' )
        {
            word += (char)hex_as_int32( string( _next_char+1, 2 ) );
            _next_char += 3;
        }
        else
            word += *_next_char++;
    }

    eat_spaces();

    if( !string_begins_with( path, "/" ) )  path = "/" + path;

    return path;
}

//-----------------------------------------------------------------------------Operation::Operation

Operation::Operation( Operation_connection* pc )
: 
    Communication::Operation( pc ), 
    _zero_(this+1)
{
    _request  = Z_NEW( C_request() );
    _parser   = Z_NEW( Parser( _request ) );
}

//---------------------------------------------------------------------------------Operation::close
    
void Operation::close()
{ 
    /* Für Order alles am Leben lassen:
    if( _web_service_operation )  _web_service_operation->close(),  _web_service_operation = NULL;
    if( _response )  _response->close(),  _response = NULL;
    if( _parser   )  _parser  ->close(),  _parser   = NULL;
    if( _request  )  _request ->close(),  _request  = NULL;
    */

    Communication::Operation::close(); 
}

//----------------------------------------------------------------------------Operation::link_order

void Operation::link_order( Order* order )
{
    _order = order;
    order->set_http_operation( this );       // Order wird zweiter Eigentümer von Web_service_operation, neben Http_operation

    if( _web_service_operation )  order->set_web_service( _web_service_operation->web_service() );
}

//--------------------------------------------------------------------------Operation::unlink_order

void Operation::unlink_order()
{
    _order = NULL;

    try
    {
        if( _response  &&  !_response->is_ready() )
        {
            if( _connection )  _connection->_log.error( message_string( "SCHEDULER-297" ) );    // "Auftrag erledigt ohne web_service_operation.send(), Operation wird abgebrochen"
            cancel();
        }
    }
    catch( exception& x ) { _spooler->log()->error( x.what() ); }
}

//-------------------------------------------------------------Operation::on_first_order_processing

void Operation::on_first_order_processing( Task* )
{
    // <web_service timeout="">: Die Frist gilt nur bis zur ersten Ausführung

    set_gmtimeout( double_time_max );       // Timeout abschalten
}

//---------------------------------------------------------------------------------Operation::begin

void Operation::begin()
{
    if( zschimmer::Log_ptr log = "scheduler.http" )    // Wird auch mit "socket.data" protokolliert (default aus)
    {
        string text = _parser->text();
        log << "HTTP: " << text;    
        if( !string_ends_with( text, "\n" ) )  log << '\n';
    }

    _response = Z_NEW( C_response( this ) );

    try
    {
        _request->check();

        if( _spooler->_web_services->need_authorization()  &&  !_spooler->_web_services->is_request_authorized( _request ) )
        {
            // 2007-08-24  Dank an Michael Collard, iinet.net.au.

            _response->set_header( "WWW-Authenticate", "Basic realm=\"Scheduler\"" );
          //_response->set_status( http::status_401_permission_denied );

            string response_content_type = "text/html";
            string response_body = "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">"
                                   "<HTML>"
                                   "<HEAD><TITLE>401 Authorization Required</TITLE></HEAD>"
                                   "<BODY>"
                                    "<H1>Authorization Required</H1>"
                                    "This server could not verify that you are authorized to access the document requested. "
                                    "Either you supplied the wrong credentials (e.g., bad password), or your browser doesn't understand how to supply the credentials required."
                                    "<P>"
                                    "</BODY>"
                                    "</HTML>";

            _response->set_chunk_reader( Z_NEW( http::String_chunk_reader( response_body, response_content_type ) ) );

            throw Http_exception( status_401_permission_denied );
        }


        if( Web_service* web_service = _spooler->_web_services->web_service_by_url_path_or_null( _request->_path ) )
        {
            _web_service_operation = web_service->new_operation( this );
            _web_service_operation->begin();
        }
        else
        {
            string path = _request->_path;
            if( !string_begins_with( path, "/" ) )  path = "/" + path;

            Http_file_directory* http_file_directory = NULL;

            size_t slash = path.find( "/", 1 );
            if( slash != string::npos )
            {
                string directory = "/" + path.substr( 1, slash );
                http_file_directory = _spooler->_web_services->http_file_directory_by_url_path_or_null( directory );

                // Statt execute_http() sollte direkt der Zweig von execute_http() aufrufen werden, der Dateien liefert (also nicht "show_log?" etc.)
                // Auch könnte execute_http() von Command_processor nach Web_service, Http_file_directory oder Http_server verschoben werden.
            }

            Command_processor command_processor ( _spooler, _connection->_security_level, _connection->peer_host(), this );
            command_processor.execute_http( request(), response(), http_file_directory );
            _response->set_ready();
        }
    }
    catch( Http_exception& x )
    {
        _connection->_log.warn( x.what() );
        _response->set_status( x._status_code, x.what() );
        _response->set_ready();
    }
    catch( exception& x )
    {
        _connection->_log.warn( x.what() );
        _response->set_status( status_500_internal_server_error, x.what() );
        _response->set_ready();
    }


    _response->set_event( &_connection->_socket_event );
    _response->recommend_block_size( 32768 );

    //_parser  = NULL;
    //_request = NULL;
}

//-----------------------------------------------------------------------Operation::async_continue_

bool Operation::async_continue_( Continue_flags flags )
{
    bool something_done = false;

    if( flags & cont_next_gmtime_reached )
    {
        if( _response &&  !_response->is_ready()  &&  _order  &&  !_order->is_touched() )         // is_touched() sollte immer false sein
        {
            if( _connection )  _connection->_log.error( message_string( "SCHEDULER-290", _order->obj_name() ) );   // "HTTP-Operation wird nach Fristablauf abgebrochen"
            
            _response->set_status( status_504_gateway_timeout );
            _response->set_ready();

            _order->remove_from_job_chain();
            _order = NULL;

            something_done = true;
        }
    }
    /*
    else
    {
        if( _web_service_operation ) 
        {
            something_done |= _web_service_operation->async_continue( flags );  // Da passiert nix
        }
    }
    */

    return something_done;
}

//--------------------------------------------------------------------------------Operation::cancel

void Operation::cancel()
{
    if( !_response  ||  _response->is_ready() )
    {
        if( _web_service_operation ) _web_service_operation->log()->warn( message_string( "SCHEDULER-308" ) );  // "cancel() ignoriert, weil die Antwort schon übertragen wird"
        return;
    }

    _response->set_status( http::status_500_internal_server_error );
    _response->set_ready();
}

//----------------------------------------------------------------------Operation::get_response_part

string Operation::get_response_part()
{ 
    return _response->read( _response->recommended_block_size() );
}

//-------------------------------------------------------------------Operation::response_is_complete

bool Operation::response_is_complete()
{ 
    return !_response || _response->eof(); 
}

//---------------------------------------------------------------Operation::should_close_connection

bool Operation::should_close_connection()
{ 
    return _response  &&  _response->close_connection_at_eof(); 
}

//---------------------------------------------------------------------------Operation::dom_element

xml::Element_ptr Operation::dom_element( const xml::Document_ptr& document, const Show_what& what ) const
{
    xml::Element_ptr result = document.createElement( "http_operation" );

    if( _order )
    result.setAttribute( "order", _order->obj_name() );

    if( _web_service_operation )
    {
        result.appendChild( _web_service_operation->dom_element( document, what ) );
    }

    return result;
}

//---------------------------------------------------------------------Operation::async_state_text_

string Operation::async_state_text_() const
{
    S result;

    result << Communication::Operation::async_state_text_();
    result << ", " <<obj_name();
    if( _web_service_operation )  result << ", " << _web_service_operation->obj_name();
    if( _order                 )  result << ", " << _order->obj_name();

    return result;
}

//---------------------------------------------------------------------------------C_request::close
/*
void C_request::close()
{
}
*/
//-----------------------------------------------------------------------------C_request::parameter
    
string C_request::parameter( const string& name )
{ 
    String_map::const_iterator it = _parameters.find( name );
    return it == _parameters.end()? "" : it->second;
}

//-----------------------------------------------------------------------------------C_request::url

string C_request::url()
{
    S result;
    result << "http://" << _headers[ "host" ] << url_path();        // Zum Beispiel "Host: hostname:80"
    return result;
}

//--------------------------------------------------------------------------C_request::charset_name

string C_request::charset_name()
{
    return _headers.charset_name();
}

//---------------------------------------------------------------------------------C_request::check

void C_request::check()
{
    if( _protocol != ""  
     && _protocol != "HTTP/1.0"  
     && _protocol != "HTTP/1.1" )  throw Http_exception( status_505_http_version_not_supported );

    if( !string_begins_with( _path, "/" ) )  throw Http_exception( status_404_bad_request );
}

//--------------------------------------------------------------------C_request::get_String_content

STDMETHODIMP C_request::get_String_content( BSTR* result )
{
    HRESULT hr = S_OK;
    
    try
    {
        string charset_name = this->charset_name();
        if( charset_name == "" )  charset_name = default_charset_name;

        const Charset* charset = Charset::for_name( charset_name );

        charset->Encoded_to_bstr( _body, result );
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }

    return hr;
}

//--------------------------------------------------------------------C_request::get_Binary_content

STDMETHODIMP C_request::get_Binary_content( SAFEARRAY** result )
{
    HRESULT hr = S_OK;
    
    try
    {
        Locked_safearray<unsigned char> safearray ( int_cast(_body.length()) );
        memcpy( safearray.data(), _body.data(), _body.length() );
        *result = safearray.take_safearray();
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }

    return hr;
}

//-------------------------------------------------------------------------------------Java_request

struct Java_request : Request {
    private: SchedulerHttpRequestJ _requestJ;

    public: Java_request(const SchedulerHttpRequestJ& requestJ) : _requestJ(requestJ) {}

    public: bool has_parameter(const string& name) { return _requestJ.hasParameter(name);  }
    public: string parameter( const string& name ) { return _requestJ.parameter(name); }
    public: string header(const string& name) { return _requestJ.header(name); }
    public: string protocol() { return _requestJ.protocol(); }
    public: string url_path() { return _requestJ.urlPath(); }
    public: string charset_name() { return _requestJ.charsetName(); }
    public: string http_method() { return _requestJ.httpMethod(); }
    public: string body() { return _requestJ.body(); }
};

ptr<Request> new_java_request(const SchedulerHttpRequestJ& requestJ) {
    ptr<Java_request> result = Z_NEW(Java_request(requestJ));
    return +result;
}

//-------------------------------------------------------------------Http_exception::Http_exception

Http_exception::Http_exception( Status_code status_code, const string& error_text ) 
: 
    _status_code( status_code ), 
    _error_text( error_text )
{
    S result;
    
    result << "HTTP " << (int)_status_code << ' ' << http_status_messages[ _status_code ].c_str();
    if( _error_text != "" )  result << ": " << _error_text;
    
    _what = result;
}

//------------------------------------------------------------------Http_exception::~Http_exception

Http_exception::~Http_exception()  throw()
{
}

//-------------------------------------------------------------------------------Response::Response

Response::Response(Request* request)
: 
    _zero_(this+1), 
    _chunked( request->is_http_1_1() ),
    _protocol(request->protocol()),
    _close_connection_at_eof( !request->is_http_1_1() )
{ 
    if( _close_connection_at_eof )  _headers.set_default( "Connection", "close" );

    if( request->header("cache-control") == "no-cache" )
        _headers.set( "Cache-Control", "no-cache" );   // Sonst bleibt z.B. die scheduler.xslt im Browser kleben und ein Wechsel der Datei wirkt nicht.
                                                       // Gut wäre eine Frist, z.B. 10s
}

//------------------------------------------------------------------------------Response::~Response

Response::~Response() {}
 
//----------------------------------------------------------------------------------Response::close
/*    
void Response::close()
{
    _operation = NULL;
    _chunk_reader = NULL;
}
*/
//-----------------------------------------------------------------------------Response::set_status
    
void Response::set_status( Status_code code, const string& )
{ 
    if( is_ready() )  z::throw_xc( "SCHEDULER-247" );

    _status_code = code; 

    /* Die Fehlermeldung sollte nicht dem Benutzer gezeigt werden. Sie kann Pfadnamen enthalten, z.B. bei ERRNO-2
    if( text != "" )
    {
        S body;
        body << "<html><body><p style='color: red; font-weight: bold;'>";

        for( const char* p = text.c_str(); *p; p++ )
        {
            switch( *p )
            {
                case '&': body << "&amp;";  break;
                case '<': body << "&lt;" ;  break;
                case '>': body << "&gt;" ;  break;
                default: body << *p;
            }
        }

        body << "</p></body></html>";
        
        set_chunk_reader( Z_NEW( String_chunk_reader( body, "text/html" ) ) );
    }
    */
}

//-----------------------------------------------------------------------------Response::set_header

void Response::set_header( const string& name, const string& value ) 
{ 
    if( is_ready() )  z::throw_xc( "SCHEDULER-247" );

    _headers.set( name, value ); 
}

//-----------------------------------------------------------------------Response::set_chunk_reader

void Response::set_chunk_reader( Chunk_reader* c )
{ 
    if( is_ready() )  z::throw_xc( "SCHEDULER-247" );

    _chunk_reader = c; 
}

//-----------------------------------------------------------------------------------Response::send

void Response::send()
{
    if( is_ready() )  z::throw_xc( "SCHEDULER-247" );

    set_ready();

    // Evtl. sofort send() aufrufen? Nicht nötig.
}

//---------------------------------------------------------------------------------Response::finish

void Response::finish()
{
    if( _finished )  return;
    if( !_status_code )  _status_code = status_200_ok;

    if( (int)_status_code >= 100  &&  (int)_status_code <= 199  ||  (int)_status_code == 204  || (int)_status_code == 304 )       // RFC 2617 4.4
    {
        if( _chunk_reader )
        {
            Z_LOG2( "scheduler", "_chunk_reader has been removed\n" ); 
            _chunk_reader = NULL;
        }
    }


    // _headers füllen

    if( !_headers.contains( "Date" ) )  _headers.set( "Date", date_string( ::time(NULL) ) );
    if( _chunk_reader )  _headers.set_default( "Content-Type", _chunk_reader->_content_type );


    // _headers_stream schreiben

    //TODO: _operation->request()->_protocol == ""  ==> Simple request, simple respone, HTTP-Header nicht senden! (laut RFC 1945)

    _headers_stream << _protocol << ' ' << _status_code << ' ' << http_status_messages[ _status_code ] << "\r\n";
    _headers.print( &_headers_stream );

    if( _chunked )  _headers_stream << "Transfer-Encoding: chunked\r\n";

    _headers_stream << "Server: JobScheduler engine " << version_string;

    _headers_stream << "\r\n\r\n";

    _chunk_size = int_cast(_headers_stream.length());
    _finished = true;

    Z_LOG2( "scheduler.http", "HTTP response: " << _headers_stream );    // Wird auch mit "socket.data" protokolliert (default aus)
}

//------------------------------------------------------------------------------------Response::eof

bool Response::eof()
{
    return _eof;
}

//-----------------------------------------------------------------------------------Response::read

string Response::read( int recommended_size )                           
{
    if( !_finished )  finish();


    string result;

    if( _eof )  return result;

    if( _chunk_index == 0  &&  _chunk_offset < _chunk_size )
    {
        result = _headers_stream;
        _chunk_offset += int_cast(_headers_stream.length());
    }

    if( _chunk_offset == _chunk_size ) 
    {
        if( _chunk_reader ) 
        {
            result += start_new_chunk();
        }
        else 
        {
            _eof = true;
            if( _chunked )  result += "0\r\n\r\n";
        }
    }

    if( _chunk_offset < _chunk_size )
    {
        string data =_chunk_reader->read_from_chunk( min( recommended_size, (int)( _chunk_size - _chunk_offset ) ) );
        _chunk_offset += int_cast(data.length());
        result += data;
    }

    if( _chunk_offset == _chunk_size  &&  !_chunk_eof ) 
    {
        _chunk_eof = true;
        if( _chunked )  result.append( "\r\n" );
    }

    //Z_LOG2( "scheduler", __PRETTY_FUNCTION__ << "() ==> " << result << "\n" );

    return result;
}

//------------------------------------------------------------------------Response::start_new_chunk

string Response::start_new_chunk()
{
    if( !_chunk_reader->next_chunk_is_ready() )  return "";

    _chunk_index++;
    _chunk_offset = 0;
    _chunk_size   = _chunk_reader->get_next_chunk_size();

    string result;
    
    if( _chunked )  result = as_hex_string( (int)_chunk_size ) + "\r\n";
    
    //Z_LOG2( "scheduler", __PRETTY_FUNCTION__ << "  chunk_size=" << _chunk_size << "\n" );

    if( _chunk_size > 0 )
    {
        _chunk_eof = false;
    }
    else
    {
        _eof = true;
        if( _chunked )  result += "\r\n";    
    }
    
    return result;
}

//--------------------------------------------------------------------Response::Assert_is_not_ready

STDMETHODIMP Response::Assert_is_not_ready()
{
    HRESULT hr = S_OK;
    
    try
    {
        if( is_ready() )  z::throw_xc( "SCHEDULER-247" );
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }

    return hr;
}

//------------------------------------------------------------------------Response::put_Status_code

STDMETHODIMP Response::put_Status_code( int code )
{ 
    HRESULT hr = S_OK;
    
    try
    {
        set_status( (Status_code)code );  
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }

    return hr;
}

//-----------------------------------------------------------------------------Response::put_Header

STDMETHODIMP Response::put_Header( BSTR name, BSTR value )
{ 
    HRESULT hr = Assert_is_not_ready();
    if( FAILED( hr ) )  return hr;

    hr = _headers.put_Header( name, value );

    return hr;
}

//-----------------------------------------------------------------------Response::put_Content_type

STDMETHODIMP Response::put_Content_type( BSTR content_type )
{ 
    HRESULT hr = Assert_is_not_ready();
    if( FAILED( hr ) )  return hr;

    return _headers.put_Content_type( content_type ); 
}

//-----------------------------------------------------------------------Response::put_Charset_name

STDMETHODIMP Response::put_Charset_name( BSTR charset_name )
{ 
    HRESULT hr = Assert_is_not_ready();
    if( FAILED( hr ) )  return hr;

    return _headers.put_Charset_name( charset_name ); 
}

//---------------------------------------------------------------------Response::put_String_content

STDMETHODIMP Response::put_String_content( BSTR content_bstr )
{
    HRESULT hr = S_OK;
    
    try
    {
        if( is_ready() )  z::throw_xc( "SCHEDULER-247" );

        string charset_name = _headers.charset_name();
        if( charset_name == "" )  charset_name = default_charset_name;

        const Charset* charset = Charset::for_name( charset_name );

        set_chunk_reader( Z_NEW( String_chunk_reader( charset->encoded_from_bstr( content_bstr ), "" ) ) );
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }

    return hr;
}

//---------------------------------------------------------------------Response::put_Binary_content

STDMETHODIMP Response::put_Binary_content( SAFEARRAY* safearray )
{
    HRESULT hr = S_OK;
    
    try
    {
        if( is_ready() )  z::throw_xc( "SCHEDULER-247" );

        Locked_safearray<Byte> a ( safearray );

        set_chunk_reader( Z_NEW( Byte_chunk_reader( a.data(), a.count(), "" ) ) );
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }

    return hr;
}

//-----------------------------------------------------------------------------------Response::Send

STDMETHODIMP Response::Send() // VARIANT* content, BSTR content_type_bstr )
{
    if( closed() )  return E_FAIL;

    HRESULT hr = S_OK;
    
    try {
        send();
    }
    catch( const exception& x )  { hr = Set_excepinfo( x, Z_FUNCTION ); }

    return hr;
}

//------------------------------------------------------------------------------Response::set_ready

void Response::set_ready()
{ 
    finish();
    _ready = true; 
    on_ready();
}

//---------------------------------------------------------------------------C_response::C_response

C_response::C_response(Operation* operation)
: 
    Response(operation->request()),
    _zero_(this+1), 
    _operation(operation)
{ 
}

//-----------------------------------------------------------------------------C_response::on_ready

void C_response::on_ready()
{ 
    if( _operation  &&  _operation->_connection )  _operation->_connection->signal( "http response is ready" );
    // Nicht sofort senden
}

//------------------------------------------------------------------------------------Java_response

Java_response::Java_response(Request* request, const SchedulerHttpResponseJ& responseJ) : 
    Response(request), _event(this, "Java_response"), _responseJ(responseJ) 
{}

Java_response::~Java_response() {
    Z_LOG2("scheduler", "~Java_response()\n");
}

void Java_response::close() {
    _closed = true;
    _responseJ = NULL;
}

bool Java_response::closed() { 
    return _closed;
}
    
void Java_response::on_ready() {
    set_event(&_event);
}

void Java_response::on_event_signaled() {
    if (_responseJ)  _responseJ.onNextChunkIsReady();
}

//---------------------------------------------------------String_chunk_reader::get_next_chunk_size

int String_chunk_reader::get_next_chunk_size()
{
    //if( _get_next_chunk_size_called )  return 0;
    //_get_next_chunk_size_called = true;

    return int_cast(_text.length());
}

//-------------------------------------------------------------String_chunk_reader::read_from_chunk

string String_chunk_reader::read_from_chunk( int recommended_size )
{ 
    string result;

    int length = min( recommended_size, (int)( _text.length() - _offset ) );

    result = _text.substr( _offset, length ); 

    _offset += length;
    if( _offset == _text.length() )  _text.clear(), _offset = 0;    // Ende, get_next_chunk_size() wird 0 liefern

    return result;
}

//---------------------------------------------------------------Log_chunk_reader::Log_chunk_reader

Log_chunk_reader::Log_chunk_reader( Prefix_log* log )
: 
    Chunk_reader( "text/html; charset=" + string_encoding),
    _zero_(this+1), 
    _log(log) 
{
}

//--------------------------------------------------------------Log_chunk_reader::~Log_chunk_reader

Log_chunk_reader::~Log_chunk_reader()
{
    if( _event )  _log->remove_event( _event );
}

//----------------------------------------------------------------------Log_chunk_reader::set_event

void Log_chunk_reader::set_event( Event_base* event )
{
    _event = event;
    _log->add_event( _event );
}

//-----------------------------------------------------------Log_chunk_reader::next_chunk_is_ready

bool Log_chunk_reader::next_chunk_is_ready()
{ 
    bool result = false;

    for( bool loop = true; loop; )
    {
        loop = false;

        switch( _state )
        {
            case s_initial:
            {
                _state = s_reading_string;
                loop = true;

                break;
            }

            case s_reading_string:
            {
                if( !_string_chunk_reader  &&  _log->started() )
                {
                    _state = s_reading_file;
                    loop = true;
                }
                else
                {
                    if( !_string_chunk_reader )
                        _string_chunk_reader = Z_NEW( String_chunk_reader( _log->as_string() ) );

                    if( _string_chunk_reader->next_chunk_is_ready() )   // Immer true
                    {
                        if( _string_chunk_reader->get_next_chunk_size() > 0 )
                        {
                            result = true;
                        }
                        else
                        {
                            _string_chunk_reader = NULL;
                            _state = s_reading_file;
                            loop = true;
                        }
                    }
                }

                break;
            }

            case s_reading_file:
            {
                if( !_log->started() )      // Wenn Log noch nicht gestartet worden ist (z.B. Order in der Warteschlange), dann gibt's noch keine Datei
                {                           // Und wenn die Datei schon geschlossen ist? Kann das passieren?
                    //if( es wird keine Log-Datei geben, z.B. bei aus Datenbank geladenem verteiltem Auftrag )  result = true;    // Ende
                }
                else
                {
                    if( !_file.opened() )
                    {
                        _file.open( _log->filename(), "rb" );
                        _log_instance_number = _log->instance_number();
                        if( _file_seek )  _file.seek( _file_seek ),  _file_seek = 0;
                    }

                    if( _file.tell() < _file.length() )
                    {
                        result = true;
                    }
                    else
                    {
                        if( _log->is_finished() )
                        {
                            _file.close();
                            _state = s_finished;
                            loop = true;
                        }
                        else
                        if( _log->filename() != _file.path()  ||  // Dateiname gewechselt (Log.start_new_file)? Dann beenden wir das Protokoll
                            _log->instance_number() != _log_instance_number )  // Dateiname gewechselt (Log.start_new_file)? Neue Datei begonnen? 
                        {
                            // Wenn dieselbe Datei erneut beschrieben wird (bei Nested_job_chain_node), dann kann der Rest der bisherigen Datei 
                            // im HTTP-Protokoll verloren gehen. Wenn die Datei schneller beschrieben als gelesen wird.
                            // Lösung: Eigene Datei (Warteschlange) fürs HTTP-Protokoll

                            _file.close();
                            _file_seek = 0;
                            _html_insertion = "<hr size='1'/>";
                            result = true;
                            break;
                        }
                    }                                       
                }

                break;
            }

            case s_finished:
            {
                result = true;
                break;
            }

            default:
                z::throw_xc( Z_FUNCTION );
        }
    }

    return result;
}

//-----------------------------------------------------------Log_chunk_reader::get_next_chunk_size

int Log_chunk_reader::get_next_chunk_size()
{
    int result = 0;     // Ende

    if( _string_chunk_reader )
    {
        result = _string_chunk_reader->get_next_chunk_size();
    }
    else
    if( _file.opened() )
    {
        uint64 size = _file.length() - _file.tell();
        if( size > 0 )  result = size < _recommended_block_size? (int)size : _recommended_block_size;
    }

    return result;
}

//----------------------------------------------------------------Log_chunk_reader::read_from_chunk

string Log_chunk_reader::read_from_chunk( int recommended_size )
{ 
    string result;

    if( _string_chunk_reader )
    {
        result = _string_chunk_reader->read_from_chunk( recommended_size );
        _file_seek += result.length();
    }
    else
    if( _file.opened() )
    {
        result = _file.read_string( recommended_size );
    }

    return result;
}

//-----------------------------------------------------------------Log_chunk_reader::html_insertion

string Log_chunk_reader::html_insertion()
{ 
    string result = _html_insertion;
    _html_insertion = "";
    return result;
}

//-------------------------------------------------------------Html_chunk_reader::Html_chunk_reader

Html_chunk_reader::Html_chunk_reader( Chunk_reader* chunk_reader, const string& base_url, const string& title )
: 
    Chunk_reader_filter( chunk_reader, "text/html; charset=" + get_content_type_parameter( chunk_reader->content_type(), "charset" ) ),
    _zero_(this+1), 
    _state(reading_prefix),
    _at_begin_of_line(true)
{
    _html_prefix = "<html>\n" 
                        "<head>\n"+
                            (base_url.empty() ? "" : "<base href='"+ base_url +"'>\n") +
                            "<style type='text/css'>\n"
                                "@import 'scheduler.css';\n"
                                "pre { font-family: Lucida Console, monospace; font-size: 10pt }\n"
                            "</style>\n"
                            "<title>Scheduler log</title>\n"
                        "</head>\n" 

                        "<body class='log'>\n" 

                            "<script type='text/javascript'><!--\n"   
                                "var title=" + quoted_string( title ) + ";\n"
                            "--></script>\n"

                            "<script type='text/javascript' src='show_log.js'></script>\n"

                            "<pre class='log'>\n\n";

    _html_suffix =          "\n</pre>\n"
                        "</body>\n"
                    "</html>\n";
}

//------------------------------------------------------------Html_chunk_reader::~Html_chunk_reader

Html_chunk_reader::~Html_chunk_reader()
{
}

//-----------------------------------------------------------Html_chunk_reader::next_chunk_is_ready

bool Html_chunk_reader::next_chunk_is_ready()
{ 
    switch( _state )
    {
        case reading_prefix:  _chunk = _html_prefix;    return true;

        case reading_text:    if( !try_fill_chunk() )   return false;
                              if( _chunk.length() > 0 ) return true;
                              _state = reading_suffix;

        case reading_suffix:  _chunk = _html_suffix;    return true;

        case reading_finished: _chunk = "";             return true;

        default:               return true;
    }
}

//-----------------------------------------------------------Html_chunk_reader::get_next_chunk_size

int Html_chunk_reader::get_next_chunk_size()
{
    return int_cast(_chunk.length());
}

//------------------------------------------------------------------------------append_html_encoded

static void append_html_encoded( string* html, const char* p, size_t length, char quote = '\0' )
{
    const char* p0    = p;
    const char* p_end = p + length;

    while( p < p_end )
    {
        const char* tail = p;
        while( p < p_end  &&  ( allowed_html_characters[ (unsigned char)*p ]  ||  *p == quote ) )  p++;
        html->append( tail, p - tail );

        if( p == p_end )  break;

        switch( char c = *p++ )
        {
            case '&' : *html += "&amp;";  break;
            case '<' : *html += "&lt;";   break;
            case '>' : if( p > p0  &&  p[-1] != ']' )  *html += '>'; 
                                                 else  *html += "&gt;";
                       break;
            case '\'': *html += "&#x27;";  break;
            case '"' : *html += "&#x22;";  break;

            default  : //if( (unsigned char)c < 0x20 )  *html += "&#" + as_string( 0x2400 + (unsigned char)c ) + ";";  // Unicode Control Pictures 
                       //else
                       //if( c == '\x7F' )  *html += "&#2401";
                       //else
                       {
                           *html += "<span class='invalid_character'>";
                           *html += hex[ (unsigned char)c >> 4 ];
                           *html += hex[ c & 0x0F ];
                           *html += "</span>";
                       }
        }
    }
}

//------------------------------------------------------------------------------append_html_encoded

inline void append_html_encoded( string* html, const string& text, char quote = '\0' )
{
    append_html_encoded( html, text.data(), text.length(), quote );
}

//----------------------------------------------------------------Html_chunk_reader::try_fill_chunk

bool Html_chunk_reader::try_fill_chunk()
{
    _chunk = "";
    _chunk.reserve( _recommended_block_size + 2*max_line_length );

    while( _chunk.length() < _recommended_block_size )
    {
        bool ok = try_fill_line();      // Liefert _html_insertion oder _line
        if( !ok )  return _chunk.length() > 0;

        if( _html_insertion != "" )
        {
            _chunk.append( _html_insertion );   // Z.B. <hr/>
            _html_insertion = "";
        }
        else
        if( _line != "" )
        {
            if( _at_begin_of_line )
            {
                size_t left_bracket  = _line.find( '[' );
                size_t right_bracket = _line.find( ']' );
                
                if( left_bracket != string::npos  &&  right_bracket != string::npos  &&  left_bracket < right_bracket )
                {
                    _chunk += "<span class='log_";
                    append_html_encoded( &_chunk, lcase( _line.substr( left_bracket + 1, right_bracket - left_bracket - 1 ) ) );
                    _chunk += "'>";

                    _in_span++;    // </span> nicht vergessen
                }
            }


            _at_begin_of_line = false;


            const char* p     = _line.data();
            const char* tail  = p;
            const char* p_end = p + _line.length();

            while( p < p_end )
            {
                switch( *p )
                {
                    case '\r': append_html_encoded( &_chunk, tail, p - tail ),  p++,  tail = p;   // Bisherigen Text ausgeben
                               // \r nicht ausgeben
                               break;

                    case '\n': append_html_encoded( &_chunk, tail, p - tail ),  p++,  tail = p;   // Bisherigen Text ausgeben
                               while( _in_span )  _chunk += "</span>", _in_span--;
                               _chunk += '\n';
                               _at_begin_of_line = true;
                               assert( p == p_end );
                               break;   

                    case 'h': 
                    {
                        if( string_begins_with( p, "http://" ) )
                        {
                            const char* p0 = p;
                            while( p < p_end  &&  !isspace( (unsigned char)*p ) ) p++;
                            while( p > p0  &&  !isalnum( (unsigned char)p[-1] )  &&  p[-1] != '/' )  p--;
                            if( p > p0 )
                            {
                                append_html_encoded( &_chunk, tail, p0 - tail );    // Bisherigen Text ausgeben
                               
                                string url ( p0, p - p0 );
                                _chunk += "<a class='log' href='";
                                append_html_encoded( &_chunk, url, '\'' );
                                _chunk += "' target='_blank'>";
                                append_html_encoded( &_chunk, url );
                                _chunk += "</a>";

                                tail = p;
                                break;
                            }
                        }

                        p++;
                        break;
                    }

                    default: p++;
                }
            }

            append_html_encoded( &_chunk, tail, p - tail );
            _line = "";
        }
        else
            return true;   // eof
    }

    return true;
}

//-----------------------------------------------------------------Html_chunk_reader::try_fill_line

bool Html_chunk_reader::try_fill_line()
// Liefert _line oder _html_insertion
{
    bool line_end_found = false;

    while( !line_end_found  &&  _line.length() < max_line_length )
    {
        if( _next_characters == "" )
        {
            _html_insertion = _chunk_reader->html_insertion();   // Z.B. <hr/>
            if( _html_insertion != "" )  return true;

            if( _available_net_chunk_size == 0 )
            {
                if( !_chunk_reader->next_chunk_is_ready() )  return false;
                _available_net_chunk_size = _chunk_reader->get_next_chunk_size();
                _next_characters.reserve( _available_net_chunk_size );
            }

            _next_characters = _chunk_reader->read_from_chunk( _available_net_chunk_size );
            if( _next_characters.length() == 0 )  return true;  // Fertig, bei _chunk_length() == 0: eof

            _available_net_chunk_size -= int_cast(_next_characters.length());
        }

        size_t end = _next_characters.find( '\n', _next_offset );
        size_t length;
        if( end == string::npos )  length = _next_characters.length() - _next_offset;
                             else  length = end + 1 - _next_offset, line_end_found = true;
        
        if( _line.length() + length > max_line_length )  length = max_line_length - _line.length();

        _line.append( _next_characters.data() + _next_offset, length );
        _next_offset += length;

        if( _next_offset == _next_characters.length() )  _next_offset = 0, _next_characters = "";
    }

    return true;
}

//---------------------------------------------------------------Html_chunk_reader::read_from_chunk

string Html_chunk_reader::read_from_chunk( int )
{ 
    switch( _state )
    {
        case reading_prefix:   _state = reading_text;     break;
        case reading_suffix:   _state = reading_finished; break;
        default: ;
    }

    return _chunk;
}

//-------------------------------------------------------------------------------------------------

} //namespace http
} //namespace scheduler
} //namespace sos
