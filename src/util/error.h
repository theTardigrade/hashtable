#ifndef __ERROR_H__
#define __ERROR_H__

	#include <stdio.h>

	#define m_logError( ... ) { \
		fprintf( stderr, __VA_ARGS__ ); \
		exit( 1 ); \
	}

#endif