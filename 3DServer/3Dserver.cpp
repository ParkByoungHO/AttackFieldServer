#include "stdafx.h"
#include"protocol.h"
#include"ServerPlayer.h"
#include"Timer.h"

CGameTimer g_GameTimer;

CLIENT client[MAX_USER];
CLIENT other[200];
PLAYER bullet[100];

bool g_isShutdown = false;
HANDLE g_hIocp;
CRITICAL_SECTION g_CriticalSection;
CRITICAL_SECTION timer_lock;
priority_queue<Event_timer, vector<Event_timer>, mycomparison> p_queue;

CServerPlayer serverplayer[10];

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
	packet.Hp = client[object].player.Hp;
	packet.Animation = client[object].player.Animation;



	//cout << object << endl;
	//ShowXMVector(packet.Animation);

	//cout << packet.x<< " " << packet.y<<" " << packet.z << endl;

	Sendpacket(id, &packet);
}

void sendbulletfire(int id, int object)
{
	sc_bullet_fire  packet;
	packet.id = object;
	packet.size = sizeof(packet);
	packet.type = SC_PUT_Bullet;

	packet.fire = client[object].player.fire;
	packet.FireDirection = client[object].player.FireDirecton;

	Sendpacket(id, &packet);
	
}

void SendLookPacket(int id, int object)
{
	sc_rotate_vector packet;
	packet.id = object;
	packet.size = sizeof(packet);
	packet.type = SC_ROTATE;
	packet.x = serverplayer[object].getfPitch();
	packet.y = serverplayer[object].getYaw();
	packet.z = 0;

	//cout << object << endl;
	//cout << packet.x << " " << packet.y << " " << packet.z << endl;

	Sendpacket(id, &packet);
}

void Timer_Thread()
{
	while (true)
	{
		g_GameTimer.Tick(60);

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

void SendPutPlayerPacket(int clients, int player)
{
	sc_packet_put_player packet;

	packet.id = player;
	packet.size = sizeof(packet);
	packet.type = SC_PUT_PLAYER;
	packet.x = client[player].player.x;
	packet.y = client[player].player.y;
	packet.z = client[player].player.z;
	packet.hp = client[player].player.Hp;
	packet.Animation = client[player].player.Animation;

	Sendpacket(clients, reinterpret_cast<unsigned char*>(&packet));
}

void processpacket(int id, unsigned char *packet)
{
	//��Ŷ �������� ó���� �޶�����.
	// 0 size 1 type
	cs_key_input *key_button;
	cs_rotate *rotate;

	BYTE packet_type = packet[1];
	switch (packet_type)	//Ű���� �޾����� ó�� ����� �Ѵ�.
	{
	case CS_KEY_TYPE:	//���⼭ Ű��ư�� �޾����� ó������� �Ѵ�.
		key_button = reinterpret_cast<cs_key_input *>(packet);
		// memcpy(&key_button, packet, packet[0]);
		client[id].player.x = key_button->x;
		client[id].player.y = key_button->y;
		client[id].player.z = key_button->z;
		client[id].player.Hp = key_button->Hp;

		client[id].player.button = key_button->key_button;
		client[id].player.FireDirecton = key_button->FireDirection;

		//cout << key_button->key_button << endl;
		
		client[id].vl_lock.lock();
		serverplayer[id].Setkey(client[id].player.button);
		//serverplayer[id].UpdateKeyInput(0.15);
		//serverplayer[id].Update(0.015);

		client[id].vl_lock.unlock();


		//client[id].player.x = serverplayer[id].Getd3dxvVelocity().x;
		//client[id].player.y = 
		//client[id].player.z = serverplayer[id].Getd3dxvVelocity().z;
		client[id].player.Animation = key_button->Animation;
		client[id].player.fire = serverplayer[id].Getfire();
		//cout << key_button.key_button<<endl;
		//cout << client[id].player.x << " " << client[id].player.y << " " << client[id].player.z << endl;

		for (int i = 0; i < MAX_USER; i++)
		{
			if (client[i].connected == true)
			{
				client[i].vl_lock.lock();
				SendPositionPacket(i, id);
				sendbulletfire(i, id);
				//SendLookPacket(i, id);
				client[i].vl_lock.unlock();


			}
		}

		//cout << serverplayer[id].Getd3dxvVelocity().x<<" "<< serverplayer[id].Getd3dxvVelocity().y<<" "<< serverplayer[id].Getd3dxvVelocity().z<<endl;
		client[id].player.button = 0;
		client[id].player.fire = false;
		serverplayer[id].setfire(false);
		serverplayer[id].Setkey(0);
		serverplayer[id].Setd3dxvVelocity(0, 0, 0);
		client[id].player.Animation = { 0,0,0 };
		break;
	case CS_ROTATE:		//cx cy�� �޾Ƽ� ������Ʈ ó���ؾ��Ѵ�.
		//memcpy(&rotate, packet, packet[0]);
		rotate = reinterpret_cast<cs_rotate *>(packet);
		//cout << rotate.cx << " " << rotate.cy << " " << rotate.cz<<endl;
		client[id].vl_lock.lock();
		serverplayer[id].Rotate(rotate->cx, rotate->cy);
		client[id].vl_lock.unlock();
		client[id].player.lookvector.x = serverplayer[id].GetLookvector().x;
		client[id].player.lookvector.y = serverplayer[id].GetLookvector().y;
		client[id].player.lookvector.z = serverplayer[id].GetLookvector().z;

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
		cout << "unknow packet : " << (int)packet[1]<<endl;
		break;

	}


	//cout << client[id].player.x << " " << client[id].player.y << " " << client[id].player.z << endl;
	//cout << client[id].player.Bulletlist->x << " " << client[id].y << " " << client[id].z << endl;



	
	


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

		//���ο� ���̵� �Ҵ�
		//static int new_id = -1;
		//for (int i = 0; i < MAX_USER; ++i)
		//{
		//	if (client[i].connected == false)
		//	{
		//		new_id = i;
		//		break;
		//	}
		//}

		static int new_id = 0;

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
			client[new_id].player.x = 65;
			client[new_id].player.y = 2;
			client[new_id].player.z = 25;
		}
		client[new_id].player.Hp = 100;

		client[new_id].player.Animation = { 0,0,0 };
		client[new_id].recv_overlap.operation = OP_RECV;
		client[new_id].recv_overlap.packet_size = 0;
		client[new_id].previous_data_size = 0;
		client[new_id].player.FireDirecton = { 0,0,0 };

		LeaveCriticalSection(&g_CriticalSection);


		//���⿡ ��Ƽ� ������.
		sc_packet_put_player put_player_packet;
		put_player_packet.id = new_id;
		put_player_packet.size = sizeof(put_player_packet);
		put_player_packet.type = SC_PUT_PLAYER;
		put_player_packet.x = client[new_id].player.x;
		put_player_packet.y = client[new_id].player.y;
		put_player_packet.z = client[new_id].player.z;
		put_player_packet.hp = client[new_id].player.Hp;
		put_player_packet.Animation = client[new_id].player.Animation;

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

			new_id++;
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
	thread* pTimerThread;

	vector<thread*> vpThread;

	Initialize_server();

	SYSTEM_INFO sys_info;

	GetSystemInfo(&sys_info);


	for (int i = 0; i < sys_info.dwNumberOfProcessors * 2; ++i)
	{
		vpThread.push_back(new thread(worker_Thread));

	}

	pAcceptThread = new thread(Accept_thread);
	pTimerThread = new thread(Timer_Thread);


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