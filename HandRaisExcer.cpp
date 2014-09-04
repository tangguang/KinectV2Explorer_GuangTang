//------------------------------------------------------------------------------
// <copyright file="HandRaisExcer.cpp" written by Yun Li, 2/27/2014>
//     All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#include "stdafx.h"
#include <NuiApi.h>
#include "HandRaisExcer.h"
#include <math.h>

#define PI 3.14159265f

HandRaisExcer::HandRaisExcer():
    m_RaisAngle(90.0f),
	m_MinLeftRaisAngle(90.0f),
	m_MinRightRaisAngle(90.0f),
	m_LeftRaisStatus(FALSE),
	m_RightRaisStatus(FALSE),
	m_LeftCount(0),
	m_RightCount(0),
	m_LeftScore(0.0f),
	m_RightScore(0.0f),
	m_dist2sensor(0.0f),
	m_isHandReinitial(FALSE),
    m_isRunningExcer(FALSE){
		m_pStrToSpeak = new WCHAR[256];
	}

HandRaisExcer::~HandRaisExcer()
{
	SafeDelete(m_pStrToSpeak);
}

FLOAT HandRaisExcer::GetAngleFromHands(const Vector4 &skeletonPos1, const Vector4 &skeletonPos2)
{
	FLOAT SlopL;
	if (skeletonPos2.x!=skeletonPos1.x)
	{
		SlopL = (skeletonPos2.y-skeletonPos1.y)/(skeletonPos2.x-skeletonPos1.x);
		return abs(atan(SlopL) * 180.0f / PI);
	}
	else
		return 90.0f;
}

void HandRaisExcer::SetLeftRaisStatus()
{
	if (!m_LeftRaisStatus)
		m_LeftRaisStatus = TRUE;
}

void HandRaisExcer::SetRightRaisStatus()
{
	if (!m_RightRaisStatus)
		m_RightRaisStatus = TRUE;
}

BOOL HandRaisExcer::HandRaisExcerProcess(const NUI_SKELETON_DATA& skeletonData)
{
	//// distance filter
	Vector4 skeletonHead = skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_HEAD];
	Vector4 skeletonShoulderCenter = skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_CENTER];
	Vector4 skeletonSpine = skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_SPINE];
	Vector4 skeletonHipCenter = skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_HIP_CENTER];

	m_dist2sensor = (skeletonHead.z + skeletonShoulderCenter.z + skeletonSpine.z + skeletonHipCenter.z) / 4;
	
	if (m_dist2sensor > 2.0f)
	{

		// caculate the min of two hands' raising angles
		Vector4 skeletonShoulderRight = skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_RIGHT];
		Vector4 skeletonWristRight = skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_WRIST_RIGHT];
		Vector4 skeletonShoulderLeft = skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_LEFT];
		Vector4 skeletonWristLeft = skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_WRIST_LEFT];

		FLOAT LeftAngle = GetAngleFromHands(skeletonWristLeft, skeletonShoulderLeft);
		FLOAT RightAngle = GetAngleFromHands(skeletonWristRight, skeletonShoulderRight);
	
		FLOAT m_RaisAngle = min(LeftAngle,RightAngle);

		// raising threshold
		FLOAT RaiseAngleThr = 60.0f;

		// detect the start of the raising and identify the right/left hand and increase the count
		if (m_RaisAngle < RaiseAngleThr)
		{
			if (LeftAngle < RaiseAngleThr && RightAngle < RaiseAngleThr)
			{
				if (!m_LeftRaisStatus)
				{
					m_LeftCount++;
				}
				if (!m_RightRaisStatus)
				{
					m_RightCount++;
				}
				m_LeftRaisStatus = TRUE;
				m_RightRaisStatus = TRUE;
				if (LeftAngle < m_MinLeftRaisAngle)
				{
					m_MinLeftRaisAngle = LeftAngle;
				}
				if (RightAngle < m_MinRightRaisAngle)
				{
					m_MinRightRaisAngle = RightAngle;
				}
			}
			else if (LeftAngle < RaiseAngleThr)
			{
				// increase the count of left hand by 1
				if ((!m_LeftRaisStatus)&&(!m_RightRaisStatus))
				{
					m_LeftCount++;
				}
				m_LeftRaisStatus = TRUE;
				if (LeftAngle < m_MinLeftRaisAngle)
				{
					m_MinLeftRaisAngle = LeftAngle;
				}
			}
			else
			{
				// increase the count of right hand by 1
				if ((!m_LeftRaisStatus)&&(!m_RightRaisStatus))
				{
					m_RightCount++;
				}
				m_RightRaisStatus = TRUE;
				if (RightAngle < m_MinRightRaisAngle)
				{
					m_MinRightRaisAngle = RightAngle;
				}

			}
		}
		else
		{
			if (m_LeftRaisStatus||m_RightRaisStatus)
			{
				computeScore();
			}
			m_LeftRaisStatus = FALSE;
			m_RightRaisStatus = FALSE;
			m_MinRightRaisAngle = 90.0f;
			m_MinLeftRaisAngle = 90.0f;
		}
		return TRUE;
	}
	else
		return FALSE;
}

void HandRaisExcer::computeScore()
{
	if (m_LeftRaisStatus && m_RightRaisStatus)
	{
		m_Score = (1 - (m_MinLeftRaisAngle + m_MinRightRaisAngle) / 180.0f) * 100.0f;
		m_LeftScore = m_RightScore = m_Score;
	}
	else if (m_LeftRaisStatus)
	{
		m_LeftScore = (1 - m_MinLeftRaisAngle / 90.0f) * 100.0f;
		m_Score = m_LeftScore;
	}
	else
	{
		m_RightScore = (1 - m_MinRightRaisAngle / 90.0f) *100.0f;
		m_Score = m_RightScore;
	}
	/// speach reminder
	if (m_Score > 0 && m_Score <= 60)
	{
		wsprintf(m_pStrToSpeak, L"%s", L"Try Once more!");
	}
	else if (m_Score > 60 && m_Score <= 85)
	{
		wsprintf(m_pStrToSpeak, L"%s", L"Good job!");
	}
	else
	{
		wsprintf(m_pStrToSpeak, L"%s", L"Excellent!");
	}
	m_isHandReinitial = TRUE;
}

void HandRaisExcer::CallProcess(const NUI_SKELETON_DATA& skeletonData)
{
	if (m_isRunningExcer)
		HandRaisExcerProcess(skeletonData);
}

void HandRaisExcer::Reset()
{ 
	m_LeftCount = m_RightCount = 0; 
	m_Score = m_LeftScore = m_RightScore = m_dist2sensor = 0.0f;
	m_MinLeftRaisAngle = m_MinRightRaisAngle = m_RaisAngle = 90.0f;
	m_LeftRaisStatus = m_RightRaisStatus = FALSE;
}

DWORD WINAPI HandRaisExcer::Txt2SpeechStaticThread(PVOID lpParam)
{
	HandRaisExcer* context = static_cast<HandRaisExcer*>(lpParam);
	if (context)
	{
		return context->Txt2SpeechThread();
	}
	return 0;
}

DWORD WINAPI HandRaisExcer::Txt2SpeechThread()
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
			cpVoice->SetRate(-1);
			while (TRUE)
			{
				if (m_isHandReinitial)
				{
					m_isHandReinitial = FALSE;
					Sleep(100);   //Refresh m_pStrToSpeak					
					cpVoice->Speak(m_pStrToSpeak, SPF_DEFAULT, NULL);	   
				}
			}
		}

		::CoUninitialize();

		if (FAILED(hr)) return FALSE;

	return TRUE;
}