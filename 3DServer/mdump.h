#define WIN32_LEAN_AND_MEAN		// ���� ������ �ʴ� ������ Windows ������� �����մϴ�.

class CMiniDump
{
public:
	static BOOL Begin(VOID);
	static BOOL End(VOID);
};