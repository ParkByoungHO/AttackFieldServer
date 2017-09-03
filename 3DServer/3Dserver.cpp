#include "stdafx.h"
#include"protocol.h"
#include"ServerPlayer.h"
#include"Timer.h"
#include "Roommanager.h"
#include "DB.h"
#include "mdump.h"

CGameTimer g_GameTimer;

SOCKET	g_socket;


CLIENT client[MAX_USER];
CLIENT other[200];

bool g_isShutdown = false;
HANDLE g_hIocp;
CRITICAL_SECTION g_CriticalSection;
CRITICAL_SECTION timer_lock;
priority_queue<Event_timer, vector<Event_timer>, mycomparison> p_queue;

queue<CLIENT *> death_mode;
queue<CLIENT *> capture_mode;

map <int, CRoommanager*>	g_room;


CRoommanager death_mode_Room[500]{ {Mode::Death},{ Mode::Death },{ Mode::Death }, };
CRoommanager capture_mode_Room[500]{ {Mode::occupy},{ Mode::Death },{ Mode::Death },{ Mode::Death }, };


CDB		g_DB;


BYTE Goal_Kill = 10;
BYTE Red_Kill = 0;
BYTE Blue_kill = 0;
BOOL Timer = false;
atomic_int death_roomnum = 1;
atomic_int catch_roomnum = 1;
atomic_int room_num = 1;



void error_display(char *msg, int err_num)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, err_num,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	//printf("[%s] %s", msg, (char *)lpMsgBuf);
	std::cout << msg;
	std::wcout << L"에러" << (int)lpMsgBuf << std::endl;
	LocalFree(lpMsgBuf);
}

void Sendpacket(int id, void* packet)
{
	//지역변수로 하짐라고 메모리 할당을 해줘야 한다.
	int psize = reinterpret_cast<unsigned char *>(packet)[0];
	int ptype = reinterpret_cast<unsigned char *>(packet)[1];
	Overlapex* send_over = new Overlapex;
	send_over->operation = OP_SEND;
	memcpy(send_over->socket_buff, packet, psize);
	ZeroMemory(&send_over->original_overlap, sizeof(send_over->original_overlap));

	send_over->recv_buffer.buf = reinterpret_cast<char *>(send_over->socket_buff);
	send_over->recv_buffer.len = psize;

	int result = WSASend(client[id].sock, &send_over->recv_buffer, 1, NULL, 0,
		&send_over->original_overlap, NULL);
	if (result != 0)
	{
		int error_num = WSAGetLastError();
		if (WSA_IO_PENDING != error_num)
			error_display("sendpacket : wsasend", error_num);

	}
	//std::cout << "Send Packet [" << ptype << "] To Client : " << id << std::endl;

}

void SendRemovePacket(int client, int object)
{
	sc_packet_remove_player remove_player;

	remove_player.id = object;
	remove_player.size = sizeof(remove_player);
	remove_player.type = ePacket_Disconnect;

	Sendpacket(client, &remove_player);
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
	packet.type = ePacket_GameTimer;
	packet.Starting_timer = g_room[id]->m_timer;

	for (auto &p : g_room[id]->m_room_id)
		Sendpacket(p, (&packet));
}


void SendPositionPacket(int id, int object)
{
	sc_packet_pos packet;
	packet.id = object;
	packet.size = sizeof(packet);
	packet.type = ePacket_Update;
	packet.key_button = client[object].player.GetKey();
	packet.x = client[object].player.GetPosition().x;
	packet.y = client[object].player.GetPosition().y;
	packet.z = client[object].player.GetPosition().z;
	packet.Hp = client[object].player.GetPlayerHp();
	packet.Animation = client[object].player.GetAnimation();
	packet.FireDirection = client[object].player.GetFireDirection();



	//cout<<object<< " " << packet.key_button << endl;



	Sendpacket(id, &packet);
}

void sendbulletfire(int id, int object)
{
	sc_bullet_fire  packet;
	packet.id = object;

	packet.size = sizeof(packet);
	packet.type = SC_PUT_Bullet;

	packet.fire = true;
	XMStoreFloat3(&packet.FireDirection, client[object].player.GetLook());

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
	packet.type = ePacket_MouseRotate;

	packet.x = client[object].player.getfPitch();
	packet.y = client[object].player.getYaw();
	packet.z = 0;

	Sendpacket(id, (&packet));
}

void SendCollisonPacket(int id, int object, bool collision, XMFLOAT3 position, XMFLOAT3 direction)
{
	SC_Collison packet;
	packet.size = sizeof(SC_Collison);
	packet.id = object;
	packet.type = ePacket_CollisionCheck;
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
			PostQueuedCompletionStatus(g_hIocp, 1, top_object.obj_id, *(reinterpret_cast<LPOVERLAPPED *>(&overlapped)));


		}

	} while (true);

}

void Disconnected(int ci)
{
	if (client[ci].room_num != 0)
	{
		closesocket(client[ci].sock);
		client[ci].connected = false;
		auto room = client[ci].room_num;
		auto& roomplayer = g_room[room]->m_room_id;
		g_room[room]->lock.lock();
		for (auto &iter = roomplayer.begin(); iter != roomplayer.end(); ++iter)
		{
			auto client = *iter;
			if (client != ci) continue;
			//delete *iter;
			roomplayer.erase(iter);
			break;

		}
		for (auto &p : roomplayer)
		{
			SendRemovePacket(p, ci);
		}
		g_room[room]->lock.unlock();
	}
}

void Initialize_server()
{

	InitializeCriticalSection(&g_CriticalSection);
	InitializeCriticalSection(&timer_lock);


	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);

	g_hIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, NULL, 0);	//처음 선언할때 이렇게 선언

	g_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	SOCKADDR_IN listen_addr;
	ZeroMemory(&listen_addr, sizeof(SOCKADDR_IN));
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_port = htons(SERVERPORT);
	listen_addr.sin_addr.s_addr = INADDR_ANY;
	//ZeroMemory(&listen_addr.sin_zero, 8);

	::bind(g_socket, reinterpret_cast<sockaddr *>(&listen_addr), sizeof(listen_addr));

	if (0 != listen(g_socket, MAX_USER))
	{
		error_display("Listen Error : ", WSAGetLastError());
	}

	for (int i = 0; i < MAX_USER; ++i)
	{
		client[i].recv_overlap.recv_buffer.buf = reinterpret_cast<char *>(client[i].recv_overlap.socket_buff);
		client[i].recv_overlap.recv_buffer.len = MAX_BUFFSIZE;
		client[i].recv_overlap.operation = OP_RECV;

		client[i].connected = false;
	}

	//네이글 알고리즘
	char opt_val = TRUE;
	setsockopt(g_socket, IPPROTO_TCP, TCP_NODELAY, &opt_val, sizeof(opt_val));

	//g_DB.connect();
}

void SendPutPlayerPacket(int clients, int player)
{
	sc_packet_put_player packet;

	packet.id = player;
	packet.size = sizeof(packet);
	packet.type = ePacket_CreateOthderPlayer;
	packet.x = client[player].player.GetPosition().x;
	packet.y = client[player].player.GetPosition().y;
	packet.z = client[player].player.GetPosition().z;
	packet.hp = client[player].player.GetPlayerHp();
	packet.Animation = client[player].player.GetAnimation();
	packet.Goal = g_room[client[player].room_num]->m_Goalkill;
	packet.RED = g_room[client[player].room_num]->m_RedKill;
	packet.Blue = g_room[client[player].room_num]->m_BlueKill;
	packet.Team = client[player].Red_Team;
	packet.mode = client[player].game_mode;

	Sendpacket(clients, (&packet));
}

void SendSystemPacket(int clients, int room_num)	//킬
{
	SC_System_kill packet;

	packet.type = ePacket_KillUpdate;
	packet.size = sizeof(packet);
	packet.RED = g_room[room_num]->m_RedKill;
	packet.BLUE = g_room[room_num]->m_BlueKill;

	Sendpacket(clients, (&packet));
}



void SendPlayerHppacket(int clients, int player, bool Head)	//타이머
{
	SC_Player_Hp packet;

	packet.type = ePacket_HP;
	packet.size = sizeof(packet);
	packet.Hp = client[player].player.GetPlayerHp();
	packet.id = player;
	packet.Head = Head;
	packet.live = client[player].player.Getlife();


	Sendpacket(clients, (&packet));
}

void SendRespond(int clients, int player)
{
	SC_Respawn packet;
	packet.size = sizeof(SC_Respawn);
	packet.type = ePacket_Respawn;
	packet.id = player;
	packet.m_f3Position = client[player].player.GetPosition();


	//cout << packet.m_f3Position.x << "	" << packet.m_f3Position.y << "	" << packet.m_f3Position.z << endl;


	Sendpacket(clients, (&packet));

}

void processpacket(volatile int id, unsigned char *packet)
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
		client[id].vl_lock.lock();
		client[id].player.Setkey(key_button->key_button);

		client[id].player.SetFireDirection(key_button->FireDirection);

		XMFLOAT3 Temp(key_button->x, key_button->y, key_button->z); //클라에서 처리된값다시 보내기.
		client[id].player.SetPosition(Temp);

		client[id].player.SetAnimation(key_button->Animation);
		client[id].player.UpdateKeyInput(0.05);
		//COLLISION_MGR->m_vecCharacterContainer[id]->SetWorldMatirx(client[id].player.GetWorldMatrix());

		client[id].vl_lock.unlock();





		//client[id].player.x = serverplayer[id].Getd3dxvVelocity().x;
		//client[id].player.y = 
		//client[id].player.z = serverplayer[id].Getd3dxvVelocity().z;



		//auto &player = g_room[client[id].room_num]->m_room_player;



		for (auto p : g_room[client[id].room_num]->m_room_id)
		{

			//if(client[id].player.Getfire())
			//	sendbulletfire(p->player.Getid(), id);


			//if(client[id].player.GetReload())
			//	sendReload(p->player.Getid(), id);

			//if(client[id].player.GetRun())
			//	SendRun(p->player.Getid(), id);
			//if (my_Pos_packet->key_button & static_cast<int>(KeyInput::eLeftMouse))
			//	SCENE_MGR->g_pMainScene->GetCharcontainer()[i]->SetIsFire(true);
			//else
			//	SCENE_MGR->g_pMainScene->GetCharcontainer()[i]->SetIsFire(false);

			if (p != id)
			{
				SendPositionPacket(p, id);	//포지셔은 계속보낸다.
				//cout << "현재 누른 플레이어 ID : " << id << " 누구한테? : " << p << endl;
			}
		}

		//cout << serverplayer[id].Getd3dxvVelocity().x<<" "<< serverplayer[id].Getd3dxvVelocity().y<<" "<< serverplayer[id].Getd3dxvVelocity().z<<endl;
		client[id].vl_lock.lock();
		//client[id].player.Setkey(0);
		//client[id].player.setfire(false);
		//client[id].player.setreload(false);
		//client[id].player.SetRun(false);
		client[id].player.Setd3dxvVelocity(0, 0, 0);
		client[id].player.SetAnimation(XMFLOAT3(0, 0, 0));
		client[id].vl_lock.unlock();



		break;
	}
	case CS_ROTATE:		//cx cy를 받아서 로테이트 처리해야한다.
	{
		cs_rotate *rotate;
		//memcpy(&rotate, packet, packet[0]);
		rotate = reinterpret_cast<cs_rotate *>(packet);
		client[id].vl_lock.lock();
		client[id].player.SetPitch(rotate->cx);
		client[id].player.SetYaw(rotate->cy);
		//client[id].player.Rotate(rotate->cx, rotate->cy);
		client[id].player.Update(0.05);
		client[id].vl_lock.unlock();


		for (auto p : g_room[client[id].room_num]->m_room_id)
		{
			if (p != id)
			{
				//cout << "Rotate - ID : " << id << "받는 ID : " << p << endl;
				SendLookPacket(p, id);
			}
		}
		break;
	}
	case CS_WEAPONE:
	{

		cs_weapon *weapon = reinterpret_cast<cs_weapon *>(packet);
		CollisionInfo info;
		volatile bool isCollisionSC = false;

		int i = 0;

		for (auto &character : COLLISION_MGR->m_vecCharacterContainer)
		{

			client[i].vl_lock.lock();
			character->SetWorldMatirx(client[i].player.GetWorldMatrix());
			client[i].vl_lock.unlock();
			i++;
		}

		//int romm_num = client[id].room_num;
		//g_room[romm_num]->lock.lock();
		//isCollisionSC = g_room[romm_num]->CollisonCheck(info, XMLoadFloat3(&weapon->position), XMLoadFloat3(&weapon->direction));
		isCollisionSC = COLLISION_MGR->RayCastCollisionToCharacter(info, XMLoadFloat3(&weapon->position), XMLoadFloat3(&weapon->direction));
		//g_room[romm_num]->lock.unlock();



		if (isCollisionSC)
		{
			if (client[id].player.Getid() != client[info.m_nObjectID].player.Getid())
			{
				if (client[id].Red_Team != client[info.m_nObjectID].Red_Team &&
					!client[info.m_nObjectID].player.Getlife() &&
					client[id].room_num == client[info.m_nObjectID].room_num)
				{
					//cout << " ID : " << client[id].player.Getid() << "	맞은 사람 ID : " << client[info.m_nObjectID].player.Getid() << endl;
					SendCollisonPacket(info.m_nObjectID, info.m_nObjectID, isCollisionSC, weapon->position, weapon->direction);
				}
			}
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

		}
		else {
			client[Hit->id].vl_lock.lock();
			client[Hit->id].player.DamegeplayerHp(30);
			client[Hit->id].vl_lock.unlock();

		}

		SC_Damegedirection target;

		target.size = sizeof(SC_Damegedirection);
		target.type = ePacket_DamageInfo;
		target.position = Hit->position;

		Sendpacket(Hit->id, &target);

		client[Hit->id].vl_lock.lock();
		int roomnum = client[Hit->id].room_num;
		client[Hit->id].vl_lock.unlock();


		if (client[Hit->id].player.GetPlayerHp() <= 0)	//&& !client[Hit->id].player.Getlife()
		{
			//static int count = 0;
			//add_timer(Hit->id, 100, OP_SYSTEM_KILL);
			add_timer(Hit->id, 5000, OP_RESPOND);
			g_room[roomnum]->lock.lock();
			g_room[roomnum]->Killupdate(client[Hit->id].Red_Team);
			g_room[roomnum]->lock.unlock();

			if (g_room[roomnum]->m_BlueKill >= g_room[roomnum]->m_Goalkill)	// 블루팀 승리.
			{
				g_room[roomnum]->lock.lock();
				g_room[roomnum]->m_BlueKill = 50;	//50으로 만들고
				g_room[roomnum]->lock.unlock();

				for (auto &p : g_room[roomnum]->m_room_id)	//방을 없애준다.
				{
					cs_temp_exit temp;
					temp.size = sizeof(temp);
					temp.type = ePacket_SceneChange;
					temp.Winner = 2;

					Sendpacket(p, &temp);
				}

				g_room[roomnum]->Release();
			}
			else if (g_room[roomnum]->m_RedKill >= g_room[roomnum]->m_Goalkill)	//레드팀이 승리
			{
				g_room[roomnum]->lock.lock();
				g_room[roomnum]->m_BlueKill = 50;	//100으로 만들고
				g_room[roomnum]->lock.unlock();
				for (auto &p : g_room[roomnum]->m_room_id)	//방을 없애준다.
				{
					cs_temp_exit temp;
					temp.size = sizeof(temp);
					temp.type = ePacket_SceneChange;
					temp.Winner = 1;

					Sendpacket(p, &temp);
				}

				g_room[roomnum]->Release();
			}
			else //두개의 조건이 다 안맞으면 보내준다.
			{
				for (auto &p : g_room[roomnum]->m_room_id)
				{
					SendSystemPacket(p, roomnum);
				}
			}


		}

		for (auto &p : g_room[client[id].room_num]->m_room_id)
		{
			SendPlayerHppacket(p, Hit->id, Hit->Head);
		}
		break;
	}
	case CS_GAME_MODE:
	{
		cs_Gamemode *mode = reinterpret_cast<cs_Gamemode *>(packet);
		CLIENT *player = &client[id];

		if (mode->mode == 1) //데스메치
		{


			death_mode_Room[death_roomnum].insert_Player(id);

			if (death_mode_Room[death_roomnum].m_room_id.size() == 2)
			{

				sc_input_game mode;
				mode.size = sizeof(sc_input_game);
				mode.type = ePacket_SuccessMyCharacter;

				for (auto p : death_mode_Room[death_roomnum].m_room_id)
				{
					client[p].vl_lock.lock();
					client[p].player.setid(p);
					client[p].room_num = room_num;
					client[p].game_mode = 1;
					client[p].vl_lock.unlock();
					client[p].player.SetPlayerHp(100);
					Sendpacket(p, &mode);
				}
				g_room.insert(make_pair((int)room_num, &death_mode_Room[death_roomnum]));
				add_timer(room_num, 1000, OP_SYSTEM_TIMEER);
				room_num++;
				death_roomnum++;
			}

		}
		else   //점령전
		{

			client[id].player.setid(id);
			capture_mode_Room[catch_roomnum].insert_Player(id);


			if (capture_mode_Room[catch_roomnum].m_room_id.size() == 2)
			{

				sc_input_game mode;
				mode.size = sizeof(sc_input_game);
				mode.type = ePacket_SuccessMyCharacter;

				for (auto p : capture_mode_Room[catch_roomnum].m_room_id)
				{
					client[p].vl_lock.lock();
					client[p].player.setid(p);
					client[p].room_num = room_num;
					client[p].game_mode = 2;
					client[p].vl_lock.unlock();
					Sendpacket(p, &mode);
				}
				g_room.insert(make_pair((int)room_num, &capture_mode_Room[catch_roomnum]));
				add_timer(room_num, 1000, OP_SYSTEM_TIMEER);
				room_num++;
				catch_roomnum++;
			}

		}

		break;
	}

	case CS_Login:
	{
		bool IsExist = false;
		int count = 0;
		int passcount = 0;
		cs_login *my_packet = reinterpret_cast<cs_login *>(packet);
		int m_id = 0;

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
			my_packet.type = ePacket_LoginFail;

			//Sendpacket(id , &my_packet);
		}


		break;

	}

	//case 7:	//키버튼 받으면 방 종료
	//{
	//	cs_temp_exit *temp;
	//	temp = reinterpret_cast<cs_temp_exit *>(packet);

	//	for (int i = 0; i < MAX_USER; i++)
	//	{
	//		if (client[i].room_num == client[id].room_num)
	//		{
	//			cs_temp_exit temp;
	//			temp.size = sizeof(temp);
	//			temp.type = 14;

	//			Sendpacket(i, &temp);
	//		}
	//	}

	//	g_room[client[id].room_num]->Release();
	//	break;

	//}

	case 8: // 클라에서 메인신 들어가서 캐릭터를 만들면 서버로 보내고 
			// 서버는 받아서 캐릭터 정보를 보내준다.
	{

		for (auto p : g_room[client[id].room_num]->m_room_id)
		{
			SendPutPlayerPacket(p, p);	//방이 만들어지고 처음 위치를 보낸다.
		}

		for (int i = 0; i < g_room[client[id].room_num]->m_room_id.size() - 1; i++)	//방안에 있는 사람들한테 보내야 한다.
		{
			if (id != g_room[client[id].room_num]->m_room_id[i])
			{
				SendPutPlayerPacket(g_room[client[id].room_num]->m_room_id[i], id);
				SendPutPlayerPacket(id, g_room[client[id].room_num]->m_room_id[i]);
				//cout << " 내 id : " << id << "    누구에게? : " << i << endl;
			}
		}


		break;
	}


	case 9:						//점령성공
	{
		auto &player_id = g_room[client[id].room_num]->m_room_id;
		sc_occupy temp;
		temp.size = sizeof(sc_occupy);
		temp.type = ePacket_OccupyTeam;

		g_room[client[id].room_num]->lock.lock();
		g_room[client[id].room_num]->m_Occupy = client[id].Red_Team;
		g_room[client[id].room_num]->m_Occupytimer = 0;
		g_room[client[id].room_num]->lock.unlock();
		for (auto p : g_room[client[id].room_num]->m_room_id)//방안에 있는 사람들한테 보내야 한다.
		{

			temp.redteam = g_room[client[id].room_num]->m_Occupy;
			Sendpacket(p, &temp);
		}
		if (g_room[client[id].room_num]->m_firstOccupy)
		{
			//add_timer(client[id].room_num, 1000, OP_OCCUPY_TIMER);
			g_room[client[id].room_num]->m_firstOccupy = false;
		}
		break;
	}

	case 10:	//총알 버튼 따로 떼어냄!
	{

		CS_Fire *Fire = reinterpret_cast<CS_Fire *>(packet);


		client[id].vl_lock.lock();
		client[id].player.SetFireDirection(Fire->FireDirection);
		client[id].player.UpdateKeyInput(0.05);
		client[id].vl_lock.unlock();

		for (auto &p : g_room[client[id].room_num]->m_room_id)
		{
			if (p != id)
			{
				sendbulletfire(p, id);
				//cout << "발포한 ID : " << id << "패킷 받는 : " << p << endl;
			}
		}
	break;
	}

	case 11:
	{
		cs_temp_exit *exit = reinterpret_cast<cs_temp_exit *>(packet);

		sc_change_scene ptr;

		ptr.size = sizeof(ptr);
		ptr.type = ePacket_SceneChange;
		ptr.Winner = exit->Winner;



		for (auto &p : g_room[client[id].room_num]->m_room_id)
		{
			Sendpacket(p, &ptr);
		}

		g_room[client[id].room_num]->Release();

		break;
	}
	default:
		cout << "unknow packet : " << (int)packet[1] << endl;
		break;

	}


}

void Accept_thread()
{

	while (true)
	{
		sockaddr_in client_addr;
		ZeroMemory(&client_addr, sizeof(SOCKADDR_IN));
		client_addr.sin_family = AF_INET;
		client_addr.sin_port = htons(SERVERPORT);
		client_addr.sin_addr.s_addr = INADDR_ANY;
		int add_size = sizeof(client_addr);

		SOCKET new_client = ::WSAAccept(g_socket, reinterpret_cast<SOCKADDR *>(&client_addr), &add_size, NULL, NULL);






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

		if (new_id == MAX_USER)
		{
			cout << "server is full\n";
			closesocket(new_client);
			continue;
		}

		cout << "Player " << new_id << " Connected " << "	" << (int)client_addr.sin_port << endl;

		// 재활용 될 소켓이므로 초기화해주어야 한다.
		client[new_id].vl_lock.lock();
		client[new_id].connected = true;
		client[new_id].sock = new_client;
		client[new_id].curr_packet_size = 0;
		client[new_id].prev_packet_data = 0;
		ZeroMemory(&client[new_id].recv_overlap, sizeof(client[new_id].recv_overlap));
		client[new_id].recv_overlap.operation = OP_RECV;
		client[new_id].recv_overlap.recv_buffer.buf =
			reinterpret_cast<CHAR *>(client[new_id].recv_overlap.socket_buff);
		client[new_id].recv_overlap.recv_buffer.len = sizeof(client[new_id].recv_overlap.socket_buff);

		client[new_id].player.SetAnimation(XMFLOAT3(0, 0, 0));
		client[new_id].player.SetFireDirection(XMFLOAT3(0, 0, 0));

		client[new_id].player.setid(new_id);

		if (client[new_id].player.Getid() % 2 == 0)	//짝수일때 레드팀
			client[new_id].Red_Team = 1;

		client[new_id].room_num = 0;
		client[new_id].vl_lock.unlock();
		//add_timer(new_id, 1000, OP_SYSTEM_TIMEER);

		CCharacterObject* pCharacter = new CCharacterObject();
		COLLISION_MGR->m_vecCharacterContainer.push_back(pCharacter);
		//입출력 포트와 클라이언트 연결


		CreateIoCompletionPort(reinterpret_cast<HANDLE>(new_client), g_hIocp, new_id, 0);
		DWORD recv_flag = 0;
		int result = WSARecv(new_client, &client[new_id].recv_overlap.recv_buffer, 1,
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

		if (0 != result) {
			int error_num = WSAGetLastError();
			if (WSA_IO_PENDING != error_num) {
				error_display("AcceptThread : WSARecv ", error_num);
			}

		}

		new_id++;
	}

}

void worker_Thread()
{
	DWORD io_size;
	unsigned long long key;
	//DWORD key;
	Overlapex *overlap;
	bool bresult;

	while (true)
	{
		bresult = GetQueuedCompletionStatus(g_hIocp, &io_size, &key,
			reinterpret_cast<LPWSAOVERLAPPED*>(&overlap), INFINITE);

		if (false == bresult)
		{
			std::cout << "Error in GQCS\n";
			int err_no = WSAGetLastError();
			if (err_no == 64) Disconnected(key);
			else error_display("GQCS : ", WSAGetLastError());
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
				unsigned psize = client[key].curr_packet_size;
				unsigned pr_size = client[key].prev_packet_data;

				//int remained = io_size;

				//남은 데이터 사이즈만큼 순회하면서 처리
				while (0 < io_size)
				{
					//if (client[key].recv_overlap.packet_size == 0)
					//{
					//	client[key].recv_overlap.packet_size = pBuff[0];
					//}
					//int required = client[key].recv_overlap.packet_size - client[key].prev_packet_data;

					if (psize == 0) psize = pBuff[0];
					if (io_size + pr_size >= psize)	//패킷완성	if (remained >= required)
					{
						unsigned char packet[MAX_PACKET_SIZE];
						memcpy(packet, client[key].packet, pr_size);
						memcpy(packet + pr_size, pBuff, psize - pr_size);
						processpacket(static_cast<volatile int>(key), packet);
						io_size -= psize - pr_size;
						pBuff += psize - pr_size;
						psize = 0;
						pr_size = 0;
						//client[key].recv_overlap.packet_size = 0;
						//client[key].prev_packet_data = 0;

					}
					else
					{
						memcpy(client[key].packet + client[key].prev_packet_data, pBuff, io_size);
						//미완성 패킷의 사이즈가 reamined만큼 증가
						pr_size += io_size;
						io_size = 0;
						pr_size = 0;
					}

				}
				client[key].curr_packet_size = psize;
				client[key].prev_packet_data = pr_size;
				DWORD flags = 0;
				WSARecv(client[key].sock,
					&client[key].recv_overlap.recv_buffer, 1,
					NULL, &flags, &client[key].recv_overlap.original_overlap, NULL);

				break;
			}
			case OP_SEND:
				delete overlap;
				break;
			case OP_MOVE:
				delete overlap;
				//add_timer((int)key, 1000, OP_MOVE);
				break;
			case OP_RESPOND:

				client[(int)key].vl_lock.lock();
				client[(int)key].player.SetPlayerHp(100);
				client[(int)key].player.SetPlayelife(false);
				client[(int)key].player.setid(key);
				client[(int)key].vl_lock.unlock();

				//SendfullHp(key, (int)key);	//처음에 자기자신에게 보내고
				SendRespond(key, (int)key);
				for (int i = 0; i < MAX_USER; i++)
				{
					if (i != key)
					{
						if (client[i].room_num == client[(int)key].room_num)
						{
							SendRespond(i, (int)key);

							//SendPositionPacket(i, (int)key);
							//cout << "리스폰 ID: " << key << " 보내는 ID : " << i << endl;
						}

					}
				}
				delete overlap;
				break;

			case OP_SYSTEM_TIMEER:
			{
				g_room[key]->lock.lock();
				g_room[key]->update();
				g_room[key]->lock.unlock();
				SendTimerpacket((int)key);
				add_timer((int)key, 10000, OP_SYSTEM_TIMEER);
				delete overlap;
				break;
			}

			default:
				cout << "Unknown Event on Worker_Thread : operation" << endl;
				cout << overlap->operation;
				delete overlap;
				break;
			}
		}
		else
		{
			cout << "overlap is NULL" << endl;
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



	CMiniDump::Begin();

	vector<thread*> vpThread;

	Initialize_server();
	for (unsigned int i = 0; i < 6; ++i)
	{
		vpThread.push_back(new thread{ worker_Thread });
	}

	thread pTimerThread{ Timer_Thread };
	thread pAcceptThread{ Accept_thread };

	pAcceptThread.join();

	pTimerThread.join();


	for (auto pThread : vpThread) {
		pThread->join();
		delete pThread;
	}

	DeleteCriticalSection(&g_CriticalSection);
	DeleteCriticalSection(&timer_lock);

	closesocket(g_socket);
	CloseHandle(g_hIocp);
	WSACleanup();

	CMiniDump::End();
}