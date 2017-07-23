#pragma once

struct CLIENT_info
{
	int num;
	wchar_t id[10];
	wchar_t password[10];
};

class CDB
{
public:
	void connect();
	void update(CLIENT_info *info);

	CDB();
	~CDB();


	std::vector<CLIENT_info *> GetPlayer_info() { return player; }
	

private:

	SQLHENV henv;
	SQLHDBC hdbc;
	SQLHSTMT hstmt = 0;
	SQLHSTMT hstmt1 = 0;

	std::vector<CLIENT_info *> player;
};

