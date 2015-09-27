

#ifndef _DEDUP_UTILS_H_
#define _DEDUP_UTILS_H_


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


#endif
