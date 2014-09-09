/*
 * global.h
 *
 *  Created on: 2014��8��9��
 *      Author: donghongqing
 */
#define HEAD 0x01
#define MAXLINE 10240
#define MIDLINE 1024
int port = 8000;
int sock_fd = -1;

//������Ϣ��Ƭ��
typedef struct {
	int seq; //���
	//unsigned char is_send; //�Ƿ���
	unsigned char retry_send_times; //�������Դ��������շ���ֵĬ��Ϊ0
	unsigned char is_recv_ack; //�Ƿ��յ�ack
	unsigned int last_send_msg_time;//�ϴη���msg��ʱ��
	unsigned int last_recv_ack_time;//�ϴν��յ�ack��ʱ��
	void * data;//��Ƭ�洢����
	int len;//���ݴ�С
	void init(seq, time, data, len)
	{
		this->seq = seq;
		this->last_send_msg_time = 0;
		this->last_recv_ack_time = 0;
		this->is_recv_ack = 0;
		this->retry_send_times = 0;
		this->len = len;
		/*
		 * ��data�����json���У�MSG_SEQ*/

		this->data=malloc(len);
		memcpy(this->data, data, len);
	}
	~SEND_MSG_SEQ()
	{
		free(data);
	}
} SEND_MSG_SEQ;

//������Ϣ��Ƭ��
typedef struct {
	int seq; //���
	//unsigned char is_recv;//�Ƿ����
	unsigned char is_send_ack;//�Ƿ��͹�ack
	unsigned int last_send_ack_time;//�ϴη���ack��ʱ��
	unsigned int last_recv_msg_time;//����msg��ʱ��
	void * data;//��Ƭ�洢����
	int len;//��Ƭ���ݴ�С
	void init(int seq, int last_recv_msg_time, char * data, int len)
	{
			this->seq = seq;
			this->last_recv_msg_time = last_recv_msg_time;
			this->last_send_ack_time = 0;
			this->is_send_ack = 0;
			this->data=malloc(len);
			memcpy(this->data, data, len);
			this->len = len;
	}
	void set_last_send_ack_time(int lsat)
	{

	}
	void * get_data(int * len)
	{
		*len = len;
		return data;
	}
	~RECV_MSG_SEQ()
	{
		free(data);
	}
} RECV_MSG_SEQ;

//����������Ϣ��Ƭӳ��
typedef struct{
	int msg_id;//������Ϣid
	ORDER order;//����
	char from[16];//���ͷ�����
	char to[16];//���շ�����
	int size;//�������ݴ�С
	int seq_num;//��Ƭ����
	unsigned char is_send;//�Ƿ������
	int send_seq_num;//�յ�ack���1
	map<int, SEND_MSG_SEQ *> send_msg_seq_map;
	//vector  <SEND_MSG_SEQ *> send_msg_seq_map;
	CLIENT * client;
	SEND_MSG_MAP(){
		//init map
	}
	void ~SEND_MSG_MAP()
	{
		map<int,SEND_MSG_SEQ *>::iterator it;
		for(it=send_msg_seq_map.begin();it!=send_msg_seq_map.end();++it)
		{
			send_msg_seq_map * sms = it->second
			delete sms;
		}
	}
	void init(int msg_id, ORDER order, char * from, char * to, int size, int seq_num)
	{
		this->msg_id = msg_id;
		this->order = order;
		memcpy(this->from, from);
		memcpy(this->to, to);
		this->size = size;
		this->seq_num = seq_num;
		this->is_send = 0;
		this->send_seq_num = 0;
	}

	RECV_MSG_SEQ * check_send_msg_seq( int seq)
	{
		SEND_MSG_SEQ * sms = NULL;
		sms = send_msg_seq_map.get(seq)
		return sms;
	}
	void add_msg(char * data, int len)
	{
		int per_seq_size = 1500;
		int seq = 0;
		int has_init = 0;
		int index = 0;
		while(len > 0)
		{

			if(per_seq_size > len)
				data_len = len;
			else
				data_len = per_seq_size;
			add_2_send_seq_map(seq, data+index, data_len, &has_init);
			len -= data_len;
			index += data_len;
			seq++;
		}
	}
	void add_2_send_seq_map(int seq, char * data, int data_len, int * has_init)
	{
		int time = 9999;//���յ�����ʱ�䣬ȡ��ǰʱ��
		SEND_MSG_SEQ * sms  = check_send_msg_seq(seq);
		if(sms == null)
		{
			sms = new SEND_MSG_SEQ();
			sms->init(seq, data, data_len);
			//��ӵ�recv_msg_seq_map��
			send_msg_seq_map.set(seq, sms);
			*has_init = 0;
		}
		else
		{
			//���ݰ��Ѿ����ڣ���¼log���ͻ����ظ�����,�ظ�ack,����Ҫ��recv_seq_num+1
			*has_init = 1;//�Ѵ���
		}

		return sms;
	}
	SEND_MSG_SEQ * get_send_msg_by_seq(int seq)
	{
		SEND_MSG_SEQ * sms  = (SEND_MSG_SEQ *)send_msg_seq_map.get(seq);
		return sms;
	}
	void clear_self()
	{
		//����Ϣ�Ѿ��������,ɾ��
		 if(is_send == 1)
		{
			 ~SEND_MSG_MAP();
			 return;
		}
	}
	void push_2_queue()
	{
		//����Ϣid���뵽���Ͷ���
		SEND_MSG_POS * smp = malloc(smp, sizeof(SEND_MSG_POS), 0);
		smp->msg_id = msg_id;
		smp->name = client->name;
		smp->type = COM_MSG;//������Ϣ


		//Ϊȫ�ַ��Ͷ��м���
		pthread_lock(global_send_queue_mutex);
		global_send_queue.push(smp);
		//Ϊȫ�ַ��Ͷ��н���
		pthread_unlock(global_send_queue_mutex);
	}
}SEND_MSG_MAP;

//����������Ϣ��Ƭӳ��
typedef struct {
	int msg_id;//������Ϣid
	ORDER order;//����
	char from[16];//���ͷ�����
	char to[16];//���շ�����
	int size;//�������ݴ�С����λ�ֽ�
	int seq_num;//��Ƭ����
	int recv_seq_num;
	//unsigned char is_recv; //�Ƿ��Ѿ����չ���msgid
	map<int, RECV_MSG_SEQ *> recv_msg_seq_map;
	//vector <RECV_MSG_SEQ *> recv_msg_seq_map;
	CLIENT * client;

	~RECV_MSG_MAP()
	{
		//��recv_msg_seq_map��RECV_MSG_SEQ��������

		map<int,RECV_MSG_SEQ *>::iterator it;
		for(it=recv_msg_seq_map.begin();it!=recv_msg_seq_map.end();++it)
		{
			RECV_MSG_SEQ * rms = it->second
			delete rms;
		}
	}
	void init(int msg_id, ORDER order, char * from, int from_len,
			     char * to, int to_len, int size,int seq_num)
	{
		//init map
		this->msg_id = msg_id;
		this->order = order;
		strcpy(this->from, from);
		strcpy(this->to, to);
		this->size = size;
		this->seq_num = seq_num;
		recv_seq_num = 0;

	}
	void add_2_recv_seq_map(int seq, char * data, int data_len, int * has_init)
	{
		int time = 9999;//���յ�����ʱ�䣬ȡ��ǰʱ��
		RECV_MSG_SEQ * rms  = check_recv_msg_seq(seq);
		if(rms == null)
		{
			rms = new RECV_MSG_SEQ();
			recv_seq_num++;

			rms->init(seq, time, data, data_len);

			//��ӵ�recv_msg_seq_map��
			recv_msg_seq_map.set(seq, rms);
			*has_init = 0;
		}
		else
		{
			//���ݰ��Ѿ����ڣ���¼log���ͻ����ظ�����,�ظ�ack,����Ҫ��recv_seq_num+1
			*has_init = 1;//�Ѵ���
		}

		return rms
	}
	RECV_MSG_SEQ * check_recv_msg_seq( int seq)
	{
		RECV_MSG_SEQ * rms = NULL;
		rms = recv_msg_seq_map.get(seq)
		return rms;
	}

	void dump_msg_2_disk(int msg_id)
	{
		char * data = malloc(size);
		int index = 0;
		for(int i = 0; i < recv_seq_num; i++)
		{
			RECV_MSG_SEQ * rms = recv_msg_seq_map.get(i);
			memcpy(data+index, rms->data, rms->len);
		}
		return data;
	}


} RECV_MSG_MAP;


//��������ͨ��Ϣ�ṹ
typedef struct{
	int msg_id;//��Ϣid
	ORDER order;//����
	char from[16];//���ͷ�����
	char to[16];//���շ�����
	int size;//��������С

	unsigned char is_send_ok; //�Ƿ��ͳɹ���������Ϣֱ���ж�
	unsigned char retry_send_times; //�������Դ��������շ���ֵĬ��Ϊ0
	unsigned char is_recv_ack; //�Ƿ��յ�ack��������͵���ack�����ֶ���Ч
	unsigned int last_send_msg_time;//�ϴη���msg��ʱ��
	unsigned int last_recv_ack_time;//���յ�ack��ʱ��,������͵���ack�����ֶ���Ч
	unsigned char * data;//������
	void init(int order, char * from, char * to, char * data,  int size)
	{
		//��ʼ��smm
		this->msg_id = rand()%65536;
		this->order = order;
		this->data = malloc(size);
		memcpy(this->data, data, size);

		this->from = from;
		this->to = to;

		this->last_send_msg_time = 0;
		this->last_recv_ack_time = 0;
		this->retry_send_times = 0;
		this->is_send_ok = 0;
		this->is_recv_ack = 0;
		this->size = size;
	}


} SYSTEM_MSG_MAP;

/*�ͻ��˽ṹ��Ϣ*/
typedef struct {

	struct sockaddr_in sin;

	char name[32];
	//�Ƿ����ڷ���push��Ϣ
	int is_push_msg;
	//�Ƿ�����
	int is_on_line;
	//��¼ʱ��
	String login_time;

	//���һ�ν��յ�����ʱ��
	int last_recv_keep_alive_time;
	//���һ�η��͵�����ʱ��
	int last_send_keep_alive_time;



	//����������Ϣ����(����)
	map <int, RECV_MSG_MAP *> recv_msg_arr;
	//����������Ϣ����(����)
	map <int, SEND_MSG_MAP *> send_msg_arr;


    //����ϵͳ��Ϣ����
	//queue <SYSTEM_MSG_MAP *> recv_sys_msg_arr;//��Ϊ������ʷ��Ϣ
	//����ϵͳ��Ϣ����
	map <int, SYSTEM_MSG_MAP *> send_sys_msg_arr;

	pthread_mutex_lock c_lock;//��������


	CLIENT() {
		is_push_msg = 0;
		is_on_line = 0;
		//init vector
	}


	void clear_recv_msg_by_id(int msg_id)
	{
		RECV_MSG_MAP * rmm = recv_msg_arr.get(msg_id);
		delete rmm;
		recv_msg_arr.remove(msg_id);
	}
	RECV_MSG_MAP * check_recv_msg_map( int msg_id, int * has_init)
	{

		RECV_MSG_MAP * rmm = (RECV_MSG_MAP *)send_msg_arr.get(msg_id);
		*has_init = 1;
		if(rmm == NULL)
		{
			rmm = new RECV_MSG_MAP();
			//�����µ�
			send_msg_arr[msg_id] = rmm;
			*has_init = 0;
		}
		return rmm;
	}
	SYSTEM_MSG_MAP * get_send_sys_msg_by_id(int msg_id)
	{
		SYSTEM_MSG_MAP * smm = (SYSTEM_MSG_MAP *)send_sys_msg_arr.get(msg_id);
		return smm;
	}
	SEND_MSG_MAP * get_send_msg_by_id(int msg_id)
	{
		SEND_MSG_MAP * smm = (SEND_MSG_MAP *)send_msg_arr.get(msg_id);
		return smm;
	}
	SEND_MSG_MAP * get_send_msg_nx_by_id(int msg_id, int * has_init)//not exist
	{
		SEND_MSG_MAP * smm = (SEND_MSG_MAP *)send_msg_arr.get(msg_id);
		*has_init = 1;
		if(smm == NULL)
		{
			smm = new SEND_MSG_MAP();
			//�����µ�
			SEND_MSG_MAP[msg_id] = smm;
			*has_init = 0;
		}
		return smm;
	}

	void add_sys_msg_by_id(int msg_id, SYSTEM_MSG_MAP * smm)
	{
		//smm���뵽client��(�ǵü�����)��
		client->send_sys_msg_arr.set(smm->msg_id,smm);
	}



	void push_sys_msg_2_queue()
	{
		//ϵͳ��Ϣ���ݶ���
		/*
		 * 1.�������δ��ɣ�continue
		 * 2.���������ɣ���Ҫ����ack����δ�յ�ack�����·���(notify)
		 * 3.���������ɣ�����Ҫ����ack��ɾ��
		 * 4.���������ɣ��յ�ack��ɾ��
		 * */
		map <int, SYSTEM_MSG_MAP *>::iterator sys_smm_it;
		for(sys_smm_it=send_sys_msg_arr.begin(); sys_smm_it !=  send_sys_msg_arr.end(); ++sys_smm_it)
		{
			SYSTEM_MSG_MAP * sys_smm = sys_smm_it->second;
			//��δ���ͣ�����Ҫɾ��
			if(sys_smm->is_send_ok != 1)
			{
				continue;
			}
			//������ɣ�����û�յ�ack
			else  if(sys_smm->is_send_ok == 1 && sys_smm->is_recv_ack ==0  && sys_smm->order == NOTIFY)
			{
				//2s��û�ܵ�ack�� �ط�����Ϣ��
				if(9999 - sys_smm->last_send_msg_time > 2 )
				{
					SEND_MSG_POS * smp = malloc(sizeof(SEND_MSG_POS));
					smp->msg_id = sys_smm->msg_id;
					smp->name =  sys_smm->name;
					smp->type = SYS_MSG;

					//���뷢�Ͷ����У�Ϊȫ�ַ��Ͷ��м���
					pthread_lock(global_send_queue_mutex);
					global_send_queue.push(smp);
					//Ϊȫ�ַ��Ͷ��н���
					pthread_unlock(global_send_queue_mutex);
				}

			}
			//������ɣ�����Ҫ����ack,�����Ѿ��յ�ack
			else if((sys_smm->is_send_ok == 1 && sys_smm->is_recv_ack == 1)
					||
					((sys_smm->is_send_ok == 1 ) && (sys_smm->order == ACK||sys_smm->order == KEEP_ALIVE)))
			{
				send_sys_msg_arr.remove(sys_smm->msg_id);
				delete sys_smm;
			}

		}
	}
	void push_com_msg_2_queue()
	{

		//��client���������е��������ݷ���ȫ�ַ��Ͷ���,�ȴ�����
		 map<int,SEND_MSG_MAP *>::iterator it;
		 for(it=send_msg_arr.begin();it!=send_msg_arr.end();++it)
		 {
			 int msg_id = it->first;
			 SEND_MSG_MAP * smm = it->second;

			 //���Լ����
			 smm->clear_self();
			 //��������push�����Ͷ���
			 if(client->is_push_msg)
				 smm->push_2_queue();
			 //���ͽ���
			 if(smm->is_send == 1)
			 {
				 send_msg_arr.clear(smm->msg_id);
			 }

		 }
	}


} CLIENT;

//�ܹ�Ψһ��λ��Ϣid�Ľṹ��,����ȫ�ַ��Ͷ�����
typedef struct{
	char name[32];//�ͻ�������
	int  msg_id;//��Ϣid���
	MSG_TYPE  type;//��Ϣ���� SYS_MSG, COM_MSG
}SEND_MSG_POS;

pthread_metux_lock global_send_queue_lock;//ȫ�ַ��Ͷ��е���
QUEUE<SEND_MSG_POS *> global_send_queue;//ȫ�ַ��Ͷ���
pthread_metux_lock client_map_lock;//cient_map����
map<String, CLIENT *> client_map;//���client

//������Ϣ����

//�������
enum {
	LOG_IN = 1, LOG_OFF, SEND_MSG, NOTIFY, PUSH_MSG, PULL_MSG, KEEP_ALIVE, ACK, ORDER_NUM
} ORDER;

enum{
	SYS_MSG = 0,
	COM_MSG = 1
}MSG_TYPE;
//���ݰ�����״̬����
/*
 enum
 {
 RECV_START,
 RECV_HEAD,
 RECV_ORDER,
 RECV_LEN,
 RECV_MSGID,
 RECV_DATA,
 RECV_END,
 RECV_ERROR

 }RECV_PACKET_STATE;
 */
//��¼��Ϣ
//��ͷ+����+len+msg_id+from+to+data
//��Ϣ������
typedef struct {
	int head;
	ORDER order;
	int len;
	int msg_id;
	char from[16];
	char to[16];
	unsigned char data[0];
	void init(ORDER order, int len, int msg_id, char *��to)
	{
		pack.head = HEAD;
		pack.order = order;
		pack.len = sizeof(PACKET) + len;
		pack.from = "server";
		strcpy(pack.to, client->name);
		pack.msg_id = msg_id;
	}
} PACKET;

