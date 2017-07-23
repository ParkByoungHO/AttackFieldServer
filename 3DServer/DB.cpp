#include "stdafx.h"
#include "DB.h"


CDB::CDB()
{
	wcout.imbue(locale("ko-kr"));
	wcin.imbue(locale("ko-kr"));
}


CDB::~CDB()
{
	SQLCancel(hstmt);
	SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
	SQLDisconnect(hdbc);
	SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
	SQLFreeHandle(SQL_HANDLE_ENV, henv);
}

// 문자열 우측 공백문자 삭제 함수
TCHAR* rtrim(TCHAR* s) {
	TCHAR t[100];
	TCHAR *end;

	// Visual C 2003 이하에서는
	// strcpy(t, s);
	// 이렇게 해야 함
	lstrcpy(t, s); // 이것은 Visual C 2005용
	end = t + lstrlen(t) - 1;
	while (end != t && isspace(*end))
		end--;
	*(end + 1) = '\0';
	lstrcpy(s, t); // 이것은 Visual C 2005용
	return s;
}


// 문자열 좌측 공백문자 삭제 함수
TCHAR* ltrim(TCHAR *s) {
	TCHAR* begin;
	begin = s;

	while (*begin != '\0') {
		if (isspace(*begin))
			begin++;
		else {
			s = begin;
			break;
		}
	}

	return s;
}
TCHAR* trim(TCHAR *s) {
	return rtrim(ltrim(s));
}

void CDB::connect()
{
	SQLINTEGER num;
	SQLWCHAR siID[10];
	SQLWCHAR sipassword[10];

	SQLLEN cbnum, cbid, cpassword = 0;


	SQLRETURN retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

	// Set the ODBC version environment attribute  
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
	{
		retcode = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER*)SQL_OV_ODBC3, 0);

		// Allocate connection handle  
		if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
		{
			retcode = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);

			// Set login timeout to 5 seconds  
			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
			{
				SQLSetConnectAttr(hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

				// Connect to data source  
				retcode = SQLConnect(hdbc, static_cast<SQLWCHAR*>(L"game"), SQL_NTS, nullptr, 0, nullptr, 0);

				// Allocate statement handle  
				if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
				{
					retcode = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);

					//   retcode = SQLExecDirect(hstmt, static_cast<SQLWCHAR *>(L"SELECT ID, Name, Lv FROM dbo.user_data ORDER BY 2, 1, 3"), SQL_NTS);
					wstring strQuery = L"EXEC dbo.Getinfo"s;
					retcode = SQLExecDirect(hstmt, static_cast<SQLWCHAR *>(const_cast<SQLWCHAR*>(strQuery.c_str())), SQL_NTS);
					if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
					{


						// Bind columns 1, 2, and 3  
						retcode = SQLBindCol(hstmt, 1, SQL_INTEGER, &num, sizeof(SQLINTEGER), &cbnum);
						retcode = SQLBindCol(hstmt, 2, SQL_C_WCHAR, &siID, sizeof(SQLWCHAR) * 10, &cbid);
						retcode = SQLBindCol(hstmt, 3, SQL_C_WCHAR, &sipassword, sizeof(SQLWCHAR) * 10, &cpassword);


						// Fetch and print each row of data. On an error, display a message and exit.  
						for (int i = 0; ; i++) {
							retcode = SQLFetch(hstmt);
							if (retcode == SQL_ERROR || retcode == SQL_SUCCESS_WITH_INFO)
								;// cout << "error" << endl;

							if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
							{
								player.push_back(new CLIENT_info);
								player[i]->num = num;
								siID[9] = NULL;
								sipassword[9] = NULL;
								lstrcpy(player[i]->id, trim(siID));
								lstrcpy(player[i]->password, trim(sipassword));
								player[i]->num = num;
							}
							else
								break;
						}
					}
					SQLCloseCursor(hstmt);



				}
			}
		}
	}



}

void CDB::update(CLIENT_info *temp)
{
	SQLWCHAR EXECORDER[256];
	SQLLEN cbnum, cbid = 0, cbx = 0, cby = 0, cbhp = 0, cblv = 0, cbex = 0, cbinventory = 0,
		cbweapon_eqip = 0, cbshield_eqip = 0, cbgold = 0;


	wsprintf(EXECORDER, L"EXEC dbo.Setinfo '%s', %s", temp->id, temp->password);







	SQLRETURN retcode = SQLExecDirect(hstmt, EXECORDER, SQL_NTS);
	if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
	{

		for (int i = 0; ; i++)
		{
			retcode = SQLFetch(hstmt);

			if (retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
			{
				// 후처리
			}
			else break;
		}

	}

	SQLCloseCursor(hstmt);

}