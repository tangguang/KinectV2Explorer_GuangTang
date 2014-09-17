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
static const UINT FramesPerFile = FileSizeInMS * FramesPerSecond / 1000;   //////

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
	SafeRelease(m_pColorFrameReader);
	SafeRelease(m_pNuiSensor);
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
    m_imageBuffer.SetImageSize(m_imageResolution);  // Set source image resolution to image buffer

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

	if (m_paused)
    {
		return;
    }
	if (WAIT_OBJECT_0 == WaitForSingleObject(reinterpret_cast<HANDLE>(GetArrivedEvent()), 0))
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
	if (!m_pColorFrameReader)
	{
		return;
	}

	IColorFrame* pColorFrame = NULL;
	HRESULT hr = m_pColorFrameReader->AcquireLatestFrame(&pColorFrame);
	if (SUCCEEDED(hr))
	{
		INT64 nTime = 0;
		IFrameDescription* pFrameDescription = NULL;
		int nWidth = 0;
		int nHeight = 0;
		ColorImageFormat imageFormat = ColorImageFormat_None;
		UINT nBufferSize = 0;
		RGBQUAD *pBuffer = NULL;
		RGBQUAD* m_pColorRGBX;

		if (m_Recording)
		{
			// Initializating a video writer
			if ((m_TimerCount++) % FramesPerFile == 0)
			{
				WCHAR szFilename[MAX_PATH] = { 0 };
				if (SUCCEEDED(GetFileName(szFilename, _countof(szFilename), m_instanceName, RGBSensor)))
				{
					char char_szFilename[MAX_PATH] = { 0 };
					size_t convertedChars;
					wcstombs_s(&convertedChars, char_szFilename, sizeof(char_szFilename), szFilename, sizeof(char_szFilename));
					m_pwriter = cvCreateVideoWriter(  char_szFilename,
						                              CV_FOURCC('M', 'J', 'P', 'G'),
													  FramesPerSecond,
						                              m_ImageRes,
													  1
						                           );
				}
				m_TimerCount %= FramesPerFile;
			}
		}
		// Get the color frame
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
			m_pColorRGBX = new RGBQUAD[nWidth * nHeight];
			if (imageFormat == ColorImageFormat_Bgra)
			{
				hr = pColorFrame->AccessRawUnderlyingBuffer(&nBufferSize, reinterpret_cast<BYTE**>(&pBuffer));
				m_imageBuffer.CopyRGB(reinterpret_cast<BYTE*>(pBuffer), nWidth * nHeight * sizeof(RGBQUAD));
			}
			else if (m_pColorRGBX)
			{
				pBuffer = m_pColorRGBX;
				nBufferSize = nWidth * nHeight * sizeof(RGBQUAD);
				hr = pColorFrame->CopyConvertedFrameDataToArray(nBufferSize, reinterpret_cast<BYTE*>(pBuffer), ColorImageFormat_Bgra);
				m_imageBuffer.CopyRGB(reinterpret_cast<BYTE*>(pBuffer), nWidth * nHeight * sizeof(RGBQUAD));
				
				// Recording
				if (m_Recording)
				{
					cvSetData(m_pcolorImage, m_imageBuffer.GetBuffer(), nWidth * sizeof(RGBQUAD));
					cvWriteFrame(m_pwriter, m_pcolorImage);
				}
				
				// Release RGBX buffer
				if (m_pColorRGBX)
				{
					delete[] m_pColorRGBX;
					m_pColorRGBX = NULL;
				}
			}
			else
			{
				hr = E_FAIL;
			}
		}
		if (m_pStreamViewer)
		{
			m_pStreamViewer->SetImage(&m_imageBuffer);
		}

		// Recording
		if (m_Recording)
		{
			if (m_TimerCount % FramesPerFile == 0)
			{
				cvReleaseVideoWriter(&m_pwriter);
			}
		}
		SafeRelease(pFrameDescription);
	}
	SafeRelease(pColorFrame);
}

void NuiColorStream::SetRecordingStatus (bool RecStatus) 
{
	m_Recording=RecStatus;
	m_TimerCount=0;
}

bool NuiColorStream::GetRecordingStauts () const {return m_Recording;}

WAITABLE_HANDLE NuiColorStream::GetArrivedEvent() { return m_hColorFrameArrived; }

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

