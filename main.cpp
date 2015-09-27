#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#include <string.h>
#include <inttypes.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>


#include <linux/btrfs.h>

#include <iostream>

#include <string>
#include <map>

#include <openssl/md5.h>
#include <openssl/sha.h>



#include "main.h"
#include "names.h"
#include "file_based_memory.h"






uint64_t files = 0;
uint64_t files_hashed = 0;
double files_hashed_proz = 0;

Names *names;

map< unsigned char* ,file_hash*, cmp_uchar_p > hashes_map;


//FileMemory* dir_memory;

void file_entry::print_path( void ){
	
	this->parrent->print_path();
	printf("/%s\n",this->name);
	
}


void add_hash( unsigned char* hash, file_entry* file_p ){
	
	map< unsigned char* ,file_hash* >::iterator hash_map_it;
	
	hash_map_it = hashes_map.find( hash );
	if (hash_map_it == hashes_map.end()) {
		
		file_hash* t = new file_hash();
		
		t->count = 1;
		
		t->files.push_back( file_p );
		
		hashes_map.insert ( pair<unsigned char* ,file_hash*>( hash , t) );
		
	}else{
		
		file_hash* t = hash_map_it->second;
		
		t->count++;
		
		t->files.push_back( file_p );
		
		
		free( hash );
		
		if ( t->count > 200 ){
			
			//printf(" hash found : %d\n", t->count );
			
			
		}
		
	}
}

#if 0
struct btrfs_ioctl_same_extent_info {
	int64_t fd;			/* in - destination file */
	uint64_t logical_offset;	/* in - start of extent in destination */
	uint64_t bytes_deduped;		/* out - total # of bytes we
					 * were able to dedupe from
					 * this file */
	/* status of this dedupe operation:
	 * 0 if dedup succeeds
	 * < 0 for error
	 * == BTRFS_SAME_DATA_DIFFERS if data differs
	 */
	int32_t status;			/* out - see above description */
	uint32_t reserved;
};

struct btrfs_ioctl_same_args {
	uint64_t logical_offset;	/* in - start of extent in source */
	uint64_t length;		/* in - length of extent */
	uint16_t dest_count;		/* in - total elements in info array */
	uint16_t reserved1;		/* out - number of files that got deduped */
	uint32_t reserved2;
	struct btrfs_ioctl_same_extent_info info[0];
};

#endif

void print_size( const char* text, size_t size){
	
	if ( size > ( 1024 * 1024 ) ){
		printf("%s%" PRIu64 " MB\n",text,size / (1024 * 1024 ));
		return;
	}
		
}

#define BTRFS_IOCTL_MAGIC 0x94
#define BTRFS_IOC_FILE_EXTENT_SAME _IOWR(BTRFS_IOCTL_MAGIC, 54, struct btrfs_ioctl_same_args)
					 
void merge_files( void ){
	
	uint32_t total_matches = 0;
	uint32_t matches = 0;
	
	size_t  reduction = 0;
	
	map< unsigned char* ,file_hash* >::iterator hash_map_it;
	
	for (hash_map_it=hashes_map.begin(); hash_map_it!=hashes_map.end(); ++hash_map_it){
		file_hash* t = hash_map_it->second;
		
		if ( t->count > 1 ){
			
			matches++;
			
			//printf("\nchecksum matches %d \n\n",t->count);
			printf("\nchecksum matches %d \n",t->count);
			
			/*
			 * 	Anzahl der fds noch testen
			 * 
			 */
			
			
			file_entry *fe;
			list<file_entry *>::iterator it2;
			
			int src_fd = 0;
			int off = 0;
			
			struct btrfs_ioctl_same_args *same = (struct btrfs_ioctl_same_args*)calloc(1, sizeof(struct btrfs_ioctl_same_args) + sizeof(struct btrfs_ioctl_same_extent_info) * t->count - 1);
			struct btrfs_ioctl_same_extent_info *info;
			
			same->dest_count = t->count - 1;
		
			for (it2=t->files.begin(); it2!=t->files.end(); ++it2){
				fe = *it2;
				
				if ( src_fd == 0 ){
					
					src_fd = fe->open_file();
					same->length = fe->length;
					
				}else{
					
					same->info[off].fd = fe->open_file();
					same->info[off].bytes_deduped = 1;
					same->info[off].status = 1;
					
					off++;
					
					reduction += fe->length;
		
				}
				
				total_matches++;
				
				if ( t->count == 2 ){
					fe->print_path();
					printf("%d ( %d )\n",fe->fd,off);
				}
			}
			
			
			
			
			int ret = ioctl(src_fd, BTRFS_IOC_FILE_EXTENT_SAME, same);
			if (ret < 0) {
				fprintf(stderr, "btrfs_same returned error: (%d) %m\n", ret);
				//return -ret;
			}
			
			for ( int i = 0 ; i < off ; i++ ){
				printf("dedeup : %" PRIu64 " %d %m\n",
					(uint64_t)same->info[i].bytes_deduped,
					same->info[i].status
					);
				
			}
			
			for (it2=t->files.begin(); it2!=t->files.end(); ++it2){
				fe = *it2;
				
				fe->close_file();
			}
			
			
			free( same );
			//close( src_fd );
			
			
		}
			
	}
	
	printf("\n");
	
	printf("Match Summary\n");
	
	printf("  Matches   : %d\n",matches);
	printf("  Total     : %d\n",total_matches);
	print_size("  Reduction : ",reduction);
	
	printf("\n");
}

int file_entry::open_file( void ){
	int ret ;
	
	char full_path[2048];
	
	uint32_t used = this->parrent->add_path( full_path, sizeof( full_path ) );
	
	full_path[used] = '/';
	strcpy( &full_path[used+1] , this->name );
	
	ret = open( full_path, O_RDWR );
	
	if (ret < 0) {
		printf("Error open : %m\n");
		return 0;
	}
	
	//printf("open : %s\n",full_path);
	
	this->fd = ret;
	
	return ret;
}

void file_entry::close_file( void ){
	
	if ( this->fd == 0 ) return;
	
	close( this->fd );
	this->fd = 0;
	
	//printf("close : \n");
	
}

void file_entry::update_hash( void ){
	
	char full_path[2048];
	
	uint32_t used = this->parrent->add_path( full_path, sizeof( full_path ) );
	
	full_path[used] = '/';
	strcpy( &full_path[used+1] , this->name );
	
	//printf("%s\n", full_path );
	
	files_hashed++;
	
	//printf("%d / %d\n",files, files_hashed );
	
	double tmp = files_hashed;
	tmp /= files;
	tmp *= 100;
	
		
	if (( tmp - files_hashed_proz ) > 0.01 ) {	
		printf("\r%.2f %%",tmp );
		fflush( stdout);
		files_hashed_proz = tmp;
	}
	
	int fd = open(full_path, O_RDONLY );
	if ( fd < 0 ) {
		//printf("Error opening file : %s\n   %m\n\n", full_path);
		return;
	}
	
	struct stat st;
	fstat(fd, &st );
	
	this->length = st.st_size;
	
	if ( (S_ISREG(st.st_mode) ) && ( st.st_size > 0 ) && ( st.st_size < 128000000L ) )
    {
	
		unsigned char* mem = (unsigned char*)mmap(NULL, st.st_size , PROT_READ , MAP_PRIVATE, fd, 0);
		
		unsigned char *md = (unsigned char*)malloc( SHA512_DIGEST_LENGTH + MD5_DIGEST_LENGTH);
		memset( md, 0 , SHA512_DIGEST_LENGTH + MD5_DIGEST_LENGTH );
		
		SHA1( mem ,st.st_size, md );
		//SHA512( mem ,st.st_size, md );
		//MD5( mem ,st.st_size, md + SHA512_DIGEST_LENGTH );
		
		add_hash( md, this );
	
		munmap( mem , st.st_size );
	}
	
	close( fd );
}


uint32_t dir_entry::add_path( char* buffer,uint32_t length ){
	
	uint32_t used = 0;
	if ( this->parrent != 0 )
		used = this->parrent->add_path( buffer, length );
	
	length -= used;
	buffer += used;
		
	buffer[0] = '/';
	memcpy( buffer+1 , this->name, strlen( this->name ) );
	
	return strlen( this->name ) + 1 + used ;
}

void dir_entry::update_hashes( void ){
	
	dir_entry *t;
	file_entry *t2;
	
	list<dir_entry *>::iterator it;
	list<file_entry *>::iterator it2;
	
	for (it=sub_dirs.begin(); it!=sub_dirs.end(); ++it){
		t = *it;
		t->update_hashes( );
	}
	
	for (it2=files.begin(); it2!=files.end(); ++it2){
		t2 = *it2;
		t2->update_hash();
	}
	
}


void dir_entry::print_path( void ){
	
	if ( this->parrent != 0 )
		this->parrent->print_path();
	
	printf("/%s",this->name);
	
}


void dir_entry::print_dirs( int level ){
		
	list<dir_entry *>::iterator it;
	list<file_entry *>::iterator it2;
	
	//printf("JO : %s\n",this->name);
	
	dir_entry *t;
	file_entry *t2;
	
	for (it=sub_dirs.begin(); it!=sub_dirs.end(); ++it){
		t = *it;
		printf( "%*s \033[01;32m%s\033[00m\n",  level *2 , " ", t->name );
		
		t->print_dirs( level + 1 );
	}
	
	for (it2=files.begin(); it2!=files.end(); ++it2){
		t2 = *it2;
		printf( "%*s %s\n",  level *2 , " ", t2->name );
		
		//t2->print_path();
		
	}
	
	
	//printf( "\n" );
}
	

void listdir(const char *name, int level, dir_entry* root)
{
    DIR *dir;
    struct dirent *entry;

    if (!(dir = opendir(name)))
        return;
    if (!(entry = readdir(dir)))
        return;

    do {
		
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;
            
		
		 
        if (entry->d_type == DT_DIR) {
            char path[1024];
            int len = snprintf(path, sizeof(path)-1, "%s/%s", name, entry->d_name);
            path[len] = 0;
            
            dir_entry* new_dir = new dir_entry( root , entry->d_name);
            
           
            root->sub_dirs.push_back( new_dir );
           
            
            /*if ( level < 1 ){
				printf("Files : %d\n\n",files);
			}
				
            if ( level < 1 ){
				printf("scan : %s\n",path);
				
			}*/
            //printf("%*s[%s]\n", level*2, "", entry->d_name);
            listdir(path, level + 1, new_dir );
        }
        else{
			char path[1024];
            //sprintf(path,"%s/%s\n", name, entry->d_name);
            
            //string s0 (path);
            //file_path.push_back ( s0 );
			
			char* name_t = names->get_name( entry->d_name );

            
            file_entry* t = new file_entry();
            t->name = name_t;
            
            t->parrent = root;
            
            root->files.push_back( t );
            
            files++;
        }
    } while (entry = readdir(dir));
    closedir(dir);
    
    
}


void scan_dir( dir_entry* dir, char* path ){
	
	
	printf("scan : %s\n",path);
	
	listdir(path, 0, dir);
}

int main(int argc, char* argv[])
{
	/*uint16_t value;
	
	FileMemory* fm= new FileMemory();
	fm->open_mem( "/mnt/entwicklung_ext4/btrfs_dedeup_data/dirs" );
	
	while( fm->has_more_data() ){
		char*text = fm->get_string();
		printf("string   : %s\n",text);
		
		value = fm->get_uint16_t();
		printf("uint16_t : %d\n", value );
	}
	
	fm->add_string( "test223" );
	fm->add_uint16_t( value + 1 );
	
	return 0;*/
	
	/*dir_memory = new FileMemory();
	dir_memory->open_mem( "/mnt/entwicklung_ext4/btrfs_dedeup_data/dirs" );*/
	
	names = new Names();
	
	printf("\nBtrfs Dedup Version : TRUNK\n\n");
	
	
	names->load();
	
	
	
	dir_entry* root_debian = new dir_entry( 0, argv[1] );
	scan_dir( root_debian, (char*)argv[1]);
		
	names->sync();
	
	//listdir("/mnt/entwicklung/build_tmp/", 0);
	
	//dir_entry* root_debian = new dir_entry( 0, "/mnt/btrfs//" );
	//scan_dir( &root_debian, (char*)"/mnt/entwicklung/build_tmp/Debian8/crosstool-ng-build");
	//scan_dir( root_debian, (char*)"/mnt/btrfs//");
	//scan_dir( &root_debian, (char*)"/mnt/entwicklung/build_tmp/Debian8");
	
	
	//dir_entry root_fedora;
	//scan_dir( &root_fedora, (char*)"/mnt/entwicklung/build_tmp/Fedora22/");
	
	
	//dir_entry root_fedora;
	//root_fedora.parrent = 0;
	//root_fedora.name = (char*) "mnt/entwicklung/build_tmp/Fedora22";
    //listdir("/mnt/entwicklung/build_tmp/Fedora22/", 0, &root_fedora);
    //listdir("/mnt/entwicklung/build_tmp/Fedora17/", 0);
    
	return 0;
    
    printf("\nHashing ...\n");
	root_debian->update_hashes();
    //root_fedora.update_hashes();
    
    printf("\nHashing finished\n");
    
    //merge_files();
    
    //root_debian.print_dirs(0);
    //root_fedora.print_dirs(0);
    
    /*cout << "mylist contains:";
	for (it=file_path.begin(); it!=file_path.end(); ++it)
		cout << ' ' << *it;
	cout << '\n';*/
    
    printf("Files : %" PRIu64 "\n",files);
    
    
    
    printf("\n");
    
     // showing contents:
	/*cout << "mymap contains:\n";
	names_map_it = names_map.begin();
	for (names_map_it=names_map.begin(); names_map_it!=names_map.end(); ++names_map_it)
		cout << names_map_it->first << " => " << names_map_it->second << '\n';
    */
    
    names->close( );
    
   
    
   // sleep( 10 );
    
    return 0;
}
