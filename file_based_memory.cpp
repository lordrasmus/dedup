

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <stdlib.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "file_based_memory.h"


void FileMemory::open_mem( const char* path ){

	this->size = 4294967296L; // 4 GB

	strncpy(this->path, path , MAX_PATH_LENGTH );

	this->fd = open( path, O_RDWR | O_CREAT);
	if ( this->fd < 0 ) {
		printf("Error opening file : %s\n   %m\n\n", this->path);
		exit(1);
	}

	// create names as sparse file
	
	lseek( this->fd , this->size,  SEEK_SET );
	lseek( this->fd , -3,  SEEK_CUR );
	write( this->fd , "END", 3 );
	lseek( this->fd , 0,  SEEK_SET );
	
	this->mem_start = (uint8_t*)mmap(NULL, this->size , PROT_READ | PROT_WRITE, MAP_SHARED, this->fd, 0);
	this->mem_cur = this->mem_start;
	
	if (this->mem_start == MAP_FAILED){
		 printf("Error: mmaping  file %s\n",this->path);
		 exit(1);
	}
	
}

void FileMemory::sync( void ){

	msync( this->mem_start ,  this->size,  MS_SYNC );
}

void FileMemory::close_mem( void ){
	
	
	munmap( this->mem_start ,  this->size );
	
	close( this->fd );
	
}

bool FileMemory::has_more_data( void ){
	uint16_t* size = (uint16_t*)this->mem_cur;
	
	if ( *size != 0 )
		return true;
		
	return false;
}


void FileMemory::add_uint16_t( uint16_t value ){
	
	uint16_t* size = (uint16_t*)this->mem_cur;
	*size= 2;
	this->mem_cur +=2;
	
	uint16_t* data = (uint16_t*)this->mem_cur;
	*data = value;
	this->mem_cur +=2;
	
	size = (uint16_t*)this->mem_cur;
	*size = 0;
	
}

uint16_t FileMemory::get_uint16_t( void ){

	uint16_t ret;

	uint16_t* size = (uint16_t*)this->mem_cur;
	if ( *size != 2 ){
		printf("next item size != 2 ( %d ) \n", *size);
		exit(1);
	}
	this->mem_cur +=2;
	
	ret = *((uint16_t*)this->mem_cur);
	this->mem_cur +=2;
	
	return ret;
}


void FileMemory::add_uint32_t( uint32_t value ){
	
	uint16_t* size = (uint16_t*)this->mem_cur;
	*size= 4;
	this->mem_cur +=2;
	
	uint32_t* data = (uint32_t*)this->mem_cur;
	*data = value;
	this->mem_cur +=2;
	
	size = (uint16_t*)this->mem_cur;
	*size = 0;
	
}

uint32_t FileMemory::get_uint32_t( void ){

	uint32_t ret;

	uint16_t* size = (uint16_t*)this->mem_cur;
	if ( *size != 4 ){
		printf("next item size != 4 ( %d ) \n", *size);
		exit(1);
	}
	this->mem_cur +=2;
	
	ret = *((uint32_t*)this->mem_cur);
	this->mem_cur +=4;
	
	return ret;
}


char* FileMemory::add_string( const char* text ){

	uint16_t* size = (uint16_t*)this->mem_cur;
	
	int length = strlen( text ) + 1;
	int l2 = length + ( length % __alignof__ ( uint16_t )  );	// alignen
	
	*size = l2;
	
	this->mem_cur += 2;
	char *ret = (char*)this->mem_cur;
	
	memcpy( this->mem_cur, text, length + 1 );
	
	this->mem_cur += l2;
	
	size = (uint16_t*)this->mem_cur;
	*size = 0;
	
	
	return ret;

}


char* FileMemory::get_string( void ){
	uint16_t* size = (uint16_t*)this->mem_cur;
	
	this->mem_cur += 2;
	
	char *ret = (char*)this->mem_cur;
	this->mem_cur += *size ;
	
		
	return ret;
}
