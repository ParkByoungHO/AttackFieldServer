#include "stdafx.h"
#include "mdump.h"


typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)( // Callback 함수의 원형
		HANDLE hProcess,
		DWORD dwPid,
		HANDLE hFile,
		MINIDUMP_TYPE DumpType,
		CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
		CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
		CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

LPTOP_LEVEL_EXCEPTION_FILTER PreviousExceptionFilter = NULL;



LONG WINAPI UnHandledExceptionFilter(struct _EXCEPTION_POINTERS *exceptionInfo)		//오류 날때 부르는 함수
{
	HMODULE	DllHandle		= NULL;

	// Windows 2000 이전에는따로DBGHELP를배포해서설정해주어야한다.
	DllHandle			= LoadLibrary(_T("DBGHELP.DLL"));

	if (DllHandle)
	{
		MINIDUMPWRITEDUMP Dump = (MINIDUMPWRITEDUMP) GetProcAddress(DllHandle, "MiniDumpWriteDump");	//처음 주소를 가져오고

		if (Dump)
		{
			TCHAR		DumpPath[MAX_PATH] = {0,};
			SYSTEMTIME	SystemTime;

			GetLocalTime(&SystemTime);			

			_sntprintf_s(DumpPath, MAX_PATH, _T("%d-%d-%d %d_%d_%d.dmp"),	//파일을 만들떄 다른파일로 만들어야 하므로  파일 이름에 
				SystemTime.wYear,											// 년도 몇월 몇시 몇초 다 넣는다.
				SystemTime.wMonth,
				SystemTime.wDay,
				SystemTime.wHour,
				SystemTime.wMinute,
				SystemTime.wSecond);
			HANDLE FileHandle = CreateFile(									//파일을 만들고 전체적으로 덤프를 넣는다.
				DumpPath, 
				GENERIC_WRITE, 
				FILE_SHARE_WRITE, 
				NULL, CREATE_ALWAYS, 
				FILE_ATTRIBUTE_NORMAL, 
				NULL);

			if (FileHandle != INVALID_HANDLE_VALUE)
			{
				_MINIDUMP_EXCEPTION_INFORMATION MiniDumpExceptionInfo;
				
				MiniDumpExceptionInfo.ThreadId			= GetCurrentThreadId();
				MiniDumpExceptionInfo.ExceptionPointers	= exceptionInfo;
				MiniDumpExceptionInfo.ClientPointers	= NULL;

				BOOL Success = Dump(
					GetCurrentProcess(), 
					GetCurrentProcessId(), 
					FileHandle, 
					MiniDumpNormal, 
					&MiniDumpExceptionInfo, 
					NULL, 
					NULL);
				if (Success)
				{
					CloseHandle(FileHandle);

					return EXCEPTION_EXECUTE_HANDLER;
				}
			}

			CloseHandle(FileHandle);
		}
	}

	return EXCEPTION_CONTINUE_SEARCH;
}

BOOL CMiniDump::Begin(VOID)	
{
	SetErrorMode(SEM_FAILCRITICALERRORS);	//에러가 날때 셋팅

	PreviousExceptionFilter = SetUnhandledExceptionFilter(UnHandledExceptionFilter); // 죽을때 이함수를 호출하고 죽어라.

	return true;
}   //시작

BOOL CMiniDump::End(VOID)
{
	SetUnhandledExceptionFilter(PreviousExceptionFilter);

	return true;
}	 //끝
