#include <stddef.h>
#include <stdio.h>
// uncomment to disable assert()
// #define NDEBUG
#include <assert.h>


#ifndef ITCH_H
#define ITCH_H
#include <stddef.h>
#define _BSD_SOURCE
#include <endian.h>
#include <stdio.h>
#include <ctype.h>
#include <cstring>

// ORIGINAL FILE: itch_s.h
typedef struct __attribute__((__packed__)){
#include "gen/itch_msg_struct_head.h"
#include "gen/itch_msg_struct_inner.h"
}tv_itch5_s;


// ORIGINAL FILE: itch.c
void fill_tv_itch5(char msg_type, void* data, size_t data_len, tv_itch5_s *itch_s){
	// write all valid signals to 0		
	// valid signals are at the top of the struct, this is a packed struct
	// calculate the offset to the first data anonymous struct and use it to
	// get the last address of the valid signals
	size_t offset_first_anon; 
	size_t exp_len;	
	offset_first_anon = offsetof( tv_itch5_s, itch_system_event_data );
	memset(itch_s,0,offset_first_anon);  
	switch( msg_type ){
		#include "gen/itch_msg_fill_case.h"
		default :
			assert(0); 
			printf("Error : Unknown itch message type %c\n", msg_type);
			break;
	} 
	assert(data_len == exp_len );
}

#endif // ITCH_H