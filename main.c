#include "./include/res.h"

#define SERV_PORT 5002
//#define SERV_IP_ADDR "192.168.7.139"
#define QUIT_STR "quit"

#define PASSWORD "linux"
#define TV_STRING "watch_tv"


#define CON_RETURN_PIN "allow_ack"  // succes return string
#define CON_RETURN_PIN_ERROR "no_allow_ack"

#define USER_IDENTIFY_CLIENT_STR "identity_client_id"
#define USER_IDENTIFY_SERVER_STR "identity_server_id"


#define  BUFSIZE 4096
#define ON  0x11
#define OFF  0x22
#define  NORMAL 0xBB
#define  FAULT    0xAA

#define  READCMD 0x40
#define  CTRLCMD 0x41
#define  RETURNSTATUS 0x42
#define  CONNECT 0x43
#define USER_CLIENT_TYPE 0xA1
#define USER_SERVER_TYPE 0xA2

#if 1
#define dprintf(fmt,args...) printf("%s:"fmt,__func__,##args)
#else
#define dprintf(fmt,args...)
#endif

//#define  sql_no_back(mysql,str)  sqlite3_exec(mysql,str,NULL,NULL,NULL);


struct user_info client_user_info_head;
struct user_info server_user_info_head;
int max_fd = -1;
fd_set event_set;	
char usernamebuff[NAME_BUFSIZE];
int server_type = -1;
void socket_init(struct sockaddr_in * pcin)
{
	pcin->sin_family = AF_INET;
	pcin->sin_port =htons(SERV_PORT);
	pcin->sin_addr.s_addr= htonl(INADDR_ANY);
	bzero(pcin->sin_zero,8);
}


int get_value_from_read_buf(char id[],char buf[])
{
	struct json_object *object,*identity;
	object = json_tokener_parse(buf);
	identity = json_object_object_get(object,id);	
	return json_object_get_int(identity);
}

int  get_user_type_buf(char buf[])
{	
	return get_value_from_read_buf("user_tpye",buf);
}


int get_ctrl_obj_tpye(char *buf)
{
	return get_value_from_read_buf("control_obj_type",buf);
}
	
void get_str_from_buf(char id[],char buf[],char passwd_buf[])
{
	struct json_object *object,*password;
	object = json_tokener_parse(buf);
	password = json_object_object_get(object,id);	
	const char *j_buf=json_object_get_string(password);
	if(j_buf != NULL)
		strcpy(passwd_buf,j_buf);
} 


enum_user_type  user_identify(int fd)
{
	char buf[BUFSIZE];
	char dst_buf[128];
	int ret_user = -1;
	
	memset(dst_buf,0,128);
	memset(buf,0,BUFSIZE);
	
	if(read(fd,buf,BUFSIZE) < 0){
		perror("%s---:read error\n");
		return INVAILD_TYPE;
	}

	dprintf("read buf is : %s\n",buf);
	ret_user = get_user_type_buf(buf);

	if(ret_user < 0){
		dprintf("no such id,get info error!\n");
		return INVAILD_TYPE;
	}
	
	if(ret_user == USER_SERVER_TYPE){
		server_type = get_ctrl_obj_tpye(buf);
		return SERVER_USER;
	}else if(ret_user == USER_CLIENT_TYPE){
		get_str_from_buf("pass_word",buf,dst_buf);		
		if(!strcmp(dst_buf,PASSWORD)){
			memset(usernamebuff,0,NAME_BUFSIZE);
			get_str_from_buf("user_name",buf,usernamebuff);
			return CLIENT_USER;
		}else{
			send(fd,CON_RETURN_PIN_ERROR,strlen(CON_RETURN_PIN_ERROR),0);
			dprintf("password is not correctly !\n");
			return INVAILD_TYPE; 
		}

	}else{
		return INVAILD_TYPE;
	}
}


void add_user_to_list(int fd)
{
	enum_user_type ret_type;
	struct user_info* new_user=NULL;

	dprintf("start....!\n");
	
	ret_type = user_identify(fd);
	if(ret_type != INVAILD_TYPE){
			/*---------------------------*/
			new_user= (struct user_info*)malloc(sizeof(struct user_info));
			if(NULL == new_user){
				perror("malloc error");
				exit(1);
			}	
			new_user->fd = fd;
			new_user->user_type = ret_type;
			if(ret_type == CLIENT_USER){
				new_user->ctrl_cmd_sending =false;
				send(fd,CON_RETURN_PIN,strlen(CON_RETURN_PIN),0);//password is valid add to list
				list_add_tail(&new_user->list,&client_user_info_head.list);
				strcpy(new_user->user_name,usernamebuff);
			}else if (ret_type == SERVER_USER){
				list_add_tail(&new_user->list,&server_user_info_head.list);
				new_user->ctrl_obj_type = server_type;
			}
			printf("new devices connect........!\n");
			/*---------------------------*/
		
	}else{
		close(fd);
	}

}


void led_status_control(char *buf)
{
	//fix led control logic
	led_statues_type led_status = get_value_from_read_buf("cmd_ctrl_led",buf);
	//led_locationt led_status = 
}

void watch_tv_cmd(char *buf)
{
	//int ir_cmd = get_value_from_read_buf("tv_control_cmd",buf);
	//dprintf("watch_tv_cmd is :%d\n",ir_cmd);
	struct list_head *pos = NULL,*q = NULL;
	struct user_info *temp = NULL;
	list_for_each_safe(pos,q,&server_user_info_head.list){
		temp = list_entry(pos,struct user_info,list);
		if(temp->ctrl_obj_type == WATCH_TV){
			dprintf("send msg start...\n");
			send(temp->fd,buf,strlen(buf),0);
		}
	}
}
void del_client_event(struct user_info *user_info,char r_buf[])
{
	ctrl_object_type object_ret = get_value_from_read_buf("ctrl_object",r_buf);
	user_info->ctrl_cmd_sending = true;
	switch(object_ret){
		case LIGHT:
			led_status_control(r_buf);
			break;
		case WATCH_TV:
			watch_tv_cmd(r_buf);
			break;
		default:
			break;
	}
}	

void send_ack_to_client(struct user_info *user_info)
{
	struct list_head *pos = NULL,*q = NULL;
	struct user_info *temp = NULL;
	list_for_each_safe(pos,q,&server_user_info_head.list){
		temp = list_entry(pos,struct user_info,list);
		if(temp->ctrl_cmd_sending){
			send(temp->fd,"cmd_linux_ack",strlen("cmd_linux_ack"),0);
		}
	}
	
}

void handle_event(struct user_info *user_info)
{
	
	int ret_r=-1;
	char buf[BUFSIZE];
	memset(buf,0,BUFSIZE);

	ret_r = read(user_info->fd,buf,BUFSIZE);
	if( ret_r < 0){
		perror("read error!");
		return;
	}else if(ret_r == 0){
		dprintf("%s:  quit....\n",user_info->user_name);		
		close(user_info->fd);
		list_del(&user_info->list);
		free(user_info);
		return;
	}
	
	dprintf(":---->%s\n",buf);
	
	if(user_info->user_type == CLIENT_USER){
		dprintf("--%s req\n",user_info->user_name);
		del_client_event(user_info,buf);
	}else if(user_info->user_type == SERVER_USER){
	    //fix server handler
	    if(strcmp(buf,"cmd_linux_ack",strlen("cmd_linux_ack") == 0)
			send_ack_to_client(user_info);
	}
}

void add_fd_to_fdset()
{
	//struct user_info *new_user = NULL;
	struct list_head *pos = NULL,*q = NULL;
	struct user_info *temp = NULL;

	//add client to list
	list_for_each_safe(pos,q,&client_user_info_head.list){
		temp = list_entry(pos,struct user_info,list);
		FD_SET(temp->fd,&event_set);
		if(temp->fd > max_fd)
			max_fd = temp->fd;
	}

	//add sercver to list
	list_for_each_safe(pos,q,&server_user_info_head.list){
		temp = list_entry(pos,struct user_info,list);
		FD_SET(temp->fd,&event_set);
		if(temp->fd > max_fd)
			max_fd = temp->fd;
	}

}


void for_each_server_client_fd()
{	

	struct list_head *pos = NULL,*q = NULL;
	struct user_info *temp = NULL;
	
	list_for_each_safe(pos,q,&client_user_info_head.list ){
		temp = list_entry(pos,struct user_info,list);
		if(FD_ISSET(temp->fd,&event_set)){	
	   		handle_event(temp);				
		}
	}

	list_for_each_safe(pos,q,&server_user_info_head.list ){
		temp = list_entry(pos,struct user_info,list);
		if(FD_ISSET(temp->fd,&event_set)){
	   		handle_event(temp);	
		}
	}

}

int main(void)
{
	int listen_fd = -1, new_fd = -1;
	//int ret = -1;

	struct sockaddr_in sin,cin;
	socklen_t slen = sizeof(cin);
		

	INIT_LIST_HEAD(&client_user_info_head.list);
	INIT_LIST_HEAD(&server_user_info_head.list);
	//INIT_LIST_HEAD(&json_head.list);
	
	if( (listen_fd = socket(AF_INET, SOCK_STREAM, 0))< 0) {
		perror("server socket error");
		exit(1);
	}



	int b_reuse = 1; 
	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &b_reuse, sizeof(int));

	socket_init(&sin);

	//struct sqlite3 *mysql = create_sqlite();


	if(bind(listen_fd,(struct sockaddr *)&sin,sizeof(sin))  != 0){
		perror("bind failed");
		exit(1);
	}
	
	listen(listen_fd,5);

	printf("server start......!\n");

	while(1){
		FD_ZERO(&event_set);
		FD_SET(listen_fd,&event_set);
		max_fd = listen_fd;
		/*--------------------------------*/
		add_fd_to_fdset();

		printf("maxfd=%d\n",max_fd);
		select(max_fd+1,&event_set,NULL,NULL,NULL);
		
		if(FD_ISSET(listen_fd,&event_set)){
			if((new_fd = accept(listen_fd,(struct sockaddr *)&cin,&slen)) < 0 ){
				perror("accept error");
				continue;
			}
			add_user_to_list(new_fd);
		}

		for_each_server_client_fd();
		//sleep(2);
		
	}
	
	close(listen_fd);
	//sqlite3_close(mysql);
	return 0;
}



















