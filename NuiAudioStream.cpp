//------------------------------------------------------------------------------
// <copyright file="NuiAudioStream.cpp" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#include "stdafx.h"
#include <uuids.h>
#include <wmcodecdsp.h>
#include "NuiAudioStream.h"
#include "Utility.h"
#include "WaveWriter.h"
// For StringCch* and such
#include <strsafe.h>


#define TIMER_PERIOD                20   /////20milliseconds Check KinectWindow
const UINT FileSizeInMS = 60000;  //60 seconds each file
const UINT TimersPerFile = FileSizeInMS / TIMER_PERIOD;   ///////

const wchar_t *knownPath = L"C:\\Users\\tg857_000\\Desktop\\KinectGUI\\DataLog\\Room1";
//const wchar_t *knownPath = L"C:\\Users\\heal1\\Desktop\\Data";
//const wchar_t *knownPath = L"C:\\Users\\heal\\Desktop\\Data";


/// <summary>
/// Constructor
/// </summary>
/// <param name="pNuiSensor">The pointer to Nui sensor object</param>
NuiAudioStream::NuiAudioStream(IKinectSensor* pNuiSensor, PCWSTR instanceName)
//NuiAudioStream::NuiAudioStream(INuiSensor* pNuiSensor)
    : m_pNuiSensor(pNuiSensor)
    //, m_pNuiAudioSource(nullptr)
	, m_pAudioBeamFrameReader(nullptr)
    //, m_pDMO(nullptr)
    , m_pPropertyStore(nullptr)
    , m_pAudioViewer(nullptr)
	, m_pWaveWriter(nullptr)
	, m_TimerCount(0)
	, m_instanceName((WCHAR*)instanceName)
	, m_Recording(false)
{
    if (m_pNuiSensor)
    {
        m_pNuiSensor->AddRef();
    }
}

/// <summary>
/// Destructor
/// </summary>
NuiAudioStream::~NuiAudioStream()
{
    SafeRelease(m_pPropertyStore);
    // SafeRelease(m_pDMO);
	SafeRelease(m_pAudioBeamFrameReader);
    SafeRelease(m_pNuiSensor);
	if (m_pWaveWriter)
    {
        m_pWaveWriter->Stop();
        delete m_pWaveWriter;
        m_pWaveWriter = NULL;
    }
}

/// <summary>
/// Start processing stream
/// </summary>
/// <returns>Indicates success or failure</returns>
HRESULT NuiAudioStream::StartStream()
{
	HRESULT hr;
	IAudioSource* pAudioSource = NULL;
	IAudioBeamList* pAudioBeamList = NULL;
    // Get audio source interface
	hr = m_pNuiSensor->Open();
	
    if (SUCCEEDED(hr))
    {
		hr = m_pNuiSensor->get_AudioSource(&pAudioSource);
    }
	if (SUCCEEDED(hr))
	{
		hr = pAudioSource->get_AudioBeams(&pAudioBeamList);
	}
	if (SUCCEEDED(hr))
	{
		hr = pAudioSource->OpenReader(&m_pAudioBeamFrameReader);
	}
	if (SUCCEEDED(hr))
	{
		m_pAudioBeamFrameReader->SubscribeFrameArrived(&m_hFrameArrivedEvent);
	}
	SafeRelease(pAudioSource);
    
	// Query dmo interface
   /* hr = m_pNuiAudioSource->QueryInterface(IID_IMediaObject, (void**)&m_pDMO);
    if (FAILED(hr))
    {
        return hr;
    }

    // Query property store interface
    hr = m_pNuiAudioSource->QueryInterface(IID_IPropertyStore, (void**)&m_pPropertyStore);
    if (FAILED(hr))
    {
        return hr;
    }*/

    // Set AEC-MicArray DMO system mode. This must be set for the DMO to work properly.
    // Possible values are:
    //   SINGLE_CHANNEL_AEC = 0
    //   OPTIBEAM_ARRAY_ONLY = 2
    //   OPTIBEAM_ARRAY_AND_AEC = 4
    //   SINGLE_CHANNEL_NSAGC = 5

    /*PROPVARIANT pvSysMode;

    // Initialize the variable
    PropVariantInit(&pvSysMode);

    // Assign properties
    pvSysMode.vt   = VT_I4;
    pvSysMode.lVal = (LONG)(2); // Use OPTIBEAM_ARRAY_ONLY setting. Set OPTIBEAM_ARRAY_AND_AEC instead if you expect to have sound playing from speakers.

    // Set properties
    m_pPropertyStore->SetValue(MFPKEY_WMAAECMA_SYSTEM_MODE, pvSysMode);

    // Release the variable
    PropVariantClear(&pvSysMode);

    // Set DMO output format
    WAVEFORMATEX   wfxOut = {AudioFormat, AudioChannels, AudioSamplesPerSecond, AudioAverageBytesPerSecond, AudioBlockAlign, AudioBitsPerSample, 0};
	memcpy_s(&m_wfxOut,sizeof(WAVEFORMATEX),&wfxOut,sizeof(WAVEFORMATEX));

    DMO_MEDIA_TYPE mt     = {0};

    // Initialize variable
    MoInitMediaType(&mt, sizeof(WAVEFORMATEX));

    // Assign format
    mt.majortype            = MEDIATYPE_Audio;
    mt.subtype              = MEDIASUBTYPE_PCM;
    mt.lSampleSize          = 0;
    mt.bFixedSizeSamples    = TRUE;
    mt.bTemporalCompression = FALSE;
    mt.formattype           = FORMAT_WaveFormatEx;	

    memcpy_s(mt.pbFormat, sizeof(WAVEFORMATEX), &m_wfxOut, sizeof(WAVEFORMATEX));

    // Set format
    // hr = m_pDMO->SetOutputType(0, &mt, 0); 

    // Release variable
    MoFreeMediaType(&mt);*/

    return hr;
}


/// <summary>
/// Attach stream viewer
/// </summary>
/// <param name="pViewer">The pointer to the viewer to attach</param>
void NuiAudioStream::SetStreamViewer(NuiAudioViewer* pViewer)
{
    m_pAudioViewer = pViewer;
}

/// <summary>
/// Get the audio readings from the stream
/// </summary>
void NuiAudioStream::ProcessStream()
{	
	HRESULT hr;
	UINT32 subFrameCount = 0;
	IAudioBeamFrameArrivedEventArgs* pAudioBeamFrameArrivedEventArgs = NULL;
	IAudioBeamFrameReference* pAudioBeamFrameReference = NULL;
	IAudioBeamFrameList* pAudioBeamFrameList = NULL;
	IAudioBeamFrame* pAudioBeamFrame = NULL;

	if (WAIT_OBJECT_0 == WaitForSingleObject(reinterpret_cast<HANDLE>(GetArrivedEvent()), 0))
	{
		// Process new audio frame
		hr = m_pAudioBeamFrameReader->GetFrameArrivedEventData(m_hFrameArrivedEvent, &pAudioBeamFrameArrivedEventArgs);
		if (SUCCEEDED(hr))
		{
			hr = pAudioBeamFrameArrivedEventArgs->get_FrameReference(&pAudioBeamFrameReference);
		}

		if (SUCCEEDED(hr))
		{
			hr = pAudioBeamFrameReference->AcquireBeamFrames(&pAudioBeamFrameList);
		}

		if (SUCCEEDED(hr))
		{
			// Only one audio beam is currently supported
			hr = pAudioBeamFrameList->OpenAudioBeamFrame(0, &pAudioBeamFrame);
		}

		if (SUCCEEDED(hr))
		{
			hr = pAudioBeamFrame->get_SubFrameCount(&subFrameCount);
		}

		if (SUCCEEDED(hr) && subFrameCount > 0)
		{
			for (UINT32 i = 0; i < subFrameCount; i++)
			{
				// Process all subframes
				IAudioBeamSubFrame* pAudioBeamSubFrame = NULL;

				hr = pAudioBeamFrame->GetSubFrame(i, &pAudioBeamSubFrame);

				if (SUCCEEDED(hr))
				{
					ProcessAudio(pAudioBeamSubFrame);
				}

				SafeRelease(pAudioBeamSubFrame);
			}
		}

		SafeRelease(pAudioBeamFrame);
		SafeRelease(pAudioBeamFrameList);
		SafeRelease(pAudioBeamFrameReference);
		SafeRelease(pAudioBeamFrameArrivedEventArgs);
	}
}

void NuiAudioStream::ProcessAudio(IAudioBeamSubFrame* pAudioBeamSubFrame)
{
	HRESULT hr;
	float* pAudioBuffer = NULL;
	UINT cbRead = 0;

	hr = pAudioBeamSubFrame->AccessUnderlyingBuffer(&cbRead, (BYTE **)&pAudioBuffer);

	////// Getting the wave writer
	if (m_Recording)
	{
		if ((m_TimerCount++) % TimersPerFile == 0)
		{
			WCHAR szFilename[MAX_PATH] = { 0 };
			if (SUCCEEDED(GetFileName(szFilename, _countof(szFilename), m_instanceName, AudioSensor)))
			{
				m_pWaveWriter = WaveWriter::Start(szFilename, &m_wfxOut);
			}
			m_TimerCount %= TimersPerFile;
		}
	}

	// if (m_pAudioBeamFrameReader && m_pDMO)
	if (SUCCEEDED(hr) && cbRead > 0)
	{
		// Set buffer
		ULONG cbProduced = 0;        //////
		BYTE *pProduced = NULL;       //////
		DWORD dwStatus;
		DMO_OUTPUT_DATA_BUFFER outputBuffer = { 0 };
		//outputBuffer.pBuffer = &m_captureBuffer;

		do
		{
			//m_captureBuffer.Init(0);
			outputBuffer.dwStatus = 0;

			// Process audio data
			// HRESULT hr = m_pDMO->ProcessOutput(0, 1, &outputBuffer, &dwStatus);
			if (S_OK == hr)
			{
				//m_captureBuffer.GetBufferAndLength(&pProduced, &cbProduced);
				//// Get the reading
				// double beamAngle, sourceAngle, sourceConfidence;
				float fBeamAngle = 0.f;
				float fBeamAngleConfidence = 0.0f;
				float sourceAngle = 0.0f;
				if (SUCCEEDED(pAudioBeamSubFrame->get_BeamAngle(&fBeamAngle)) &&
					SUCCEEDED(pAudioBeamSubFrame->get_BeamAngleConfidence(&fBeamAngleConfidence)))
				{
					if (m_pAudioViewer)
					{
						// Set reading sto viewer
						m_pAudioViewer->SetAudioReadings(fBeamAngle, sourceAngle, fBeamAngleConfidence);
					}
				}
				///////write buffer into wav files
				if (m_Recording)
				{
					if (m_pWaveWriter)
					{
						m_pWaveWriter->WriteBytes(pProduced, cbProduced);
					}
				}
			}

			if (S_FALSE == hr)
			{
				cbProduced = 0;
			}

		} while (outputBuffer.dwStatus & DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE);//Check if there is still remaining data

		if (m_Recording)
		{
			if (m_TimerCount%TimersPerFile == 0)
			{
				m_pWaveWriter->Stop();
				delete m_pWaveWriter;
				m_pWaveWriter = NULL;
			}
		}
	}
}

/////////////////////////////
HRESULT GetFileName(wchar_t *FileName, UINT FileNameSize, WCHAR* instanceName, SensorType senserID)
{
	wchar_t DevIdOut[20];
	HRESULT hr;
	GetDevIdforFile(instanceName, DevIdOut);

    // Get the time
    wchar_t timeString[MAX_PATH];
	wchar_t dateString[MAX_PATH];
    GetTimeFormatEx(NULL, TIME_FORCE24HOURFORMAT, NULL, L"hh'-'mm'-'ss", timeString, _countof(timeString));
	GetDateFormatEx(NULL, 0, NULL, L"yyyy'-'MMM'-'dd", dateString, _countof(dateString), NULL);

	// File name will be DeviceConnectID-KinectAudio-HH-MM-SS.wav
	switch (senserID)
	{
	case AudioSensor:
		hr=StringCchPrintfW(FileName, FileNameSize, L"%s\\KinectAudio\\Audio-%s-%s-%s.wav", knownPath, DevIdOut, dateString, timeString);
		break;
	case RGBSensor:
		hr=StringCchPrintfW(FileName, FileNameSize, L"%s\\KinectRGB\\RGB-%s-%s-%s.avi", knownPath, DevIdOut, dateString, timeString);
		break;
	case DepthSensor:
		hr=StringCchPrintfW(FileName, FileNameSize, L"%s\\KinectDepth\\Depth-%s-%s-%s.avi", knownPath, DevIdOut, dateString, timeString);
		break;
	case SpeechRecog:
		hr=StringCchPrintfW(FileName, FileNameSize, L"%s\\KinectSpeechRecog\\SpeechRecog-%s-%s-%s.txt", knownPath, DevIdOut, dateString, timeString);
		break;
	default:
		break;
	}
    return hr;
}

bool GetDevIdforFile(wchar_t *USBDeviceInstancePath, wchar_t *pDevIdOut)
{
	int k=0, n=k;
	while (USBDeviceInstancePath[k]) k++;
	while (--k>0)
	{
		pDevIdOut[n++]=USBDeviceInstancePath[k];
		if (n==10) break;
	};
	if (k==0) return false;
	   pDevIdOut[n]=L'\0';
	return true;
}

void NuiAudioStream::SetRecordingStatus (bool RecStatus) 
{
	m_Recording=RecStatus;
	m_TimerCount=0;
}

bool NuiAudioStream::GetRecordingStauts () const {return m_Recording;}

WAITABLE_HANDLE NuiAudioStream::GetArrivedEvent() { return m_hFrameArrivedEvent; }