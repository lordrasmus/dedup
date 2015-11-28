
#ifndef _NAMES_H_
#define _NAMES_H_

#include <inttypes.h>

#include "utils.h"
#include "file_based_memory.h"

#include "ui.h"


class Names{
	
	private:

		uint64_t cur_id = 0;
		map< char* ,uint64_t, cmp_char_p > names_map;

		FileMemory* name_memory;
		
		UI *main;
		string data_path;
		
	public:
	
		
		Names( UI *main_p, string data_path_p ){
			this->main = main_p;
			this->data_path = data_path_p;
		}
	
		char* get_name( char* name ){
			return get_name( name, 0 );
		}
		
		char* get_name( const char* name , uint64_t* id){
			
			char* name_t;
			
			map< char* ,uint64_t >::iterator names_map_it;
			
			names_map_it = names_map.find( (char*)name );
			
			if (names_map_it == names_map.end()) {
				
				name_t = name_memory->add_string( name );
				
				//printf("%" PRIu64 " %s\n",cur_id, name_t);
				uint64_t id_tmp = cur_id++;
				if ( id != 0 )
					*id = id_tmp;
				
				names_map.insert ( pair<char* ,uint64_t>( name_t , id_tmp ) );
			}else{
				name_t = names_map_it->first;
				if ( id != 0 )
					*id = names_map_it->second;
			}
			
			return name_t;
			
		}
		
		
		void load( void ){
			main->add_progress_win_line("loading names  ...");
	
			name_memory= new FileMemory( main );
			char buffer[1000];
			sprintf(buffer,"%s/names",data_path.c_str());
			if ( -1 == name_memory->open_mem( buffer ) ){
				main->add_progress_win_line("loading names  Error");
				return;
			}
			
			main->add_progress_win_line("loading names  OK");
			
			while( name_memory->has_more_data() ){
				
				char *text = name_memory->get_string();
				names_map.insert ( pair<char* ,uint64_t>( text ,cur_id++) );
				
			}
			
			char tmp[100];
			sprintf(tmp,"loading names finished ( %" PRIu64 " ) ",cur_id);
			main->add_progress_win_line(tmp);
	
		}
		
		void sync( void ){
			
			name_memory->sync( );
		}
		
		void close( void ){
			
			name_memory->close_mem( );
		}
		
};


#endif
