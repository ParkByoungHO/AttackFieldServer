
#include<WinSock2.h>
#include<windows.h>
#include<iostream>
#include<thread>
#include<mutex>
#include<list>
#include<set>
#include<queue>
#include<vector>

#define DIR_FORWARD		0x01
#define DIR_BACKWARD	0x02
#define DIR_UP			0x10
#define DIR_DOWN		0x20
#define DIR_LEFT		0x04
#define DIR_RIGHT		0x08

#define SERVERPORT		9000

#pragma comment(lib,"Ws2_32")

#define OP_RECV				1
#define OP_SEND				2
#define OP_MOVE				3

#define		SC_POS			1
#define		SC_PUT_PLAYER	2


#define MAX_USER			10

#define MAX_BUFFSIZE		4000



using namespace std;

struct sc_packet_put_player {
	BYTE size;
	BYTE type;
	WORD id;
	int x;
	int y;
	int z;

};

struct sc_packet_pos
{
	BYTE size;
	BYTE type;
	WORD id;
	int x;
	int y;
	int z;
};

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

struct PLAYER {		//플레이어 좌표.
	float x;
	float y;
	float z;

};

struct Event_timer {		//타이머

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
	unsigned char	socket_buff[MAX_BUFFSIZE];
	int				packet_size;

};
//클라이언트는 확장 오버렙 구조체를 가져야 한다.
struct CLIENT {
	int				id;
	bool			connected;
	SOCKET			sock;
	PLAYER			player;
	Overlapex		recv_overlap;
	int				previous_data_size;
	mutex			vl_lock;
	unsigned char	packet[MAX_BUFFSIZE];

};

CLIENT client[MAX_USER];
CLIENT other[200];
bool g_isShutdown = false;
HANDLE g_hIocp;
CRITICAL_SECTION g_CriticalSection;
CRITICAL_SECTION timer_lock;
priority_queue<Event_timer, vector<Event_timer>, mycomparison> p_queue;

void add_timer(int obj_id, int m_sec, int event_type)
{
	Event_timer event_object = { obj_id, m_sec + GetTickCount(), event_type };
	EnterCriticalSection(&timer_lock);
	p_queue.push(event_object);
	LeaveCriticalSection(&timer_lock);
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

void Create_NPC()
{
	int i = 0;
	EnterCriticalSection(&timer_lock);
	for (auto& npc : other)
	{
		npc.player.x = 0;
		npc.player.y = 0;
		npc.player.z = 0;

		npc.id = 500 + (i++);
		add_timer(npc.id, 1000, OP_MOVE);
	}
	LeaveCriticalSection(&timer_lock);
}


void Initialize_server()
{
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);

	InitializeCriticalSection(&g_CriticalSection);
	InitializeCriticalSection(&timer_lock);

	g_hIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);	//처음 선언할때 이렇게 선언

	for (int i = 0; i < MAX_USER; ++i)
	{
		client[i].recv_overlap.recv_buffer.buf = reinterpret_cast<char *>(client[i].recv_overlap.socket_buff);
		client[i].recv_overlap.recv_buffer.len = MAX_BUFFSIZE;
		client[i].recv_overlap.operation = OP_RECV;

		client[i].connected = false;
	}

	Create_NPC();

}


void Sendpacket(int id, unsigned char* packet)
{
	//지역변수로 하짐라고 메모리 할당을 해줘야 한다.
	Overlapex* send_over = new Overlapex;
	memset(send_over, 0, sizeof(Overlapex));
	send_over->operation = OP_SEND;
	send_over->recv_buffer.buf = reinterpret_cast<char *>(send_over->socket_buff);
	send_over->recv_buffer.len = packet[0];
	memcpy(send_over->socket_buff, packet, packet[0]);

	int result = WSASend(client[id].sock, &send_over->recv_buffer, 1, NULL, 0, &send_over->original_overlap, NULL);
	if ((result != 0) && (WSA_IO_PENDING != result))
	{
		int error_num = WSAGetLastError();

		if (WSA_IO_PENDING != error_num)
			error_display("sendpacket : wsasend", error_num);
		while (true);
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
	//패킷 종류별로 처리가 달라진다.
	// 0 size 1 type

	//int dx = client[id].player.x;
	//int dy = client[id].player.y;
	//int dz = client[id].player.z;

	int dx = 0;
	int dy = 0;
	int dz = 0;

	unsigned char packet_type = packet[1];

	//switch (packet_type)	//키값을 받았을때 처리 해줘야 한다.
	//{
	//case DIR_FORWARD:		dz++; break;
	//case DIR_BACKWARD:		dz--; break;
	//case DIR_LEFT:			dx--; break;
	//case DIR_RIGHT:			dx++; break;
	//case 9:					dx++; dz++; break;
	//case 10:				dx++; dz--; break;
	//case 5:					dx--; dz++; break;
	//case 6:					dx--; dz--; break;
	//case DIR_UP:					break;
	//case DIR_DOWN:					break;
	//case 0:							break;
	//
	////default: {
	////	cout << "Unknown Packet Type Detected!!\n";
	////	cout << packet_type << endl;
	////	exit(-1);
	////}
	//}


	cout << (FLOAT)packet_type << endl;

	//이동처리
	client[id].player.x = (packet[1]);

	sc_packet_pos pos_packet;
	pos_packet.id = id;
	pos_packet.size = sizeof(sc_packet_pos);
	pos_packet.type = SC_POS;

	//pos_packet.x = dx;
	//pos_packet.y = dy;
	//pos_packet.z = dz;

	cout << client[id].player.x << "	" << client[id].player.y << " " << client[id].player.z << endl;

	Sendpacket(id, reinterpret_cast<unsigned char*>(&pos_packet));

}

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

		//새로운 아이디 할당
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

		//입출력 포트와 클라이언트 연결
		CreateIoCompletionPort(reinterpret_cast<HANDLE>(new_client), g_hIocp, new_id, 0);

		cout << "Player " << new_id << " Connected " << endl;

		EnterCriticalSection(&g_CriticalSection);
		// 재활용 될 소켓이므로 초기화해주어야 한다.
		client[new_id].sock = new_client;
		client[new_id].connected = true;
		client[new_id].id = new_id;

		// DB에서 이전에 로그아웃 한 위치로 다시 재접속
		client[new_id].player.x = 0;
		client[new_id].player.y = 0;
		client[new_id].player.z = 0;
		client[new_id].recv_overlap.operation = OP_RECV;
		client[new_id].recv_overlap.packet_size = 0;
		client[new_id].previous_data_size = 0;
		LeaveCriticalSection(&g_CriticalSection);


		//여기에 담아서 보낸다.
		sc_packet_put_player put_player_packet;
		put_player_packet.id = new_id;
		put_player_packet.size = sizeof(put_player_packet);
		put_player_packet.type = SC_PUT_PLAYER;
		put_player_packet.x = client[new_id].player.x;
		put_player_packet.y = client[new_id].player.y;
		put_player_packet.z = client[new_id].player.z;

		for (int i = 0; i < MAX_USER; ++i)
		{
			if (client[i].connected == true)
			{
				Sendpacket(i, reinterpret_cast<unsigned char*>(&put_player_packet));
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
	DWORD key;
	Overlapex *overlap;
	bool bresult;

	while (true)
	{
		bresult = GetQueuedCompletionStatus(g_hIocp, &io_size, &key, reinterpret_cast<LPOVERLAPPED*>(&overlap), INFINITE);

		if (false == bresult)
		{
			//에러처리 하면된다.
		}
		if (overlap != NULL)
		{
			switch (overlap->operation)
			{
			case OP_RECV:
			{
				unsigned char* pBuff = overlap->socket_buff;
				int remained = io_size;

				//남은 데이터 사이즈만큼 순회하면서 처리
				while (0 < remained)
				{
					if (client[key].recv_overlap.packet_size == 0)
					{
						client[key].recv_overlap.packet_size = pBuff[0];
					}
					int required = client[key].recv_overlap.packet_size - client[key].previous_data_size;

					//패킷완성
					if (remained >= required)
					{
						//지난번에 받은 데이터 뒷부분에 복사
						memcpy(client[key].packet + client[key].previous_data_size, pBuff, required);
						processpacket(key, reinterpret_cast<unsigned char *>(&client[key].packet));
						remained -= required;
						pBuff += required;
						client[key].recv_overlap.packet_size = 0;
						client[key].previous_data_size = 0;

					}
					else
					{
						memcpy(client[key].packet + client[key].previous_data_size, pBuff, remained);
						//미완성 패킷의 사이즈가 reamined만큼 증가
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
					SendMovePacket(i, key - 500);
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
	thread* pAcceptThread;
	thread* pTimerthread;
	vector<thread*> vpThread;

	Initialize_server();

	SYSTEM_INFO sys_info;

	GetSystemInfo(&sys_info);


	for (int i = 0; i < sys_info.dwNumberOfProcessors * 2; ++i)
	{
		vpThread.push_back(new thread(worker_Thread));

	}

	pAcceptThread = new thread(Accept_thread);
	pTimerthread = new thread(Timer_Thread);

	while (g_isShutdown == false) {
		Sleep(1000);
	}



	for (thread* pThread : vpThread) {
		pThread->join();
		delete pThread;
	}

	pAcceptThread->join();
	delete pAcceptThread;

	pTimerthread->join();
	delete pTimerthread;

	DeleteCriticalSection(&g_CriticalSection);
	DeleteCriticalSection(&timer_lock);

	WSACleanup();
}