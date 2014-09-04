//------------------------------------------------------------------------------
// <copyright file="LegRaisExcer.cpp" written by Yun Li, 10/23/2013>
//     All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#include "stdafx.h"
#include <NuiApi.h>
#include "LegRaisExcer.h"
#include <math.h>


#define PI 3.14159265f


LegRaisExcer::LegRaisExcer():
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
	m_isLeginitial(FALSE),
    m_isRunningExcer(FALSE){
		m_pStrToSpeak = new WCHAR[256];
	}

LegRaisExcer::~LegRaisExcer()
{
	SafeDelete(m_pStrToSpeak);
}
    

FLOAT LegRaisExcer::GetAngleFromLegs(const Vector4 &skeletonPos1, const Vector4 &skeletonPos2)
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


void LegRaisExcer::SetLeftRaisStatus()
{
	if (!m_LeftRaisStatus)
		m_LeftRaisStatus = TRUE;
}

void LegRaisExcer::SetRightRaisStatus()
{
	if (!m_RightRaisStatus)
		m_RightRaisStatus = TRUE;
}

BOOL LegRaisExcer::LegRaisExcerProcess(const NUI_SKELETON_DATA& skeletonData)
{
	//// distance filter
	Vector4 skeletonHead = skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_HEAD];
	Vector4 skeletonShoulderCenter = skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_SHOULDER_CENTER];
	Vector4 skeletonSpine = skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_SPINE];
	Vector4 skeletonHipCenter = skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_HIP_CENTER];

	m_dist2sensor = (skeletonHead.z + skeletonShoulderCenter.z + skeletonSpine.z + skeletonHipCenter.z) / 4;

	if (m_dist2sensor > 2.0f)
	{
		// caculate the min of two legs' raising angles
		Vector4 skeletonHipRight = skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_HIP_RIGHT];
		Vector4 skeletonFootRight = skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_FOOT_RIGHT];
		Vector4 skeletonHipLeft = skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_HIP_LEFT];
		Vector4 skeletonFootLeft = skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_FOOT_LEFT];

		FLOAT LeftAngle = GetAngleFromLegs(skeletonFootLeft, skeletonHipLeft);
		FLOAT RightAngle = GetAngleFromLegs(skeletonFootRight, skeletonHipRight);
	
		FLOAT m_RaisAngle = min(LeftAngle,RightAngle);

		// raising threshold
		FLOAT RaiseAngleThr = 70.0f;

		// detect the start of the raising and identify the right/left leg and increase the count
		if (m_RaisAngle < RaiseAngleThr)
		{
			// one leg is raising
			if (m_RaisAngle == LeftAngle)
			{
				// increase the count of left leg by 1
				if ((!m_LeftRaisStatus)&&(!m_RightRaisStatus))
				{
					m_LeftCount++;
				}
				m_LeftRaisStatus = TRUE;
				if (m_RaisAngle < m_MinLeftRaisAngle)
				{
					m_MinLeftRaisAngle = m_RaisAngle;
					//if (m_LeftCount > 1);
	//					PrintMessagesToScreen(); 
				}
			}
			else
			{
				// increase the count of right leg by 1
				if ((!m_LeftRaisStatus)&&(!m_RightRaisStatus))
				{
					m_RightCount++;
				}
				m_RightRaisStatus = TRUE;
				if (m_RaisAngle < m_MinRightRaisAngle)
				{
					m_MinRightRaisAngle = m_RaisAngle;
					//if (m_RightCount > 1);
					//	PrintMessagesToScreen(); 
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

void LegRaisExcer::computeScore()
{
	FLOAT Score = 0.0f;
	if (m_LeftRaisStatus)
	{
		m_LeftScore = (1 - m_MinLeftRaisAngle/90.0f)*100.0f;
		Score = m_LeftScore;
	}
	else
	{
		m_RightScore = (1 - m_MinRightRaisAngle/90.0f)*100.0f;
		Score = m_RightScore;
	}

	/// speach reminder
	if (Score != 0.0f)
	{
		if (Score > 0 && Score <= 60)
		{
			wsprintf(m_pStrToSpeak, L"%s", L"Try Once more!");
		}
		else if (Score > 60 && Score <= 85)
		{
			wsprintf(m_pStrToSpeak, L"%s", L"Good job!");
		}
		else
		{
			wsprintf(m_pStrToSpeak, L"%s", L"Excellent!");
		}
	}
	m_isLeginitial = TRUE;
}

FLOAT LegRaisExcer::GetLeftScore() const 
{
	if (m_LeftCount)
		return m_LeftScore;
	else
		return 0.0f;
}

FLOAT LegRaisExcer::GetRightScore() const 
{
	if (m_RightCount)
		return m_RightScore;
	else
		return 0.0f;
}

void LegRaisExcer::CallProcess(const NUI_SKELETON_DATA& skeletonData)
{
	if (m_isRunningExcer)
		LegRaisExcerProcess(skeletonData);
}

void LegRaisExcer::Reset()
{
	m_LeftCount = m_RightCount = 0; 
	m_LeftScore = m_RightScore = m_dist2sensor = 0.0f;
	m_MinLeftRaisAngle = m_MinRightRaisAngle = m_RaisAngle = 90.0f;
	m_LeftRaisStatus = m_RightRaisStatus = FALSE;
}

DWORD WINAPI LegRaisExcer::Txt2SpeechStaticThread(PVOID lpParam)
{
	LegRaisExcer* context = static_cast<LegRaisExcer*>(lpParam);
	if (context)
	{
		return context->Txt2SpeechThread();
	}
	return 0;
}

DWORD WINAPI LegRaisExcer::Txt2SpeechThread()
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
				if (m_isLeginitial)
				{
					m_isLeginitial = FALSE;
					Sleep(100);   //Refresh m_pStrToSpeak
					cpVoice->Speak(m_pStrToSpeak, SPF_DEFAULT, NULL);	   
				}
			}
		}

		::CoUninitialize();

		if (FAILED(hr)) return FALSE;

	return TRUE;
}