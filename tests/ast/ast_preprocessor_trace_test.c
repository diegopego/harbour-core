// tests/ast/ast_preprocessor_trace_test.c
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <errno.h>
#include <stdlib.h>
#include "hbpp.h"
#include "hbapi.h"
#include <stdio.h>
#include <string.h>

typedef struct
{
   char * buffer;
   size_t length;
   size_t capacity;
}
TRACE_BUFFER;

static void trace_buffer_init( TRACE_BUFFER * buf )
{
   buf->buffer = NULL;
   buf->length = 0;
   buf->capacity = 0;
}

static void trace_buffer_reserve( TRACE_BUFFER * buf, size_t extra )
{
   size_t needed = buf->length + extra + 1;

   if( needed > buf->capacity )
   {
      size_t capacity = buf->capacity == 0 ? 256 : buf->capacity;

      while( capacity < needed )
         capacity *= 2;

      buf->buffer = ( char * ) hb_xrealloc( buf->buffer, capacity );
      buf->capacity = capacity;
   }
}

static void trace_buffer_append_char( TRACE_BUFFER * buf, char ch )
{
   trace_buffer_reserve( buf, 1 );
   buf->buffer[ buf->length++ ] = ch;
   buf->buffer[ buf->length ] = '\0';
}

static void trace_buffer_append_strn( TRACE_BUFFER * buf, const char * str, size_t len )
{
   if( len == 0 )
      return;

   trace_buffer_reserve( buf, len );
   memcpy( buf->buffer + buf->length, str, len );
   buf->length += len;
   buf->buffer[ buf->length ] = '\0';
}

static void trace_buffer_append_str( TRACE_BUFFER * buf, const char * str )
{
   if( str && *str )
      trace_buffer_append_strn( buf, str, strlen( str ) );
}

static void trace_buffer_append_int( TRACE_BUFFER * buf, int value )
{
   char tmp[ 32 ];

   hb_snprintf( tmp, sizeof( tmp ), "%d", value );
   trace_buffer_append_str( buf, tmp );
}

static void trace_buffer_append_size( TRACE_BUFFER * buf, HB_SIZE value )
{
   char tmp[ 32 ];

   if( value == ( HB_SIZE ) -1 )
      hb_snprintf( tmp, sizeof( tmp ), "%d", -1 );
   else
      hb_snprintf( tmp, sizeof( tmp ), "%lu", ( unsigned long ) value );

   trace_buffer_append_str( buf, tmp );
}

static void trace_buffer_append_json_string( TRACE_BUFFER * buf, const char * str )
{
   trace_buffer_append_char( buf, '"' );

   if( str )
   {
      while( *str )
      {
         unsigned char ch = ( unsigned char ) *str++;

         switch( ch )
         {
            case '\\':
               trace_buffer_append_str( buf, "\\\\" );
               break;
            case '"':
               trace_buffer_append_str( buf, "\\\"" );
               break;
            case '\n':
               trace_buffer_append_str( buf, "\\n" );
               break;
            case '\r':
               trace_buffer_append_str( buf, "\\r" );
               break;
            case '\t':
               trace_buffer_append_str( buf, "\\t" );
               break;
            default:
               if( ch < 0x20 )
               {
                  char tmp[ 7 ];
                  hb_snprintf( tmp, sizeof( tmp ), "\\u%04x", ch );
                  trace_buffer_append_str( buf, tmp );
               }
               else
                  trace_buffer_append_char( buf, ( char ) ch );
         }
      }
   }

   trace_buffer_append_char( buf, '"' );
}

static void trace_buffer_release( TRACE_BUFFER * buf )
{
   if( buf->buffer )
   {
      hb_xfree( buf->buffer );
      buf->buffer = NULL;
   }
   buf->length = buf->capacity = 0;
}

static void hb_pp_collect_trace( void * cargo, const HB_PP_TRACE_EVENT * event )
{
   TRACE_BUFFER * buf = ( TRACE_BUFFER * ) cargo;

   trace_buffer_append_char( buf, '{' );
   trace_buffer_append_str( buf, "\"rule\":" );
   trace_buffer_append_json_string( buf, event->szRuleKind ? event->szRuleKind : "" );
   trace_buffer_append_str( buf, ",\"macro\":" );
   if( event->szMacroName )
      trace_buffer_append_json_string( buf, event->szMacroName );
   else
      trace_buffer_append_str( buf, "null" );
   trace_buffer_append_str( buf, ",\"module\":" );
   if( event->szCallModule )
      trace_buffer_append_json_string( buf, event->szCallModule );
   else
      trace_buffer_append_str( buf, "null" );
   trace_buffer_append_str( buf, ",\"line\":" );
   trace_buffer_append_int( buf, event->iCallLine );
   trace_buffer_append_str( buf, ",\"column\":" );
   trace_buffer_append_int( buf, event->iCallColumn );
   trace_buffer_append_str( buf, ",\"endLine\":" );
   trace_buffer_append_int( buf, event->iCallEndLine );
   trace_buffer_append_str( buf, ",\"endColumn\":" );
   trace_buffer_append_int( buf, event->iCallEndColumn );
   trace_buffer_append_str( buf, ",\"offset\":" );
   trace_buffer_append_size( buf, event->nCallOffset );
   trace_buffer_append_str( buf, ",\"endOffset\":" );
   trace_buffer_append_size( buf, event->nCallEndOffset );
   trace_buffer_append_str( buf, ",\"expansionId\":" );
   trace_buffer_append_size( buf, event->nExpansionId );
   trace_buffer_append_str( buf, ",\"source\":" );
   trace_buffer_append_json_string( buf, event->pszSource );
   trace_buffer_append_str( buf, ",\"result\":" );
   trace_buffer_append_json_string( buf, event->pszResult );
   trace_buffer_append_char( buf, '}' );
   trace_buffer_append_char( buf, '\n' );

}

static void preprocess_fixture( const char * fixturePath, TRACE_BUFFER * traceBuf, TRACE_BUFFER * ppoBuf )
{
   PHB_PP_STATE pState = hb_pp_new();
   HB_SIZE nLen;
   char * szLine;

   assert_non_null( pState );

   hb_pp_init( pState, HB_TRUE, HB_FALSE, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL );
   hb_pp_setTraceCallback( pState, hb_pp_collect_trace, traceBuf );

   assert_true( hb_pp_inFile( pState, fixturePath, HB_TRUE, NULL, HB_TRUE ) );

   while( ( szLine = hb_pp_nextLine( pState, &nLen ) ) != NULL )
   {
      if( nLen > 0 )
         trace_buffer_append_strn( ppoBuf, szLine, ( size_t ) nLen );
      trace_buffer_append_char( ppoBuf, '\n' );

      if( hb_pp_eof( pState ) && nLen == 0 )
         break;
   }

   hb_pp_free( pState );
}

static char * load_file( const char * path, size_t * outLen )
{
   FILE * fp = fopen( path, "rb" );
   char * buffer;
   size_t nRead;

   assert_non_null( fp );

   assert_return_code( fseek( fp, 0, SEEK_END ), 0 );
   long size = ftell( fp );
   assert_true( size >= 0 );
   assert_return_code( fseek( fp, 0, SEEK_SET ), 0 );

   buffer = ( char * ) hb_xgrab( ( size_t ) size + 1 );
   nRead = fread( buffer, 1, ( size_t ) size, fp );
   assert_int_equal( nRead, ( size_t ) size );
   buffer[ nRead ] = '\0';

   fclose( fp );

   if( outLen )
      *outLen = nRead;

   return buffer;
}

static void write_file( const char * path, const char * data, size_t len )
{
   FILE * fp = fopen( path, "wb" );
   size_t written = 0;

   assert_non_null( fp );

   if( len > 0 )
      written = fwrite( data, 1, len, fp );

   assert_int_equal( ( int ) written, ( int ) len );
   fclose( fp );
}

static void assert_fixture_outputs( const char * fixtureName )
{
   char pathBuffer[ 256 ];
   TRACE_BUFFER traceBuf;
   TRACE_BUFFER ppoBuf;
   char * goldenTrace;
   char * goldenPpo;
   size_t goldenTraceLen, goldenPpoLen;
   const char * regenEnv;
   int fRegen;

   trace_buffer_init( &traceBuf );
   trace_buffer_init( &ppoBuf );

   hb_snprintf( pathBuffer, sizeof( pathBuffer ),
                "tests/ast/preprocessor/fixtures/%s.prg", fixtureName );
   preprocess_fixture( pathBuffer, &traceBuf, &ppoBuf );

   regenEnv = getenv( "HB_PP_REGEN_FIXTURES" );
   fRegen = ( regenEnv && regenEnv[ 0 ] && regenEnv[ 0 ] != '0' );

   hb_snprintf( pathBuffer, sizeof( pathBuffer ),
                "tests/ast/preprocessor/fixtures/%s.trace.json", fixtureName );
   if( fRegen )
   {
      write_file( pathBuffer, traceBuf.buffer ? traceBuf.buffer : "", traceBuf.length );
      goldenTrace = NULL;
      goldenTraceLen = 0;
   }
   else
      goldenTrace = load_file( pathBuffer, &goldenTraceLen );

   hb_snprintf( pathBuffer, sizeof( pathBuffer ),
                "tests/ast/preprocessor/fixtures/%s.ppo", fixtureName );
   if( fRegen )
   {
      write_file( pathBuffer, ppoBuf.buffer ? ppoBuf.buffer : "", ppoBuf.length );
      goldenPpo = NULL;
      goldenPpoLen = 0;
   }
   else
      goldenPpo = load_file( pathBuffer, &goldenPpoLen );

   if( ! fRegen )
   {
      assert_int_equal( ( int ) goldenTraceLen, ( int ) traceBuf.length );
      assert_string_equal( goldenTrace, traceBuf.buffer ? traceBuf.buffer : "" );

      assert_int_equal( ( int ) goldenPpoLen, ( int ) ppoBuf.length );
      assert_string_equal( goldenPpo, ppoBuf.buffer ? ppoBuf.buffer : "" );

      hb_xfree( goldenTrace );
      hb_xfree( goldenPpo );
   }

   trace_buffer_release( &traceBuf );
   trace_buffer_release( &ppoBuf );
}

static void test_macro_trace_fixture( void ** state )
{
   ( void ) state;
   assert_fixture_outputs( "macro_trace" );
}

static void test_command_trace_fixture( void ** state )
{
   ( void ) state;
   assert_fixture_outputs( "command_trace" );
}

int main( void )
{
   const struct CMUnitTest tests[] =
   {
      cmocka_unit_test( test_macro_trace_fixture ),
      cmocka_unit_test( test_command_trace_fixture )
   };

   return cmocka_run_group_tests( tests, NULL, NULL );
}
