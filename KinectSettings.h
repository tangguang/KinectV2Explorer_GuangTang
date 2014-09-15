//------------------------------------------------------------------------------
// <copyright file="KinectSettings.h" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#pragma once

#include "NuiStreamViewer.h"
#include "NuiColorStream.h"
#include "NuiDepthStream.h"
#include "NuiAudioStream.h"
//#include "NuiSkeletonStream.h"
//#include "CameraSettingsViewer.h"

class KinectSettings
{
public:
    /// <summary>
    /// Constructor
    /// </summary>
    /// <param name="pNuiSensor">The pointer to NUI sensor instance</param>
    /// <param name="pPrimaryView">The pointer to primary viewer instance</param>
    /// <param name="pSecondaryView">The pointer to secondary viewer instance</param>
    /// <param name="pColorStream">The pointer to color stream object instance</param>
    /// <param name="pDepthStream">The pointer to depth stream object instance</param>
    /// <param name="pSkeletonStream">The pointer to skeleton stream object instance</param>
    KinectSettings(
		IKinectSensor*        pNuiSensor,
		NuiStreamViewer*      pPrimaryView, 
		NuiStreamViewer*      pSecondarView, 
		NuiColorStream*       pColorStream, 
		NuiDepthStream*       pDepthStream, 
		NuiAudioStream*       pAudioStream, 
		//NuiSkeletonStream*    pSkeletonStream, 
		//CameraSettingsViewer* pColorSettingsView, 
		//CameraSettingsViewer* pExposureSettingsView, 
		KinectWindow*         pKinectWindow);

    /// <summary>
    /// Destructor
    /// </summary>
   ~KinectSettings();

public:
    /// <summary>
    /// Process Kinect window menu commands
    /// </summary>
    /// <param name="commanId">ID of the menu item</param>
    /// <param name="param">Parameter passed in along with the commmand ID</param>
    /// <param name="previouslyChecked">Check status of menu item before command is issued</param>
    void ProcessMenuCommand(WORD commandId, WORD param, bool previouslyChecked);

	BOOL GetViewDetecResProcessStatus(LPDWORD pExitCode);

private:
	IKinectSensor*           m_pNuiSensor;
    // Stream viewers
    NuiStreamViewer*         m_pPrimaryView;
    NuiStreamViewer*         m_pSecondaryView;

    // Streams
    NuiColorStream*          m_pColorStream;
    NuiDepthStream*          m_pDepthStream;
	NuiAudioStream*          m_pAudioStream;
    //NuiSkeletonStream*       m_pSkeletonStream;

    // Camera settings
    //CameraSettingsViewer*    m_pColorSettingsView;
    //CameraSettingsViewer*    m_pExposureSettingsView;

	KinectWindow*            m_pKinectWindow;

	BOOL                     m_FallDetectThreadRun;
	BOOL                     m_NurseCallThreadRun;
	BOOL                     m_HandRaisexcerThreadRun;
	BOOL                     m_LegRaisexcerThreadRun;
	HANDLE                   m_hSpeechRecogThread;
	HANDLE                   m_hFallDetectTxt2SpeechThread;
	HANDLE                   m_hNurseCallTxt2SpeechThread;
	HANDLE                   m_hHandRaisExcerTxt2SpeechThread;
	HANDLE                   m_hLegRaisExcerTxt2SpeechThread;

	PROCESS_INFORMATION      m_pi; 
	BOOL                     m_processFlag;

	void ViewDetectionRes();
};
