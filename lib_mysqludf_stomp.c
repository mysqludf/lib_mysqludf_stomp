/* 
	lib_mysqludf_stomp - a library to send STOMP messages
	Copyright 2005 LogicBlaze Inc.
	Copyright (C) 2011 Dmitry Demianov aka barlone

	this library use part of libstomp code 

	web of STOMP project: http://stomp.codehaus.org/
	email: barlone@yandex.ru
	
	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.

	Compile with (adapt the include and lib path to your environment):
	> gcc -Wall -O2 -I/usr/local/include/apr-1 -I/usr/local/include/mysql \
	  lib_mysqludf_stomp.c  /usr/local/lib/libapr-1.so -shared \
	  -o lib_mysqludf_stomp.so
        > strip ./lib_mysqludf_stomp.so
	
	Add the functions to MySQL with:
	mysql> CREATE FUNCTION stompsend RETURNS STRING SONAME "lib_mysqludf_stomp.so";
	mysql> CREATE FUNCTION stompsend1 RETURNS STRING SONAME "lib_mysqludf_stomp.so";
	mysql> CREATE FUNCTION stompsend2 RETURNS STRING SONAME "lib_mysqludf_stomp.so";
*/

#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__) || defined(WIN32)
#define DLLEXP __declspec(dllexport) 
#else
#define DLLEXP
#endif

#ifdef STANDARD
/* STANDARD is defined, don't use any mysql functions */
#include <stdlib.h>
#include <string.h>

#ifdef __WIN__
typedef unsigned __int64 ulonglong;	/* Microsofts 64 bit types */
typedef __int64 longlong;
#else
typedef unsigned long long ulonglong;
typedef long long longlong;
#endif /*__WIN__*/

#else
#include <my_global.h>
#include <my_sys.h>
#include <m_string.h>
#endif
#include <mysql.h>
#include <ctype.h>
#include "apr.h"
#include "apr_strings.h"
#include "apr_general.h"
#include "apr_network_io.h"
#include "apr_hash.h"

#ifdef HAVE_DLOPEN

#define LIBVERSION "lib_mysqludf_stomp version 0.2.0"

/******************************************************************************
** function declarations
******************************************************************************/
#ifdef	__cplusplus
extern "C" {
#endif

DLLEXP 
my_bool stompsend_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
DLLEXP 
void stompsend_deinit(UDF_INIT *initid);
DLLEXP 
char *stompsend(UDF_INIT *initid, UDF_ARGS *args, char *result,
		unsigned long *res_length, char *null_value, char *error);

DLLEXP 
my_bool stompsend1_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
DLLEXP 
void stompsend1_deinit(UDF_INIT *initid);
DLLEXP 
char *stompsend1(UDF_INIT *initid, UDF_ARGS *args, char *result,
		unsigned long *res_length, char *null_value, char *error);

DLLEXP 
my_bool stompsend2_init(UDF_INIT *initid, UDF_ARGS *args, char *message);
DLLEXP 
void stompsend2_deinit(UDF_INIT *initid);
DLLEXP 
char *stompsend2(UDF_INIT *initid, UDF_ARGS *args, char *result,
		unsigned long *res_length, char *null_value, char *error);

typedef struct stomp_connection {
      apr_socket_t *socket;
      apr_sockaddr_t *local_sa;
      char *local_ip;
      apr_sockaddr_t *remote_sa;
      char *remote_ip;
} stomp_connection;

typedef struct stomp_frame {
   char *command;
   apr_hash_t *headers;
   char *body;
   int body_length;
} stomp_frame;

static apr_pool_t *pool;

#ifdef __cplusplus
}
#endif
// ------------------------------------------------------------------------------

// STOMP functions 

/******************************************************************************
 * 
 * Used to establish a connection
 *
 ********************************************************************************/
APR_DECLARE(apr_status_t) stomp_connect(stomp_connection **connection_ref, const char *hostname, int port, apr_pool_t *pool)
{
apr_status_t rc;
int socket_family;
stomp_connection *connection=NULL;
   
	//
	// Allocate the connection and a memory pool for the connection.
	//
	connection = apr_pcalloc(pool, sizeof(stomp_connection));
	if( connection == NULL )
		return APR_ENOMEM;
   
#define CHECK_SUCCESS if( rc!=APR_SUCCESS ) { return rc; }
   
	// Look up the remote address
	rc = apr_sockaddr_info_get(&connection->remote_sa, hostname, APR_UNSPEC, port, APR_IPV4_ADDR_OK, pool);
	CHECK_SUCCESS;
	
	// Create the socket.
	socket_family = connection->remote_sa->sa.sin.sin_family;
	rc = apr_socket_create(&connection->socket, socket_family, SOCK_STREAM, APR_PROTO_TCP, pool);
	CHECK_SUCCESS;	

	// Set socket options.
	rc = apr_socket_opt_set(connection->socket, APR_SO_NONBLOCK, 1);
   	CHECK_SUCCESS;
	rc = apr_socket_timeout_set(connection->socket, 1 * APR_USEC_PER_SEC);
   	CHECK_SUCCESS;

   	// Try connect
	rc = apr_socket_connect(connection->socket, connection->remote_sa);
	CHECK_SUCCESS;
   
	// Get the Socket Info
	rc = apr_socket_addr_get(&connection->remote_sa, APR_REMOTE, connection->socket);
	CHECK_SUCCESS;
	rc = apr_sockaddr_ip_get(&connection->remote_ip, connection->remote_sa);
	CHECK_SUCCESS;
	rc = apr_socket_addr_get(&connection->local_sa, APR_LOCAL, connection->socket);
	CHECK_SUCCESS;
	rc = apr_sockaddr_ip_get(&connection->local_ip, connection->local_sa);
	CHECK_SUCCESS;	
   
 
#undef CHECK_SUCCESS
   
	*connection_ref = connection;
	return rc;	
} // stomp_connect

APR_DECLARE(apr_status_t) stomp_disconnect(stomp_connection **connection_ref)
{
apr_status_t result, rc;
stomp_connection *connection = *connection_ref;
   
	if( connection_ref == NULL || *connection_ref==NULL )
		return APR_EGENERAL;
   
	result = APR_SUCCESS;	
	rc = apr_socket_shutdown(connection->socket, APR_SHUTDOWN_WRITE);	
	if( result!=APR_SUCCESS )
		result = rc;
   
	if( connection->socket != NULL ) {
		rc = apr_socket_close(connection->socket);
		if( result!=APR_SUCCESS )
			result = rc;
		connection->socket=NULL;
	}   

	*connection_ref=NULL;
	return rc;	
} // stomp_disconnect


/********************************************************************************
 * 
 * Wrappers around the apr_socket_send and apr_socket_recv calls so that they 
 * read/write their buffers fully.
 *
 ********************************************************************************/
APR_DECLARE(apr_status_t) stomp_write_buffer(stomp_connection *connection, const char *data, apr_size_t size)
{
apr_size_t remaining = size;

	size=0;
	while( remaining>0 ) {
		apr_size_t length = remaining;
		apr_status_t rc = apr_socket_send(connection->socket, data, &length);
		data+=length;
		remaining -= length;
		//      size += length;
		if( rc != APR_SUCCESS )
		         return rc;

	}
	return APR_SUCCESS;
} // stomp_write_buffer


/********************************************************************************
 * 
 * Handles reading and writing stomp_frames to and from the connection
 *
 ********************************************************************************/
APR_DECLARE(apr_status_t) stomp_write(stomp_connection *connection, stomp_frame *frame, apr_pool_t* pool) {
apr_status_t rc;
   
#define CHECK_SUCCESS if( rc!=APR_SUCCESS ) { return rc; }
	// Write the command.
	rc = stomp_write_buffer(connection, frame->command, strlen(frame->command));
	CHECK_SUCCESS;               
	rc = stomp_write_buffer(connection, "\n", 1);
	CHECK_SUCCESS;
   
   // Write the headers
   if( frame->headers != NULL ) {
      
      apr_hash_index_t *i;
      const void *key;
      void *value;
      for (i = apr_hash_first(NULL, frame->headers); i; i = apr_hash_next(i)) {
         apr_hash_this(i, &key, NULL, &value);
         
         rc = stomp_write_buffer(connection, key, strlen(key));
         CHECK_SUCCESS;
         rc = stomp_write_buffer(connection, ":", 1);
         CHECK_SUCCESS;
         rc = stomp_write_buffer(connection, value, strlen(value));
         CHECK_SUCCESS;
         rc = stomp_write_buffer(connection, "\n", 1);
         CHECK_SUCCESS;  
      }

	  if(frame->body_length >= 0) {
		  apr_pool_t *length_pool;
		  char *length_string;

		  apr_pool_create(&length_pool, pool);
		  rc = stomp_write_buffer(connection, "content-length:", 15);
		  CHECK_SUCCESS;
		  
		  length_string = apr_itoa(length_pool, frame->body_length);
		  rc = stomp_write_buffer(connection, length_string, strlen(length_string));
		  CHECK_SUCCESS;
		  rc = stomp_write_buffer(connection, "\n", 1);
		  CHECK_SUCCESS;

		  apr_pool_destroy(length_pool);
	  }
   }
   rc = stomp_write_buffer(connection, "\n", 1);
   CHECK_SUCCESS;
   
   // Write the body.
   if( frame->body != NULL ) {
      int body_length = frame->body_length;
	  if(body_length < 0)
		  body_length = strlen(frame->body);
      rc = stomp_write_buffer(connection, frame->body, body_length);
      CHECK_SUCCESS;
   }
   rc = stomp_write_buffer(connection, "\0\n", 2);
   CHECK_SUCCESS;
      
#undef CHECK_SUCCESS
                    
   return APR_SUCCESS;
} // stomp_write


my_bool stompsend_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{

	/* make sure user has provided exactly three string arguments */
	if (args->arg_count != 3 || (args->arg_type[0] != STRING_RESULT)
			 	 || (args->arg_type[1] != STRING_RESULT)
				 || (args->arg_type[2] != STRING_RESULT)){
		strcpy(message, "stompsend requires 3 string arguments");
		return 1;
	}

	if ((args->lengths[0] == 0) || (args->lengths[1] == 0) || (args->lengths[2] == 0)){
		strcpy(message, "stompsend arguments can not be empty");
		return 1;
	}
	
	// init APR
	if (apr_initialize() != APR_SUCCESS) {
		strcpy(message, "stompsend could not initialize APR");
		return 2;
	}

	if (apr_pool_create(&pool, NULL) != APR_SUCCESS) {
		strcpy(message, "stompsend could not allocate APR pool");
		return 3;
	}

	initid->maybe_null=0;

	return 0;
}

/******************************************************************************
** purpose:	deallocate memory allocated by str_translate_init(); this func
**					is called once for each query which invokes str_translate(),
**					it is called after all of the calls to str_translate() are done
** receives:	pointer to UDF_INIT struct (the same which was used by
**					str_translate_init() and str_translate())
** returns:	nothing
******************************************************************************/
void stompsend_deinit(UDF_INIT *initid __attribute__((unused)))
{
	apr_pool_destroy(pool);	   
}
/******************************************************************************/

char *stompsend(UDF_INIT *initid, UDF_ARGS *args,
			char *result, unsigned long *res_length, 
			char *null_value, char *error)
{
stomp_connection *connection;
stomp_frame frame;
char *host = args->args[0];
char *topic = args->args[1];
char *message = args->args[2];
	
	if (stomp_connect( &connection, host, 61613, pool) != APR_SUCCESS) { 
		strcpy(error, "stompsend could not connect to broker");
		*null_value = 1;
		return NULL;
	}
	
	frame.command = "CONNECT";
	frame.headers = apr_hash_make(pool);
	frame.body = NULL;
	frame.body_length = -1;
	if (stomp_write(connection, &frame, pool) != APR_SUCCESS) { 
		strcpy(error, "stompsend could not send CONNECT frame");
		*null_value = 1;
		return NULL;
	}

	frame.command = "SEND";
	frame.headers = apr_hash_make(pool);
	apr_hash_set(frame.headers, "destination", APR_HASH_KEY_STRING, topic);
	frame.body_length = -1;
	frame.body = message;
	if (stomp_write(connection, &frame, pool) != APR_SUCCESS) { 
		strcpy(error, "stompsend could not send SEND frame");
		*null_value = 1;
		return NULL;
	}

	frame.command = "DISCONNECT";
	frame.headers = NULL;
	frame.body_length = -1;
	frame.body = NULL;
	stomp_write(connection, &frame, pool); // ignore errors

	stomp_disconnect(&connection);

	*res_length = 2;
	strcpy(result, "OK");

	return result;
} // stompsend


my_bool stompsend1_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{

	/* make sure user has provided exactly three string arguments */
	if (args->arg_count != 5 || (args->arg_type[0] != STRING_RESULT)
			 	 || (args->arg_type[1] != STRING_RESULT)
				 || (args->arg_type[2] != STRING_RESULT)
				 || (args->arg_type[3] != STRING_RESULT)
				 || (args->arg_type[4] != STRING_RESULT)){
		strcpy(message, "stompsend1 requires 5 string arguments");
		return 1;
	}

	if ((args->lengths[0] == 0) || (args->lengths[1] == 0) || (args->lengths[2] == 0) || 
	   (args->lengths[3] == 0) || (args->lengths[4] == 0)){
		strcpy(message, "stompsend1 arguments can not be empty");
		return 1;
	}
	
	// init APR
	if (apr_initialize() != APR_SUCCESS) {
		strcpy(message, "stompsend1 could not initialize APR");
		return 2;
	}

	if (apr_pool_create(&pool, NULL) != APR_SUCCESS) {
		strcpy(message, "stompsend1 could not allocate APR pool");
		return 3;
	}

	initid->maybe_null=0;

	return 0;
} // stompsend1_init

/******************************************************************************
** purpose:	deallocate memory allocated by str_translate_init(); this func
**					is called once for each query which invokes str_translate(),
**					it is called after all of the calls to str_translate() are done
** receives:	pointer to UDF_INIT struct (the same which was used by
**					str_translate_init() and str_translate())
** returns:	nothing
******************************************************************************/
void stompsend1_deinit(UDF_INIT *initid __attribute__((unused)))
{
	apr_pool_destroy(pool);	   
} // stompsend1_deinit
/******************************************************************************/

char *stompsend1(UDF_INIT *initid, UDF_ARGS *args,
			char *result, unsigned long *res_length, 
			char *null_value, char *error)
{
stomp_connection *connection;
stomp_frame frame;
char *host = args->args[0];
char *topic = args->args[1];
char *message = args->args[2];
char *hdr1name = args->args[3];
char *hdr1val = args->args[4];
	
	if (stomp_connect( &connection, host, 61613, pool) != APR_SUCCESS) { 
		strcpy(error, "stompsend1 could not connect to broker");
		*null_value = 1;
		return NULL;
	}
	
	frame.command = "CONNECT";
	frame.headers = apr_hash_make(pool);
	frame.body = NULL;
	frame.body_length = -1;
	if (stomp_write(connection, &frame, pool) != APR_SUCCESS) { 
		strcpy(error, "stompsend1 could not send CONNECT frame");
		*null_value = 1;
		return NULL;
	}

	frame.command = "SEND";
	frame.headers = apr_hash_make(pool);
	apr_hash_set(frame.headers, "destination", APR_HASH_KEY_STRING, topic);
	apr_hash_set(frame.headers, hdr1name, APR_HASH_KEY_STRING, hdr1val);
	frame.body_length = -1;
	frame.body = message;
	if (stomp_write(connection, &frame, pool) != APR_SUCCESS) { 
		strcpy(error, "stompsend1 could not send SEND frame");
		*null_value = 1;
		return NULL;
	}

	frame.command = "DISCONNECT";
	frame.headers = NULL;
	frame.body_length = -1;
	frame.body = NULL;
	stomp_write(connection, &frame, pool); // ignore errors

	stomp_disconnect(&connection);

	*res_length = 2;
	strcpy(result, "OK");

	return result;
} // stompsend1

my_bool stompsend2_init(UDF_INIT *initid, UDF_ARGS *args, char *message)
{

	/* make sure user has provided exactly three string arguments */
	if (args->arg_count != 7 || (args->arg_type[0] != STRING_RESULT)
			 	 || (args->arg_type[1] != STRING_RESULT)
				 || (args->arg_type[2] != STRING_RESULT)
				 || (args->arg_type[3] != STRING_RESULT)
				 || (args->arg_type[4] != STRING_RESULT)
				 || (args->arg_type[5] != STRING_RESULT)
				 || (args->arg_type[6] != STRING_RESULT)){
		strcpy(message, "stompsend2 requires 7 string arguments");
		return 1;
	}

	if ((args->lengths[0] == 0) || (args->lengths[1] == 0) || (args->lengths[2] == 0) || 
	   (args->lengths[3] == 0) || (args->lengths[4] == 0) ||
	   (args->lengths[5] == 0) || (args->lengths[6] == 0)){
		strcpy(message, "stompsend2 arguments can not be empty");
		return 1;
	}
	
	// init APR
	if (apr_initialize() != APR_SUCCESS) {
		strcpy(message, "stompsend2 could not initialize APR");
		return 2;
	}

	if (apr_pool_create(&pool, NULL) != APR_SUCCESS) {
		strcpy(message, "stompsend2 could not allocate APR pool");
		return 3;
	}

	initid->maybe_null=0;

	return 0;
} // stompsend1_init

/******************************************************************************
** purpose:	deallocate memory allocated by str_translate_init(); this func
**					is called once for each query which invokes str_translate(),
**					it is called after all of the calls to str_translate() are done
** receives:	pointer to UDF_INIT struct (the same which was used by
**					str_translate_init() and str_translate())
** returns:	nothing
******************************************************************************/
void stompsend2_deinit(UDF_INIT *initid __attribute__((unused)))
{
	apr_pool_destroy(pool);	   
} // stompsend2_deinit
/******************************************************************************/

char *stompsend2(UDF_INIT *initid, UDF_ARGS *args,
			char *result, unsigned long *res_length, 
			char *null_value, char *error)
{
stomp_connection *connection;
stomp_frame frame;
char *host = args->args[0];
char *topic = args->args[1];
char *message = args->args[2];
char *hdr1name = args->args[3];
char *hdr1val = args->args[4];
char *hdr2name = args->args[5];
char *hdr2val = args->args[6];
	
	if (stomp_connect( &connection, host, 61613, pool) != APR_SUCCESS) { 
		strcpy(error, "stompsend2 could not connect to broker");
		*null_value = 1;
		return NULL;
	}
	
	frame.command = "CONNECT";
	frame.headers = apr_hash_make(pool);
	frame.body = NULL;
	frame.body_length = -1;
	if (stomp_write(connection, &frame, pool) != APR_SUCCESS) { 
		strcpy(error, "stompsend2 could not send CONNECT frame");
		*null_value = 1;
		return NULL;
	}

	frame.command = "SEND";
	frame.headers = apr_hash_make(pool);
	apr_hash_set(frame.headers, "destination", APR_HASH_KEY_STRING, topic);
	apr_hash_set(frame.headers, hdr1name, APR_HASH_KEY_STRING, hdr1val);
	apr_hash_set(frame.headers, hdr2name, APR_HASH_KEY_STRING, hdr2val);
	frame.body_length = -1;
	frame.body = message;
	if (stomp_write(connection, &frame, pool) != APR_SUCCESS) { 
		strcpy(error, "stompsend2 could not send SEND frame");
		*null_value = 1;
		return NULL;
	}

	frame.command = "DISCONNECT";
	frame.headers = NULL;
	frame.body_length = -1;
	frame.body = NULL;
	stomp_write(connection, &frame, pool); // ignore errors

	stomp_disconnect(&connection);

	*res_length = 2;
	strcpy(result, "OK");

	return result;
} // stompsend2

#endif /* HAVE_DLOPEN */
