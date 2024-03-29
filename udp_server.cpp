/*
 * udp_server.c
 *
 *  Created on: 2014年8月9日
 *      Author: donghongqing
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include "global.h"
#include "json/json.h"
//#include <signal.h>

using namespace std;

char * SERVER  = "SERVER";
void dispatch(struct sockaddr_in rin, char *buf, int len);
void deal_log_in(string name, struct sockaddr_in rin, int msg_id);
void deal_ack_msg(PACKET * rcv_pack,  struct sockaddr_in sin);
void deal_log_off(string name, struct sockaddr_in sin, int msg_id);
void deal_pull_msg(string name , struct sockaddr_in sin);
void deal_send_msg(PACKET * rcv_pack, struct sockaddr_in sin);
void deal_heartbeat_msg(string name, struct sockaddr_in sin);
CLIENT * off_client(string name , struct sockaddr_in sin);
CLIENT * check_client(string name);
void create_and_send_com_packet(CLIENT *client, int msg_id);
void create_and_send_system_packet(CLIENT * client, int msg_id);
void parse_send_msg(unsigned char * rcv_data, int rcv_len, int *seq, char *data, int * data_len, int * seq_num, int *total_len)
{

}
void ack_send_msg(CLIENT *client,int msg_id, int seq);
void ack_log_off(CLIENT *client, int msg_id);
void notify(CLIENT * client);
void ack_log_in(CLIENT * client, int msg_id);
CLIENT * update_client_4_log_in(string name, struct sockaddr_in sin);
#if 0
void init() {
	struct sockaddr_in sin;

	int ret = -1;

	sin.sin_family - AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(port);

	sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (-1 == sock_fd) {
		perror("call to socket");
		exit(1);
	}
	//bind socket
	ret = bind(sock_fd, (struct sockaddr *) &sin, sizeof(sin));
	if (-1 == ret) {
			perror("call to bind");
			exit(1);
	}
}
#endif 

void * monitor_server(void * para);
void * send_msg(void * para);
void do_business();

int main()
{

    //SERVER = (char *)malloc(16);
    //memcpy(server, "server", 16);
	int ret;
	 pthread_t id_1,id_2;
	/*创建线程一*/
	ret=pthread_create(&id_1,NULL,monitor_server,NULL);
	ret=pthread_create(&id_2,NULL,send_msg,NULL);
	do_business();

	return 0;
}

void do_business()
{
	char buf[MAXLINE];
	socklen_t address_size = sizeof(struct sockaddr_in);
	struct sockaddr_in rin;
	char str[MIDLINE];//store ip which  readable
	int len  = 0;
	while(1)
	{
		len = -1;
		address_size = sizeof(rin);
		len = recvfrom(sock_fd, buf, MAXLINE, 0, (struct sockaddr *) &rin, &address_size);
		if (-1 == len) {
			perror("call to recvfrom.\n");
			exit(1);
		}
		printf("you ip is %s at port %d:%s\n",	inet_ntop(AF_INET, &rin.sin_addr, str, sizeof(str)),
												ntohs(rin.sin_port),
												buf);
		//分发消息,buf中只会包含一个包
		dispatch(rin, buf, len);
	}
}

void dispatch(struct sockaddr_in rin, char *buf, int len)
{
	//接收数据，分割数据包
	int i = 0;
	PACKET * rcv_pack = (PACKET *) buf;

	int pack_size = sizeof(PACKET);
	//check
	if((rcv_pack->len + pack_size != len)
			||(rcv_pack->head != 0x01)
			||(rcv_pack->order < LOG_IN)
			||(rcv_pack->order > ORDER_NUM))

	{
		//包数据错误
		return;
	}


	//real dispatch order

	switch(rcv_pack->order)
	{
		case LOG_IN:
			/*
			 * 1.判断是否在线,如果已经在线不做处理
			 * 2.到库中加载离线消息
			 * 3.更新sockaddr_in
			 * 4.更新客户端最后接收心跳时间
			 * */
			//检查客户端是否存在
            {
			string name= rcv_pack->from;

			deal_log_in( name, rin, rcv_pack->msg_id);
			break;
            }

        case LOG_OFF:
			/*
			 * 1.获得用户名
			 * 2.判断用户是否在线，如果已离线，不做任何处理
			 * 3.判断是否有push消息在传输，如果有，则把所传输消息清空
			 * 4.设置已下线状态
			 * */
            {
			string name= rcv_pack->from;
			deal_log_off(name, rin, rcv_pack->msg_id);
			break;
            }
		case PULL_MSG:
			/*收到客户端拉取消息队列的消息
			 * 1.确认客户端已注册，
			 * 2.确认拉取还未开始，更新sockaddr
			 * 3.将数据包分片放入内存中
			 * 4.放入发送队列，由发送线程进行数据发送
			 * 5.更新客户端最后接收心跳时间
			 *
			 * */
            {
                string name = rcv_pack->from;
                deal_pull_msg(name, rin);
                break;
            }
		case SEND_MSG:
			/* 1.收到客户端推送的语音消息
			 * 2.检查客户端是否存在,检查该消息是否已经接收过，如已经接收，回复ack。更新sockaddr
			 * 3.如果该msgid在列表中不存在，为存储的推送语音消息开辟映射空间
			 * 4.保存语音消息
			 * 5.为msgid和seq对应的消息回复ack
			 * 6.检查该msgid对应的消息是否全部接收完毕,如果接收完毕，将数据dump在磁盘，将msgid置为接收完成状态，清空映射空间(可保留)
			 * 7.更新客户端最后接收心跳时间
			 * */
            {
			    string name = rcv_pack->from;
			    deal_send_msg(rcv_pack, rin);
			    break;
            }
		case KEEP_ALIVE:
			/*1.收到客户端推送的心跳消息
			 * 2.检查客户端是否存在。
			 * 3.更新sockaddr
			 * 4.更新客户端最后接收心跳时间
			 * 5.服务端收到心跳才会立马回复心跳，服务端不主动发送心跳
			 * */
            {
                string name = rcv_pack->from;
                deal_heartbeat_msg(name, rin);
                break;
            }
		case ACK:
			/*1.服务端可能收到的两类ack。1）notify的ack 2)push_msg的ack
			 * 2.确认client是否存在、是否在线
			 * 3.更新客户端最后接收心跳时间，更新sockaddr
			 * 4.如果是notify的ack:
			 * 	  1)设置已发送notify
			 * 5.如果是push_msg的ack
			 *    1)更新对应msgid和seq的映射表的状态、时间
			 *    2)检查msgid对应的消息是否全部接收完毕，如果接收完毕，将数据清空，将msgid置为发送完成状态，清空映射空间(可保留)
			 * */
            {
                string name = rcv_pack->from;
                deal_ack_msg(rcv_pack,rin);
                break;
            }
	}

}

void*  monitor_server(void * para)
{
	while(1)
	{
		/* 1.检查客户端状态
		 *  服务器当前时间-客户端上次心跳时间>15s，将客户端踢下线
		 * 2.检查客户端的系统消息队列,找到要发送的数据放入全局发送队列
		 *   a)如果是ack消息，发送后直接删除
		 *   b)如果是心跳消息，发送后直接删除
		 *   c)如果是notify，确认接收到ack后删除
		 * 3.检查客户端的语音消息队列，找到已经全部收到ack的消息，后删除
		 *
		 * */

		map<string, CLIENT *>::iterator client_it;//存放client
		for(client_it=client_map.begin();client_it!=client_map.end();++client_it)
		{
			CLIENT * client = client_it->second;
			//当前时间-上次接收心跳时间 > 15s
			if(9999 - client->last_recv_keep_alive_time > 15)
			{
				//将客户端踢下线
                struct sockaddr_in rin;
				off_client(client->name, rin);
			}

			/*
			 * 系统消息队列发送*/
			client->push_sys_msg_2_queue();

			//语音消息队列
			/*检查消息是否发送完成
			 * 是否开启发送语音消息，开启之后才发送
			 *
			 */

			client->push_com_msg_2_queue();

		}

		//休眠10ms
			sleep(10);
	}
    return (void *)0;
}

void* send_msg(void * para)
{
	//读取全局发送队列，发送数据.
	while(1)
	{
		SEND_MSG_POS *smp = global_send_queue.front();
        global_send_queue.pop();
		if(smp == NULL)
		{
			sleep(10);//没有可发送的数据，休息一下
			continue;
		}
		CLIENT * client = check_client(smp->name);
		if(client == NULL)
		{
			//log error;
			continue;
		}
		switch(smp->type)
		{
		case COM_MSG:
			create_and_send_com_packet(client, smp->msg_id);
			break;
		case SYS_MSG:
			create_and_send_system_packet( client,  smp->msg_id);
			break;
		}
	}
    return (void *)0;
}
//服务端打包语音消息
void create_and_send_com_packet(CLIENT *client, int msg_id)
{
	SEND_MSG_MAP * smm = NULL;
	smm = client->get_send_msg_by_id(msg_id);
	if(smm == NULL)
	{
		//log error
		return;
	}

	//打包公共的消息包头
	PACKET packet;
	packet.init(smm->order, smm->size, smm->msg_id, smm->to);
	//遍历语音消息包，发送数据
	for(int i = 0; i < smm->seq_num; i++)
	{
		SEND_MSG_SEQ * sms = smm->get_send_msg_by_seq(i);

		//当前发送分片是否已经接收ack
		if(sms->is_recv_ack == 0)
		{
			/*判断当前分片是否需要重发
			 * 间隔时间2s，没收到，重发
			 * 小于2s，等待下次检测
			 * */
			if(9999 - sms->last_send_msg_time < 2)
				continue;
			int send_len = sizeof(PACKET) + sms->len;
			char * send_msg = (char *)malloc(send_len);
			memcpy(send_msg, &packet, sizeof(PACKET));
			memcpy(send_msg+sizeof(PACKET), sms->data, sms->len);

			int ret = sendto(sock_fd, send_msg, send_len, 0, (struct sockaddr *) &(client->sin),sizeof(client->sin));
			free(send_msg);
			if(ret == -1)
			{
				perror("error call to sendto\n");
				return;
			}
			sms->retry_send_times++;
			sms->last_send_msg_time=9999;//上次发送消息时间
		}
	}


}
//服务端打包发送system消息
void create_and_send_system_packet(CLIENT * client, int msg_id)
{
	SYSTEM_MSG_MAP * sys_smm = NULL;
	sys_smm = client->get_send_sys_msg_by_id(msg_id);
	if(sys_smm == NULL)
	{
		//log error
		return;
	}

	PACKET pack;
	pack.init(sys_smm->order, sys_smm->size, sys_smm->msg_id, sys_smm->to);
	/*讲数据包拼装到一起后发送*/
	int send_len =sizeof(PACKET) + sys_smm->size;
	char * send_msg = (char *)malloc(sizeof(PACKET) + sys_smm->size);
	memcpy(send_msg, &pack, sizeof(PACKET));
	memcpy(send_msg+sizeof(PACKET), sys_smm->data, sys_smm->size);

	int ret = sendto(sock_fd, send_msg, send_len, 0, (struct sockaddr *) &(client->sin),sizeof(client->sin));
	free(send_msg);
	if(ret == -1)
	{
		perror("error call to sendto\n");
		return;
	}
	sys_smm->retry_send_times++;
	sys_smm->last_send_msg_time = 9999;//当前时间
}

void deal_heartbeat_msg(string name, struct sockaddr_in sin)
{

	CLIENT * client = check_client(name);
	if(client == NULL)
	{
		return;
	}
    memcpy(&client->sin, &sin, sizeof(struct sockaddr_in));
	client->last_recv_keep_alive_time = 9999;//当前时间
}
/*
 * 接收客户端发送的语音message,*/
void deal_send_msg(PACKET * rcv_pack, struct sockaddr_in sin)
{
	CLIENT * client = check_client(rcv_pack->from);
	if(client == NULL)
	{
		//回复未登录
		return;
	}
	//client加锁
    //lock(client->c_lock);
    memcpy(&client->sin, &sin, sizeof(struct sockaddr_in));
	client->last_recv_keep_alive_time = 9999;//当前时间
    //unlock(client->c_lock);
	//解析send消息
	int seq = -1;
	char * data = NULL;
	int data_len = -1;
	int total_len = 0;
	int seq_num = 0;
	int has_init = 0;


	//解析语音包，查看是接收的[msg_id, seq]，并做存储
	parse_send_msg(rcv_pack->data, rcv_pack->len, &seq, data,
				   &data_len, &seq_num, &total_len);

	//检查发送消息区，是否创建并初始化了该msgid
	RECV_MSG_MAP * rmm = client->check_recv_msg_map(rcv_pack->msg_id,  & has_init);
	//初次收到该msgid的包，为保存数据，初始化
	if(has_init == 0)
	{
		rmm->init(rcv_pack->msg_id,  SEND_MSG, rcv_pack->from, sizeof(rcv_pack->from),
							     rcv_pack->to, sizeof(rcv_pack->to), total_len, seq_num);
	}

	//为分包映射开辟空间
	rmm->add_2_recv_seq_map(seq, data, data_len, &has_init);
	//log is has_init
	ack_send_msg(client,rcv_pack->msg_id, seq);

	//检验是否全部发送完毕
	if(rmm->recv_seq_num == seq_num)
	{
		//数据回写磁盘，把此数据区从recv上摘除
		char * data = rmm->dump_msg_2_disk(rcv_pack->msg_id);
		client->clear_recv_msg_by_id(rcv_pack->msg_id);

		//重新分片，挂载到to的send map上（先挂载，再判断pull_msg）
		CLIENT * to_client = check_client(rcv_pack->to);

		//随机一个发送msgid
		int to_msg_id = rand()%65536;
		SEND_MSG_MAP * smm = to_client->get_send_msg_nx_by_id(to_msg_id, &has_init);
		if(has_init == 1)
		{
			//log 已经有相同的msg_id,exit
			printf("has same msgid:[%d]\n", to_msg_id);
			exit(0);
		}
		//将数据放入发送映射关系数据表中
		smm->add_msg(data, total_len);

		//释放数据空间
		free(data);

		//检查to是否已经开启了pull_msg，如果开启了，直接挂载，不用发送notify
		if(to_client->is_push_msg == 0)
		{
			notify(to_client);
		}
	}
}

void deal_pull_msg(string name , struct sockaddr_in sin)
{
	CLIENT * client = check_client(name);
	if(client == NULL)
	{
		//回复未登录
		return;
	}
	//lock(client->c_lock);
	client->sin = sin;//更新sock addr
	client->last_recv_keep_alive_time = 9999;//当前时间
	client->is_push_msg = 1;

	//unlock(client->c_lock);
}

void deal_ack_msg(PACKET * rcv_pack,  struct sockaddr_in sin)
{
	char *data = (char *)rcv_pack->data;
	CLIENT * client = check_client(rcv_pack->from);
	client->sin = sin;
	client->last_recv_keep_alive_time = 9999;//当前时间
	if(client == NULL)
	{
		//log client 为空
		printf("client:[%s] is null, ignore msg", rcv_pack->from);
		return;
	}

	//ACK_MSG_ID,获取msg_id
	//ACK_MSG_ORDER， 获取msg命令
    //等待修改
	int msg_id ;//= data["ACK_MSG_ID"];
	ORDER ack_order;// =(int)data["ACK_MSG_ORDER"];
	//如果是notify的响应，将notify消息置为已经发送
	if(ack_order == NOTIFY)
	{
		SYSTEM_MSG_MAP * smm = client->get_send_sys_msg_by_id(msg_id);
		if(smm == NULL)
		{
			//log error,msg_id为空
			printf("SYSTEM_MSG_MAP is null, msgid:[%d]", msg_id);
		}
		smm->is_recv_ack = 1;
		smm->last_recv_ack_time = 9999;//当前时间
	}
	else if(ack_order == PUSH_MSG)
	{
		int seq;// = (int)data["ACK_MSG_SEQ"];
		SEND_MSG_MAP * smm = client->get_send_msg_by_id(msg_id);
		if(smm == NULL)
		{
			return;
		}
		SEND_MSG_SEQ * sms = smm->get_send_msg_by_seq(seq);
		sms->is_recv_ack = 1;
		sms->last_recv_ack_time = 9999;//取当前时间
	}

}

void deal_log_off(string name, struct sockaddr_in sin, int msg_id)
{
	CLIENT * client =	off_client(name, sin);
	//回ack,放入回ack的消息队列
	if (client != NULL)
		ack_log_off(client, msg_id);
}

void deal_log_in(string name, struct sockaddr_in  rin, int msg_id)
{
	CLIENT * client = update_client_4_log_in(name, rin);

	//将当前发送消息队列已发送标志位全部置为0，标记需要全部发送
	//send_msg_arr 正在发送的清空发送标记
	ack_log_in(client, msg_id);


	//判断send_msg_arr，是否有离线消息， 将notify放入系统消息队列,通知客户端来pull数据

	if(client->send_msg_arr.size() != 0)
	{
		//发送notify消息
		notify(client);

	}
}

CLIENT * off_client(string  name , struct sockaddr_in sin)
{
	CLIENT * client = check_client(name);

	if(client == NULL)
	{
		//log error
		//回ack
		return NULL;
	}
	client->sin = sin;
	client->last_recv_keep_alive_time = 9999;//当前时间
	if(client->is_on_line == 0)
	{
		//已下线，记录客户端重复发送下线消息

	}
	//不再push语音消息
	client->is_push_msg = 0;
	//正在发送和接收的语音消息清空
	//client->recv_msg_arr.clear();
	//client->send_msg_arr.clear();
	return client;
}


CLIENT * update_client_4_log_in(string name, struct sockaddr_in sin)
{
	CLIENT * client = check_client(name);
	if(client == NULL)
	{
		client = new CLIENT();
		//client_map加锁
		//lock(client_map_lock);
		//保存登录信息
		client_map.insert(pair<string, CLIENT*>(name, client));
		//client_map解锁
		//unlock(client_map_lock);
	}
	//client 加锁
	//lock(client->c_lock);
	//保存数据到客户端
	//更新心跳时间
	client->last_recv_keep_alive_time = 45454554;//当前时间
	client->last_send_keep_alive_time = 0;

	client->is_on_line = 1;//是否在线
	client->is_push_msg = 0;//是否发送push
	client->login_time = "2000-01-01 00:00:00";
	client->sin = sin;//更新
	//client 解锁
	//unlock(client->c_lock);

	return client;
}

void create_system_msg(CLIENT * client, ORDER order,char * from, char * to, char * buf, int len, MSG_TYPE type)
{

	//新建ack消息结构体,并挂载在client下
	SYSTEM_MSG_MAP * smm  =	(SYSTEM_MSG_MAP *)malloc( sizeof(SYSTEM_MSG_MAP));

	//初始化SYSTEM_MSG_MAP
	smm->init( order,  from,  to,  buf, len);


	//smm加入到client中(记得加锁噢)，
	client->add_sys_msg_by_id(smm->msg_id,smm);


	//将消息id放入到发送队列
	SEND_MSG_POS * smp = (SEND_MSG_POS *)malloc(sizeof(SEND_MSG_POS));
	smp->msg_id = smm->msg_id;
	memcpy(smp->name ,  client->name, 16);
	smp->type = type;


	//为全局发送队列加锁，（发送消息线程，在对应的client结构体中寻找对应的data数据，然后发送，
	//如果是不需要响应的消息，发送完后就在client中删除；如果是需要响应的消息则在recv线程中处理
	//pthread_lock(global_send_queue_mutex);
	global_send_queue.push(smp);
	//为全局发送队列解锁
	//pthread_unlock(global_send_queue_mutex);


}

CLIENT * check_client(string name)
{
	CLIENT * client = NULL;

    map<string, CLIENT*>::iterator iter;
    iter = client_map.find(name);
	if((client=iter->second) == NULL)
	{
		return NULL;
	}
	return client;
}

void ack_send_msg(CLIENT *client,int msg_id, int seq)
{
	string data;
	Json::Value value;
	value["ACK_MSG_ID"] = int(msg_id);
	value["ACK_MSG_ORDER"] = SEND_MSG;
	value["ACK_MSG_SEQ"] = int(seq);

	Json::FastWriter writer;
	data = writer.write( value );
    //临时
    char * buf = (char *)malloc(data.length()+1);
    memset(buf, 0, data.length()+1);
    strcpy(buf, data.c_str());
    //临时拷贝
	int len = data.length();
	create_system_msg(client, ACK, SERVER, client->name, buf, len, SYS_MSG );

}
//发送下线的ack
void ack_log_off(CLIENT *client, int msg_id)
{
	string data;
	Json::Value value;
	value["ACK_MSG_ID"] = int(msg_id);
	value["ACK_MSG_ORDER"] = LOG_OFF;

	Json::FastWriter writer;
	data = writer.write( value );
    //临时
    char * buf = (char *)malloc(data.length()+1);
    memset(buf, 0, data.length()+1);
    strcpy(buf, data.c_str());
    //临时拷贝
    int len = data.length();
	create_system_msg(client, ACK, SERVER, client->name,buf, len, SYS_MSG );

}
//发送登录的ack
void ack_log_in(CLIENT * client, int msg_id)
{
	string data;
	Json::Value value;
	value["ACK_MSG_ID"] = int(msg_id);
	value["ACK_MSG_ORDER"] = LOG_IN;

	Json::FastWriter writer;
	data = writer.write( value );
    //临时
    char * buf = (char *)malloc(data.length()+1);
    memset(buf, 0, data.length()+1);
    strcpy(buf, data.c_str());
    //临时拷贝
    int len = data.length();

	create_system_msg(client, ACK,SERVER, client->name,buf, len, SYS_MSG );
}
//发送通知消息
void notify(CLIENT * client)
{
	create_system_msg(client, NOTIFY,SERVER, client->name,NULL, 0, SYS_MSG);
	//发送完notify后，等待客户端的pull消息，client中不做变化
}

