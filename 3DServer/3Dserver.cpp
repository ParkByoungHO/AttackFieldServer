#include "stdafx.h"
#include"protocol.h"
#include"ServerPlayer.h"
#include"Timer.h"

CGameTimer g_GameTimer;

CLIENT client[MAX_USER];
CLIENT other[200];

bool g_isShutdown = false;
HANDLE g_hIocp;
CRITICAL_SECTION g_CriticalSection;
CRITICAL_SECTION timer_lock;
priority_queue<Event_timer, vector<Event_timer>, mycomparison> p_queue;

BYTE Goal_Kill = 100;
BYTE Red_Kill = 0;
BYTE Blue_kill = 0;
BOOL Timer = false;





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

void Sendpacket(int id, void* packet)
{
	//지역변수로 하짐라고 메모리 할당을 해줘야 한다.
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

	//Sendpacket(client, &remove_player);
}

void add_timer(int obj_id, int m_sec, int event_type)
{
	Event_timer event_object = { obj_id, m_sec + GetTickCount(), event_type };
	EnterCriticalSection(&timer_lock);
	p_queue.push(event_object);
	LeaveCriticalSection(&timer_lock);

}

void SendTimerpacket(int id)	
{
	SC_Starting_Timer packet;

	packet.size = sizeof(SC_Starting_Timer);
	packet.type = 9;
	packet.Starting_timer = client[id].Starting_Time;

	Sendpacket(id, reinterpret_cast<unsigned char *>(&packet));
}


void SendPositionPacket(int id, int object)
{
	sc_packet_pos packet;
	packet.id = object;
	packet.size = sizeof(packet);
	packet.type = SC_POS;
	packet.x = client[object].player.GetPosition().x;
	packet.y = client[object].player.GetPosition().y;
	packet.z = client[object].player.GetPosition().z;
	packet.Hp = client[object].player.GetPlayerHp();
	packet.Animation = client[object].player.GetAnimation();





	Sendpacket(id, &packet);
}

void sendbulletfire(int id, int object)
{
	sc_bullet_fire  packet;
	packet.id = object;

	packet.size = sizeof(packet);
	packet.type = SC_PUT_Bullet;

	packet.fire = client[object].player.Getfire();
	XMStoreFloat3(&packet.FireDirection , client[object].player.GetLook());

	Sendpacket(id, (&packet));
	
}

void sendReload(int id, int object)
{
	sc_Reload packet;
	packet.id = object;

	packet.size = sizeof(packet);
	packet.type = SC_RELOAD;

	packet.reload = client[object].player.GetReload();

	Sendpacket(id, &packet);

}

void SendLookPacket(int id, int object)
{
	sc_rotate_vector packet;
	packet.id = object;

	packet.size = sizeof(packet);
	packet.type = SC_ROTATE;

	packet.x = client[object].player.getfPitch();
	packet.y = client[object].player.getYaw();
	packet.z = 0;

	Sendpacket(id,(&packet));
}

void SendCollisonPacket(int id, int object, bool collision, XMFLOAT3 position, XMFLOAT3 direction)
{
	SC_Collison packet;
	packet.size = sizeof(SC_Collison);
	packet.id = object;
	packet.type = SC_ColliSion;
	packet.collision = collision;
	packet.position = position;
	packet.direction = direction;

	Sendpacket(id, (&packet));
}

void Timer_Thread()
{
	do
	{
		Sleep(1);

		while (!p_queue.empty())
		{
			Event_timer top_object = p_queue.top();

			if (top_object.wakeup_time > GetTickCount())
				break;

			EnterCriticalSection(&timer_lock);
			p_queue.pop();
			LeaveCriticalSection(&timer_lock);
			Overlapex *overlapped = new Overlapex;
			overlapped->operation = top_object.event_type;
			ZeroMemory(&overlapped->original_overlap, sizeof(overlapped->original_overlap));
			PostQueuedCompletionStatus(g_hIocp, 1, top_object.obj_id , *(reinterpret_cast<LPOVERLAPPED *>(&overlapped)));
		

		} 

	} while (true);

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

	g_hIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0);	//처음 선언할때 이렇게 선언

	for (int i = 0; i < MAX_USER; ++i)
	{
		client[i].recv_overlap.recv_buffer.buf = reinterpret_cast<char *>(client[i].recv_overlap.socket_buff);
		client[i].recv_overlap.recv_buffer.len = MAX_BUFFSIZE;
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
	packet.x = client[player].player.GetPosition().x;
	packet.y = client[player].player.GetPosition().y;
	packet.z = client[player].player.GetPosition().z;
	packet.hp = client[player].player.GetPlayerHp();
	packet.Animation = client[player].player.GetAnimation();
	packet.Goal = Goal_Kill;
	packet.RED = Red_Kill;
	packet.Blue = Blue_kill;

	Sendpacket(clients, (&packet));
}

void SendSystemPacket(int clients)	//타이머
{
	SC_System_kill packet;

	packet.type = SC_SYSTEM;
	packet.size = sizeof(packet);
	packet.RED = Red_Kill;
	packet.BLUE = Blue_kill;

	Sendpacket(clients,(&packet));
}



void SendPlayerHppacket(int clients, int player, bool Head)	//타이머
{
	SC_Player_Hp packet;

	packet.type = SC_PUT_HP;
	packet.size = sizeof(packet);
	packet.Hp = client[player].player.GetPlayerHp();
	packet.id = player;
	packet.Head = Head;
	packet.live = client[player].player.Getlife();
	

	Sendpacket(clients,(&packet));
}

void processpacket(int id, unsigned char *packet)
{
	//패킷 종류별로 처리가 달라진다.
	// 0 size 1 type

	BYTE packet_type = packet[1];
	switch (packet_type)	//키값을 받았을때 처리 해줘야 한다.
	{
	case CS_KEY_TYPE:	//여기서 키버튼을 받았을때 처리해줘야 한다.
	{
		cs_key_input *key_button;
		key_button = reinterpret_cast<cs_key_input *>(packet);
		XMFLOAT3 Temp(key_button->x, key_button->y, key_button->z);
		client[id].vl_lock.lock();
		client[id].player.SetPosition(Temp);
		client[id].player.Setkey(key_button->key_button);
		client[id].player.SetFireDirection(key_button->FireDirection);
		client[id].player.SetAnimation(key_button->Animation);
		client[id].vl_lock.unlock();
		//cout << key_button->key_button << endl;

	

		//client[id].player.x = serverplayer[id].Getd3dxvVelocity().x;
		//client[id].player.y = 
		//client[id].player.z = serverplayer[id].Getd3dxvVelocity().z;

		//cout << key_button.key_button<<endl;
		//cout << client[id].player.x << " " << client[id].player.y << " " << client[id].player.z << endl;

		for (int i = 0; i < MAX_USER; i++)
		{
			if (client[i].connected == true)
			{

				SendPositionPacket(i, id);
				if(client[i].player.Getfire())
					sendbulletfire(i, id);
				if (client[i].player.GetReload())
					sendReload(i, id);
			}
		}

		//cout << serverplayer[id].Getd3dxvVelocity().x<<" "<< serverplayer[id].Getd3dxvVelocity().y<<" "<< serverplayer[id].Getd3dxvVelocity().z<<endl;
		client[id].vl_lock.lock();
		client[id].player.Setkey(0);
		client[id].player.setfire(false);
		client[id].player.setreload(false);
		client[id].player.Setd3dxvVelocity(0, 0, 0);
		client[id].player.SetAnimation(XMFLOAT3(0,0,0));
		client[id].vl_lock.unlock();
		break;
	}
	case CS_ROTATE:		//cx cy를 받아서 로테이트 처리해야한다.
	{
		cs_rotate *rotate;
		//memcpy(&rotate, packet, packet[0]);
		rotate = reinterpret_cast<cs_rotate *>(packet);
		//cout << rotate.cx << " " << rotate.cy << " " << rotate.cz<<endl;
		client[id].vl_lock.lock();
		client[id].player.Rotate(rotate->cx, rotate->cy);
		client[id].vl_lock.unlock();

		for (int i = 0; i < MAX_USER; i++)
		{
			if (client[i].connected == true)
			{
				SendLookPacket(i, id);
			}
		}
		break;
	}
	case CS_WEAPONE:
	{
		cs_weapon *weapon;
		weapon = reinterpret_cast<cs_weapon *>(packet);
		CollisionInfo info;
		bool isCollisionSC = false;

		int i = 0;
		
		for (auto &character : COLLISION_MGR->m_vecCharacterContainer)
		{
		
			client[i].vl_lock.lock();
			character->SetWorldMatirx(client[i].player.GetWorldMatrix());
			client[i].vl_lock.unlock();
			//ShowXMMatrix(character->GetWorldMatrix());
			i++;
		}
	
		isCollisionSC = COLLISION_MGR->RayCastCollisionToCharacter(info, XMLoadFloat3(&weapon->position), XMLoadFloat3(&weapon->direction));
			
		if (isCollisionSC) {
		
				if(client[id].Red_Team != client[info.m_nObjectID].Red_Team && !client[info.m_nObjectID].player.Getlife())
					SendCollisonPacket(info.m_nObjectID, info.m_nObjectID, isCollisionSC, weapon->position, weapon->direction);
			}
		break;
	}
	case CS_HEAD_HIT:
	{
		CS_Head_Collison	*Hit;
		Hit = reinterpret_cast<CS_Head_Collison *>(packet);
		if (Hit->Head) {
			client[Hit->id].vl_lock.lock();
			client[Hit->id].player.DamegeplayerHp(70);
			client[Hit->id].vl_lock.unlock();
			cout << "head" << endl;
		}
		else {
			client[Hit->id].vl_lock.lock();
			client[Hit->id].player.DamegeplayerHp(30);
			client[Hit->id].vl_lock.unlock();

		}
		for (int i = 0; i < MAX_USER; i++)
		{
			if (client[i].connected)
			{

				SendPlayerHppacket(i, Hit->id , Hit->Head);


				if (client[i].player.GetPlayerHp() <= 0)
				{
					if (client[i].Red_Team)
						Blue_kill++;
					else
						Red_Kill++;

					
					add_timer(i, 5000, OP_RESPOND);
				}
				
				
			}

		}


	}
		break;
	default:
		cout << "unknow packet : " << (int)packet[1] << endl;
		break;
	
	}

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

		if (new_id == 10)
		{
			cout << "server is full\n";
			closesocket(new_client);
			continue;
		}

		cout << "Player " << new_id << " Connected " << endl;

		// 재활용 될 소켓이므로 초기화해주어야 한다.
		client[new_id].vl_lock.lock();
		client[new_id].connected = true;
		client[new_id].sock = new_client;
		client[new_id].player.setid(new_id);

		if (client[new_id].player.Getid() % 2 == 0)	//짝수일때 레드팀
			client[new_id].Red_Team = true;


		// DB에서 이전에 로그아웃 한 위치로 다시 재접속
		if (client[new_id].Red_Team)
		{
			client[new_id].player.SetPosition(XMFLOAT3(60,2,12));

		}
		else
		{
			client[new_id].player.SetPosition(XMFLOAT3(65,2,25));
		}


		client[new_id].player.SetAnimation(XMFLOAT3(0,0,0));
		client[new_id].recv_overlap.operation = OP_RECV;
		client[new_id].recv_overlap.packet_size = 0;
		client[new_id].previous_data_size = 0;
		client[new_id].player.SetFireDirection(XMFLOAT3(0,0,0));



		Timer = true;

		client[new_id].vl_lock.unlock();
		add_timer(new_id, 1000, OP_SYSTEM_TIMEER);
		

		CCharacterObject* pCharacter = new CCharacterObject();
		COLLISION_MGR->m_vecCharacterContainer.push_back(pCharacter);
		//입출력 포트와 클라이언트 연결
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
			if (err_no == 64) Disconnected(int(key));
			while (true);
		}
		if (0 == io_size) {
			Disconnected(int(key));
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
						processpacket((int)key, client[key].packet);
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

				//add_timer((int)key, 1000, OP_MOVE);
				break;
			case OP_RESPOND:

				client[(int)key].vl_lock.lock();
				client[(int)key].player.SetPlayerHp(100);		//피를 100만들어주고
				client[(int)key].vl_lock.unlock();

				cout << client[key].player.Getlife();

				for (int i = 0; i < MAX_USER; i++)
				{
					if (client[i].connected)
					{
						//SendTemp(i, (int)key);
						SendPlayerHppacket(i, (int)key, false);
						add_timer(i, 1000, OP_SYSTEM_KILL);
						
						//add_timer((int)key, 1, OP_RECV);
					}
				}
				//add_timer((int)key, 1, OP_RECV);
			break;
			case OP_SYSTEM_KILL:
				for (int i = 0; i < MAX_USER; i++)
				{
					if (client[i].connected)
					{
						
						SendSystemPacket(i);
						
					}
				}
				break;
			case OP_SYSTEM_TIMEER:
			{

				static float Time = 600;
				--Time;	
				client[(int)key].Starting_Time = Time;
				if (client[(int)key].starting == false)
				{
				
					SendTimerpacket((int)key);
					client[(int)key].starting = true;
				}
				add_timer((int)key, 1000, OP_SYSTEM_TIMEER);
			}
				break;
			default:
				cout << overlap->operation;
				cout << "Unknown Event on Worker_Thread" << endl;
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


	for (unsigned int i = 0; i < sys_info.dwNumberOfProcessors * 2; ++i)
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

	pTimerThread->join();
	delete pTimerThread;



	DeleteCriticalSection(&g_CriticalSection);
	DeleteCriticalSection(&timer_lock);

	WSACleanup();
}