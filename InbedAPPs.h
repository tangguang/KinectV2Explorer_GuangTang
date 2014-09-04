//------------------------------------------------------------------------------
// <copyright file="InbedAPPs.h" written by Yun Li, 11/7/2013>
//     All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#pragma once

#include "Utility.h"
#include <sapi.h>
#include <sphelper.h>

typedef enum _HandsRaiseEvaluation
{
	InValidEvaluation = 0,
	RaiseUp,
	RaiseDown,
	RaiseRight1,
	RaiseRight2,
	RaiseRight3,
	RaiseLeft1,
	RaiseLeft2,
	RaiseLeft3,
	EvaluationCount
} HandsRaiseEvaluation;

class InbedAPPs
{
public:
	InbedAPPs();
	~InbedAPPs();
	void CallProcess(const NUI_SKELETON_DATA& skeletonData, const NUI_SKELETON_FRAME& skeletonFrame);
	void setIsRunFallDetect (BOOL isRun) {m_isRunFallDetect = isRun; m_FallStatus = FALSE; }
	void setIsRunMovementDetect (BOOL isRun) {m_isRunMovementDetect = isRun; m_MoveStatus = FALSE;}
	inline void setIsRunHandsMovementRIC (BOOL isRun) {m_isRunHandsMovementRIC = isRun; m_isLeftHandRaise = FALSE;
	m_isRightHandRaise = FALSE; m_HandRaiseEvaluate = InValidEvaluation;}
	void setReferHandPos(Vector4& ReferHandPos);
	void setRightHandRaise(BOOL RightHandRaise) { m_isRightHandRaise = RightHandRaise;}
	void setLeftHandRaise(BOOL LeftHandRaise) { m_isLeftHandRaise = LeftHandRaise;}
	BOOL getFallStatus() const {return m_FallStatus;}
	BOOL getMovementStatus() const {return m_MoveStatus;}
	BOOL getIsRunFallDetect() const {return m_isRunFallDetect;}
	BOOL getIsRunMovementDetect() const {return m_isRunMovementDetect;}
	BOOL getIsRunHandsMovementRIC() const {return m_isRunHandsMovementRIC;}
	BOOL getRightHandRaise() const {return m_isRightHandRaise;}
	BOOL getLeftHandRaise() const {return m_isLeftHandRaise;}
	HandsRaiseEvaluation getHandRaiseEvaluate() const {return m_HandRaiseEvaluate; }
	Vector4 getLeftHandPos() const {return m_LeftHandPos;}
	Vector4 getRightHandPos() const {return m_RightHandPos;}

	static DWORD WINAPI Txt2SpeechStaticThread(PVOID lpParam);

private:
	UINT    m_FrameCount;
	FLOAT** m_ppDists;
	FLOAT*  m_pCulmuSum;
	FLOAT*  m_pLastAvg;
	FLOAT   m_CurDistMean;
	FLOAT   m_armLength;
	BOOL    m_FallStatus;
	BOOL    m_MoveStatus;
	BOOL    m_isRunFallDetect;
	BOOL    m_isRunMovementDetect;
	BOOL    m_isRunHandsMovementRIC;
	BOOL    m_isLeftHandRaise;
	BOOL    m_isRightHandRaise;
	UINT    m_countFalls;
	UINT    m_countMoves;
	HandsRaiseEvaluation    m_HandRaiseEvaluate;
	Vector4 m_LeftHandPos;
	Vector4 m_RightHandPos;
	Vector4 m_ReferHandPos;
	Vector4 m_HeadPos;
	LPWSTR  m_pStrToSpeak;

private:
	FLOAT ComputeDist(const Vector4& JointPos, FLOAT A=-0.045f, FLOAT B=0.93f, FLOAT C=-0.364, FLOAT D=1.546);
	FLOAT ComputeMSE();
    void  normDist();
	void  setFallStatus();
	void  setMoveStatus();
	void  addToSum(const Vector4& JointPos, UINT startIdx);
	void  SaveMoveTime();
	void  SaveFallTime();
	inline void  setHandsPos(const Vector4& LeftHandPos, const Vector4& RightHandPos)
	{m_LeftHandPos = LeftHandPos; m_RightHandPos = RightHandPos;}
	void HandPosClassify();


	DWORD WINAPI Txt2SpeechThread();
};

