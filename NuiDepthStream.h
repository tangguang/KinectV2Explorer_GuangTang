//------------------------------------------------------------------------------
// <copyright file="NuiDepthStream.h" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#pragma once

#include "NuiStream.h"
#include "NuiImageBuffer.h"
#include <FaceTrackLib.h>
#include "DepthInbedAPPs.h"

#include "opencv\cv.h"
#include "opencv\highgui.h"

class NuiDepthStream : public NuiStream
{
public:
    /// <summary>
    /// Constructor
    /// <summary>
    /// <param name="pNuiSensor">The pointer to NUI sensor device instance</param>
    NuiDepthStream(INuiSensor* pNuiSensor, PCWSTR instanceName, NuiStreamViewer* pPrimaryView);

    /// <summary>
    /// Destructor
    /// </summary>
   ~NuiDepthStream();

public:
    /// <summary>
    /// Attach viewer object to stream object
    /// </summary>
    /// <param name="pStreamViewer">The pointer to viewer object to attach</param>
    /// <returns>Previously attached viewer object. If none, returns nullptr</returns>
    virtual NuiStreamViewer* SetStreamViewer(NuiStreamViewer* pStreamViewer);

    /// <summary>
    /// Start stream processing.
    /// </summary>
    /// <returns>Indicate success or failure.</returns>
    virtual HRESULT StartStream();

    /// <summary>
    /// Open stream with a certain image resolution.
    /// </summary>
    /// <param name="resolution">Frame image resolution</param>
    /// <returns>Indicates success or failure.</returns>
    HRESULT OpenStream(NUI_IMAGE_RESOLUTION resolution);

    /// <summary>
    /// Process an incoming stream frame
    /// </summary>
    virtual void ProcessStreamFrame();

    /// <summary>
    /// Set and reset near mode
    /// </summary>
    /// <param name="nearMode">True to enable near mode. False to disable</param>
    void SetNearMode(bool nearMode);

    /// <summary>
    /// Set depth treatment
    /// </summary>
    /// <param name="treatment">Depth treatment mode to set</param>
    void SetDepthTreatment(DEPTH_TREATMENT treatment);

	friend HRESULT GetFileName(wchar_t *FileName, UINT FileNameSize, WCHAR* instanceName, SensorType senserID);   ////////
	friend bool GetDevIdforFile(wchar_t *USBDeviceInstancePath, wchar_t *pDevIdOut);  //////// 
	void SetRecordingStatus(bool RecStatus);
	bool NuiDepthStream::GetRecordingStauts () const; 

	CvVideoWriter       *m_pwriter;
	WCHAR*               m_instanceName;
	
	/// Face tracking initialization
	HRESULT FTdepthInit(FT_CAMERA_CONFIG DepthCameraConfig);
	IFTImage*   GetDepthBuffer() const { return(m_pFTdepthBuffer); };

	/// Get depth in-bed apps object
	void GetDepthInbedAPPs(DepthInbedAPPs** ppDepthInbedAPPs) const {*ppDepthInbedAPPs = m_pDepthInbedAPPs;}

private:
    /// <summary>
    /// Retrieve depth data from stream frame
    /// </summary>
    void ProcessDepth();

private:
    bool             m_nearMode;
    NUI_IMAGE_TYPE   m_imageType;
    NuiImageBuffer   m_imageBuffer;
    DEPTH_TREATMENT  m_depthTreatment;
	IplImage         *m_pcolorImage;
	UINT             m_TimerCount;
	CvSize           m_ImageRes;
	bool             m_Recording, m_FTRecording;
	IFTImage*        m_pFTdepthBuffer;
	DepthInbedAPPs*  m_pDepthInbedAPPs;     
	NuiStreamViewer* m_pPrimaryViewer;
};