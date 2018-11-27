#ifndef _LK_GLOBAL_H_INCLUDED_
#define _LK_GLOBAL_H_INCLUDED_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <limits.h>
#include <signal.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/select.h>
#include <malloc.h>
#include <errno.h>
#include <sys/un.h>
#include <pthread.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <assert.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <math.h>
#include <ctype.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/sendfile.h>
//
#define 	L_PATH_PIDFILE			"/usr/local/ktunnel/logs/pid"
#define 	L_PATH_CONFIG			"/usr/local/ktunnel/config/config.json"
#define 	L_PATH_LOG_MAIN			"/usr/local/ktunnel/logs/l_log"
#define 	L_PATH_LOG_ACCESS		"/usr/local/ktunnel/logs/l_access"

#define OK 					0
#define ERROR 				-1
#define NOT_FOUND			-2
#define AGAIN 				-18
#define DONE				2

#define 	HTTP_PARSE_HEADER_DONE						14
#define 	HTTP_PARSE_INVALID_HEADER					13

#define l_unused( x ) 			 	((void)x)
#define l_safe_free( x )			(free(x))
#define l_safe_malloc( len )		(malloc((size_t)(len)))
#define l_strlen( str )				((uint32)strlen( str ))
#define l_min( x, y )				( (x<y) ? x : y )
#define l_memcpy( dst, src, len ) \
	( memcpy((char*)dst, (char*)src, (size_t)len) );
#define l_get_struct( ptr, struct_type, struct_member ) \
(\
	(struct_type *)\
	( ((char*)ptr) - offsetof( struct_type, struct_member ) )\
)

typedef char  		byte;
typedef int32_t		int32;
typedef uint32_t	uint32;
typedef int32_t     status;
typedef volatile uint32	l_atomic_t;
typedef struct connection_t connection_t;

#endif
