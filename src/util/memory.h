#ifndef __MEMORY_H__
#define __MEMORY_H__

	#include "error.h"

	#include <stdlib.h>

	#define m_allocMemory( px, x_t, n ) ({ \
		x_t* px_2 = ( x_t* )( \
			( px == NULL ) \
				? malloc( sizeof( x_t ) * n ) \
				: realloc( px, sizeof( x_t ) * n ) \
		); \
		if ( px_2 == NULL ) \
			m_logError( "cannot allocate memory\n" ); \
		px_2; \
	})

	#define m_freeMemory( px ) { if ( px != NULL ) { free( px ); px = NULL; } }

#endif