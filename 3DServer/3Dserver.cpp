#include "stdafx.h"
#include"protocol.h"
#include"ServerPlayer.h"
#include"Timer.h"
#include "Roommanager.h"
#include "DB.h"

CGameTimer g_GameTimer;



CLIENT client[MAX_USER];
CLIENT other[200];

bool g_isShutdown = false;
HANDLE g_hIocp;
CRITICAL_SECTION g_CriticalSection;
CRITICAL_SECTION timer_lock;
priority_queue<Event_timer, vector<Event_timer>, mycomparison> p_queue;

queue<CLIENT *> death_mode;
queue<CLIENT *> capture_mode;

map <int, CRoommanager *>	g_room;
CDB		g_DB;


BYTE Goal_Kill = 10;
BYTE Red_Kill = 0;
BYTE Blue_kill = 0;
BOOL Timer = false;
atomic_int roomnum = 0;



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
	packet.Starting_timer = g_room[id]->m_timer;

	for(auto p : g_room[id]->m_room_player)
		Sendpacket(p->player.Getid(), reinterpret_cast<unsigned char *>(&packet));
}


void SendPositionPacket(int id, int object)
{
	sc_packet_pos packet;
	packet.id = object;
	packet.size = sizeof(packet);
	packet.type = SC_POS;
	packet.key_button = client[object].player.GetKey();
	packet.x = client[object].player.GetPosition().x;
	packet.y = client[object].player.GetPosition().y;
	packet.z = client[object].player.GetPosition().z;
	packet.Hp = client[object].player.GetPlayerHp();
	packet.Animation = client[object].player.GetAnimation();

	//cout<<object<< " " << packet.key_button << endl;



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

void SendRun(int id, int object)
{
	SC_Run packet;
	packet.id = object;
	packet.size = sizeof(SC_Run);

	packet.type = SC_RUN;

	packet.Run = client[object].player.GetRun();

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

	g_DB.connect();

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
	packet.Team = client[player].Red_Team;

	Sendpacket(clients, (&packet));
}

void SendSystemPacket(int clients , int room_num)	//타이머
{
	SC_System_kill packet;

	packet.type = SC_SYSTEM;
	packet.size = sizeof(packet);
	packet.RED = g_room[room_num]->m_RedKill;
	packet.BLUE = g_room[room_num]->m_BlueKill;

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

void SendRespond(int clients, int player)
{
	SC_Respawn packet;
	packet.size = sizeof(SC_Respawn);
	packet.type = SC_RESPAWN;
	packet.id = player;
	packet.m_bIsRespawn = true;
	packet.m_f3Position = client[player].player.GetPosition();

	
	Sendpacket(clients, (&packet));
		
}

void processpacket(int id, unsigned char *packet)
{
	//패킷 종류별로 처리가 달라진다.
	// 0 size 1 type

	BYTE packet_type = packet[1];

	switch (packet_type)	//키값을 받았을때 처리 해줘야 한다.
	{
	case CS_Login:
	{
		bool IsExist = false;
		int count = 0;
		int passcount = 0;
		cs_login *my_packet = reinterpret_cast<cs_login *>(packet);
		int m_id = 0;
		//cout << (char *)my_packet->id;//<<p->name << endl;
		for (auto &p : g_DB.GetPlayer_info())
		{
			//SQLWCHAR
			for (int i = 0; i < my_packet->strlen; i++)
			{
				if (p->id[i] == my_packet->id[i])
				{
					IsExist = true;
					count++;
				}
				else
				{
					count = 0;
					IsExist = false;
					break;
				}

			}
			for (int i = 0; i < my_packet->passstrlen; i++)
			{
				if (p->password[i] == my_packet->password[i])
				{
					IsExist = true;
					passcount++;
				}
				else
				{
					passcount = 0;
					IsExist = false;
					break;
				}
			}

			if (count == my_packet->strlen && passcount == my_packet->passstrlen)
			{
				//m_id++;
				break;
			}

		}

		if (IsExist == false)	//id가 안맞으면 강제종료하는 패킷을 보낸다.
		{
			SC_login_CONNECT my_packet;
			my_packet.connect = false;
			my_packet.id = id;
			my_packet.size = sizeof(my_packet);
			my_packet.type = 13;

			Sendpacket(id , &my_packet);
		}
	

		break;
	
	}
	case CS_KEY_TYPE:	//여기서 키버튼을 받았을때 처리해줘야 한다.
	{
		cs_key_input *key_button;
		key_button = reinterpret_cast<cs_key_input *>(packet);
		client[id].vl_lock.lock();
		client[id].player.Setkey(key_button->key_button);


		XMFLOAT3 Temp(key_button->x, key_button->y, key_button->z); //클라에서 처리된값다시 보내기.
		client[id].player.SetPosition(Temp);
		client[id].player.SetFireDirection(key_button->FireDirection);
		client[id].player.SetAnimation(key_button->Animation);

		//COLLISION_MGR->m_vecCharacterContainer[id]->SetWorldMatirx(client[id].player.GetWorldMatrix());

		client[id].vl_lock.unlock();
		//cout << key_button->key_button << endl;

	

		//client[id].player.x = serverplayer[id].Getd3dxvVelocity().x;
		//client[id].player.y = 
		//client[id].player.z = serverplayer[id].Getd3dxvVelocity().z;

		//cout << key_button.key_button<<endl;
		//cout << client[id].player.x << " " << client[id].player.y << " " << client[id].player.z << endl;


		for (int i = 0; i < MAX_USER; i++)
		{
			if (client[i].connected == true && client[id].room_num == client[i].room_num)
			{

				SendPositionPacket(i, id);
				//if(client[id].player.Getfire())
					sendbulletfire(i, id);
				//if (client[id].player.GetReload())
					sendReload(i, id);
				//if (client[id].player.GetRun())
					SendRun(i, id);
			}
		}

		//cout << serverplayer[id].Getd3dxvVelocity().x<<" "<< serverplayer[id].Getd3dxvVelocity().y<<" "<< serverplayer[id].Getd3dxvVelocity().z<<endl;
		client[id].vl_lock.lock();
		client[id].player.Setkey(0);
		client[id].player.setfire(false);
		client[id].player.setreload(false);
		client[id].player.Setd3dxvVelocity(0, 0, 0);
		client[id].player.SetAnimation(XMFLOAT3(0,0,0));
		client[id].player.SetRun(false);
		client[id].player.Update(0.05);
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
		client[id].player.SetPitch(rotate->cx);
		client[id].player.SetYaw(rotate->cy);
		//client[id].player.Rotate(rotate->cx, rotate->cy);
		client[id].player.Update(0.05);
		client[id].vl_lock.unlock();

		for (int i = 0; i < MAX_USER; i++)
		{
			if (client[i].connected == true && !client[id].player.Getlife() && client[id].room_num == client[i].room_num)
			{
				SendLookPacket(i, id);
			}
		}
		break;
	}
	case CS_WEAPONE:
	{
		
		cs_weapon *weapon = reinterpret_cast<cs_weapon *>(packet);
		CollisionInfo info;
		bool isCollisionSC = false;

		int i = 0;
		
		for (auto &character : COLLISION_MGR->m_vecCharacterContainer)
		{
		
			client[i].vl_lock.lock();
			character->SetWorldMatirx(client[i].player.GetWorldMatrix());
			client[i].vl_lock.unlock();
			i++;
		}
	
		int romm_num = client[id].room_num;
		isCollisionSC = g_room[romm_num]->CollisonCheck(info, XMLoadFloat3(&weapon->position), XMLoadFloat3(&weapon->direction));
		//isCollisionSC = COLLISION_MGR->RayCastCollisionToCharacter(info, XMLoadFloat3(&weapon->position), XMLoadFloat3(&weapon->direction));
			
		if (isCollisionSC) {
		
				if(client[id].Red_Team != client[info.m_nObjectID].Red_Team && !client[info.m_nObjectID].player.Getlife() && client[id].room_num == client[info.m_nObjectID].room_num)

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
			//cout << "head" << endl;
		}
		else {
			client[Hit->id].vl_lock.lock();
			client[Hit->id].player.DamegeplayerHp(30);
			client[Hit->id].vl_lock.unlock();

		}

		static int k = 0;
		

		if (client[Hit->id].player.GetPlayerHp() <= 0 )//&& !client[Hit->id].player.Getlife()
		{
			g_room[client[Hit->id].room_num]->Killupdate(client[Hit->id].Red_Team);


			add_timer(Hit->id, 100, OP_SYSTEM_KILL);
			add_timer(Hit->id, 5000, OP_RESPOND);
		}
		

		for (int i = 0; i < MAX_USER; i++)
		{
			if (client[i].connected && client[id].room_num == client[i].room_num)
			{

				SendPlayerHppacket(i, Hit->id , Hit->Head);
				
			}

		}

	break;
	}
	case CS_GAME_MODE:
	{
		cs_Gamemode *mode = reinterpret_cast<cs_Gamemode *>(packet);
		CLIENT *player = &client[id];

		if (mode->mode == 1) //데스메치
		{ 
			death_mode.push(player);		//이부분


			if (death_mode.size() == 2)
			{

				roomnum++;
				CRoommanager *death_mode_Room = new CRoommanager(Mode :: Death);
				while (!death_mode.empty())
				{
					death_mode.front()->room_num = roomnum;
					death_mode_Room->insert_Player(death_mode.front());
					g_room.insert(make_pair((int)roomnum, death_mode_Room));


					sc_input_game mode;
					mode.size = sizeof(sc_input_game);
					mode.type = 15;
			
					Sendpacket(death_mode.front()->player.Getid(), &mode);

					
	

					death_mode.pop();

				}

				add_timer(roomnum, 1000, OP_SYSTEM_TIMEER);

				
			}
			
		}
		else   //점령전
		{
			capture_mode.push(player);
			if (capture_mode.size() == 2)
			{
				roomnum++;

				CRoommanager *capture_mode_Room = new CRoommanager(Mode::occupy);
				while (!capture_mode.empty())
				{
				//	capture_mode_Room->insert_Player(capture_mode.front());
				//	g_room.insert(make_pair((int)roomnum, capture_mode_Room));
					capture_mode.pop();


				}

			}
		}
	
		break;
	}

	case 7:	//키버튼 받으면 방 종료
	{
		cs_temp_exit *temp;
		temp = reinterpret_cast<cs_temp_exit *>(packet);

		for (int i = 0; i < MAX_USER; i++)
		{
			if (client[i].room_num == client[id].room_num)
			{
				cs_temp_exit temp;
				temp.size = sizeof(temp);
				temp.type = 14;

				Sendpacket(i, &temp);
			}
		}

		g_room[client[id].room_num]->Release();
		break;

	}

	case 8: // 클라에서 메인신 들어가서 캐릭터를 만들면 서버로 보내고 
			// 서버는 받아서 캐릭터 정보를 보내준다.
	{

		auto &player = g_room[client[id].room_num]->m_room_player;

		for(auto &p : g_room[client[id].room_num]->m_room_player)
			SendPutPlayerPacket(p->player.Getid(), p->player.Getid());	//방이 만들어지고 처음 위치를 보낸다.
		
		for (int i = 0; i < player.size(); ++i)
		{
			if (client[i].connected && client[i].room_num == client[id].room_num )	//방안에 있는 사람들한테 보내야 한다.
			{

				SendPutPlayerPacket(player[i]->player.Getid(), id);
				SendPutPlayerPacket(id, player[i]->player.Getid());

			}
		}




		
		break;
	}

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
		client[new_id].room_num = 0;
		client[new_id].player.setid(new_id);
		//client[new_id].player.SetPosition(XMFLOAT3(270, 2, 230));


		if (client[new_id].player.Getid() % 2 == 0)	//짝수일때 레드팀
			client[new_id].Red_Team = 1;



		//if (client[new_id].Red_Team)
		//{
		//	client[new_id].player.SetPosition(XMFLOAT3(60,2,12));

		//}



		client[new_id].player.SetAnimation(XMFLOAT3(0,0,0));
		client[new_id].recv_overlap.operation = OP_RECV;
		client[new_id].recv_overlap.packet_size = 0;
		client[new_id].prev_packet_data = 0;
		client[new_id].player.SetFireDirection(XMFLOAT3(0,0,0));



		Timer = true;

		client[new_id].vl_lock.unlock();
		//add_timer(new_id, 1000, OP_SYSTEM_TIMEER);
		
		CCharacterObject* pCharacter = new CCharacterObject();
		COLLISION_MGR->m_vecCharacterContainer.push_back(pCharacter);
		//입출력 포트와 클라이언트 연결
		CreateIoCompletionPort(reinterpret_cast<HANDLE>(new_client), g_hIocp, new_id, 0);


		DWORD recv_flag = 0;
		WSARecv(new_client, &client[new_id].recv_overlap.recv_buffer, 1,
			NULL, &recv_flag, &client[new_id].recv_overlap.original_overlap, NULL);

		//SendPutPlayerPacket(new_id, new_id);

		//for (int i = 0; i < MAX_USER; ++i)
		//{
		//	if (client[i].connected == true )	//&& client[i].room_num == client[new_id].room_num 
		//	{
		//		if (i != new_id)
		//		{
		//			SendPutPlayerPacket(new_id, i);
		//			SendPutPlayerPacket(i, new_id);
		//		}
		//	}
		//}

		

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
					int required = client[key].recv_overlap.packet_size - client[key].prev_packet_data;

					//패킷완성
					if (remained >= required)
					{
						//지난번에 받은 데이터 뒷부분에 복사
						memcpy(client[key].packet + client[key].prev_packet_data, pBuff, required);
						processpacket((int)key, client[key].packet);
						remained -= required;
						pBuff += required;
						client[key].recv_overlap.packet_size = 0;
						client[key].prev_packet_data = 0;

					}
					else
					{
						memcpy(client[key].packet + client[key].prev_packet_data, pBuff, remained);
						//미완성 패킷의 사이즈가 reamined만큼 증가
						client[key].prev_packet_data += remained;
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


				//if (client[(int)key].Red_Team)						//처음 위치로
				//	client[(int)key].player.SetPosition(XMFLOAT3(60, 2, 12));
				//else
				//	client[(int)key].player.SetPosition(XMFLOAT3(270, 2, 230));

				client[(int)key].player.setid(key);

				client[(int)key].vl_lock.unlock();


				SendPlayerHppacket(key, (int)key, false);	//처음에 자기자신에게 보내고
				SendRespond(key, (int)key);
				for (int i = 0; i < MAX_USER; i++)
				{
					if (client[i].connected && i != key)
					{
						//SendTemp(i, (int)key);
						SendRespond(i, (int)key);
						SendPlayerHppacket(i, (int)key, false);	//전체적으로 뿌린다.

						//add_timer((int)key, 1, OP_RECV);
					}
				}
				break;
			case OP_SYSTEM_KILL:
				for (int i = 0; i < MAX_USER; i++)
				{
					if (client[i].connected && client[i].room_num == client[key].room_num)
					{

						SendSystemPacket(i , client[i].room_num);

					}
				}
				break;
			case OP_SYSTEM_TIMEER:
			{

				//static float Time = 600;
				//--Time;
				//client[(int)key].Starting_Time = Time;
				//if (client[(int)key].starting == false)
				//{

				//	SendTimerpacket((int)key);
				//	client[(int)key].starting = true;
				//}
				g_room[key]->update();
				SendTimerpacket((int)key);
				add_timer((int)key, 1000, OP_SYSTEM_TIMEER);
				break;
			}

			

			default:
				cout << overlap->operation;
				cout << "Unknown Event on Worker_Thread" << endl;
				break;
			}
		}
	}
}

bool CreateMap()
{
	MAPDATA_MGR->InitializeManager();
	MESHDATA_MGR->InitializeManager();
	CGameObject *pObject = nullptr;
	BoundingBox boundingBox;
	vector<MapData> vecMapData;

	boundingBox = MESHDATA_MGR->GetBoundingBox(MeshTag::eRoad);
	vecMapData = MAPDATA_MGR->GetDataVector(ObjectTag::eRoad1);

	for (int count = 0; count < vecMapData.size(); ++count) {
		pObject = new CGameObject();
		pObject->SetBoundingBox(boundingBox);
		pObject->SetPosition(vecMapData[count].m_Position);
		pObject->Rotate(vecMapData[count].m_Rotation);
	
		COLLISION_MGR->m_vecStaticMeshContainer.push_back(pObject);
	}


	boundingBox = MESHDATA_MGR->GetBoundingBox(MeshTag::eRoad);
	vecMapData = MAPDATA_MGR->GetDataVector(ObjectTag::eRoad2);

	for (int count = 0; count < vecMapData.size(); ++count) {
		pObject = new CGameObject();
		pObject->SetBoundingBox(boundingBox);
		pObject->SetPosition(vecMapData[count].m_Position);
		pObject->Rotate(vecMapData[count].m_Rotation);

		COLLISION_MGR->m_vecStaticMeshContainer.push_back(pObject);
	}


	boundingBox = MESHDATA_MGR->GetBoundingBox(MeshTag::eCrossRoad);
	vecMapData = MAPDATA_MGR->GetDataVector(ObjectTag::eCrossRoad);

	for (int count = 0; count < vecMapData.size(); ++count) {
		pObject = new CGameObject();
		pObject->SetBoundingBox(boundingBox);
		pObject->SetPosition(vecMapData[count].m_Position);
		pObject->Rotate(vecMapData[count].m_Rotation);

		COLLISION_MGR->m_vecStaticMeshContainer.push_back(pObject);
	}


	boundingBox = MESHDATA_MGR->GetBoundingBox(MeshTag::eCenterRoad);
	vecMapData = MAPDATA_MGR->GetDataVector(ObjectTag::eCenterRoad);

	for (int count = 0; count < vecMapData.size(); ++count) {
		pObject = new CGameObject();
		pObject->SetBoundingBox(boundingBox);
		pObject->SetPosition(vecMapData[count].m_Position);
		pObject->Rotate(vecMapData[count].m_Rotation);

		COLLISION_MGR->m_vecStaticMeshContainer.push_back(pObject);
	}


	boundingBox = MESHDATA_MGR->GetBoundingBox(MeshTag::eBuilding19);
	vecMapData = MAPDATA_MGR->GetDataVector(ObjectTag::eBuilding19);

	for (int count = 0; count < vecMapData.size(); ++count) {
		pObject = new CGameObject();
		pObject->SetBoundingBox(boundingBox);
		pObject->SetPosition(vecMapData[count].m_Position);
		pObject->Rotate(vecMapData[count].m_Rotation);

		COLLISION_MGR->m_vecStaticMeshContainer.push_back(pObject);
	}

	boundingBox = MESHDATA_MGR->GetBoundingBox(MeshTag::eBuilding20);
	vecMapData = MAPDATA_MGR->GetDataVector(ObjectTag::eBuilding20);

	for (int count = 0; count < vecMapData.size(); ++count) {
		pObject = new CGameObject();
		pObject->SetBoundingBox(boundingBox);
		pObject->SetPosition(vecMapData[count].m_Position);
		pObject->Rotate(vecMapData[count].m_Rotation);

		COLLISION_MGR->m_vecStaticMeshContainer.push_back(pObject);
	}

	boundingBox = MESHDATA_MGR->GetBoundingBox(MeshTag::eBuilding21);
	vecMapData = MAPDATA_MGR->GetDataVector(ObjectTag::eBuilding21);

	for (int count = 0; count < vecMapData.size(); ++count) {
		pObject = new CGameObject();
		pObject->SetBoundingBox(boundingBox);
		pObject->SetPosition(vecMapData[count].m_Position);
		pObject->Rotate(vecMapData[count].m_Rotation);

		COLLISION_MGR->m_vecStaticMeshContainer.push_back(pObject);
	}

	boundingBox = MESHDATA_MGR->GetBoundingBox(MeshTag::eBuilding22);
	vecMapData = MAPDATA_MGR->GetDataVector(ObjectTag::eBuilding22);

	for (int count = 0; count < vecMapData.size(); ++count) {
		pObject = new CGameObject();
		pObject->SetBoundingBox(boundingBox);
		pObject->SetPosition(vecMapData[count].m_Position);
		pObject->Rotate(vecMapData[count].m_Rotation);

		COLLISION_MGR->m_vecStaticMeshContainer.push_back(pObject);
	}

	boundingBox = MESHDATA_MGR->GetBoundingBox(MeshTag::eBuilding30);
	vecMapData = MAPDATA_MGR->GetDataVector(ObjectTag::eBuilding30);

	for (int count = 0; count < vecMapData.size(); ++count) {
		pObject = new CGameObject();
		pObject->SetBoundingBox(boundingBox);
		pObject->SetPosition(vecMapData[count].m_Position);
		pObject->Rotate(vecMapData[count].m_Rotation);

		COLLISION_MGR->m_vecStaticMeshContainer.push_back(pObject);
	}

	boundingBox = MESHDATA_MGR->GetBoundingBox(MeshTag::eBuilding33);
	vecMapData = MAPDATA_MGR->GetDataVector(ObjectTag::eBuilding33);

	for (int count = 0; count < vecMapData.size(); ++count) {
		pObject = new CGameObject();
		pObject->SetBoundingBox(boundingBox);
		pObject->SetPosition(vecMapData[count].m_Position);
		pObject->Rotate(vecMapData[count].m_Rotation);

		COLLISION_MGR->m_vecStaticMeshContainer.push_back(pObject);
	}

	boundingBox = MESHDATA_MGR->GetBoundingBox(MeshTag::eBuilding34);
	vecMapData = MAPDATA_MGR->GetDataVector(ObjectTag::eBuilding34);

	for (int count = 0; count < vecMapData.size(); ++count) {
		pObject = new CGameObject();
		pObject->SetBoundingBox(boundingBox);
		pObject->SetPosition(vecMapData[count].m_Position);
		pObject->Rotate(vecMapData[count].m_Rotation);

		COLLISION_MGR->m_vecStaticMeshContainer.push_back(pObject);
	}

	boundingBox = MESHDATA_MGR->GetBoundingBox(MeshTag::eBuilding77);
	vecMapData = MAPDATA_MGR->GetDataVector(ObjectTag::eBuilding77);

	for (int count = 0; count < vecMapData.size(); ++count) {
		pObject = new CGameObject();
		pObject->SetBoundingBox(boundingBox);
		pObject->SetPosition(vecMapData[count].m_Position);
		pObject->Rotate(vecMapData[count].m_Rotation);

		COLLISION_MGR->m_vecStaticMeshContainer.push_back(pObject);
	}

	boundingBox = MESHDATA_MGR->GetBoundingBox(MeshTag::eBuilding78);
	vecMapData = MAPDATA_MGR->GetDataVector(ObjectTag::eBuilding78);

	for (int count = 0; count < vecMapData.size(); ++count) {
		pObject = new CGameObject();
		pObject->SetBoundingBox(boundingBox);
		pObject->SetPosition(vecMapData[count].m_Position);
		pObject->Rotate(vecMapData[count].m_Rotation);

		COLLISION_MGR->m_vecStaticMeshContainer.push_back(pObject);
	}

	boundingBox = MESHDATA_MGR->GetBoundingBox(MeshTag::eBuilding100);
	vecMapData = MAPDATA_MGR->GetDataVector(ObjectTag::eBuilding100);

	for (int count = 0; count < vecMapData.size(); ++count) {
		pObject = new CGameObject();
		pObject->SetBoundingBox(boundingBox);
		pObject->SetPosition(vecMapData[count].m_Position);
		pObject->Rotate(vecMapData[count].m_Rotation);

		COLLISION_MGR->m_vecStaticMeshContainer.push_back(pObject);
	}

	boundingBox = MESHDATA_MGR->GetBoundingBox(MeshTag::eBuilding103);
	vecMapData = MAPDATA_MGR->GetDataVector(ObjectTag::eBuilding103);

	for (int count = 0; count < vecMapData.size(); ++count) {
		pObject = new CGameObject();
		pObject->SetBoundingBox(boundingBox);
		pObject->SetPosition(vecMapData[count].m_Position);
		pObject->Rotate(vecMapData[count].m_Rotation);

		COLLISION_MGR->m_vecStaticMeshContainer.push_back(pObject);
	}

	boundingBox = MESHDATA_MGR->GetBoundingBox(MeshTag::eBuilding104);
	vecMapData = MAPDATA_MGR->GetDataVector(ObjectTag::eBuilding104);

	for (int count = 0; count < vecMapData.size(); ++count) {
		pObject = new CGameObject();
		pObject->SetBoundingBox(boundingBox);
		pObject->SetPosition(vecMapData[count].m_Position);
		pObject->Rotate(vecMapData[count].m_Rotation);

		COLLISION_MGR->m_vecStaticMeshContainer.push_back(pObject);
	}

	boundingBox = MESHDATA_MGR->GetBoundingBox(MeshTag::eParkingLot);
	vecMapData = MAPDATA_MGR->GetDataVector(ObjectTag::eParkingLot);

	for (int count = 0; count < vecMapData.size(); ++count) {
		pObject = new CGameObject();
		pObject->SetBoundingBox(boundingBox);
		pObject->SetPosition(vecMapData[count].m_Position);
		pObject->Rotate(vecMapData[count].m_Rotation);

		COLLISION_MGR->m_vecStaticMeshContainer.push_back(pObject);
	}

	boundingBox = MESHDATA_MGR->GetBoundingBox(MeshTag::eBench);
	vecMapData = MAPDATA_MGR->GetDataVector(ObjectTag::eBench);

	for (int count = 0; count < vecMapData.size(); ++count) {
		pObject = new CGameObject();
		pObject->SetBoundingBox(boundingBox);
		pObject->SetPosition(vecMapData[count].m_Position);
		pObject->Rotate(vecMapData[count].m_Rotation);

		COLLISION_MGR->m_vecStaticMeshContainer.push_back(pObject);
	}

	boundingBox = MESHDATA_MGR->GetBoundingBox(MeshTag::eBusStop);
	vecMapData = MAPDATA_MGR->GetDataVector(ObjectTag::eBusStop);

	for (int count = 0; count < vecMapData.size(); ++count) {
		pObject = new CGameObject();
		pObject->SetBoundingBox(boundingBox);
		pObject->SetPosition(vecMapData[count].m_Position);
		pObject->Rotate(vecMapData[count].m_Rotation);

		COLLISION_MGR->m_vecStaticMeshContainer.push_back(pObject);
	}

	boundingBox = MESHDATA_MGR->GetBoundingBox(MeshTag::eStreetLamp);
	vecMapData = MAPDATA_MGR->GetDataVector(ObjectTag::eStreetLamp);

	for (int count = 0; count < vecMapData.size(); ++count) {
		pObject = new CGameObject();
		pObject->SetBoundingBox(boundingBox);
		pObject->SetPosition(vecMapData[count].m_Position);
		pObject->Rotate(vecMapData[count].m_Rotation);

		COLLISION_MGR->m_vecStaticMeshContainer.push_back(pObject);
	}

	boundingBox = MESHDATA_MGR->GetBoundingBox(MeshTag::eBarricade);
	vecMapData = MAPDATA_MGR->GetDataVector(ObjectTag::eBarricade);

	for (int count = 0; count < vecMapData.size(); ++count) {
		pObject = new CGameObject();
		pObject->SetBoundingBox(boundingBox);
		pObject->SetPosition(vecMapData[count].m_Position);
		pObject->Rotate(vecMapData[count].m_Rotation);

		COLLISION_MGR->m_vecStaticMeshContainer.push_back(pObject);
	}

	//boundingBox = MESHDATA_MGR->GetBoundingBox(MeshTag:: );
	vecMapData = MAPDATA_MGR->GetDataVector(ObjectTag::eSideWalk1);
	boundingBox.Extents = XMFLOAT3(vecMapData[0].m_Scale);
	for (int count = 0; count < vecMapData.size(); ++count) {
		pObject = new CGameObject();
		pObject->SetBoundingBox(boundingBox);
		pObject->SetPosition(vecMapData[count].m_Position);
		pObject->Rotate(vecMapData[count].m_Rotation);

		COLLISION_MGR->m_vecStaticMeshContainer.push_back(pObject);
	}

	vecMapData = MAPDATA_MGR->GetDataVector(ObjectTag::eSideWalk2);
	boundingBox.Extents = XMFLOAT3(vecMapData[0].m_Scale);
	for (int count = 0; count < vecMapData.size(); ++count) {
		pObject = new CGameObject();
		pObject->SetBoundingBox(boundingBox);
		pObject->SetPosition(vecMapData[count].m_Position);
		pObject->Rotate(vecMapData[count].m_Rotation);

		COLLISION_MGR->m_vecStaticMeshContainer.push_back(pObject);
	}

	vecMapData = MAPDATA_MGR->GetDataVector(ObjectTag::eStoneWall);
	for (int count = 0; count < vecMapData.size(); ++count) {
		pObject = new CGameObject();
		pObject->SetBoundingBox(boundingBox);
		pObject->SetPosition(vecMapData[count].m_Position);
		pObject->Rotate(vecMapData[count].m_Rotation);

		COLLISION_MGR->m_vecStaticMeshContainer.push_back(pObject);
	}
	return true;

}

int main(int argv, char* argc[])
{
	//CreateMap();
	Initialize_server();

	thread* pAcceptThread;
	thread* pTimerThread;

	vector<thread*> vpThread;


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