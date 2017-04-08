
#include <iostream>

#include <Windows.h>

#include <math.h>
#include <tchar.h>
#include <stdlib.h>
#include <Mmsystem.h>

#include "Timer.h"

using namespace std;

CTimer::CTimer()
{
	if (QueryPerformanceFrequency((LARGE_INTEGER *)&m_PerformanceFrequency))
	{
		m_bHardwareHasPerformanceCounter = TRUE;
		QueryPerformanceCounter((LARGE_INTEGER *)&m_nLastTime);
		m_fTimeScale = 1.0f / m_PerformanceFrequency;
	}
	else
	{
		m_bHardwareHasPerformanceCounter = FALSE;
		m_nLastTime = ::timeGetTime();
		m_fTimeScale = 0.001f;
	}

	m_nSampleCount = 0;
	m_nCurrentFrameRate = 0;
	m_FramePerSecond = 0;
	m_fFPSTimeElapsed = 0.0f;

	//------------------------
	m_bEventState = false;
	m_fTimerCycle = 0.0f;
	CurrentTimeCounting = 0.0f;
}

CTimer::~CTimer()
{
}

void CTimer::SetTimer(float _cycle)
{
	m_fTimerCycle = _cycle;

	if (m_bHardwareHasPerformanceCounter)
	{
		QueryPerformanceCounter((LARGE_INTEGER *)&m_nInitTime);
	}
	else
	{
		m_nInitTime = ::timeGetTime();
	}
}

void CTimer::Tick(float fLockFPS)
{
	m_bEventState = false;
	//----------------------------

	if (m_bHardwareHasPerformanceCounter)
	{
		QueryPerformanceCounter((LARGE_INTEGER *)&m_nCurrentTime);
	}
	else
	{
		m_nCurrentTime = ::timeGetTime();
	}

	//마지막으로 이 함수를 호출한 이후 경과한 시간을 계산한다.
	float fTimeElapsed = (m_nCurrentTime - m_nLastTime) * m_fTimeScale;

	if (fLockFPS > 0.0f)
	{
		//이 함수의 파라메터(fLockFPS)가 0보다 크면 이 시간만큼 호출한 함수를 기다리게 한다.
		while (fTimeElapsed < (1.0f / fLockFPS))
		{
			if (m_bHardwareHasPerformanceCounter)
			{
				QueryPerformanceCounter((LARGE_INTEGER *)&m_nCurrentTime);
			}
			else
			{
				m_nCurrentTime = ::timeGetTime();
			}
			//마지막으로 이 함수를 호출한 이후 경과한 시간을 계산한다.
			fTimeElapsed = ((m_nCurrentTime - m_nInitTime) - m_nLastTime) * m_fTimeScale;
		}
	}

	//현재 시간을 m_nLastTime에 저장한다.
	m_nLastTime = m_nCurrentTime;

	/* 마지막 프레임 처리 시간과 현재 프레임 처리 시간의 차이가 1초보다 작으면 현재 프레임 처리 시간을 m_fFrameTime[0]에 저장한다. */
	if (fabsf(fTimeElapsed - m_fTimeElapsed) < 1.0f)
	{
		memmove(&m_fFrameTime[1], m_fFrameTime, (MAX_SAMPLE_COUNT - 1) * sizeof(float));
		m_fFrameTime[0] = fTimeElapsed;
		if (m_nSampleCount < MAX_SAMPLE_COUNT) m_nSampleCount++;
	}

	//초당 프레임 수를 1 증가시키고 현재 프레임 처리 시간을 누적하여 저장한다.
	m_FramePerSecond++;
	m_fFPSTimeElapsed += fTimeElapsed;


	if (m_fFPSTimeElapsed >(0.001f * m_fTimerCycle))
	{
		CurrentTimeCounting += 1.0f;

		m_nCurrentFrameRate = m_FramePerSecond;
		m_FramePerSecond = 0;
		m_fFPSTimeElapsed = 0.0f;

		//----------------------------
		m_bEventState = true;
	}

	//누적된 프레임 처리 시간의 평균을 구하여 프레임 처리 시간을 구한다.
	m_fTimeElapsed = 0.0f;
	for (ULONG i = 0; i < m_nSampleCount; i++) m_fTimeElapsed += m_fFrameTime[i];
	if (m_nSampleCount > 0) m_fTimeElapsed /= m_nSampleCount;
}


float CTimer::GetTimeElapsed() const
{
	return(m_fTimeElapsed);
}

volatile float CTimer::GetGameCurrentTime() const
{
	return (CurrentTimeCounting);
}