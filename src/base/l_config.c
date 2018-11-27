#include "lk.h"

config_t conf;

// config_get ----
status config_get ( meta_t ** meta, char * path )
{
	int fd;
	struct stat stconf;
	uint32 length;
	meta_t * new;

	if( stat( path, &stconf ) < 0 ) {
		err_log( "%s --- config.json not found", __func__  );
		return ERROR;
	}
	length = (uint32)stconf.st_size;
	if( length > CONF_SETTING_LENGTH ) {
		err_log( "%s --- config.json > CONF_SETTING_LENGTH" );
		return ERROR;
	}
	if( OK != meta_alloc( &new, length ) ) {
		err_log("%s --- meta alloc", __func__ );
		return ERROR;
	}
	fd = open( path, O_RDONLY );
	if( ERROR == fd ) {
		err_log("%s --- can't open config.json", __func__ );
		meta_free( new );
		return ERROR;
	}
	if( ERROR == read( fd, new->last, length ) ) {
		err_log("%s --- read config.json", __func__ );
		meta_free( new );
		close( fd );
		return ERROR;
	}
	new->last += length;
	close(fd);
	*meta = new;
	return OK;
}
// config_parse_global ----------
static status config_parse_global( json_t * json )
{
	char str[1024] = {0};
	json_t *root_obj, *v;

	json_get_child( json, 1, &root_obj );
	if( OK == json_get_obj_bool(root_obj, "daemon", l_strlen("daemon"), &v ) ) {
		conf.daemon = (v->type == JSON_TRUE) ? 1 : 0;
	}
	if ( OK == json_get_obj_num(root_obj, "worker_process", l_strlen("worker_process"), &v ) ) {
		conf.worker_process = (uint32)v->num;
		if( conf.worker_process > MAXPROCESS ) {
			err_log("%s --- work_procrss too much, [%d]", __func__, conf.worker_process );
			return ERROR;
		}
	}
	if( OK == json_get_obj_bool(root_obj, "reuse_port", l_strlen("reuse_port"), &v ) ) {
		conf.reuse_port = (v->type == JSON_TRUE) ? 1 : 0;
	}
	if( OK == json_get_obj_bool(root_obj, "accept_mutex", l_strlen("accept_mutex"), &v ) ) {
		conf.accept_mutex = (v->type == JSON_TRUE) ? 1 : 0;
	}
	if( OK == json_get_obj_str(root_obj, "sslcrt", l_strlen("sslcrt"), &v ) ) {
		conf.sslcrt.data = v->name.data;
		conf.sslcrt.len = v->name.len;
		memset( str, 0, sizeof(str) );
		l_memcpy( str, conf.sslcrt.data, conf.sslcrt.len );
		if( OK != access( str, F_OK ) ) {
			err_log("%s --- sslcrt file [%.*s] not found", __func__,
			conf.sslcrt.len, conf.sslcrt.data );
			return ERROR;
		}
	}
	if( OK == json_get_obj_str(root_obj, "sslkey", l_strlen("sslkey"), &v ) ) {
		conf.sslkey.data = v->name.data;
		conf.sslkey.len = v->name.len;
		memset( str, 0, sizeof(str) );
		l_memcpy( str, conf.sslkey.data, conf.sslkey.len );
		if( OK != access( str, F_OK ) ) {
			err_log("%s --- sslkey file [%.*s] not found", __func__,
		 	conf.sslkey.len, conf.sslkey.data );
			return ERROR;
		}
	}
	if( OK == json_get_obj_bool(root_obj, "log_error", l_strlen("log_error"), &v ) ) {
		conf.log_error = (v->type == JSON_TRUE) ? 1 : 0;
	}
	if( OK == json_get_obj_bool(root_obj, "log_debug", l_strlen("log_debug"), &v ) ) {
		conf.log_debug = (v->type == JSON_TRUE) ? 1 : 0;
	}
	return OK;
}
// config_parse_tunnel -------------
static status config_parse_tunnel( json_t * json )
{
	json_t * root_obj, *tunnel_obj, *v;
	status rc;

	json_get_child( json, 1, &root_obj );
	if( OK == json_get_obj_obj(root_obj, "tunnel", l_strlen("tunnel"), &tunnel_obj ) ) {
		rc = json_get_obj_str(tunnel_obj, "mode", l_strlen("mode"), &v );
		if( rc == ERROR ) {
			err_log("%s --- tunnel need specify a valid 'mode'", __func__ );
			return ERROR;
		}
		// server
		// client
		// single
		if( v->name.len == l_strlen("xxxxxx") ) {
			if( strncmp( v->name.data, "single", v->name.len ) == 0 ) {
				conf.tunnel_mode = TUNNEL_SINGLE;
			} else if ( strncmp( v->name.data, "client", v->name.len ) == 0 ) {
				conf.tunnel_mode = TUNNEL_CLIENT;
				rc = json_get_obj_str(tunnel_obj, "serverip", l_strlen("serverip"), &v );
				if( rc == ERROR ) {
					err_log("%s --- tunnel mode client need specify a valid 'serverip'", __func__ );
					return ERROR;
				}
				conf.serverip.data = v->name.data;
				conf.serverip.len = v->name.len;
			} else if ( strncmp( v->name.data, "server", v->name.len ) == 0 ) {
				conf.tunnel_mode = TUNNEL_SERVER;
			} else {
				err_log("%s --- tunnel invalid 'mode' [%.*s]", __func__,
				v->name.len, v->name.data );
				return ERROR;
			}
		} else {
			err_log("%s --- tunnel invalid 'mode' length [%d]", __func__, v->name.len );
			return ERROR;
		}
	}
	return OK;
}

// config_parse -----
static status config_parse( json_t * json )
{
	if( OK != config_parse_global( json ) ) {
		err_log( "%s --- parse global", __func__ );
		return ERROR;
	}
	if( OK != config_parse_tunnel( json ) ) {
		err_log( "%s --- parse tunnel", __func__  );
		return ERROR;
	}
	return OK;
}
// config_start ----
static status config_start( void )
{
	json_t * json;
	meta_t * meta;

	if( OK != config_get( &meta, L_PATH_CONFIG ) ) {
		err_log( "%s --- configuration file open", __func__  );
		return ERROR;
	}
	debug_log( "%s --- configuration file:\n[%.*s]",
		__func__,
		meta_len( meta->pos, meta->last ),
		meta->pos
	);
	if( OK != json_decode( &json, meta->pos, meta->last ) ) {
		err_log("%s --- configuration file decode failed", __func__ );
		meta_free( meta );
		return ERROR;
	}
	conf.meta = meta;
	if( OK != config_parse( json ) ) {
		json_free( json );
		return ERROR;
	}
	json_free( json );
	return OK;
}
// config_init ----
status config_init ( void )
{
	memset( &conf, 0, sizeof(config_t) );
	conf.log_debug = 1;
	conf.log_error = 1;

	if( OK != config_start(  ) ) {
		return ERROR;
	}
	return OK;
}
// config_end -----
status config_end ( void )
{
	meta_free( conf.meta );
	return OK;
}
