//------------------------------------------------------------------------------
// <copyright file="LegRaisExcer.h" written by Yun Li, 10/23/2013>
//     All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#pragma once

#include "Utility.h"
#include <sapi.h>
#include <sphelper.h>


class LegRaisExcer
{
public:
	LegRaisExcer();
	~LegRaisExcer();
	BOOL GetLeftRaisStatus() const { return m_LeftRaisStatus;}
	BOOL GetRightRaisStatus() const { return m_RightRaisStatus;}
	void SetLeftRaisStatus();
	void SetRightRaisStatus();
	UINT GetLeftCount() const {return m_LeftCount;}
	UINT GetRightCount() const {return m_RightCount;}
	void Reset(); 
	FLOAT GetLeftScore() const;
	FLOAT GetRightScore() const;
	void CallProcess(const NUI_SKELETON_DATA& skeletonData);
	BOOL isRunningExcer() const {return m_isRunningExcer;}
	void setRunningExer(BOOL status) {m_isRunningExcer=status;}
	static DWORD WINAPI Txt2SpeechStaticThread(PVOID lpParam);

private:
	FLOAT GetAngleFromLegs(const Vector4 &skeletonPos1, const Vector4 &skeletonPos2);
	void computeScore();
	BOOL LegRaisExcerProcess(const NUI_SKELETON_DATA& skeletonData);
	DWORD WINAPI Txt2SpeechThread();
	
private:
	FLOAT   m_RaisAngle;
	FLOAT   m_MinLeftRaisAngle;
	FLOAT   m_MinRightRaisAngle;
	BOOL    m_LeftRaisStatus;
	BOOL    m_RightRaisStatus;
	UINT    m_LeftCount;
	UINT    m_RightCount;
	FLOAT   m_LeftScore;
	FLOAT   m_RightScore;
	BOOL    m_isRunningExcer;
	BOOL    m_isLeginitial;
	LPWSTR  m_pStrToSpeak;
	FLOAT   m_dist2sensor;
};