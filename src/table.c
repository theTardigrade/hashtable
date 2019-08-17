#include "table.h"
#include "util/memory.h"
#include "util/error.h"

#include <stdbool.h>
#include <string.h>
#include <math.h>

// uses the FNV-1a algorithm
static uint64_t __f_generateTableEntryKeyHash__( const char* pc_content, int n_length )
{
	uint64_t n_hash = 0xcbf29ce484222325;

	for ( int n = 0; n < n_length; ++n )
	{
		n_hash ^= pc_content[n];
		n_hash *= 0x100000001b3;
	}

	return n_hash;
}

static void __f_validateTableEntryKeyContent__( const char* pc_content, int* pn_length )
{
	if ( *pn_length < 0 )
		*pn_length = strnlen( pc_content, HT_n_TABLE_ENTRY_KEY_MAX_LENGTH + 1 );

	if ( *pn_length > HT_n_TABLE_ENTRY_KEY_MAX_LENGTH )
		m_logError( "table entry key length cannot be greater than %d\n", HT_n_TABLE_ENTRY_KEY_MAX_LENGTH );
}

static HT_s_tableEntryKey_t* __f_newTableEntryKey__( const char* pc_content, int n_length )
{
	__f_validateTableEntryKeyContent__( pc_content, &n_length );

	HT_s_tableEntryKey_t* ps_tableEntryKey = m_allocMemory( NULL, HT_s_tableEntryKey_t, 1 );

	char* pc_tableEntryKeyContent = ( ps_tableEntryKey->pc_content = m_allocMemory( NULL, char, n_length + 1 ) );

	for ( int n = n_length - 1; n >= 0; --n )
		*( pc_tableEntryKeyContent + n ) = *( pc_content + n );

	*( pc_tableEntryKeyContent + n_length ) = '\0';

	ps_tableEntryKey->n_hash = __f_generateTableEntryKeyHash__( pc_content, n_length );
	ps_tableEntryKey->n_length = n_length;

	return ps_tableEntryKey;
}

static HT_s_tableEntryKey_t* __f_newCopyTableEntryKey__( const HT_s_tableEntryKey_t* ps_oldKey )
{
	HT_s_tableEntryKey_t* ps_newKey = m_allocMemory( NULL, HT_s_tableEntryKey_t, 1 );

	int n_length = ps_oldKey->n_length;
	const char* pc_oldKeyContent = ps_oldKey->pc_content;
	char* pc_newKeyContent = ( ps_newKey->pc_content = m_allocMemory( NULL, char, n_length + 1 ) );

	for ( int n = n_length - 1; n >= 0; --n )
		*( pc_newKeyContent + n ) = *( pc_oldKeyContent + n );

	*( pc_newKeyContent + n_length ) = '\0';

	ps_newKey->n_hash = ps_oldKey->n_hash;
	ps_newKey->n_length = n_length;

	return ps_newKey;
}

static void __f_freeTableEntryKey__( HT_s_tableEntryKey_t* ps_tableEntryKey )
{
	if ( ps_tableEntryKey == NULL )
		return;

	m_freeMemory( ps_tableEntryKey->pc_content );
	m_freeMemory( ps_tableEntryKey );
}

static void __f_freeTableEntryKeyGarbage__( HT_s_table_t* ps_table )
{
	HT_s_tableEntryKey_t** pps_tableEntryKeyGarbage = ps_table->aps_entryKeyGarbage;

	for ( int n = ps_table->n_entryKeyGarbageCount - 1; n >= 0; --n )
	{
		HT_s_tableEntryKey_t* ps_tableEntryKey = *( pps_tableEntryKeyGarbage + n );
		__f_freeTableEntryKey__( ps_tableEntryKey );
	}

	ps_table->n_entryKeyGarbageCount = 0;
}

static void __f_addTableEntryKeyToGarbage__( HT_s_table_t* ps_table, HT_s_tableEntryKey_t* ps_tableEntryKey )
{
	if ( ps_table->n_entryKeyGarbageCount == HT_n_TABLE_ENTRY_KEY_MAX_GARBAGE_COUNT )
		__f_freeTableEntryKeyGarbage__( ps_table );
	
	ps_table->aps_entryKeyGarbage[ps_table->n_entryKeyGarbageCount++] = ps_tableEntryKey;
}

// do not call when n_capacity == 0, because of potential divide-by-zero error
static HT_s_tableEntry_t* __f_findTableEntry__( HT_s_tableEntry_t* ps_entries, int n_capacity, HT_s_tableEntryKey_t* ps_key )
{
	HT_s_tableEntry_t* ps_tombstoneEntry = NULL;

	for ( uint64_t n_index = ps_key->n_hash % n_capacity; 1; n_index = ( n_index + 1 ) % n_capacity )
	{
		HT_s_tableEntry_t* ps_entry = ( ps_entries + n_index );

		if ( ps_entry->ps_key == NULL )
		{
			if ( ps_entry->pv_value == NULL )
				return ( ( ps_tombstoneEntry == NULL ) ? ps_entry : ps_tombstoneEntry );
			
			if ( ps_tombstoneEntry == NULL )
				ps_tombstoneEntry = ps_entry;
		}
		else if ( strncmp( ps_entry->ps_key->pc_content, ps_key->pc_content, ps_key->n_length ) == 0 )
			return ps_entry;
	}
}

static bool __f_hasTableExceededFillRatio__( int n_count, int n_capacity )
{
	double r_count = ( double )( n_count );
	double r_relativeCapacity = ( double )( n_capacity ) * HT_r_TABLE_MAX_FILL_RATIO;

	return ( r_count > r_relativeCapacity );
}

static int __f_calculateNewTableCapacity__( int n_oldCapacity )
{
	if ( n_oldCapacity == 0 )
		return HT_n_TABLE_INITAL_CAPACITY;

	return n_oldCapacity * HT_n_TABLE_CAPACITY_MULTIPLIER;
}

static void __f_increaseTableCapacity__( HT_s_table_t* ps_table, int n_newCapacity )
{
	HT_s_tableEntry_t* ps_newEntries = m_allocMemory( NULL, HT_s_tableEntry_t, n_newCapacity );
	HT_s_tableEntry_t* ps_oldEntries = ps_table->ps_entries;

	for ( int n = n_newCapacity - 1; n >= 0; --n )
	{
		HT_s_tableEntry_t* ps_entry = ( ps_newEntries + n );

		ps_entry->ps_key = NULL;
		ps_entry->pv_value = NULL;
	}

	for ( int n = ps_table->n_capacity - 1; n >= 0; --n )
	{
		HT_s_tableEntry_t* ps_entry = ( ps_oldEntries + n );
		if ( ps_entry->ps_key == NULL )
			continue;

		HT_s_tableEntry_t* ps_newEntry = __f_findTableEntry__( ps_newEntries, n_newCapacity, ps_entry->ps_key );
		ps_newEntry->ps_key = ps_entry->ps_key;
		ps_newEntry->pv_value = ps_entry->pv_value;
	}

	m_freeMemory( ps_oldEntries );

	ps_table->ps_entries = ps_newEntries;
	ps_table->n_capacity = n_newCapacity;
}

static void __f_validateNull__( const void* pv_item, const char* pc_itemName )
{
	if ( pv_item == NULL )
		m_logError( "%s cannot be null\n", pc_itemName );
}

static bool __f_setWithKey__( HT_s_table_t* ps_table, HT_s_tableEntryKey_t* ps_key, void* pv_value )
{
	int n_oldCapacity = ps_table->n_capacity;

	if ( __f_hasTableExceededFillRatio__( ps_table->n_count + 1, n_oldCapacity ) )
	{
		int n_newCapacity = __f_calculateNewTableCapacity__( n_oldCapacity );
		__f_increaseTableCapacity__( ps_table, n_newCapacity );
	}

	HT_s_tableEntry_t* ps_entry = __f_findTableEntry__( ps_table->ps_entries, ps_table->n_capacity, ps_key );

	bool b_isNewKey = ( ps_entry->ps_key == NULL );
	if ( b_isNewKey )
	{
		++ps_table->n_count;

		ps_entry->ps_key = ps_key;
	}

	ps_entry->pv_value = pv_value;

	return b_isNewKey;
}

HT_s_table_t* HT_f_new()
{
	HT_s_table_t * ps_table = m_allocMemory( NULL, HT_s_table_t, 1 );

	ps_table->n_count = 0;
	ps_table->n_capacity = 0;
	ps_table->ps_entries = NULL;
	ps_table->n_entryKeyGarbageCount = 0;

	return ps_table;
}

HT_s_table_t* HT_f_newCopy( HT_s_table_t* ps_sourceTable )
{
	HT_s_table_t* ps_table = HT_f_new();

	HT_f_grow( ps_table, ps_sourceTable->n_capacity );
	HT_f_copy( ps_table, ps_sourceTable );

	return ps_table;
}

void HT_f_free( HT_s_table_t* ps_table )
{
	if ( ps_table == NULL )
		return;

	for ( int n = ps_table->n_capacity - 1; n >= 0; --n )
	{
		HT_s_tableEntry_t* ps_tableEntry = ( ps_table->ps_entries + n );

		if ( ps_tableEntry != NULL )
			__f_freeTableEntryKey__( ps_tableEntry->ps_key );
	}

	__f_freeTableEntryKeyGarbage__( ps_table );

	m_freeMemory( ps_table->ps_entries );
	m_freeMemory( ps_table );
}

bool HT_f_set( HT_s_table_t* ps_table, const char* pc_keyContent, int n_keyLength, void* pv_value )
{
	__f_validateNull__( ps_table, "table" );
	__f_validateNull__( pc_keyContent, "key content" );
	__f_validateNull__( pv_value, "value" );

	HT_s_tableEntryKey_t* ps_key = __f_newTableEntryKey__( pc_keyContent, n_keyLength );

	bool b_isNewKey = __f_setWithKey__( ps_table, ps_key, pv_value );
	if ( !b_isNewKey )
		__f_addTableEntryKeyToGarbage__( ps_table, ps_key );

	return b_isNewKey;
}

void* HT_f_get( HT_s_table_t* ps_table, const char* pc_keyContent, int n_keyLength )
{
	__f_validateNull__( ps_table, "table" );
	__f_validateNull__( pc_keyContent, "key content" );

	int n_capacity = ps_table->n_capacity;
	if ( n_capacity == 0 )
		return NULL;

	HT_s_tableEntryKey_t* ps_key = __f_newTableEntryKey__( pc_keyContent, n_keyLength );
	HT_s_tableEntry_t* ps_entry = __f_findTableEntry__( ps_table->ps_entries, n_capacity, ps_key );

	__f_addTableEntryKeyToGarbage__( ps_table, ps_key );

	if ( ps_entry->ps_key == NULL )
		return NULL;

	return ps_entry->pv_value;
}

bool HT_f_exists( HT_s_table_t* ps_table, const char* pc_keyContent, int n_keyLength )
{
	__f_validateNull__( ps_table, "table" );
	__f_validateNull__( pc_keyContent, "key content" );

	int n_capacity = ps_table->n_capacity;
	if ( n_capacity == 0 )
		return false;

	HT_s_tableEntryKey_t* ps_key = __f_newTableEntryKey__( pc_keyContent, n_keyLength );
	HT_s_tableEntry_t* ps_entry = __f_findTableEntry__( ps_table->ps_entries, n_capacity, ps_key );

	__f_addTableEntryKeyToGarbage__( ps_table, ps_key );

	return ( ps_entry->ps_key != NULL );
}

bool HT_f_unset( HT_s_table_t* ps_table, const char* pc_keyContent, int n_keyLength )
{
	__f_validateNull__( ps_table, "table" );
	__f_validateNull__( pc_keyContent, "key content" );

	int n_capacity = ps_table->n_capacity;
	if ( n_capacity == 0 )
		return false;

	HT_s_tableEntryKey_t* ps_key = __f_newTableEntryKey__( pc_keyContent, n_keyLength );
	HT_s_tableEntry_t* ps_entry = __f_findTableEntry__( ps_table->ps_entries, n_capacity, ps_key );

	__f_addTableEntryKeyToGarbage__( ps_table, ps_key );

	if ( ps_entry->ps_key == NULL )
		return false;

	ps_entry->ps_key = NULL;
	ps_entry->pv_value = ( void* )1; // tombstone

	return true;
}

void HT_f_clear( HT_s_table_t* ps_table )
{
	__f_validateNull__( ps_table, "table" );

	HT_s_tableEntry_t* ps_entries = ps_table->ps_entries;

	for ( int n = ps_table->n_capacity - 1; n >= 0; --n )
		( ps_entries + n )->ps_key = NULL;

	ps_table->n_count = 0;
}

bool HT_f_grow( HT_s_table_t* ps_table, int n_newCapacity )
{
	__f_validateNull__( ps_table, "table" );

	int n_oldCapacity = ps_table->n_capacity;

	if ( n_newCapacity <= n_oldCapacity )
		return false;

	int n_newCalculatedCapacity = __f_calculateNewTableCapacity__( n_oldCapacity );

	if ( n_newCalculatedCapacity > n_newCapacity )
		n_newCapacity = n_newCalculatedCapacity;

	__f_increaseTableCapacity__( ps_table, n_newCapacity );

	return true;
}

void HT_f_copy( HT_s_table_t* ps_destTable, HT_s_table_t* ps_sourceTable )
{
	for ( int n = ps_sourceTable->n_capacity - 1; n >= 0; --n )
	{
		HT_s_tableEntry_t* ps_entry = ( ps_sourceTable->ps_entries + n );

		if ( ps_entry->ps_key != NULL ) {
			HT_s_tableEntryKey_t* ps_newKey = __f_newCopyTableEntryKey__( ps_entry->ps_key );
			__f_setWithKey__( ps_destTable, ps_newKey, ps_entry->pv_value ); // b_isNewKey should always be true
		}
	}
}