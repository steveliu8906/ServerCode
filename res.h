#ifndef __RES_H__
#define __RES_H__

#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
//#include<sqlite3.h>
#include <sys/types.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

//#include </usr/local/include/json/json.h>
#define NAME_BUFSIZE 32

typedef enum enum_user_type{
	 INVAILD_TYPE,
	 CLIENT_USER,
	 SERVER_USER
}enum_user_type;


typedef struct led_info{
	int led_status;
	int location;
}*p_led_info;

typedef struct water_heart{
	int action_status;
}*p_water_heart;

typedef struct tv_info{
	int control_action;
	int key_val;
}*p_tv_info;

typedef struct user_info{
	struct led_info *p_led_info;
	struct water_heart  *water_heart;
	struct list_head  list;
	struct tv_info *tv_info_option;
	int fd;
	int user_type;//client or server
	char user_name[NAME_BUFSIZE];
}*p_user_info;


#endif
