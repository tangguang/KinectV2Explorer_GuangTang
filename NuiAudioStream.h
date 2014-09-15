//------------------------------------------------------------------------------
// <copyright file="NuiAudioStream.h" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#pragma once

// #include <NuiApi.h>
#include "NuiAudioViewer.h"
#include "StaticMediaBuffer.h"
#include "WaveWriter.h"

extern const wchar_t *knownPath;


class NuiAudioStream
{
public:
    /// <summary>
    /// Constructor
    /// </summary>
    /// <param name="pNuiSensor">The pointer to Nui sensor object</param>
	NuiAudioStream(IKinectSensor* pNuiSensor, PCWSTR instanceName);
	//NuiAudioStream(INuiSensor* pNuiSensor);

    /// <summary>
    /// Destructor
    /// </summary>
   ~NuiAudioStream();

public:
    /// <summary>
    /// Attach stream viewer
    /// </summary>
    /// <param name="pViewer">The pointer to the viewer to attach</param>
    void SetStreamViewer(NuiAudioViewer* pViewer);

    /// <summary>
    /// Start processing stream
    /// </summary>
    /// <returns>Indicates success or failure</returns>
    HRESULT StartStream();
    
    /// <summary>
    /// Get the audio readings from the stream
    /// </summary>
    void ProcessStream();

	/// <summary>
	/// Capture new audio data.
	/// </summary>
	void ProcessAudio(IAudioBeamSubFrame* pAudioBeamSubFrame);

	friend HRESULT GetFileName(wchar_t *FileName, UINT FileNameSize, WCHAR* instanceName, SensorType senserID);   ////////
	friend bool GetDevIdforFile(wchar_t *USBDeviceInstancePath, wchar_t *pDevIdOut);  //////// 
	void SetRecordingStatus(bool RecStatus);
	bool GetRecordingStauts() const;
	WAITABLE_HANDLE GetArrivedEvent();

	WCHAR*              m_instanceName;
	WaveWriter* 		m_pWaveWriter; 

private:
	IKinectSensor*          m_pNuiSensor;
    // INuiAudioBeam*        m_pNuiAudioSource;
	IAudioBeamFrameReader*  m_pAudioBeamFrameReader;
	WAITABLE_HANDLE         m_hFrameArrivedEvent;
    // IMediaObject*           m_pDMO;
    IPropertyStore*			m_pPropertyStore;
    NuiAudioViewer*			m_pAudioViewer;
    // CStaticMediaBuffer		m_captureBuffer;
	WAVEFORMATEX		    m_wfxOut;
	UINT					m_TimerCount;
	bool                    m_Recording;

};