#include "lk.h"

static status json_parse_token( json_t * parent, json_content_t * content );
status json_free( json_t * json );

// json_get_child -----
status json_get_child( json_t * parent, uint32 index, json_t ** child )
{
	if( parent->list->elem_num < 1 ) {
		return ERROR;
	}
	if( index > parent->list->elem_num ) {
		return ERROR;
	}
	*child = mem_list_get( parent->list, index );
	return OK;
}
// json_get_child_by_name -----
status json_get_child_by_name( json_t * parent, char * str, uint32 length, json_t ** child )
{
	json_t *v, *k;
	uint32 i;

	if( parent->type != JSON_OBJ ) {
		return ERROR;
	}
	for( i = 1; i <= parent->list->elem_num; i ++ ) {
		json_get_child( parent, i, &k );
		if( strncmp( k->name.data, str, length ) == 0 &&
		k->name.len == length ) {
			json_get_child( k, 1, &v );
			*child = v;
			return OK;
		}
	}
	return ERROR;
}
// json_get_obj_str ---
status json_get_obj_str( json_t * obj, char * str, uint32 length, json_t ** child )
{
	if( OK != json_get_child_by_name( obj, str, length, child ) ) {
		return ERROR;
	}
	if( (*child)->type != JSON_STR ) {
		return ERROR;
	}
	return OK;
}
// json_get_obj_bool ---
status json_get_obj_bool( json_t * obj, char * str, uint32 length, json_t ** child )
{
	if( OK != json_get_child_by_name( obj, str, length, child ) ) {
		return ERROR;
	}
	if( (*child)->type != JSON_TRUE && (*child)->type != JSON_FALSE ) {
		return ERROR;
	}
	return OK;
}
// json_get_obj_num ---
status json_get_obj_num( json_t * obj, char * str, uint32 length, json_t ** child )
{
	if( OK != json_get_child_by_name( obj, str, length, child ) ) {
		return ERROR;
	}
	if( (*child)->type != JSON_NUM ) {
		return ERROR;
	}
	return OK;
}
// json_get_obj_null ----
status json_get_obj_null( json_t * obj, char * str, uint32 length, json_t ** child )
{
	if( OK != json_get_child_by_name( obj, str, length, child ) ) {
		return ERROR;
	}
	if( (*child)->type != JSON_NULL ) {
		return ERROR;
	}
	return OK;
}
// json_get_obj_arr ---
status json_get_obj_arr( json_t * obj, char * str, uint32 length, json_t ** child )
{
	if( OK != json_get_child_by_name( obj, str, length, child ) ) {
		return ERROR;
	}
	if( (*child)->type != JSON_ARR ) {
		return ERROR;
	}
	return OK;
}
// json_get_obj_obj ----
status json_get_obj_obj( json_t * obj, char * str, uint32 length, json_t ** child )
{
	if( OK != json_get_child_by_name( obj, str, length, child ) ) {
		return ERROR;
	}
	if( (*child)->type != JSON_OBJ ) {
		return ERROR;
	}
	return OK;
}

// json_parse_true -------
static status json_parse_true( json_content_t * content )
{
	char * p;

	enum {
		t = 0,
		tr,
		tru,
		true
	} state;
	state = t;
	for( ; content->p < content->end; content->p ++ ) {
		p = content->p;

		if( state == t ) {
			state = tr;
			continue;
		} else if ( state == tr ) {
			if( *p != 'r' ) {
				return ERROR;
			}
			state = tru;
			continue;
		} else if ( state == tru ) {
			if( *p != 'u' ) return ERROR;
			state = true;
			continue;
		} else if ( state == true ) {
			if( *p != 'e' ) return ERROR;
			return OK;
		}
	}
	return ERROR;
}

// json_parse_false ------
static status json_parse_false( json_content_t * content )
{
	char * p;

	enum {
		f = 0,
		fa,
		fal,
		fals,
		false
	} state;
	state = f;
	for( ; content->p < content->end; content->p ++ ) {
		p = content->p;

		if( state == f ) {
			state = fa;
			continue;
		} else if ( state == fa ) {
			if( *p != 'a' ) return ERROR;
			state = fal;
			continue;
		} else if ( state == fal ) {
			if( *p != 'l' ) return ERROR;
			state = fals;
			continue;
		} else if ( state == fals ) {
			if( *p != 's' ) return ERROR;
			state = false;
			continue;
		} else if ( state == false ) {
			if( *p != 'e' ) return ERROR;
			return OK;
		}
	}
	return ERROR;
}

// json_parse_null -------
static status json_parse_null( json_content_t * content )
{
	char * p;

	enum {
		n = 0,
		nu,
		nul,
		null
	} state;
	state = n;
	for( ; content->p < content->end; content->p ++ ) {
		p = content->p;

		if( state == n ) {
			state = nu;
			continue;
		} else if ( state == nu ) {
			if( *p != 'u' ) return ERROR;
			state = nul;
			continue;
		} else if ( state == nul ) {
			if( *p != 'l' ) return ERROR;
			state = null;
			continue;
		} else if ( state == null ) {
			if( *p != 'l' ) return ERROR;
			return OK;
		}
	}
	return ERROR;
}

// json_parse_string ----------
static status json_parse_string( json_t * json, json_content_t * content )
{
	char * p = NULL;

	enum {
		l_quotes = 0,
		string_start,
		string,
		transfer,
		transfer_ux,
		transfer_uxx,
		transfer_uxxx,
		transfer_uxxxx
	} state;
	state = l_quotes;
	for( ;content->p < content->end; content->p++ ) {
		p = content->p;

		if( state == l_quotes ) {
			state = string_start;
			continue;
		}
		if( state == string_start ) {
			json->name.data = p;
			if( *p == '\\' ) {
				state = transfer;
				continue;
			} else if( *p == '"' ) {
				json->name.len = meta_len( json->name.data, p );
				return OK;
			} else {
				state = string;
				continue;
			}
		}
		if( state == string ) {
			if( *p == '\\' ) {
				state = transfer;
				continue;
			} else if( *p == '"' ) {
				json->name.len = meta_len( json->name.data, p );
				return OK;
			}
		}
		if( state == transfer ) {
			if(
			*p == '\\'||
			*p == '/' ||
			*p == '"' ||
			*p == 'b' ||
			*p == 'f' ||
			*p == 'n' ||
			*p == 'r' ||
			*p == 't' ) {
				state = string;
				continue;
			} else if( *p == 'u' ) {
				state = transfer_ux;
				continue;
			} else {
				return ERROR;
			}
		}
		if( state == transfer_ux ) {
			if(
			( *p >= '0' && *p <= '9' ) ||
			( *p >= 'a' && *p <= 'f' ) ||
			( *p >= 'A' && *p <= 'F' ) ) {
				state = transfer_uxx;
				continue;
			} else {
				return ERROR;
			}
		}
		if( state == transfer_uxx ) {
			if(
			( *p >= '0' && *p <= '9' ) ||
			( *p >= 'a' && *p <= 'f' ) ||
			( *p >= 'A' && *p <= 'F' ) ) {
				state = transfer_uxxx;
				continue;
			} else {
				return ERROR;
			}
		}
		if( state == transfer_uxxx ) {
			if(
			( *p >= '0' && *p <= '9' ) ||
			( *p >= 'a' && *p <= 'f' ) ||
			( *p >= 'A' && *p <= 'F' ) ) {
				state = transfer_uxxxx;
				continue;
			} else {
				return ERROR;
			}
		}
		if( state == transfer_uxxxx ) {
			if(
			( *p >= '0' && *p <= '9' ) ||
			( *p >= 'a' && *p <= 'f' ) ||
			( *p >= 'A' && *p <= 'F' ) ) {
				state = string;
				continue;
			} else {
				return ERROR;
			}
		}
	}
	return ERROR;
}

// json_parse_obj_find_repeat ------
static status json_parse_obj_find_repeat( json_t * parent, json_t * child )
{
	uint32 i, time = 0;
	json_t * t;

	for( i = 1; i <= parent->list->elem_num; i ++ ) {
		t = mem_list_get( parent->list, i );
		if( t->name.len == child->name.len &&
				strncmp( t->name.data, child->name.data, t->name.len ) == 0 ) {
			time ++;
		}
	}
	if( time > 1 ) {
		return OK;
	}
	return ERROR;
}

// json_parse_obj ------------
static status json_parse_obj( json_t * json, json_content_t * content )
{
	json_t * child = NULL;
	char * p;

	enum {
		obj_start = 0,
		obj_name,
		obj_colon,
		obj_value,
		obj_part
	} state;
	state = obj_start;
	for( ; content->p < content->end; content->p ++ ) {
		p = content->p;

		if( state == obj_start ) {
			state = obj_name;
			continue;
		}
		if( state == obj_name ) {
			if(
			*p == ' '  ||
			*p == '\r' ||
			*p == '\n' ||
			*p == '\t' ) {
				continue;
			} else if( *p == '"' ) {
				child = mem_list_push( json->list );
				child->parent = json;
				if( OK != json_parse_string( child, content ) ) {
					return ERROR;
				}
				// same level find repeat
				if( OK == json_parse_obj_find_repeat( json, child ) ) {
					return ERROR;
				}
				state = obj_colon;
				continue;
			} else if( *p == '}' ) {
				return OK;
			} else {
				return ERROR;
			}
		}
		if( state == obj_colon ) {
			if(
			*p == ' '  ||
			*p == '\r' ||
			*p == '\n' ||
			*p == '\t' ) {
				continue;
			} else if ( *p == ':' ) {
				state = obj_value;
				continue;
			} else {
				return ERROR;
			}
		}
		if( state == obj_value ) {
			mem_list_create( &child->list, sizeof(json_t) );
			if( OK != json_parse_token( child, content ) ) {
				return ERROR;
			}
			state = obj_part;
			continue;
		}
		if( state == obj_part ) {
			if(
			*p == ' '  ||
			*p == '\r' ||
			*p == '\n' ||
			*p == '\t' ) {
				continue;
			} else if ( *p == ',' ) {
				state = obj_name;
				continue;
			} else if ( *p == '}' ) {
				return OK;
			} else {
				return ERROR;
			}
		}
	}
	return ERROR;
}

// json_parse_array -----------
static status json_parse_array ( json_t * json, json_content_t * content )
{
	char * p;

	enum {
		arr_start = 0,
		arr_value_start,
		arr_value,
		arr_part,
		arr_end
	} state;
	state = arr_start;
	for( ; content->p < content->end; content->p ++ ) {
		p = content->p;

		if( state == arr_start ) {
			state = arr_value_start;
			continue;
		}
		if( state == arr_value_start ) {
			if(
			*p == ' '  ||
			*p == '\r' ||
			*p == '\n' ||
			*p == '\t' ) {
				continue;
			} else if ( *p == ']' ) {
				return OK;
			} else {
				if( OK != json_parse_token( json, content ) ) {
					return ERROR;
				}
				state = arr_part;
				continue;
			}
		}
		if( state == arr_part ) {
			if(
			*p == ' '  ||
			*p == '\r' ||
			*p == '\n' ||
			*p == '\t' ) {
				continue;
			} else if( *p == ',' ) {
				state = arr_value;
				continue;
			} else if( *p == ']' ) {
				return OK;
			} else {
				return ERROR;
			}
		}
		if( state == arr_value ) {
			if(
			*p == ' '  ||
			*p == '\r' ||
			*p == '\n' ||
			*p == '\t' ) {
				continue;
			} else {
				if( OK != json_parse_token( json, content ) ) {
					return ERROR;
				}
				state = arr_part;
				continue;
			}
		}
	}
	return ERROR;
}

/// json_parse_num ------------
static status json_parse_num ( json_t * json, json_content_t * content )
{
	char * p = NULL;
	string_t num_string;
	char * end;

	num_string.data = NULL;
	num_string.len = 0;

	enum {
		num_start = 0,
		inte_string_start,
		inte_string,
		inte_before_end,
		dec_string_start,
		dec_string,
		dec_before_end,
		over
	} state;
	state = num_start;
	for( ; content->p < content->end; content->p++ ) {
		p = content->p;

		if( state == num_start ) {
			if( *p == ' ' ||
			*p == '\r' ||
			*p == '\n' ||
			*p == '\t' ) {
				continue;
			} else if (
			*p == '+' ||
			*p == '-' ) {
				num_string.data = p;
				state = inte_string_start;
				continue;
			} else if ( ( *p >= '0' && *p <= '9' ) ) {
				num_string.data = p;
				state = inte_string_start;
			} else {
				return ERROR;
			}
		}
		if( state == inte_string_start ) {
			if( *p >= '0' && *p <= '9' ) {
				state = inte_string;
				continue;
			} else {
				return ERROR;
			}
		}
		if( state == inte_string ) {
			if( *p >= '0' && *p <= '9' ) {
				continue;
			} else if ( *p == '.' ) {
				state = dec_string_start;
				continue;
			} else if ( *p == ',' || *p == '}' || *p == ']' ) {
				state = over;
			} else if( *p == ' ' ) {
				state = inte_before_end;
				continue;
			} else {
				return ERROR;
			}
		}
		if( state == inte_before_end ) {
			if( *p == ' ' ||
			*p == '\r' ||
			*p == '\n' ||
			*p == '\t' ) {
				continue;
			} else if( *p == ',' || *p == '}' || *p == ']' ) {
				state = over;
			} else {
				return ERROR;
			}
		}
		if( state == dec_string_start ) {
			if( *p >= '0' && *p <= '9' ) {
				state = dec_string;
				continue;
			} else {
				return ERROR;
			}
		}
		if( state == dec_string ) {
			if( *p >= '0' && *p <= '9' ) {
				continue;
			} else if ( *p == ',' || *p == '}' || *p == ']' ) {
				state = over;
			} else if( *p == ' ' ) {
				state = dec_before_end;
				continue;
			} else {
				return ERROR;
			}
		}
		if( state == dec_before_end ) {
			if( *p == ' ' ||
			*p == '\r' ||
			*p == '\n' ||
			*p == '\t' ) {
				continue;
			} else if ( *p == ',' || *p == '}' || *p == ']' ) {
				state = over;
			} else {
				return ERROR;
			}
		}
		if( state == over ) {
			num_string.len = meta_len( num_string.data, p );

			end = num_string.data + num_string.len;
			json->num = strtod( num_string.data, &end );
			if( errno == ERANGE && ( json->num == HUGE_VAL || json->num == -HUGE_VAL ) ) {
				err_log ( "%s --- number is too big or to less", __func__ );
				return ERROR;
			}
			if( num_string.data == end ) {
				err_log ( "%s --- number str empty", __func__ );
				return ERROR;
			}

			content->p-=1;
			return OK;
		}
	}
	if( json->parent->type == JSON_ROOT ) {
		num_string.len = meta_len( num_string.data, p );

		end = num_string.data + num_string.len;
		json->num = strtod( num_string.data, &end );
		if( errno == ERANGE && ( json->num == HUGE_VAL || json->num == -HUGE_VAL ) ) {
			err_log ( "%s --- number is too big or to less", __func__ );
			return ERROR;
		}
		if( num_string.data == end ) {
			err_log ( "%s --- number str empty", __func__ );
			return ERROR;
		}
		return OK;
	}
	return ERROR;
}

// json_parse_token -----------
static status json_parse_token( json_t * parent, json_content_t * content )
{
	json_t * json = NULL;
	char * p;

	for( ; content->p < content->end; content->p ++ ) {
		p = content->p;
		if(
		*p == ' '  ||
		*p == '\r' ||
		*p == '\n' ||
		*p == '\t' ) continue;
		if( *p == 't' ) {
			if( OK != json_parse_true( content ) ) {
				return ERROR;
			}
			json = mem_list_push( parent->list );
			json->parent = parent;
			json->type = JSON_TRUE;
			return OK;

		} else if( *p == 'f' ) {
			if( OK != json_parse_false( content ) ) {
				return ERROR;
			}
			json = mem_list_push( parent->list );
			json->parent = parent;
			json->type = JSON_FALSE;
			return OK;

		} else if( *p == 'n' ) {
			if( OK != json_parse_null( content ) ) {
				return ERROR;
			}
			json = mem_list_push( parent->list );
			json->parent = parent;
			json->type = JSON_NULL;
			return OK;

		} else if( *p == '"' ) {
			json = mem_list_push( parent->list );
			json->parent = parent;
			if( OK != json_parse_string( json, content ) ) {
				return ERROR;
			}
			json->type = JSON_STR;
			return OK;

		} else if( *p == '{' ) {
			json = mem_list_push( parent->list );
			json->parent = parent;
			mem_list_create( &json->list, sizeof(json_t) );
			if( OK != json_parse_obj( json, content ) ) {
				return ERROR;
			}
			json->type = JSON_OBJ;
			return OK;

		} else if( *p == '[' ) {
			json = mem_list_push( parent->list );
			json->parent = parent;
			mem_list_create( &json->list, sizeof(json_t) );
			if( OK != json_parse_array( json, content ) ) {
				return ERROR;
			}
			json->type = JSON_ARR;
			return OK;

		} else {
			json = mem_list_push( parent->list );
			json->parent = parent;
			if( OK != json_parse_num( json, content ) ) {
				return ERROR;
			}
			json->type = JSON_NUM;
			return OK;
		}
	}
	return ERROR;
}

// json_decode_check ---------
static status json_decode_check( json_content_t * content )
{
	char * p;

	content->p++;
	for( ; content->p < content->end; content->p++ ) {
		p = content->p;
		if( *p == ' '  ||
			*p == '\r' ||
			*p == '\n' ||
			*p == '\t' ) {
			continue;
		} else {
			err_log("%s --- after parse token, illegal [%d]", __func__, *p );
			return ERROR;
		}
	}
	return OK;
}
// json_decode ------------
status json_decode( json_t ** json, char * p, char * end )
{
	json_content_t json_content;
	json_t * new;

	new = (json_t*)l_safe_malloc( sizeof(json_t) );
	if( !new ) {
		err_log("%s --- l_safe_malloc new", __func__ );
		return ERROR;
	}
	memset( new, 0, sizeof(json_t) );
	new->type = JSON_ROOT;

	json_content.p = p;
	json_content.end = end;

	mem_list_create( &new->list, sizeof(json_t) );
	if( OK != json_parse_token( new, &json_content ) ) {
		json_free( new );
		return ERROR;
	}
	if( OK != json_decode_check( &json_content ) ) {
		json_free( new );
		return ERROR;
	}
	*json = new;
	return OK;
}

// json_free_token ------
static status json_free_token( json_t * json )
{
	uint32 i;
	json_t * l_safe_free;

	if( json->list != NULL ) {
		for( i=1; i <= json->list->elem_num; i ++ ) {
			l_safe_free = mem_list_get( json->list, i );
			json_free_token( l_safe_free );
		}
		mem_list_free( json->list );
	}
	return OK;
}
// json_free -----------
status json_free( json_t * json )
{
	if( OK != json_free_token( json ) ) {
		return ERROR;
	}
	l_safe_free( json );
	return OK;
}
// json_stringify_token_len -------
static uint32 json_stringify_token_len( json_t * json )
{
	uint32 len = 0;
	uint32 i;
	json_t * temp;
	char str[100];

	if( json->type == JSON_TRUE ) {
		return 4;
	} else if( json->type == JSON_FALSE ) {
		return 5;
	} else if( json->type == JSON_NULL ) {
		return 4;
	} else if( json->type == JSON_STR ) {
		len += json->name.len;
		len += 2;
		return len;
	} else if( json->type == JSON_OBJ ) {
		len += 1; // {

		if( json->list != NULL ) {
			for( i=1; i <= json->list->elem_num; i ++ ) {
				len += 1; // "
				temp = mem_list_get( json->list, i );
				len += temp->name.len;
				len += 1;  // "
				len += 1; // :
				temp = mem_list_get( temp->list, 1 );
				len += json_stringify_token_len( temp );

				len += 1; // ,
			}
			if( json->list->elem_num > 0 ) {
				len --;
			}
		}

		len += 1; // }
		return len;
	} else if( json->type == JSON_ARR ) {
		len += 1; // [
		if( json->list != NULL ) {
			for( i=1; i<=json->list->elem_num;i ++ ) {
				temp = mem_list_get( json->list, i );
				len += json_stringify_token_len( temp );

				len += 1; // ,
			}
			if( json->list->elem_num > 0 ) {
				len--;
			}
		}
		len += 1; // ]
		return len;
	} else if ( json->type == JSON_NUM ) {
		memset( str, 0, sizeof(str) );
		sprintf( str, "%.2f", json->num );
		len += l_strlen( str );
		return len;
	}
	return 0;
}
// json_stringify_token -------
static char* json_stringify_token( json_t * json, char * p )
{
	uint32 i;
	json_t * temp;
	char str[100];

	if( json->type == JSON_TRUE ) {
		*p = 't';
		*(p+1) = 'r';
		*(p+2) = 'u';
		*(p+3) = 'e';
		p+=4;
		return p;
	} else if( json->type == JSON_FALSE ) {
		*p = 'f';
		*(p+1) = 'a';
		*(p+2) = 'l';
		*(p+3) = 's';
		*(p+4) = 'e';
		p+=5;
		return p;
	} else if( json->type == JSON_NULL ) {
		*p = 'n';
		*(p+1) = 'u';
		*(p+2) = 'l';
		*(p+3) = 'l';
		p+=4;
		return p;
	} else if( json->type == JSON_STR ) {
		*p = '"';
		p++;
		memcpy( p, json->name.data, json->name.len );
		p+= json->name.len;
		*p = '"';
		p++;
		return p;
	} else if( json->type == JSON_OBJ ) {
		*p = '{';
		p++;

		if( json->list != NULL ) {
			for( i=1; i <= json->list->elem_num; i ++ ) {
				*p = '"';
				p++;
				temp = mem_list_get( json->list, i );
				memcpy( p, temp->name.data, temp->name.len );
				p +=  temp->name.len;
				*p = '"';
				p++;
				*p = ':';
				p++;
				temp = mem_list_get( temp->list, 1 );
				p = json_stringify_token( temp ,p );
				*p = ',';
				p++;
			}
			if( json->list->elem_num > 0 ) {
				p --;
			}
		}
		*p = '}';
		p++;
		return p;
	} else if( json->type == JSON_ARR ) {
		*p = '[';
		p++;
		if( json->list != NULL ) {
			for( i=1; i<=json->list->elem_num;i ++ ) {
				temp = mem_list_get( json->list, i );
				p = json_stringify_token( temp, p );
				if( NULL == p ) return NULL;

				*p = ',';
				p++;
			}
			if( json->list->elem_num > 0 ) {
				p--;
			}
		}
		*p = ']';
		p++;
		return p;
	} else if ( json->type == JSON_NUM ) {
		memset( str, 0, sizeof(str) );
		sprintf( str, "%.2f", json->num );
		memcpy( p, str, strlen(str) );
		p += strlen(str);
		return p;
	}
	return NULL;
}

// json_encode ----------
status json_encode( json_t * json, meta_t ** string )
{
	uint32 len;
	json_t * root;
	meta_t * meta;

	root = mem_list_get( json->list, 1 );

	len = json_stringify_token_len( root );
	if( len == 0 ) {
		return ERROR;
	}
	if( OK != meta_alloc( &meta, len ) ) {
		return ERROR;
	}
	meta->last = json_stringify_token( root, meta->pos );
	if( !meta->last ) {
		meta_free( meta );
		return ERROR;
	} else if( meta->last != meta->end ) {
		meta_free( meta );
		return ERROR;
	}
	*string = meta;
	return OK;
}

// json_create -----------
status json_create( json_t ** json )
{
	json_t * new = NULL;

	new = (json_t*)l_safe_malloc( sizeof(json_t) );
	if( !new ) {
		return ERROR;
	}
	memset( new, 0, sizeof(json_t) );
	if( OK != mem_list_create( &new->list, sizeof(json_t) ) ) {
		l_safe_free( new );
		return ERROR;
	}
	*json = new;
	return OK;
}
