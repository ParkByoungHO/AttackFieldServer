#include<WinSock2.h>
#include<windows.h>
#include<iostream>
#include<thread>
#include<mutex>
#include<list>
#include<set>
#include<queue>
#include<vector>

#include"protocol.h"
#include"ServerPlayer.h"
#include"Timer.h"

#define SERVERPORT		9000

#pragma comment(lib,"Ws2_32")

#define OP_RECV					1
#define OP_SEND					2
#define OP_MOVE					3

#define		SC_POS				1
#define		SC_PUT_PLAYER		2
#define		SC_PUT_Bullet		3
#define		SC_REMOVE_PLAYER	4
#define		SC_ROTATE			5

#define MAX_USER				10

#define MAX_BUFFSIZE			4000



using namespace std;

void Sendpacket(int id, void * packet);

void error_display(char *msg, int err_num)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_num,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s", msg, (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

struct vector3 {
	float x;
	float y;
	float z;


	vector3(int x, int y, int z) :x(x), y(y), z(z) {  };
	vector3() {};
	vector3 operator * (float f)
	{
		return vector3(f * x, f*y, f * z);
	}

};

struct PLAYER {		//�÷��̾� ��ǥ.

	int x;
	int y;
	int z;

	DWORD button;
	vector3 lookvector;

};



struct Event_timer {		//Ÿ�̸�

	int obj_id;
	unsigned wakeup_time;
	int event_id;

};

class mycomparison
{
	bool reserve;
public:
	bool operator() (const Event_timer lhs, const Event_timer rhs) const
	{
		return (lhs.wakeup_time > rhs.wakeup_time);
	}

};

typedef struct Overlapex {
	WSAOVERLAPPED	original_overlap;
	int				operation;
	WSABUF			recv_buffer;
	BYTE			socket_buff[MAX_BUFFSIZE];
	int				packet_size;

};


//Ŭ���̾�Ʈ�� Ȯ�� ������ ����ü�� ������ �Ѵ�.
struct CLIENT {
	int				id;
	bool			connected;
	SOCKET			sock;
	PLAYER			player;
	Overlapex		recv_overlap;
	int				previous_data_size;
	mutex			vl_lock;
	BYTE			packet[MAX_BUFFSIZE];
	bool			Team = false;

};

CLIENT client[MAX_USER];
CLIENT other[200];
PLAYER bullet[100];


bool g_isShutdown = false;
HANDLE g_hIocp;
CRITICAL_SECTION g_CriticalSection;
CRITICAL_SECTION timer_lock;
priority_queue<Event_timer, vector<Event_timer>, mycomparison> p_queue;

CServerPlayer serverplayer[10];


void add_timer(int obj_id, int m_sec, int event_type)
{
	Event_timer event_object = { obj_id, m_sec + GetTickCount(), event_type };
	EnterCriticalSection(&timer_lock);
	p_queue.push(event_object);
	LeaveCriticalSection(&timer_lock);

}

void SendPositionPacket(int id, int object)
{
	sc_packet_pos packet;
	packet.id = object;
	packet.size = sizeof(packet);
	packet.type = SC_POS;
	packet.x = client[object].player.x;
	packet.y = client[object].player.y;
	packet.z = client[object].player.z;

	cout << packet.x<< " " << packet.y<<" " << packet.z << endl;

	Sendpacket(id, &packet);
}

void SendLookPacket(int id, int object)
{
	sc_rotate_vector packet;
	packet.id = object;
	packet.size = sizeof(packet);
	packet.type = SC_ROTATE;
	packet.matrix = serverplayer[id].GetMatrix();


	Sendpacket(id, &packet);
}

void Timer_Thread()
{
	while (true)
	{
		EnterCriticalSection(&timer_lock);
		while (!p_queue.empty())
		{
			Event_timer top_object = p_queue.top();

			if (top_object.wakeup_time > GetTickCount())
				break;

			p_queue.pop();

			Overlapex *overlapped = new Overlapex;
			overlapped->operation = OP_MOVE;
			ZeroMemory(&overlapped->original_overlap, sizeof(overlapped->original_overlap));
			PostQueuedCompletionStatus(g_hIocp, 1, top_object.obj_id, reinterpret_cast<LPOVERLAPPED>(&overlapped));

		}
		LeaveCriticalSection(&timer_lock);
	}

}

void Sendpacket(int id, void * packet)
{
	//���������� ������� �޸� �Ҵ��� ����� �Ѵ�.
	Overlapex* send_over = new Overlapex;
	memset(send_over, 0, sizeof(Overlapex));
	send_over->operation = OP_SEND;
	send_over->recv_buffer.buf = reinterpret_cast<char *>(send_over->socket_buff);
	send_over->recv_buffer.len = reinterpret_cast<unsigned char *>(packet)[0];
	memcpy(send_over->socket_buff, packet, reinterpret_cast<unsigned char *>(packet)[0]);

	int result = WSASend(client[id].sock, &send_over->recv_buffer, 1, NULL, 0, &send_over->original_overlap, NULL);
	if ((result != 0) && (WSA_IO_PENDING != result))
	{
		int error_num = WSAGetLastError();

		if (WSA_IO_PENDING != error_num)
			error_display("sendpacket : wsasend", error_num);
		while (true);
	}

}

void SendRemovePacket(int client, int object)
{
	sc_packet_remove_player remove_player;

	remove_player.id = client;
	remove_player.size = sizeof(remove_player);
	remove_player.type = SC_REMOVE_PLAYER;

	Sendpacket(client, &remove_player);
}

void Disconnected(int ci)
{
	closesocket(client[ci].sock);
	client[ci].connected = false;
	for (int i = 0; i < MAX_USER; i++)
	{
		if (client[i].connected == true)
			SendRemovePacket(ci, i);
	}
}

void Initialize_server()
{
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);

	InitializeCriticalSection(&g_CriticalSection);
	InitializeCriticalSection(&timer_lock);

	g_hIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);	//ó�� �����Ҷ� �̷��� ����

	for (int i = 0; i < MAX_USER; ++i)
	{
		client[i].recv_overlap.recv_buffer.buf = reinterpret_cast<char *>(client[i].recv_overlap.socket_buff);
		client[i].recv_overlap.recv_buffer.len = 10000;
		client[i].recv_overlap.operation = OP_RECV;

		client[i].connected = false;
	}

}




void SendMovePacket(int client, int npcID)
{
	sc_packet_pos  packet;

	packet.id = 500 + npcID;
	packet.size = sizeof(sc_packet_pos);
	packet.type = SC_POS;
	packet.x = other[npcID].player.x;
	packet.y = other[npcID].player.y;
	packet.z = other[npcID].player.z;
	Sendpacket(client, reinterpret_cast<unsigned char*>(&packet));

	//cout << packet.id << endl;
}

void SendPutPlayerPacket(int clients, int player)
{
	sc_packet_put_player packet;

	packet.id = player;
	packet.size = sizeof(packet);
	packet.type = SC_PUT_PLAYER;
	packet.x = client[player].player.x;
	packet.y = client[player].player.y;
	packet.z = client[player].player.z;

	Sendpacket(clients, reinterpret_cast<unsigned char*>(&packet));
}



void processpacket(int id, unsigned char *packet)
{
	//��Ŷ �������� ó���� �޶�����.
	// 0 size 1 type
	cs_key_input key_button;
	cs_rotate rotate;

	BYTE packet_type = packet[1];
	switch (packet_type)	//Ű���� �޾����� ó�� ����� �Ѵ�.
	{
	case CS_KEY_TYPE:	//���⼭ Ű��ư�� �޾����� ó������� �Ѵ�.
		memcpy(&key_button, packet, packet[0]);
		//client[id].player.x = key_button.x;
		//client[id].player.y = key_button.y;
		//client[id].player.z = key_button.z;
		//client[id].player.button = key_button.key_button;
		client[id].player.button = key_button.key_button;
		client[id].vl_lock.lock();
		serverplayer[id].Move(client[id].player.button, 1, false);
		client[id].vl_lock.unlock();
		client[id].player.x = serverplayer[id].GetPosition().x;
		client[id].player.y = 2;//serverplayer.GetPosition().y;
		client[id].player.z = serverplayer[id].GetPosition().z;
		//cout << key_button.key_button<<endl;
		cout << client[id].player.x << " " << client[id].player.y << " " << client[id].player.z << endl;

		for (int i = 0; i < MAX_USER; i++)
		{
			if (client[i].connected == true)
			{
				client[i].vl_lock.lock();
				SendPositionPacket(i, id);
				//SendLookPacket(i, id);
				client[i].vl_lock.unlock();
			}
		}
		client[id].player.button = 0;
		break;
	case CS_ROTATE:		//cx cy�� �޾Ƽ� ������Ʈ ó���ؾ��Ѵ�.
		memcpy(&rotate, packet, packet[0]);
		//cout << rotate.cx << " " << rotate.cy << " " << rotate.cz<<endl;
		serverplayer[id].Rotate(rotate.cx, rotate.cy, 0);

		for (int i = 0; i < MAX_USER; i++)
		{
			if (client[i].connected == true)
			{
				client[i].vl_lock.lock();
				SendLookPacket(i, id);
				client[i].vl_lock.unlock();
			}
		}
		break;
	case 3:		//���� ������ ó���� �ؾ��Ѵ�.
		break;
	default:
		cout << "unknow packet : " << (int)packet[1];
		break;

	}


	//cout << client[id].player.x << " " << client[id].player.y << " " << client[id].player.z << endl;
	//cout << client[id].player.Bulletlist->x << " " << client[id].y << " " << client[id].z << endl;




}

CTimer timer1;



void Accept_thread()
{
	sockaddr_in listen_addr;
	SOCKET accept_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	ZeroMemory(&listen_addr, sizeof(listen_addr));
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	listen_addr.sin_port = htons(SERVERPORT);
	ZeroMemory(&listen_addr.sin_zero, 8);

	::bind(accept_socket, reinterpret_cast<SOCKADDR *>(&listen_addr), sizeof(listen_addr));

	listen(accept_socket, 10);

	while (true)
	{
		sockaddr_in client_addr;

		int add_size = sizeof(client_addr);

		SOCKET new_client = ::WSAAccept(accept_socket, reinterpret_cast<SOCKADDR *>(&client_addr), &add_size, NULL, NULL);

		//���ο� ���̵� �Ҵ�
		int new_id = -1;
		for (int i = 0; i < MAX_USER; ++i)
		{
			if (client[i].connected == false)
			{
				new_id = i;
				break;
			}
		}

		if (new_id == -1)
		{
			cout << "server is full\n";
			closesocket(new_client);
			continue;
		}

		cout << "Player " << new_id << " Connected " << endl;

		EnterCriticalSection(&g_CriticalSection);
		// ��Ȱ�� �� �����̹Ƿ� �ʱ�ȭ���־�� �Ѵ�.
		client[new_id].connected = true;
		client[new_id].sock = new_client;
		client[new_id].id = new_id;
		serverplayer[new_id].setid(new_id);

		if (client[new_id].id % 2 == 0)
			client[new_id].Team = true;


		// DB���� ������ �α׾ƿ� �� ��ġ�� �ٽ� ������
		if (client[new_id].Team)
		{
			client[new_id].player.x = 65;
			client[new_id].player.y = 2;
			client[new_id].player.z = 12;
		}
		else
		{
			client[new_id].player.x = 265;
			client[new_id].player.y = 2;
			client[new_id].player.z = 230;
		}
		client[new_id].recv_overlap.operation = OP_RECV;
		client[new_id].recv_overlap.packet_size = 0;
		client[new_id].previous_data_size = 0;
		LeaveCriticalSection(&g_CriticalSection);


		//���⿡ ��Ƽ� ������.
		sc_packet_put_player put_player_packet;
		put_player_packet.id = new_id;
		put_player_packet.size = sizeof(put_player_packet);
		put_player_packet.type = SC_PUT_PLAYER;
		put_player_packet.x = client[new_id].player.x;
		put_player_packet.y = client[new_id].player.y;
		put_player_packet.z = client[new_id].player.z;

		//����� ��Ʈ�� Ŭ���̾�Ʈ ����
		CreateIoCompletionPort(reinterpret_cast<HANDLE>(new_client), g_hIocp, new_id, 0);


		DWORD recv_flag = 0;
		WSARecv(new_client, &client[new_id].recv_overlap.recv_buffer, 1,
			NULL, &recv_flag, &client[new_id].recv_overlap.original_overlap, NULL);

		SendPutPlayerPacket(new_id, new_id);

		for (int i = 0; i < MAX_USER; ++i)
		{
			if (client[i].connected == true)
			{
				if (i != new_id)
				{
					SendPutPlayerPacket(new_id, i);
					SendPutPlayerPacket(i, new_id);
				}
			}
		}

		

		DWORD flags = 0;
		int result = WSARecv(new_client, &client[new_id].recv_overlap.recv_buffer, 1, NULL, &flags, &client[new_id].recv_overlap.original_overlap, NULL);

		if (0 != result) {
			int error_num = WSAGetLastError();
			if (WSA_IO_PENDING != error_num) {
				error_display("AcceptThread : WSARecv ", error_num);
			}

		}
	}
}



void worker_Thread()
{
	DWORD io_size;
	unsigned long long key;
	Overlapex *overlap;
	bool bresult;

	while (true)
	{
		bresult = GetQueuedCompletionStatus(g_hIocp, &io_size, &key, reinterpret_cast<LPOVERLAPPED*>(&overlap), INFINITE);

		if (false == bresult)
		{
			std::cout << "Error in GQCS\n";
			int err_no = WSAGetLastError();
			if (err_no == 64) Disconnected(key);
			while (true);
		}
		if (0 == io_size) {
			Disconnected(key);
			continue;
		}
		
		if (overlap != NULL)
		{
			switch (overlap->operation)
			{
			case OP_RECV:
			{
				BYTE* pBuff = overlap->socket_buff;
				int remained = io_size;

				//���� ������ �����ŭ ��ȸ�ϸ鼭 ó��
				while (0 < remained)
				{
					if (client[key].recv_overlap.packet_size == 0)
					{
						client[key].recv_overlap.packet_size = pBuff[0];
					}
					int required = client[key].recv_overlap.packet_size - client[key].previous_data_size;

					//��Ŷ�ϼ�
					if (remained >= required)
					{
						//�������� ���� ������ �޺κп� ����
						memcpy(client[key].packet + client[key].previous_data_size, pBuff, required);
						processpacket(key, reinterpret_cast<BYTE *>(&client[key].packet));
						remained -= required;
						pBuff += required;
						client[key].recv_overlap.packet_size = 0;
						client[key].previous_data_size = 0;

					}
					else
					{
						memcpy(client[key].packet + client[key].previous_data_size, pBuff, remained);
						//�̿ϼ� ��Ŷ�� ����� reamined��ŭ ����
						client[key].previous_data_size += remained;
						remained = 0;
						pBuff++;
					} 

				}
				DWORD flags = 0;
				WSARecv(client[key].sock, &client[key].recv_overlap.recv_buffer, 1, NULL, &flags, reinterpret_cast<LPWSAOVERLAPPED>(&client[key].recv_overlap), NULL);

				break;
			}
			case OP_SEND:
				delete overlap;
				break;
			case OP_MOVE:
				for (int i = 0; i < 10; i++)
				{

					if (client[i].connected == false) continue;
					client[i].vl_lock.lock();
				//	SendMovePacket(i, key - 500);
					client[i].vl_lock.unlock();
				}
				add_timer(key, 1000, OP_MOVE);
				break;
			default:
				cout << overlap->operation;
				cout << "Unknown Event on Worker_Thread" << endl;
				while (true);
				break;
			}

		}

	}


}


int main(int argv, char* argc[])
{

	//timer1.SetTimer(20);

	thread* pAcceptThread;
	//thread* pTimerThread;

	vector<thread*> vpThread;

	Initialize_server();

	SYSTEM_INFO sys_info;

	GetSystemInfo(&sys_info);


	for (int i = 0; i < sys_info.dwNumberOfProcessors * 2; ++i)
	{
		vpThread.push_back(new thread(worker_Thread));

	}

	pAcceptThread = new thread(Accept_thread);
	//pTimerThread = new thread(Timer_Thread);


	while (g_isShutdown == false) {
		Sleep(1000);
	}



	for (thread* pThread : vpThread) {
		pThread->join();
		delete pThread;
	}

	pAcceptThread->join();
	delete pAcceptThread;

	//pTimerThread->join();
	//delete pTimerThread;



	DeleteCriticalSection(&g_CriticalSection);
	DeleteCriticalSection(&timer_lock);

	WSACleanup();
}