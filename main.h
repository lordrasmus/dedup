
#include <list>
using namespace std;


#include "utils.h"
#include "names.h"

extern Names *names;

extern FileMemory* dir_memory;
		

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

uint64_t cur_dir_id =0;

class dir_entry{
	
	private:
		uint64_t id;
		char* name;
		dir_entry * parrent;
		
		
	public:
	
		list<dir_entry *> sub_dirs;   
		list<file_entry *> files;   
		
		
		dir_entry( dir_entry *parrent_p, const char* name_p ){
			
			this->id=cur_dir_id++;
			this->parrent = parrent_p;
			
			uint64_t id_t;
			uint64_t parrent_id = 0;
			if ( parrent_p != 0 )
				parrent_id = parrent_p->id;
			
			char* name_t = names->get_name( name_p, &id_t );

			this->name = name_t;
			
			printf("new dir : %s ( %" PRIu64 " / %" PRIu64 " / %" PRIu64 " )\n", name_t, this->id, parrent_id , id_t);
			print_path();printf("\n");
			
			dir_memory->add_uint16_t( this->id );
			dir_memory->add_uint16_t( id_t );
			
			dir_memory->add_uint64_t( parrent_id );
			
			if ( parrent_p != 0 ){
				//exit ( 1 );
				sleep(1);
			}
		}
		
		
		
		void print_path( void );

		void print_dirs( int level );
		
		void update_hashes( void );
		
		// Utility um den original Pfad zusammen zu bauen
		uint32_t add_path( char* buffer,uint32_t length );	

};


