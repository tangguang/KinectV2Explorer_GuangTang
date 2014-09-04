//------------------------------------------------------------------------------
// <copyright file="HandRaisExcer.h" written by Yun Li, 2/27/2014>
//     All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#pragma once

#include "Utility.h"
#include <sapi.h>
#include <sphelper.h>


class HandRaisExcer
{
public:
	HandRaisExcer();
	~HandRaisExcer();
	BOOL GetLeftRaisStatus() const { return m_LeftRaisStatus;}
	BOOL GetRightRaisStatus() const { return m_RightRaisStatus;}
	void SetLeftRaisStatus();
	void SetRightRaisStatus();
	UINT GetLeftCount() const {return m_LeftCount;}
	UINT GetRightCount() const {return m_RightCount;}
	void Reset(); 
	FLOAT GetLeftScore() const { if (m_LeftScore) return m_LeftScore; else return 0.0f;}
	FLOAT GetRightScore() const { if (m_RightScore) return m_RightScore; else return 0.0f;}
	void CallProcess(const NUI_SKELETON_DATA& skeletonData);
	BOOL isRunningExcer() const {return m_isRunningExcer;}
	void setRunningExer(BOOL status) {m_isRunningExcer=status;}	
	static DWORD WINAPI Txt2SpeechStaticThread(PVOID lpParam);
	FLOAT GetDist2Sensor() const {return m_dist2sensor;}

private:
	FLOAT GetAngleFromHands(const Vector4 &skeletonPos1, const Vector4 &skeletonPos2);
	void computeScore();
	BOOL HandRaisExcerProcess(const NUI_SKELETON_DATA& skeletonData);
	DWORD WINAPI Txt2SpeechThread();

private:
	FLOAT   m_RaisAngle;
	FLOAT   m_MinLeftRaisAngle;
	FLOAT   m_MinRightRaisAngle;
	BOOL    m_LeftRaisStatus;
	BOOL    m_RightRaisStatus;
	UINT    m_LeftCount;
	UINT    m_RightCount;
	FLOAT   m_Score;
	FLOAT   m_LeftScore;
	FLOAT   m_RightScore;
	BOOL    m_isRunningExcer;
	BOOL    m_isHandReinitial;
	LPWSTR  m_pStrToSpeak;
	FLOAT   m_dist2sensor;
};