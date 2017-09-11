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

//#include "json.h"
#include </usr/local/include/json/json.h>

#define NAME_BUFSIZE 32

typedef enum enum_user_type{
	 INVAILD_TYPE,
	 CLIENT_USER,
	 SERVER_USER
}enum_user_type;


typedef enum ctrl_object_type{
	 LIGHT=0xA00,
	 WATER_HEAT,
	 WATCH_TV,
	 COMPUTER 
}ctrl_object_type;

typedef enum led_statues_type{
	 LED_OFF=0xD0,
	 LED_ON,
	 INVAILED
}led_statues_type;



typedef struct led_info{
	int led_status;
	int location;
}*pled_info;

typedef struct water_heart{
	int action_status;
}*pwater_heart;

typedef struct tv_info{
	int control_action;
	int key_val;
}*ptv_info;

typedef struct user_info{
	struct led_info *p_led_info;
	struct water_heart  *water_heart;
	struct list_head  list;
	struct tv_info *tv_info_option;
	int fd;
	int user_type;//client or server
	char user_name[NAME_BUFSIZE];
	int ctrl_obj_type;
	bool ctrl_cmd_sending;
}*p_user_info;


#endif

