#ifndef _L_CONFIG_H_INCLUDED_
#define _L_CONFIG_H_INCLUDED_

#define 		CONF_SETTING_LENGTH				32768

typedef struct config_t
{
	meta_t* 	meta;
	// global
	uint32		daemon;
	uint32		worker_process;
	uint32		reuse_port;
	uint32		accept_mutex;
	string_t	sslcrt;
	string_t	sslkey;
	// log
	uint32		log_debug;
	uint32		log_error;
	uint32		http_access_log;
	// tunnel
	uint32		tunnel_mode;
	string_t	serverip;
}config_t;

extern config_t conf;

status config_get ( meta_t ** meta, char * path );
status config_init( void );
status config_end( void );

#endif
