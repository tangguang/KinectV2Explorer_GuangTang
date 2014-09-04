//------------------------------------------------------------------------------
// <copyright file="NuiDepthStream.cpp" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#include "stdafx.h"
#include <cmath>
#include "NuiDepthStream.h"
#include "NuiStreamViewer.h"
#include <FaceTrackLib.h>

#include "opencv\cv.h"
#include "opencv\highgui.h"

#define FramesPerSecond 25          //Kinect RGB 30 frames per second
static const UINT FileSizeInMS = 60000;  //60 seconds each file
static const UINT FramesPerFile = FileSizeInMS*FramesPerSecond/1000;   ///////


/// <summary>
/// Constructor
/// <summary>
/// <param name="pNuiSensor">The pointer to NUI sensor device instance</param>
NuiDepthStream::NuiDepthStream(INuiSensor* pNuiSensor, PCWSTR instanceName, NuiStreamViewer* pPrimaryView)
    : NuiStream(pNuiSensor)
    , m_imageType(NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX)
    , m_nearMode(false)
    , m_depthTreatment(CLAMP_UNRELIABLE_DEPTHS)
	, m_pwriter(nullptr)
	, m_pcolorImage(nullptr)
	, m_TimerCount(0)
	, m_instanceName((WCHAR*)instanceName)
	, m_Recording(false)
	, m_FTRecording(false)
	, m_pFTdepthBuffer(nullptr)
	, m_pPrimaryViewer(pPrimaryView)
{
	m_pDepthInbedAPPs = new DepthInbedAPPs;
}

/// <summary>
/// Destructor
/// </summary>
NuiDepthStream::~NuiDepthStream()
{
	if (m_pwriter) cvReleaseVideoWriter(&m_pwriter);
	if (m_pFTdepthBuffer) {
		m_pFTdepthBuffer->Release();
		m_pFTdepthBuffer = nullptr;
	}
	SafeDelete(m_pDepthInbedAPPs);
}

/// <summary>
/// Attach viewer object to stream object
/// </summary>
/// <param name="pStreamViewer">The pointer to viewer object to attach</param>
/// <returns>Previously attached viewer object. If none, returns nullptr</returns>
NuiStreamViewer* NuiDepthStream::SetStreamViewer(NuiStreamViewer* pStreamViewer)
{
    if (pStreamViewer)
    {
        // Set image data to newly attached viewer object as well
        pStreamViewer->SetImage(&m_imageBuffer);
        pStreamViewer->SetImageType(m_imageType);
    }

    return NuiStream::SetStreamViewer(pStreamViewer);
}

/// <summary>
/// Set and reset near mode
/// </summary>
/// <param name="nearMode">True to enable near mode. False to disable</param>
void NuiDepthStream::SetNearMode(bool nearMode)
{
    m_nearMode = nearMode;
    if (INVALID_HANDLE_VALUE != m_hStreamHandle)
    {
        m_pNuiSensor->NuiImageStreamSetImageFrameFlags(m_hStreamHandle, (m_nearMode ? NUI_IMAGE_STREAM_FLAG_ENABLE_NEAR_MODE : 0));
    }
}

/// <summary>
/// Set depth treatment
/// </summary>
/// <param name="treatment">Depth treatment mode to set</param>
void NuiDepthStream::SetDepthTreatment(DEPTH_TREATMENT treatment)
{
    m_depthTreatment = treatment;
}

/// <summary>
/// Start stream processing.
/// </summary>
/// <returns>Indicate success or failure.</returns>
HRESULT NuiDepthStream::StartStream()
{
    return OpenStream(NUI_IMAGE_RESOLUTION_640x480); // Start stream with default resolution 640x480
}

/// <summary>
/// Open stream with a certain image resolution.
/// </summary>
/// <param name="resolution">Frame image resolution</param>
/// <returns>Indicates success or failure.</returns>
HRESULT NuiDepthStream::OpenStream(NUI_IMAGE_RESOLUTION resolution)
{
    m_imageType = HasSkeletalEngine(m_pNuiSensor) ? NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX : NUI_IMAGE_TYPE_DEPTH;

    // Open depth stream
    HRESULT hr = m_pNuiSensor->NuiImageStreamOpen(m_imageType,
                                                  resolution,
                                                  0,
                                                  2,
                                                  GetFrameReadyEvent(),
                                                  &m_hStreamHandle);
    if (SUCCEEDED(hr))
    {
        m_pNuiSensor->NuiImageStreamSetImageFrameFlags(m_hStreamHandle, m_nearMode ? NUI_IMAGE_STREAM_FLAG_ENABLE_NEAR_MODE : 0);   // Set image flags
        m_imageBuffer.SetImageSize(resolution); // Set source image resolution to image buffer
    }

	//////////////Video writer
	m_ImageRes=cvSize(static_cast<int>(m_imageBuffer.GetWidth()),static_cast<int>(m_imageBuffer.GetHeight()));
	m_pcolorImage =cvCreateImage(m_ImageRes, 8, 1);    //openCV only supports <=8 bits

    return hr;
}

/// <summary>
/// Process an incoming stream frame
/// </summary>
void NuiDepthStream::ProcessStreamFrame()
{
    if (WAIT_OBJECT_0 == WaitForSingleObject(GetFrameReadyEvent(), 0))
    {
        // if we have received any valid new depth data we may need to draw
        ProcessDepth();
    }
}

/// <summary>
/// Retrieve depth data from stream frame
/// </summary>
void NuiDepthStream::ProcessDepth()
{
    HRESULT         hr;
    NUI_IMAGE_FRAME imageFrame;

	if (m_Recording)
	{
	//////Initializaing a video writer and allocate an image for recording /////////
		if ((m_TimerCount++)%FramesPerFile==0)
		{
		WCHAR szFilename[MAX_PATH] = { 0 };
		if (SUCCEEDED(GetFileName(szFilename,_countof(szFilename), m_instanceName, DepthSensor)))
			{
				char char_szFilename[MAX_PATH] = {0};
				size_t convertedChars;
				wcstombs_s(&convertedChars,char_szFilename,sizeof(char_szFilename),szFilename,sizeof(char_szFilename));
				m_pwriter=cvCreateVideoWriter(char_szFilename,
				CV_FOURCC('L', 'A', 'G', 'S'),
				//-1,  //user specified 
				FramesPerSecond,m_ImageRes);
				//2,m_ImageRes);
			}
			m_TimerCount%=FramesPerFile;
		}
	}

    // Attempt to get the depth frame
    hr = m_pNuiSensor->NuiImageStreamGetNextFrame(m_hStreamHandle, 0, &imageFrame);
    if (FAILED(hr))
    {
        return;
    }

    if (m_paused)
    {
        // Stream paused. Skip frame process and release the frame.
        goto ReleaseFrame;
    }

    BOOL nearMode;
    INuiFrameTexture* pTexture;

	///FT image texture
	INuiFrameTexture* pFTTexture;

	pFTTexture=imageFrame.pFrameTexture;

    // Get the depth image pixel texture
    hr = m_pNuiSensor->NuiImageFrameGetDepthImagePixelFrameTexture(m_hStreamHandle, &imageFrame, &nearMode, &pTexture);
    if (FAILED(hr))
    {
        goto ReleaseFrame;
    }

    NUI_LOCKED_RECT lockedRect;

	///FT locked rect
	NUI_LOCKED_RECT FTlockedRect;

    // Lock the frame data so the Kinect knows not to modify it while we're reading it
    pTexture->LockRect(0, &lockedRect, NULL, 0);

	// Lock the FT frame data
	pFTTexture->LockRect(0, &FTlockedRect, NULL, 0);

    // Make sure we've received valid data
    if (lockedRect.Pitch != 0)
    {
        // Conver depth data to color image and copy to image buffer
        m_imageBuffer.CopyDepth(lockedRect.pBits, lockedRect.size, nearMode, m_depthTreatment);
		// Convert 8 bits depth frame to 12 bits
		NUI_DEPTH_IMAGE_PIXEL* depthBuffer = (NUI_DEPTH_IMAGE_PIXEL*)lockedRect.pBits;
		cv::Mat depthMat(m_ImageRes.height, m_ImageRes.width, CV_8UC1);
		INT cn = 1;
		for(int i=0;i<depthMat.rows;i++){
		   for(int j=0;j<depthMat.cols;j++){
			  USHORT realdepth = ((depthBuffer->depth)&0x0fff); //Taking 12LSBs for depth
			  BYTE intensity = realdepth == 0 || realdepth > 4095 ? 0 : 255 - (BYTE)(((float)realdepth / 4095.0f) * 255.0f);//Scaling to 255 scale grayscale
			  depthMat.data[i*depthMat.cols*cn + j*cn + 0] = intensity;
			  depthBuffer++;
		   }
		}


		// Copy FT depth data to IFTImage buffer
		memcpy(m_pFTdepthBuffer->GetBuffer(), PBYTE(FTlockedRect.pBits), std::min(m_pFTdepthBuffer->GetBufferSize(), UINT(pFTTexture->BufferLen())));
		
		if (m_Recording)
		{
			//*m_pcolorImage = depthMat;
			//cvWriteFrame(m_pwriter,m_pcolorImage);

			const NUI_SKELETON_FRAME* pSkeletonFrame = m_pPrimaryViewer->getSkeleton();
			m_pDepthInbedAPPs->processFrame(depthMat, lockedRect.pBits, m_ImageRes, pSkeletonFrame);

			//if (m_TimerCount%FramesPerFile==0)
			//{
			//	cvReleaseVideoWriter(&m_pwriter);
			//}
		}

        // Draw out the data with Direct2D
        if (m_pStreamViewer)
        {
            m_pStreamViewer->SetImage(&m_imageBuffer);
        }
    }

    // Done with the texture. Unlock and release it
	pFTTexture->UnlockRect(0);
    pTexture->UnlockRect(0);
    pTexture->Release();

ReleaseFrame:
    // Release the frame
    m_pNuiSensor->NuiImageStreamReleaseFrame(m_hStreamHandle, &imageFrame);
}

void NuiDepthStream::SetRecordingStatus (bool RecStatus) 
{
	m_Recording=RecStatus;
	m_TimerCount=0;
}

bool NuiDepthStream::GetRecordingStauts () const {return m_Recording;}


HRESULT NuiDepthStream::FTdepthInit(FT_CAMERA_CONFIG DepthCameraConfig)
{
	HRESULT hr;
    m_pFTdepthBuffer=FTCreateImage();
	hr = m_pFTdepthBuffer->Allocate(DepthCameraConfig.Width, DepthCameraConfig.Height, FTIMAGEFORMAT_UINT16_D13P3);
	if (!m_pFTdepthBuffer||FAILED(hr)) 
		return E_OUTOFMEMORY;
	return hr;
}