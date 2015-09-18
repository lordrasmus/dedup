
#include <list>


using namespace std;


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
		
		
	
		void print_path( void );
		
		void update_hash( void );
};


class dir_entry{
	
	public:
		
		
		dir_entry * parrent;
		
		char* name;
		
		list<dir_entry *> sub_dirs;   
		list<file_entry *> files;   
		
		
		void print_path( void );

		void print_dirs( int level );
		
		void update_hashes( void );
		
		uint32_t add_path( char* buffer,uint32_t length );

};


