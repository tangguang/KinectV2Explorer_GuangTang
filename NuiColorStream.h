//------------------------------------------------------------------------------
// <copyright file="NuiColorStream.h" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#pragma once

#include "NuiStream.h"
#include "NuiImageBuffer.h"
#include <FaceTrackLib.h>

#include "opencv\cv.h"
#include "opencv\highgui.h"

class NuiColorStream : public NuiStream
{
public:
    /// <summary>
    /// Constructor
    /// </summary>
    /// <param name="pNuiSensor">The pointer to Nui sensor object</param>
    NuiColorStream(INuiSensor* pNuiSensor, PCWSTR instanceName);

    /// <summary>
    /// Destructor
    /// </summary>
   ~NuiColorStream();

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
    /// <returns>Indicates success or failure.</returns>
    HRESULT OpenStream();

    /// <summary>
    /// Process a incoming stream frame
    /// </summary>
    //virtual void ProcessStreamFrame(NuiImageBuffer DepthImageBuffer);
	void ProcessStreamFrame();

    /// <summary>
    /// Set image type. Only color image types are acceptable
    /// </summary>
    /// <param name="type">Image type to be set</param>
    void SetImageType(NUI_IMAGE_TYPE type);

    /// <summary>
    /// Set image resolution. Only 640x480 and 1280x960 formats are acceptable
    /// </summary>
    /// <param name="resolution">Image resolution to be set</param>
    void SetImageResolution(NUI_IMAGE_RESOLUTION resolution);

	friend HRESULT GetFileName(wchar_t *FileName, UINT FileNameSize, WCHAR* instanceName, SensorType senserID);   ////////
	friend bool GetDevIdforFile(wchar_t *USBDeviceInstancePath, wchar_t *pDevIdOut);  //////// 
	void SetRecordingStatus(bool RecStatus);
	bool GetRecordingStauts() const;

	CvVideoWriter       *m_pwriter;
	WCHAR*               m_instanceName;

	/// Face tracking initialization
	HRESULT FTcolorInit(FT_CAMERA_CONFIG ColorCameraConfig);
	IFTImage*   GetColorBuffer() const { return(m_pFTcolorBuffer); };
	IplImage*   GetCvColorImage() const { return(m_pcolorImage);}

private:
    /// <summary>
    /// Process the incoming color frame
    /// </summary>
    void ProcessColor();

private:
    NUI_IMAGE_TYPE       m_imageType;
    NUI_IMAGE_RESOLUTION m_imageResolution;
    NuiImageBuffer       m_imageBuffer;
	IplImage            *m_pcolorImage;
	UINT                 m_TimerCount;
	CvSize               m_ImageRes;
	bool                 m_Recording, m_FTRecording;
	IFTImage*            m_pFTcolorBuffer;
};