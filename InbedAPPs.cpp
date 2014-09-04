//------------------------------------------------------------------------------
// <copyright file="InbedAPPs.cpp" written by Yun Li, 11/7/2013>
//     All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#include "stdafx.h"
#include <stdio.h>
#include <NuiApi.h>
#include <math.h>
#include "InbedAPPs.h"
#include "NuiAudioStream.h"

#define PI 3.141592654

static const UINT BufferSizeOfFallDetect = 50;
static const UINT WindowSizeOfMovementDetect = 100;
static const UINT BufferSizeOfLyAngleDetect = 30;
static const UINT NumOfJoints = 10;
static const FLOAT DistThr = 0.2f;
static const FLOAT MaxDist = 1.5f;
static const FLOAT MinDist = 0.0f;
static const FLOAT MovingThr = 0.01f;
static const FLOAT RaisUpDownThr = 0.3f;
static const FLOAT LeftRightThr1 = 0.2f;
static const FLOAT LeftRightThr2 = 0.4f;
static const FLOAT LeftRightThr3 = 0.6f;

InbedAPPs::InbedAPPs():
	m_FrameCount(0),
	m_CurDistMean(0.0f),
	m_MoveStatus(FALSE),
	m_FallStatus(FALSE),
	m_isRunFallDetect(FALSE),
	m_isRunMovementDetect(FALSE),
	m_isRunHandsMovementRIC(FALSE),
	m_isLeftHandRaise(FALSE),
	m_isRightHandRaise(FALSE),
	m_countFalls(0),
	m_countMoves(0),
	m_HandRaiseEvaluate(InValidEvaluation),
	m_armLength(0.0f)
{
	m_ppDists =new FLOAT*[NumOfJoints];
	for (int joint = 0; joint < NumOfJoints; joint++)
	{
		m_ppDists[joint]=new FLOAT[BufferSizeOfFallDetect];
		for (int buf = 0; buf < BufferSizeOfFallDetect; buf++)
			m_ppDists[joint][buf] = 0.0f;
	}
	m_pCulmuSum = new FLOAT[NumOfJoints*3];
	m_pLastAvg = new FLOAT[NumOfJoints*3];
	for (int k=0; k<NumOfJoints*3; k++)
	{
		m_pCulmuSum[k] = 0.0f;
		m_pLastAvg[k] = 0.0f;
	}
	m_pStrToSpeak = new WCHAR[256];
}

InbedAPPs::~InbedAPPs()
{
	for (int joint = 0; joint < NumOfJoints; joint++)
		SafeDelete(m_ppDists[joint]);
	SafeDelete(m_ppDists);
	SafeDelete(m_pCulmuSum);
	SafeDelete(m_pLastAvg);
	SafeDelete(m_pStrToSpeak);
}

FLOAT InbedAPPs::ComputeDist(const Vector4& JointPos, FLOAT A, FLOAT B, FLOAT C, FLOAT D)
{
	FLOAT distNumer = A * JointPos.x + B * JointPos.y + C* JointPos.z + D ;
	FLOAT distDenom = sqrtf (pow(A, 2) + pow(B, 2) + pow(C, 2)) ;
	return abs(distNumer/distDenom);
}

void InbedAPPs::CallProcess(const NUI_SKELETON_DATA& skeletonData, const NUI_SKELETON_FRAME& skeletonFrame)
{
	//// Set head position
	m_HeadPos = skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_HEAD];
	//// Get estimated floor coefficients
	FLOAT A = skeletonFrame.vFloorClipPlane.x;
	FLOAT B = skeletonFrame.vFloorClipPlane.y;
	FLOAT C = skeletonFrame.vFloorClipPlane.z;
	FLOAT D = skeletonFrame.vFloorClipPlane.w;
	////////////////////////////////Fall detection///////////////////////////////////////////
	if (m_isRunFallDetect)
	{
		//// Write the distance of each joint into buffer
		UINT idx = m_FrameCount % BufferSizeOfFallDetect;
		m_ppDists[0][idx] = (ComputeDist (skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_HEAD], A, B, C, D) - MinDist) / (MaxDist - MinDist);
		m_ppDists[1][idx] = (ComputeDist (skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_CENTER], A, B, C, D) - MinDist) / (MaxDist - MinDist);
		m_ppDists[2][idx] = (ComputeDist (skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_LEFT], A, B, C, D) - MinDist) / (MaxDist - MinDist);
		m_ppDists[3][idx] = (ComputeDist (skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_ELBOW_LEFT], A, B, C, D) - MinDist) / (MaxDist - MinDist);
		m_ppDists[4][idx] = (ComputeDist (skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_WRIST_LEFT], A, B, C, D) - MinDist) / (MaxDist - MinDist);
		m_ppDists[5][idx] = (ComputeDist (skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_HAND_LEFT], A, B, C, D) - MinDist) / (MaxDist - MinDist);
		m_ppDists[6][idx] = (ComputeDist (skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_RIGHT], A, B, C, D) - MinDist) / (MaxDist - MinDist);
		m_ppDists[7][idx] = (ComputeDist (skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_ELBOW_RIGHT], A, B, C, D) - MinDist) / (MaxDist - MinDist);
		m_ppDists[8][idx] = (ComputeDist (skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_WRIST_RIGHT], A, B, C, D) - MinDist) / (MaxDist - MinDist);
		m_ppDists[9][idx] = (ComputeDist (skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_HAND_RIGHT], A, B, C, D) - MinDist) / (MaxDist - MinDist);

		if ((m_FrameCount+1) % (BufferSizeOfFallDetect/2) == 0)   ///overlap rate 0.5
		{
			//// Compute the mean of each joint distances
			FLOAT SUM, SUM1 = 0;
			for (int joint = 0; joint < NumOfJoints; joint++)
			{
				SUM = 0;
				for (int k = 0; k < BufferSizeOfFallDetect; k++)
				{
					SUM += m_ppDists[joint][k];
				}
				SUM1 += SUM/BufferSizeOfFallDetect;
			}
			//// Compute the mean across the joints
			m_CurDistMean = SUM1 / NumOfJoints;
			setFallStatus();
		}
	}
	////////////////////////////////Movement detection///////////////////////////////////////////
	if (m_isRunMovementDetect)
	{
		addToSum(skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_HEAD], 0);
		addToSum(skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_CENTER], 3);
		addToSum(skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_LEFT], 6);
		addToSum(skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_ELBOW_LEFT], 9);
		addToSum(skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_WRIST_LEFT], 12);
		addToSum(skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_HAND_LEFT], 15);
		addToSum(skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_RIGHT], 18);
		addToSum(skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_ELBOW_RIGHT], 21);
		addToSum(skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_WRIST_RIGHT], 24);
		addToSum(skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_HAND_RIGHT], 27);

		if ((m_FrameCount+1) % WindowSizeOfMovementDetect == 0)
		{
			setMoveStatus();
		}
	}

	////////////////////////////////Hands moving RIC///////////////////////////////////////////
	if (m_isRunHandsMovementRIC)
	{
		setHandsPos(skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_HAND_LEFT], 
			skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_HAND_RIGHT]);
		if (m_isLeftHandRaise || m_isRightHandRaise)
		{
			HandPosClassify();
		}
	}

	////Increase or reset m_FrameCount
	if ((m_FrameCount+1) % 10000 !=0) m_FrameCount++;
	else m_FrameCount = 0;
}

void InbedAPPs::setFallStatus()
{
	if (m_FrameCount >= 150 && m_CurDistMean < DistThr)   /// A fall is detected
	{
		if (!m_FallStatus)
		{
			wsprintf(m_pStrToSpeak, L"%s", L"Fall");
			m_FallStatus = TRUE;
			m_countFalls++;
			////Record the time of the detected fall event
			SaveFallTime();
		}
	}
	else
		m_FallStatus = FALSE;
}

void InbedAPPs::addToSum(const Vector4& JointPos, UINT startIdx)
{
	m_pCulmuSum[startIdx] += JointPos.x;
	m_pCulmuSum[startIdx + 1] += JointPos.y;
	m_pCulmuSum[startIdx + 2] += JointPos.z;
}

FLOAT InbedAPPs::ComputeMSE()
{
	FLOAT Sum = 0.0f;
	for (int k = 0; k < NumOfJoints*3; k++)
		Sum += pow(m_pCulmuSum[k]- m_pLastAvg[k], 2);
	return Sum/NumOfJoints;
}

void InbedAPPs::setMoveStatus()
{
	////Take avearge
	for (int k = 0; k < NumOfJoints*3; k++)
		m_pCulmuSum[k] /= WindowSizeOfMovementDetect;

	FLOAT MSE = ComputeMSE();

	if (m_FrameCount >= 300 && MSE > MovingThr)   /// A movement is detected
	{
		if (!m_MoveStatus)
		{
			m_MoveStatus = TRUE;
			++m_countMoves;

			////Record the time of the detected movement event
			SaveMoveTime();
			if ((m_countMoves) % 10000 == 0) m_countMoves = 0;
		}
	}
	else
		m_MoveStatus = FALSE;

	////Update m_pLastAvg and reset m_pCulmuSum
	for (int k = 0; k < NumOfJoints*3; k++)
	{
		m_pLastAvg[k] = m_pCulmuSum[k];
		m_pCulmuSum[k] = 0.0f;
	}
	
}

void InbedAPPs::setReferHandPos(Vector4& ReferHandPos) 
{
	m_ReferHandPos = ReferHandPos;
	//// Measure the rough length of the arm
	m_armLength = abs (m_HeadPos.z - m_ReferHandPos.z);
}

void InbedAPPs::HandPosClassify()
{
	if (!m_isRightHandRaise || !m_isLeftHandRaise)
	{
		if ((m_isRightHandRaise && RaisUpDownThr < (m_RightHandPos.y - m_ReferHandPos.y)) || 
			(m_isLeftHandRaise && RaisUpDownThr < (m_LeftHandPos.y - m_ReferHandPos.y)))
			m_HandRaiseEvaluate = RaiseUp;   //UP
		else if ((m_isRightHandRaise && RaisUpDownThr < (m_ReferHandPos.y - m_RightHandPos.y)) || 
			(m_isLeftHandRaise && RaisUpDownThr < (m_ReferHandPos.y - m_LeftHandPos.y)))
			m_HandRaiseEvaluate = RaiseDown;   //DOWN
		else if ((m_isRightHandRaise && LeftRightThr1 < (m_RightHandPos.x - m_ReferHandPos.x) &&
			(m_RightHandPos.x - m_ReferHandPos.x) <= LeftRightThr2) ||
			(m_isLeftHandRaise && LeftRightThr1 < (m_LeftHandPos.x - m_ReferHandPos.x) &&
			(m_LeftHandPos.x - m_ReferHandPos.x) <= LeftRightThr2))
			m_HandRaiseEvaluate = RaiseRight1;  //Right1
		else if ((m_isRightHandRaise && LeftRightThr2 < (m_RightHandPos.x - m_ReferHandPos.x) &&
			(m_RightHandPos.x - m_ReferHandPos.x) <= LeftRightThr3) ||
			(m_isLeftHandRaise && LeftRightThr2 < (m_LeftHandPos.x - m_ReferHandPos.x) &&
			(m_LeftHandPos.x - m_ReferHandPos.x) <= LeftRightThr3))
			m_HandRaiseEvaluate = RaiseRight2;  //Right2
		else if ((m_isRightHandRaise && LeftRightThr3 < (m_RightHandPos.x - m_ReferHandPos.x)) ||
			(m_isLeftHandRaise && LeftRightThr3 < (m_LeftHandPos.x - m_ReferHandPos.x)))
			m_HandRaiseEvaluate = RaiseRight3;  //Right3
		else if ((m_isRightHandRaise && LeftRightThr1 < (m_ReferHandPos.x - m_RightHandPos.x) &&
			(m_ReferHandPos.x - m_RightHandPos.x) <= LeftRightThr2) ||
			(m_isLeftHandRaise && LeftRightThr1 < (m_ReferHandPos.x - m_LeftHandPos.x) &&
			(m_ReferHandPos.x - m_LeftHandPos.x) <= LeftRightThr2))
			m_HandRaiseEvaluate = RaiseLeft1;  //Left1
		else if ((m_isRightHandRaise && LeftRightThr2 < (m_ReferHandPos.x - m_RightHandPos.x) &&
			(m_ReferHandPos.x - m_RightHandPos.x) <= LeftRightThr3) ||
			(m_isLeftHandRaise && LeftRightThr2 < (m_ReferHandPos.x - m_LeftHandPos.x) &&
			(m_ReferHandPos.x - m_LeftHandPos.x) <= LeftRightThr3))
			m_HandRaiseEvaluate = RaiseLeft2;  //Left2
		else if ((m_isRightHandRaise && LeftRightThr3 < (m_ReferHandPos.x - m_RightHandPos.x)) ||
			(m_isLeftHandRaise && LeftRightThr3 < (m_ReferHandPos.x - m_LeftHandPos.x)))
			m_HandRaiseEvaluate = RaiseLeft3;  //Left3
		else
			m_HandRaiseEvaluate = InValidEvaluation;
	}
	else
		m_HandRaiseEvaluate = InValidEvaluation;
}


void InbedAPPs::SaveMoveTime()
{
	wchar_t FilePath[MAX_PATH];
	swprintf_s(FilePath, MAX_PATH, L"%s\\KinectSkeletonPos\\MoveRecords\\MoveRec.txt", knownPath);
	FILE* pFile;
	_wfopen_s(&pFile, FilePath, L"a+");
	
	// Get the time
    wchar_t timeString[MAX_PATH];
	wchar_t dateString[MAX_PATH];
    GetTimeFormatEx(NULL, TIME_FORCE24HOURFORMAT, NULL, L"hh':'mm':'ss", timeString, _countof(timeString));
	GetDateFormatEx(NULL, 0, NULL, L"yyyy'-'MMM'-'dd", dateString, _countof(dateString), NULL);
	
	//print the time
	fwprintf(pFile, L"%d %s %s\n", m_countMoves, dateString, timeString);
	fclose(pFile);
}

void InbedAPPs::SaveFallTime()
{
	wchar_t FilePath[MAX_PATH];
	swprintf_s(FilePath, MAX_PATH, L"%s\\KinectSkeletonPos\\FallRecords\\FallRec.txt", knownPath);
	FILE* pFile;
	_wfopen_s(&pFile, FilePath, L"a+");

	// Get the time
    wchar_t timeString[MAX_PATH];
	wchar_t dateString[MAX_PATH];
    GetTimeFormatEx(NULL, TIME_FORCE24HOURFORMAT, NULL, L"hh':'mm':'ss", timeString, _countof(timeString));
	GetDateFormatEx(NULL, 0, NULL, L"yyyy'-'MMM'-'dd", dateString, _countof(dateString), NULL);
	
	//print the time
	fwprintf(pFile, L"%d %s %s\n", m_countFalls, dateString, timeString);
	fclose(pFile);
}

DWORD WINAPI InbedAPPs::Txt2SpeechStaticThread(PVOID lpParam)
{
	InbedAPPs* context = static_cast<InbedAPPs*>(lpParam);
	if (context)
	{
		return context->Txt2SpeechThread();
	}
	return 0;
}

DWORD WINAPI InbedAPPs::Txt2SpeechThread()
{
		
		HRESULT                        hr = S_OK;
		CComPtr<ISpObjectToken>        cpAudioOutToken;
		CComPtr<IEnumSpObjectTokens>   cpEnum;
		CComPtr<ISpVoice>              cpVoice;
		ULONG                          ulCount = 0;

		if (FAILED(::CoInitialize(NULL)))
				return FALSE; 
		// Create the SAPI voice.
		hr = cpVoice.CoCreateInstance(CLSID_SpVoice);

		if (SUCCEEDED (hr))
		{
		   // Enumerate the available audio output devices.
		   hr = SpEnumTokens( SPCAT_AUDIOOUT, NULL, NULL, &cpEnum);
		}

		if (SUCCEEDED (hr))
		{
		   // Get the number of audio output devices.
		   hr = cpEnum->GetCount( &ulCount);
		}

		if (SUCCEEDED (hr))
		{
			hr = cpEnum->Next( 1, &cpAudioOutToken, NULL );
		}

		if (SUCCEEDED (hr))
		{
			hr = cpVoice->SetOutput( cpAudioOutToken, TRUE );
		}

		if (SUCCEEDED (hr))
		{
			while (TRUE)
			{
				if (m_FallStatus)
				{
					   Sleep(100);   //Refresh m_pStrToSpeak
					   cpVoice->SetRate(-1);
					   cpVoice->Speak(m_pStrToSpeak, SPF_DEFAULT, NULL);
				}
			    
			}
		}

		::CoUninitialize();

		if (FAILED(hr)) return FALSE;

	return TRUE;
}

