//------------------------------------------------------------------------------
// <copyright file="NuiColorStream.cpp" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "NuiColorStream.h"
#include "NuiStreamViewer.h"
#include <strsafe.h>
// #include <FaceTrackLib.h>

#include "opencv\cv.h"
#include "opencv\highgui.h"

#define FramesPerSecond 25          //Kinect RGB 30 frames per second
static const UINT FileSizeInMS = 60000;  //60 seconds each file
static const UINT FramesPerFile = FileSizeInMS*FramesPerSecond/1000;   //////

/// <summary>
/// Constructor
/// </summary>
/// <param name="pNuiSensor">The pointer to Nui sensor object</param>
NuiColorStream::NuiColorStream(IKinectSensor* pNuiSensor, PCWSTR instanceName)
    : NuiStream(pNuiSensor)
	, m_imageType(ColorImageFormat_Bayer)
	, m_imageResolution(NUI_IMAGE_RESOLUTION_1920x1080)
	, m_pwriter(nullptr)
	, m_pcolorImage(nullptr)
	, m_TimerCount(0)
	, m_instanceName((WCHAR*)instanceName)
	, m_pColorFrameReader(nullptr)
	, m_Recording(false)
	, m_FTRecording(false)
	//, m_pFTcolorBuffer(nullptr)
{}

/// <summary>
/// Destructor
/// </summary>
NuiColorStream::~NuiColorStream()
{
	if (m_pwriter) cvReleaseVideoWriter(&m_pwriter);
    /*if (m_pFTcolorBuffer) {
		m_pFTcolorBuffer->Release();
		m_pFTcolorBuffer=nullptr;
	}*/

}

/// <summary>
/// Attach viewer object to stream object
/// </summary>
/// <param name="pStreamViewer">The pointer to viewer object to attach</param>
/// <returns>Previously attached viewer object. If none, returns nullptr</returns>
NuiStreamViewer* NuiColorStream::SetStreamViewer(NuiStreamViewer* pStreamViewer)
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
/// Start stream processing.
/// </summary>
/// <returns>Indicate success or failure.</returns>
HRESULT NuiColorStream::StartStream()
{
	HRESULT hr;
	SetImageType(ColorImageFormat_Bayer);                 // Set default image type to color image
	SetImageResolution(NUI_IMAGE_RESOLUTION_1920x1080);   // Set default image resolution to 1920x1080
	
	// Initialize Nui sensor
	IColorFrameSource* pColorFrameSource = NULL;
	hr = m_pNuiSensor->Open();
	if (SUCCEEDED(hr))
	{
		hr = m_pNuiSensor->get_ColorFrameSource(&pColorFrameSource);
	}
	if (SUCCEEDED(hr))
	{
		hr = pColorFrameSource->OpenReader(&m_pColorFrameReader);
	}
	if (SUCCEEDED(hr))
	{
		m_pColorFrameReader->SubscribeFrameArrived(&m_hColorFrameArrived);
	}
	SafeRelease(pColorFrameSource);
    return OpenStream();
}

/// <summary>
/// Open stream with a certain image resolution.
/// </summary>
/// <returns>Indicates success or failure.</returns>
HRESULT NuiColorStream::OpenStream()
{
	//HRESULT hr;
	// Open color stream.
    /*hr = m_pNuiSensor->NuiImageStreamOpen(m_imageType,m_imageResolution,2,GetFrameReadyEvent(),&m_hStreamHandle);*/

    // Reset image buffer
   // if (SUCCEEDED(hr))
    //{
    m_imageBuffer.SetImageSize(m_imageResolution);  // Set source image resolution to image buffer
    //}

	// Video writer
	m_ImageRes = cvSize(static_cast<int>(m_imageBuffer.GetWidth()),static_cast<int>(m_imageBuffer.GetHeight()));
	m_pcolorImage = cvCreateImage(m_ImageRes, 8, 4);    //openCV only supports <=8 bits
    return S_OK;
}

/// <summary>
/// Set image type. Only color image types are acceptable
/// </summary>
/// <param name="type">Image type to be set</param>
void NuiColorStream::SetImageType(ColorImageFormat type)
{
    switch (type)
    {
	case ColorImageFormat_Bayer:

    //case NUI_IMAGE_TYPE_COLOR_YUV:

    //case NUI_IMAGE_TYPE_COLOR_INFRARED:

    //case NUI_IMAGE_TYPE_COLOR_RAW_BAYER:
        m_imageType   = type;
        break;
    default:
        break;
    }

    // Sync the stream viewer image type
    m_pStreamViewer->SetImageType(m_imageType);
}

/// <summary>
/// Set image resolution. Only 640x480 and 1280x960 formats are acceptable
/// </summary>
/// <param name="resolution">Image resolution to be set</param>
void NuiColorStream::SetImageResolution(NUI_IMAGE_RESOLUTION resolution)
{
    switch (resolution)
    {
    //case NUI_IMAGE_RESOLUTION_640x480:
	//	break;
    case NUI_IMAGE_RESOLUTION_1920x1080:
        m_imageResolution = resolution;
        break;
    default:
        break;
    }
}

/// <summary>
/// Process a incoming stream frame
/// </summary>
void NuiColorStream::ProcessStreamFrame()
{

    if (WAIT_OBJECT_0 == WaitForSingleObject(GetFrameReadyEvent(), 0))
    {
        // Frame ready event has been set. Proceed to process incoming frame
        ProcessColor();
    }
}

/// <summary>
/// Process the incoming color frame
/// </summary>
void NuiColorStream::ProcessColor()
{
	HRESULT hr;
	IColorFrame* imageFrame = nullptr;
	INT64 nTime = 0;
	RGBQUAD *pBuffer = nullptr;
	UINT nBufferSize = 0;
	IColorFrameArrivedEventArgs* pArgs = nullptr;
	POINT Point;
	Point.x = Point.y = 0;

	if (!m_pColorFrameReader)
	{
		return;
	}
	// HRESULT hr = m_pColorFrameReader->GetFrameArrivedEventData(m_hColorFrameArrived, &pArgs);
	IColorFrame* pColorFrame = NULL;
	hr = m_pColorFrameReader->AcquireLatestFrame(&pColorFrame);
	if (SUCCEEDED(hr))
	{
		INT64 nTime = 0;
		IFrameDescription* pFrameDescription = NULL;
		int nWidth = 0;
		int nHeight = 0;
		ColorImageFormat imageFormat = ColorImageFormat_None;
		UINT nBufferSize = 0;
		RGBQUAD *pBuffer = NULL;

		hr = pColorFrame->get_RelativeTime(&nTime);

		if (SUCCEEDED(hr))
		{
			hr = pColorFrame->get_FrameDescription(&pFrameDescription);
		}

		if (SUCCEEDED(hr))
		{
			hr = pFrameDescription->get_Width(&nWidth);
		}

		if (SUCCEEDED(hr))
		{
			hr = pFrameDescription->get_Height(&nHeight);
		}

		if (SUCCEEDED(hr))
		{
			hr = pColorFrame->get_RawColorImageFormat(&imageFormat);
		}

		if (SUCCEEDED(hr))
		{
			if (imageFormat == ColorImageFormat_Bgra)
			{
				hr = pColorFrame->AccessRawUnderlyingBuffer(&nBufferSize, reinterpret_cast<BYTE**>(&pBuffer));
				m_imageBuffer.CopyBayer(reinterpret_cast<BYTE*>(pBuffer), nWidth * nHeight * sizeof(RGBQUAD));
				if (m_pStreamViewer)
				{
					m_pStreamViewer->SetImage(&m_imageBuffer);
				}
			}
			else
			{
				hr = E_FAIL;
			}
		}

		/*if (SUCCEEDED(hr))
		{
			ProcessColor();
		}*/
		SafeRelease(pFrameDescription);
	}
	SafeRelease(pColorFrame);
	
}
	/*if (m_Recording)
	{
	    //////Initializaing a video writer and allocate an image for recording /////////
		if ((m_TimerCount++) % FramesPerFile==0)
		{
		    WCHAR szFilename[MAX_PATH] = { 0 };
		    if (SUCCEEDED(GetFileName(szFilename,_countof(szFilename), m_instanceName, RGBSensor)))
			{
				char char_szFilename[MAX_PATH] = {0};
				size_t convertedChars;
				wcstombs_s(&convertedChars,char_szFilename,sizeof(char_szFilename),szFilename,sizeof(char_szFilename));
				m_pwriter = cvCreateVideoWriter(char_szFilename,
				CV_FOURCC('L', 'A', 'G', 'S'),
				//-1,  //user specified 
				FramesPerSecond,m_ImageRes);
				//2,m_ImageRes);
			}
			m_TimerCount %= FramesPerFile;
		}
	}*/

    // Attempt to get the color frame
	// HRESULT hr = m_pColorFrameReader->AcquireLatestFrame(&imageFrame);
    // hr = m_pNuiSensor->NuiImageStreamGetNextFrame(m_hStreamHandle, 0, &imageFrame);
    /*if (FAILED(hr))
    {
        return;
    }

    if (m_paused)
    {
        // Stream paused. Skip frame process and release the frame.
        goto ReleaseFrame;
    }

	hr = imageFrame->AccessRawUnderlyingBuffer(&nBufferSize, reinterpret_cast<BYTE**>(&pBuffer));*/

    // INuiFrameTexture* pTexture = imageFrame.pFrameTexture;

    // Lock the frame data so the Kinect knows not to modify it while we are reading it
   /// NUI_LOCKED_RECT lockedRect;
   // pTexture->LockRect(0, &lockedRect, NULL, 0);

    // Make sure we've received valid data
    /*if (lockedRect.Pitch != 0)
    {
        switch (m_imageType)
        {
		case ColorImageFormat_Bayer:                 // Convert raw bayer data to color image and copy to image buffer
            m_imageBuffer.CopyBayer(lockedRect.pBits, lockedRect.size);
			/////Recording
			if (m_Recording)
			{
				cvSetData(m_pcolorImage,lockedRect.pBits,lockedRect.Pitch);
				cvWriteFrame(m_pwriter,m_pcolorImage);
			}
            break;

        case NUI_IMAGE_TYPE_COLOR_INFRARED:             // Convert infrared data to color image and copy to image buffer
            m_imageBuffer.CopyInfrared(lockedRect.pBits, lockedRect.size);
			/////Recording
			if (m_Recording)
			{
				cvSetData(m_pcolorImage,lockedRect.pBits,lockedRect.Pitch);
				cvWriteFrame(m_pwriter,m_pcolorImage);
			}
            break;

        default:   
            m_imageBuffer.CopyRGB(lockedRect.pBits, lockedRect.size);
			memcpy(m_pFTcolorBuffer->GetBuffer(), PBYTE(lockedRect.pBits), std::min(m_pFTcolorBuffer->GetBufferSize(), UINT(pTexture->BufferLen())));
			cvSetData(m_pcolorImage,lockedRect.pBits,lockedRect.Pitch);

			/////Recording
			if (m_Recording)
			{
				cvWriteFrame(m_pwriter,m_pcolorImage);
			}
			
            break;
        }

        if (m_pStreamViewer)
        {
            // Set image data to viewer
            m_pStreamViewer->SetImage(&m_imageBuffer);
        }
		
		///Recording
		if (m_Recording)
		{
			if (m_TimerCount%FramesPerFile==0)
			{
				cvReleaseVideoWriter(&m_pwriter);
			}
		}
    }

    // Unlock frame data
    pTexture->UnlockRect(0);

    // ReleaseFrame:
    m_pNuiSensor->NuiImageStreamReleaseFrame(m_hStreamHandle, &imageFrame);*/


void NuiColorStream::SetRecordingStatus (bool RecStatus) 
{
	m_Recording=RecStatus;
	m_TimerCount=0;
}

bool NuiColorStream::GetRecordingStauts () const {return m_Recording;}

/*HRESULT NuiColorStream::FTcolorInit(FT_CAMERA_CONFIG ColorCameraConfig)
{
	HRESULT hr;
    m_pFTcolorBuffer=FTCreateImage();
	hr = m_pFTcolorBuffer->Allocate(ColorCameraConfig.Width, ColorCameraConfig.Height, FTIMAGEFORMAT_UINT8_B8G8R8X8);
	if (!m_pFTcolorBuffer||FAILED(hr)) 
		return E_OUTOFMEMORY;
	return hr;
}
*/

