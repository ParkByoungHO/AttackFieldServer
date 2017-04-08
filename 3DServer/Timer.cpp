
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

	//���������� �� �Լ��� ȣ���� ���� ����� �ð��� ����Ѵ�.
	float fTimeElapsed = (m_nCurrentTime - m_nLastTime) * m_fTimeScale;

	if (fLockFPS > 0.0f)
	{
		//�� �Լ��� �Ķ����(fLockFPS)�� 0���� ũ�� �� �ð���ŭ ȣ���� �Լ��� ��ٸ��� �Ѵ�.
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
			//���������� �� �Լ��� ȣ���� ���� ����� �ð��� ����Ѵ�.
			fTimeElapsed = ((m_nCurrentTime - m_nInitTime) - m_nLastTime) * m_fTimeScale;
		}
	}

	//���� �ð��� m_nLastTime�� �����Ѵ�.
	m_nLastTime = m_nCurrentTime;

	/* ������ ������ ó�� �ð��� ���� ������ ó�� �ð��� ���̰� 1�ʺ��� ������ ���� ������ ó�� �ð��� m_fFrameTime[0]�� �����Ѵ�. */
	if (fabsf(fTimeElapsed - m_fTimeElapsed) < 1.0f)
	{
		memmove(&m_fFrameTime[1], m_fFrameTime, (MAX_SAMPLE_COUNT - 1) * sizeof(float));
		m_fFrameTime[0] = fTimeElapsed;
		if (m_nSampleCount < MAX_SAMPLE_COUNT) m_nSampleCount++;
	}

	//�ʴ� ������ ���� 1 ������Ű�� ���� ������ ó�� �ð��� �����Ͽ� �����Ѵ�.
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

	//������ ������ ó�� �ð��� ����� ���Ͽ� ������ ó�� �ð��� ���Ѵ�.
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