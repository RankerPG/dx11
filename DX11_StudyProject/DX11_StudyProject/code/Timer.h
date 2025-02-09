#pragma once

#include "framework.h"

class CTimer
{
public:
	CTimer();
	~CTimer() = default;

public:
	inline const float Get_MainTime() const { return (float)m_MainTime; }
	inline const float Get_DeltaTime() const { return (float)m_DeltaTime; }
	
	inline const BOOL& Get_isStopped() const { return m_IsStopped; }

	float Get_TotalTime();

public:
	void Reset();
	void Start();
	void Stop();
	void Tick();

private:
	double		m_SecondPerCount;
	double		m_MainTime;
	double		m_DeltaTime;

	__int64		m_iBaseTime;
	__int64		m_iPauseTime;
	__int64		m_iStopTime;
	__int64		m_iPrevTime;
	__int64		m_iCurTime;

	BOOL		m_IsStopped;
};

