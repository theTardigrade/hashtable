#ifndef __TABLE_H__
#define __TABLE_H__

	#include <stdbool.h>
	#include <stdint.h>

	#define HT_n_TABLE_INITAL_CAPACITY              128
	#define HT_n_TABLE_CAPACITY_MULTIPLIER          2
	#define HT_n_TABLE_MAX_CAPACITY                 ( 1 << 30 )
	#define HT_n_TABLE_ENTRY_KEY_MAX_LENGTH         ( 1 << 20 )
	#define HT_n_TABLE_ENTRY_KEY_MAX_GARBAGE_COUNT  128

	typedef struct {
		char*     pc_content;
		int       n_length;
		uint64_t  n_hash;
	} HT_s_tableEntryKey_t;

	typedef struct {
		HT_s_tableEntryKey_t*  ps_key;
		void*                  pv_value;
	} HT_s_tableEntry_t;

	typedef struct {
		HT_s_tableEntry_t*     ps_entries;
		int                    n_count;
		int                    n_capacity;
		HT_s_tableEntryKey_t*  aps_entryKeyGarbage[HT_n_TABLE_ENTRY_KEY_MAX_GARBAGE_COUNT];
		int                    n_entryKeyGarbageCount;
	} HT_s_table_t;

	HT_s_table_t*  HT_f_new( void );
	HT_s_table_t*  HT_f_newCopy( const HT_s_table_t* );
	void           HT_f_free( HT_s_table_t* );
	bool           HT_f_set( HT_s_table_t*, const char*, int, void* );
	void*          HT_f_get( HT_s_table_t*, const char*, int );
	bool           HT_f_exists( HT_s_table_t*, const char*, int );
	bool           HT_f_unset( HT_s_table_t*, const char*, int );
	void           HT_f_clear( HT_s_table_t* );
	bool           HT_f_grow( HT_s_table_t*, int );
	void           HT_f_copy( HT_s_table_t*, const HT_s_table_t* );

#endif