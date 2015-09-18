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

#include <iostream>

#include <string>
#include <map>

#include <openssl/md5.h>
#include <openssl/sha.h>

#include "main.h"

uint64_t files = 0;
uint64_t files_hashed = 0;
double files_hashed_proz = 0;



struct cmp_char_p {
    bool operator()(const char* a, const char* b) const {
		int ret = strcmp(a,b);
		
		if ( ret < 0 ){
			//printf(" a %s < b: %s\n",a,b);
			return true;
		}
		
		/*if ( ret == 0 )
			printf(" a %s == b: %s\n",a,b);
		
		if ( ret > 0 )
			printf(" a %s > b: %s\n",a,b);*/
			
        return false;
    }
};

struct cmp_uchar_p {
    bool operator()(const unsigned char* a, const unsigned char* b) const {
		int ret = memcmp((const char*)a,(const char*)b, SHA512_DIGEST_LENGTH + MD5_DIGEST_LENGTH );
		
		if ( ret < 0 ){
			//printf(" a %s < b: %s\n",a,b);
			return true;
		}
		
		/*if ( ret == 0 )
			printf(" a %s == b: %s\n",a,b);
		
		if ( ret > 0 )
			printf(" a %s > b: %s\n",a,b);*/
			
        return false;
    }
};

map< unsigned char* ,file_hash*, cmp_uchar_p > hashes_map;

uint32_t cur_id = 0;
map< char* ,uint32_t, cmp_char_p > names_map;
map< char* ,uint32_t >::iterator names_map_it;


int   names_fd;
off_t names_size = 4294967296L; // 4 GB

char* names_mem;
off_t names_mem_offset = 0;





void open_names_file(void ){
	
	char names_path[] = "/mnt/entwicklung_ext4/btrfs_dedeup_data/names";
	
	names_fd = open(names_path, O_RDWR | O_CREAT);
	if ( names_fd < 0 ) {
		printf("Error opening names files : %s\n   %m\n\n", names_path);
		exit(1);
	}

	// create names as sparse file
	
	lseek( names_fd , names_size,  SEEK_SET );
	lseek( names_fd , -3,  SEEK_CUR );
	write( names_fd , "END", 3 );
	lseek( names_fd , 0,  SEEK_SET );
	
	names_mem = (char*)mmap(NULL, names_size , PROT_READ | PROT_WRITE, MAP_SHARED, names_fd, 0);
	if (names_mem == MAP_FAILED){
		 printf("Error mmap : names file");
		 exit(1);
	}
   
}

char* names_write( char* data ){
	
	uint16_t* size = (uint16_t*)names_mem;
	
	
	
	int l = strlen( data ) +1;
	int l2 = l + ( l % 2 );	// auf 2 byte alignen
	
	//printf("add: %s  %d / %d\n",data, l , l2);
	//printf("mem: %X \n",names_mem);
	
	*size = l2;
	
	names_mem += 2;
	char * ret = names_mem;
	
	memcpy( names_mem, data, l + 1 );
	
	names_mem += l2;
	
	//sleep(1);
	
	return ret;
	
	
}

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

void print_size( char* text, size_t size){
	
	if ( size > ( 1024 * 1024 ) ){
		printf("%s%d MB\n",text,size / (1024 * 1024 ));
		return;
	}
		
}

void merge_files( void ){
	
	uint32_t total_matches = 0;
	uint32_t matches = 0;
	
	size_t  reduction = 0;
	
	map< unsigned char* ,file_hash* >::iterator hash_map_it;
	
	for (hash_map_it=hashes_map.begin(); hash_map_it!=hashes_map.end(); ++hash_map_it){
		file_hash* t = hash_map_it->second;
		
		if ( t->count > 1 ){
			
			matches++;
			
			//printf("\nchecksum matches\n\n");
			
			struct btrfs_ioctl_same_args *same = (struct btrfs_ioctl_same_args*)calloc(1, sizeof(struct btrfs_ioctl_same_args) + sizeof(struct btrfs_ioctl_same_extent_info) * t->count - 1);
			struct btrfs_ioctl_same_extent_info *info;
		
			file_entry *fe;
			list<file_entry *>::iterator it2;
		
			for (it2=t->files.begin(); it2!=t->files.end(); ++it2){
				fe = *it2;
				//fe->print_path();
				
				total_matches++;
				
				reduction += fe->length;
			}
			
			reduction -= fe->length;
			
			
			
			/*int ret = ioctl(src_fd, BTRFS_IOC_FILE_EXTENT_SAME, same);
			if (ret < 0) {
				ret = errno;
				fprintf(stderr, "btrfs_same returned error: (%d) %s\n", ret,
					strerror(ret));
				return -ret;
			}*/
		}
			
	}
	
	printf("\n");
	
	printf("Match Summary\n");
	
	printf("  Matches   : %d\n",matches);
	printf("  Total     : %d\n",total_matches);
	print_size("  Reduction : ",reduction);
	
	printf("\n");
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
	
	if ( (S_ISREG(st.st_mode) ) && ( st.st_size > 0 ) )
    {
	
		unsigned char* mem = (unsigned char*)mmap(NULL, st.st_size , PROT_READ , MAP_PRIVATE, fd, 0);
		
		unsigned char *md = (unsigned char*)malloc( SHA512_DIGEST_LENGTH + MD5_DIGEST_LENGTH);
		
		SHA512( mem ,st.st_size, md );
		MD5( mem ,st.st_size, md + SHA512_DIGEST_LENGTH );
		
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
            
        char* name_t;
             
		//printf("search: %s \n",entry->d_name);
		// prüfen ob schon in der map vorhanden
		names_map_it = names_map.find( entry->d_name );
		if (names_map_it == names_map.end()) {
			
			name_t = names_write( entry->d_name );
			
			names_map.insert ( pair<char* ,uint32_t>( name_t ,cur_id++) );
		}else{
			name_t = names_map_it->first;
		}
		
		 
        if (entry->d_type == DT_DIR) {
            char path[1024];
            int len = snprintf(path, sizeof(path)-1, "%s/%s", name, entry->d_name);
            path[len] = 0;
            
            dir_entry* new_dir = new dir_entry();
            new_dir->name = name_t;
            
            new_dir->parrent = root;
            
            root->sub_dirs.push_back( new_dir );
           
            
            if ( level < 1 ){
				printf("Files : %d\n\n",files);
			}
				
            if ( level < 1 ){
				
				printf("scan : %s\n",path);
			}
            //printf("%*s[%s]\n", level*2, "", entry->d_name);
            listdir(path, level + 1, new_dir );
        }
        else{
			char path[1024];
            //sprintf(path,"%s/%s\n", name, entry->d_name);
            
            //string s0 (path);
            //file_path.push_back ( s0 );
            
            
            file_entry* t = new file_entry();
            t->name = name_t;
            
            t->parrent = root;
            
            root->files.push_back( t );
            
            files++;
        }
    } while (entry = readdir(dir));
    closedir(dir);
    
    msync( names_mem,  names_size,  MS_SYNC );
}


void scan_dir( dir_entry* dir, char* path ){
	
	dir->parrent = 0;
	dir->name = path;
	
	listdir(path, 0, dir);
}

int main(void)
{
	
	printf("\nBtrfs Dedup Version : TRUNK\n\n");
	
	
	open_names_file();
		
	
	//listdir("/mnt/entwicklung/build_tmp/", 0);
	
	dir_entry root_debian;
	scan_dir( &root_debian, (char*)"/mnt/entwicklung/build_tmp/Debian8/crosstool-ng-build");
	//scan_dir( &root_debian, (char*)"/mnt/entwicklung/build_tmp/Debian8");
	
	
	dir_entry root_fedora;
	//scan_dir( &root_fedora, (char*)"/mnt/entwicklung/build_tmp/Fedora22/");
	
	
	//dir_entry root_fedora;
	//root_fedora.parrent = 0;
	//root_fedora.name = (char*) "mnt/entwicklung/build_tmp/Fedora22";
    //listdir("/mnt/entwicklung/build_tmp/Fedora22/", 0, &root_fedora);
    //listdir("/mnt/entwicklung/build_tmp/Fedora17/", 0);
    
   
    
    printf("Hashing ...\n");
	root_debian.update_hashes();
    //root_fedora.update_hashes();
    
    printf("\nHashing finished\n");
    
    merge_files();
    
    //root_debian.print_dirs(0);
    //root_fedora.print_dirs(0);
    
    /*cout << "mylist contains:";
	for (it=file_path.begin(); it!=file_path.end(); ++it)
		cout << ' ' << *it;
	cout << '\n';*/
    
    printf("Files : %d\n",files);
    printf("Names : %d\n",cur_id);
    
    
    printf("\n");
    
     // showing contents:
	/*cout << "mymap contains:\n";
	names_map_it = names_map.begin();
	for (names_map_it=names_map.begin(); names_map_it!=names_map.end(); ++names_map_it)
		cout << names_map_it->first << " => " << names_map_it->second << '\n';
    */
    
    close( names_fd );
    
   
    
   // sleep( 10 );
    
    return 0;
}
