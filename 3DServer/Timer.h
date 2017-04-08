
#pragma once

const ULONG MAX_SAMPLE_COUNT = 60; //50회의 프레임 처리시간을 누적하여 평균한다.

class CTimer
{
public:
	CTimer();
	virtual ~CTimer();

	void Tick(float fLockFPS = 0.0f);
	//타이머의 시간을 갱신
	unsigned long GetFrameRate(LPTSTR lpszString = NULL, int nCharacters = 0);
	//프레임 레이트를 반환

	volatile float CurrentTimeCounting;

	//------------------------
	//타이머 세팅
	void SetTimer(float _cycle);

	//이벤트 발생여부 리턴
	bool GetEventState() { return m_bEventState; }

	//프레임의 평균 경과 시간을 반환
	float GetTimeElapsed() const;

	volatile float GetGameCurrentTime() const;

private:
	bool m_bHardwareHasPerformanceCounter;
	//컴퓨터가 performance counter를 가지고 있는가.
	float m_fTimeScale;
	//scale counter의 양
	float m_fTimeElapsed;
	//마지막 프레임 이후 지나간 시간
	__int64 m_nCurrentTime;
	//현재의 시간
	__int64 m_nLastTime;
	//마지막 프레임의 시간
	__int64 m_PerformanceFrequency;
	//컴퓨터의 performance frequency

	float m_fFrameTime[MAX_SAMPLE_COUNT];
	//프레임시간을 누적하기 위한 배열
	ULONG m_nSampleCount;
	//누적된 프레임 획수

	unsigned long m_nCurrentFrameRate;
	//현재의 프레임 레이트
	unsigned long m_FramePerSecond;
	//초당 프레임 수
	float m_fFPSTimeElapsed;
	//프레임 레이트 계산 소요 시간

	//------------------------
	//현재까지 흐른시간을 빼주기 위함
	__int64 m_nInitTime;

	//얼마 주기로 돌건지(밀리세컨드 기준 / 1초 = 1000밀리세컨드)
	volatile float m_fTimerCycle;

	//이벤트 발생여부
	volatile bool m_bEventState;
};


