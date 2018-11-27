#include "lk.h"

static queue_t usable;
static queue_t in_use;
static http_entitybody_t * pool = NULL;

// http_entitybody_recv ------------------------------------------
static ssize_t http_entitybody_recv( http_entitybody_t * bd )
{
	ssize_t rc;

	if ( bd->body_type == HTTP_ENTITYBODY_CHUNK ) {
		if( bd->body_last->last - bd->chunk_pos > 0 ) {
			debug_log("%s --- chunk already have", __func__ );
			return OK;
		}
	}
	rc = bd->c->recv( bd->c, bd->body_last->last,
		meta_len( bd->body_last->last, bd->body_last->end ) );
	if( rc == ERROR ) {
		err_log(  "%s --- recv", __func__ );
		return ERROR;
	} else if ( rc == AGAIN ) {
		debug_log("%s --- again", __func__ );
		return AGAIN;
	} else {
		if( bd->body_type == HTTP_ENTITYBODY_CONTENT ) {
			bd->content_need -= rc;
		} else if( bd->body_type == HTTP_ENTITYBODY_CHUNK ) {
			bd->chunk_all_length += (uint32)rc;
		}
		debug_log("%s --- recv [%d]", __func__, rc );
		bd->body_last->last += rc;
		return rc;
	}
}
// http_entitybody_decode_chunk -----------------------------------------------
static status http_entitybody_decode_chunk( http_entitybody_t * bd )
{
	status rc = AGAIN;
	char * p;
	uint32 recvd;
	int32  length;

	enum {
		entity_body_state_start = 0,
		entity_body_hex,
		entity_body_hex_rn,
		entity_body_part,
		entity_body_part_r,
		entity_body_part_rn,
		entity_body_last_r,
		entity_body_last_rn
	} state;

	state = bd->state;
	for( p = bd->chunk_pos; p < bd->body_last->last; p ++ ) {
		if( state == entity_body_state_start ) {
			if(
			( *p >= '0' && *p <= '9' ) ||
			( *p >= 'a' && *p <= 'f' ) ||
			( *p >= 'A' && *p <= 'F' )
			) {
				bd->hex_str[bd->hex_length] = *p;
				bd->hex_length ++;
				state = entity_body_hex;
				continue;
			} else {
				err_log(  "%s --- start illegal, [%d]", __func__, *p );
				return ERROR;
			}
		}
		if ( state == entity_body_hex ) {
			if(
			( *p >= '0' && *p <= '9' ) ||
			( *p >= 'a' && *p <= 'f' ) ||
			( *p >= 'A' && *p <= 'F' ) ||
			( *p == 'x' ) ||
			( *p == 'X' ) ||
			(*p == '\r' )
			) {
				if( *p == '\r' ) {
					state = entity_body_hex_rn;
				} else {
					bd->hex_str[bd->hex_length] = *p;
					bd->hex_length++;
				}
				continue;
			} else {
				err_log(  "%s --- num illegal, [%d]", __func__, *p );
				return ERROR;
			}
		}
		if ( state == entity_body_hex_rn ) {
			if( *p != '\n' ) {
				err_log(  "%s --- num_rn illegal, [%d]", __func__, *p );
				return ERROR;
			}
			if( OK != l_hex2dec( bd->hex_str, bd->hex_length,
			&length ) ) {
				err_log("%s --- hex2dec [%.*s]", __func__,
				bd->hex_length, bd->hex_str );
				return ERROR;
			}
			bd->hex_length = 0;
			bd->chunk_length = (uint32)length;
			if( bd->chunk_length == 0 ) {
				state = entity_body_last_r;
				continue;
			} else {
				bd->chunk_recvd = 0;
				bd->chunk_need = bd->chunk_length;
				state = entity_body_part;
				continue;
			}
		}
		if ( state == entity_body_part ) {
			recvd = meta_len( p, bd->body_last->last );
			if( recvd >= bd->chunk_need ) {
				p += bd->chunk_need;
				state = entity_body_part_r;
			} else {
				bd->chunk_recvd += recvd;
				bd->chunk_need -= recvd;

				p = bd->body_last->last;
				state = entity_body_part;
				break;
			}
		}
		if( state == entity_body_part_r ) {
			if( *p != '\r' ) {
				err_log(  "%s --- part_r illegal, [%d]", __func__, *p );
				return ERROR;
			}
			state = entity_body_part_rn;
			continue;
		}
		if( state == entity_body_part_rn ) {
			if( *p != '\n' ) {
				err_log(  "%s --- part_rn illegal, [%d]", __func__, *p );
				return ERROR;
			}
			state = entity_body_state_start;
			continue;
		}
		if( state == entity_body_last_r ) {
			if( *p != '\r' ) {
				err_log(  "%s --- last r illegal, [%d]", __func__, *p );
				return ERROR;
			}
			state = entity_body_last_rn;
			continue;
		}
		if ( state == entity_body_last_rn ) {
			if( *p != '\n' ) {
				err_log(  "%s --- last rn illegal, [%d]", __func__, *p );
				return ERROR;
			}
			state = entity_body_state_start;
			p++;
			rc = DONE;
			break;
		}
	}
	bd->chunk_pos = p;
	bd->state = state;
	return rc;
}
// http_entitybody_decode_content -----------
static status http_entitybody_decode_content( http_entitybody_t * bd )
{
	if( bd->content_need <= 0 ) {
		if( bd->content_need < 0 ) {
			bd->content_end = bd->body_last->last + bd->content_need;
		} else {
			bd->content_end = bd->body_last->last;
		}
		return DONE;
	}
	return AGAIN;
}
// http_entitybody_process -----------
static status http_entitybody_process( http_entitybody_t * bd )
{
	ssize_t rc;
	meta_t * new = NULL;

	while(1) {
		if( ( bd->cache && !bd->body_last ) ||
		bd->body_last->last == bd->body_last->end ) {
			if( !bd->cache ) {
				bd->body_last->last = bd->body_last->pos;
			} else {
				if( OK != meta_alloc( &new, ENTITY_BODY_BUFFER_SIZE ) ) {
					err_log( "%s --- alloc new meta", __func__ );
					return ERROR;
				}
				if( !bd->body_last ) {
					bd->body_last = new;
					bd->body_head = bd->body_last;
				} else {
					bd->body_last->next = new;
					bd->body_last = bd->body_last->next;
				}
			}
			if( bd->body_type == HTTP_ENTITYBODY_CHUNK ) {
				bd->chunk_pos = bd->body_last->pos;
			}
		}
		rc = http_entitybody_recv( bd );
		if( rc == ERROR ) {
			return ERROR;
		} else if ( rc == AGAIN ) {
			return AGAIN;
		}
		if( bd->body_type == HTTP_ENTITYBODY_CONTENT ) {
			rc = http_entitybody_decode_content( bd );
		} else if ( bd->body_type == HTTP_ENTITYBODY_CHUNK ) {
			rc = http_entitybody_decode_chunk( bd );
		}
		if( rc == ERROR )  {
			return ERROR;
		} else if ( rc == DONE ) {
			debug_log("%s --- success", __func__ );
			if( bd->body_type == HTTP_ENTITYBODY_CONTENT ) {
				bd->all_length = bd->content_length;
			} else if ( bd->body_type == HTTP_ENTITYBODY_CHUNK ) {
				bd->all_length = bd->chunk_all_length;
			}
			return DONE;
		}
	}
}
// http_entitybody_start ------
static status http_entitybody_start( http_entitybody_t * bd )
{
	uint32 length, alloc_length;

	length = meta_len( bd->c->meta->pos, bd->c->meta->last );
	alloc_length = ( length < ENTITY_BODY_BUFFER_SIZE ) ?
						ENTITY_BODY_BUFFER_SIZE : length;
	if( OK != meta_alloc( &bd->body_last, alloc_length ) ) {
		err_log("%s --- body_last meta alloc", __func__ );
		return ERROR;
	}
	bd->body_head = bd->body_last;
	if( bd->body_type == HTTP_ENTITYBODY_CONTENT ) {
		bd->content_need = (ssize_t)bd->content_length;
		if( length ) {
			if( bd->cache ) {
				memcpy( bd->body_last->last, bd->c->meta->pos, length );
				bd->body_last->last += length;
			}
			bd->content_need -= (ssize_t)length;
		}
		if( bd->content_need <= 0 ) {
			if( bd->content_need < 0 ) {
				bd->content_end = bd->body_last->last + bd->content_need;
			} else {
				bd->content_end = bd->body_last->last;
			}
			bd->all_length = bd->content_length;
			return DONE;
		}
	} else if( bd->body_type == HTTP_ENTITYBODY_CHUNK ) {
		if( length ) {
			memcpy( bd->body_last->last, bd->c->meta->pos, length );
			bd->body_last->last += length;
		}
		bd->chunk_pos = bd->body_last->pos;
	}
	bd->handler = http_entitybody_process;
	return bd->handler ( bd );
}
//http_entitybody_alloc -----------
static status http_entitybody_alloc( http_entitybody_t ** body )
{
	http_entitybody_t * new;
	queue_t * q;

	if( queue_empty( &usable ) == 1 ) {
		err_log(  "%s --- queue usabel empty", __func__ );
		return ERROR;
	}
	q = queue_head( &usable );
	queue_remove( q );
	queue_insert( &in_use, q );
	new = l_get_struct( q, http_entitybody_t, queue );
	*body = new;
	return OK;
}
// http_entitybody_create ------------------------------
status http_entitybody_create( connection_t * c, http_entitybody_t ** body )
{
	http_entitybody_t * new;

	if( OK != http_entitybody_alloc( &new ) ) {
		err_log(  "%s --- alloc new", __func__ );
		return ERROR;
	}
	new->c = c;
	new->handler = http_entitybody_start;
	new->state = 0;
	*body = new;
	return OK;
}
// http_entitybody_free -------------------
status http_entitybody_free( http_entitybody_t * bd )
{
	meta_t *t, *n;
	queue_remove( &bd->queue );
	queue_insert_tail( &usable, &bd->queue );

	bd->c = NULL;
	bd->handler = NULL;
	bd->state = 0;

	bd->body_type = HTTP_ENTITYBODY_NULL;
	bd->cache = 0;

	bd->content_length = 0;
	bd->content_need = 0;
	bd->content_end = NULL;

	bd->chunk_pos = NULL;
	bd->chunk_length = 0;
	bd->chunk_recvd = 0;
	bd->chunk_need = 0;
	bd->chunk_all_length = 0;

	// clear hex_str ?
	bd->hex_length = 0;

	if( bd->body_head ) {
		t = bd->body_head;
		while(t) {
			n = t->next;
			meta_free( t );
			t = n;
		}
	}
	bd->body_head = NULL;
	bd->body_last = NULL;
	return OK;
}
// http_entitybody_init_module ------------
status http_entitybody_init_module( void )
{
	uint32 i;

	queue_init( &usable );
	queue_init( &in_use );
	pool = (http_entitybody_t*)l_safe_malloc( sizeof(http_entitybody_t)*MAXCON );
	if( !pool ) {
		err_log(  "%s --- l_safe_malloc pool", __func__ );
		return ERROR;
	}
	memset( pool, 0, sizeof(http_entitybody_t)*MAXCON );
	for( i = 0; i < MAXCON; i ++ ) {
		queue_insert_tail( &usable, &pool[i].queue );
	}
	return OK;
}
// http_entitybody_end_module -----------
status http_entitybody_end_module( void )
{
	if( pool ) {
		l_safe_free( pool );
		pool = NULL;
	}
	return OK;
}
