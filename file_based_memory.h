

#ifndef _FILE_BASED_MEMORY_
#define _FILE_BASED_MEMORY_

#define MAX_PATH_LENGTH 1024

class FileMemory{
	
	
	public:
		
		int      fd;
		uint8_t*    mem_cur;
		uint8_t*    mem_start;
		uint64_t size;
		
		
		char path[MAX_PATH_LENGTH];
		
		
		void     open_mem( const char* path );
		void     close_mem( void );
		
		void     sync( void );
		
		bool     has_more_data( void );
		
		void     add_uint16_t( uint16_t value );
		uint16_t get_uint16_t( void );
		
		void     add_uint32_t( uint32_t value );
		uint32_t get_uint32_t( void );
		
		void     add_uint64_t( uint64_t value );
		uint64_t get_uint64_t( void );
				
		char*    add_string( const char* text );
		char*    get_string( void );
	
};



#endif
