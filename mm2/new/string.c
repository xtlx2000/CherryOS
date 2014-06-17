#include "string.h"


char *strchr(const char *s, const char ch)
{ 	 
	if (NULL == s)		 
		return NULL;	 	 	 

	const char *pSrc = s;	 
	while ('\0' != *pSrc){
		if (*pSrc == ch){
			return (char *)pSrc;
		}		   	  	   
		++ pSrc;	 
	}	 	 
	
	return NULL;
} 



size_t  strlen(const char * str)
{
	const char *eos = str;        
	while( *eos++ ) ;        
	return( eos - str - 1 );
}


int strncmp(const char *src,const char *dest,size_t count)  
{  
    if(count == 0)  
        return 0;  
    while(count-- && *src && *src == *dest)  
    {  
        src++;  
        dest++;  
    }  
    return (*(unsigned char *)src - *(unsigned char *)dest);  
}  


