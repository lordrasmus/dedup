
#include <list>
using namespace std;


#include "utils.h"
#include "names.h"

extern Names *names;


class dir_entry;
class file_entry;


class file_hash{
	
	public:
	
		uint32_t count;
		list<file_entry *> files;   
		
};

class file_entry{
	
	public:
	
		dir_entry * parrent;
		
		char* name;
		
		size_t length;
		int fd = 0;
		
		
		int  open_file( void );
		void close_file( void );
	
		void print_path( void );
		
		void update_hash( void );
};


class dir_entry{
	
	private:
		char* name;
		dir_entry * parrent;
		
	public:
		
		dir_entry( dir_entry *parrent_p, const char* name_p ){
			this->parrent = parrent;
			
			uint64_t id_t;
			
			char* name_t = names->get_name( name_p, &id_t );

			this->name = name_t;
			
			printf("new dir : %s ( %" PRIu64 " )\n", name_t, id_t );
		}
		
		
		
		
		
		list<dir_entry *> sub_dirs;   
		list<file_entry *> files;   
		
		
		void print_path( void );

		void print_dirs( int level );
		
		void update_hashes( void );
		
		uint32_t add_path( char* buffer,uint32_t length );

};


