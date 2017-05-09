#pragma once

const ULONG MAX_SAMPLE_COUNT = 50; // Maximum frame time sample count

class CGameTimer
{
public:
	CGameTimer();
	virtual ~CGameTimer();

	void Tick(float fLockFPS = 0.0f);

	unsigned long GetFrameRate() const { return m_nCurrentFrameRate; }
	unsigned long GetRealFrameRate() const { return m_nRealFrameRate; }

	float GetTimeElapsed() const { return m_fDeltaTime; }

private:
    bool							m_bHardwareHasPerformanceCounter;   // Has Performance Counter
	float							m_fTimeScale;						// Amount to scale counter
	float							m_fDeltaTime;						// Time elapsed since previous frame
    __int64							m_nCurrentTime;						// Current Performance Counter
    __int64							m_nLastTime;						// Performance Counter last frame
	__int64							m_PerformanceFrequency;				// Performance Frequency

    float							m_fFrameTime[MAX_SAMPLE_COUNT];
    ULONG							m_nSampleCount;

    unsigned long					m_nCurrentFrameRate;				// Stores current framerate
	unsigned long					m_nFramePerSecond;					// Elapsed frames in any given second
	float							m_fFPSTimeElapsed;					// How much time has passed during FPS sample
	
	__int64							m_nRealCurrentTime;
	unsigned long					m_nRealFrameRate = 0;
	unsigned long					m_nRealFramePerSecond;
	float							m_fRealFPSTimeElapsed = 0.0f;
};