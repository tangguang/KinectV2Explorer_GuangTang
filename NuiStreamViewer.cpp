//------------------------------------------------------------------------------
// <copyright file="NuiStreamViewer.cpp" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#include "stdafx.h"
#include <stdio.h>

#include <math.h>
#include <malloc.h>

#include "NuiStreamViewer.h"
#include "NuiAudioStream.h"
#include "resource.h"
#include <strsafe.h>

static const FLOAT CircleCenterX = 280.0f;
static const FLOAT CircleCenterY = 180.0f;

static const FLOAT TargetDistThr = 10.0f;

/// <summary>
/// Constructor
/// </summary>
/// <param name="pParent">The pointer to parent window</param>
NuiStreamViewer::NuiStreamViewer(const NuiViewer* pParent)
    : NuiViewer(pParent)
    , m_imageType(NUI_IMAGE_TYPE_COLOR)
    , m_pImage(nullptr)
    , m_pauseSkeleton(false)
	, m_isHandReinitial(FALSE)
	, m_isHitTarget(FALSE)
    , m_pSkeletonFrame(nullptr)
    , m_drawEdgeFlags(0)
    , m_frameCount(0)
    , m_lastFrameCount(0)
    , m_fps(0)
	, m_ImageRendererBrush(ImageRendererBrushRed)
{
    m_pImageRenderer = new ImageRenderer();
	m_pLegRaisExcer = new LegRaisExcer();
	m_pHandRaisExcer = new HandRaisExcer();
	m_pInbedAPPs = new InbedAPPs();
	m_pStrToSpeak = new WCHAR[256];
    m_lastTick = GetTickCount();
}

/// <summary>
/// Destructor
/// </summary>
NuiStreamViewer::~NuiStreamViewer()
{
	SafeDelete(m_pInbedAPPs);
	SafeDelete(m_pLegRaisExcer);
	SafeDelete(m_pHandRaisExcer);
    SafeDelete(m_pImageRenderer);
	SafeDelete(m_pStrToSpeak);
}

/// <summary>
/// Dispatch window message to message handlers.
/// </summary>
/// <param name="hWnd">Handle to window</param>
/// <param name="uMsg">Message type</param>
/// <param name="wParam">Extra message parameter</param>
/// <param name="lParam">Extra message parameter</param>
/// <returns>
/// If message is handled, non-zero is returned. Otherwise FALSE is returned and message is passed to default dialog procedure
/// </returns>
LRESULT NuiStreamViewer::DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CTLCOLORDLG:
        // Set background color as black
        return (LRESULT)GetStockObject(BLACK_BRUSH);

    case WM_PAINT:
        OnPaint(wParam, lParam);
        break;

    case WM_SIZE:
        {
            UINT width  = LOWORD(lParam);
            UINT height = HIWORD(lParam);
            m_pImageRenderer->ResizeRenderTarget(width, height);
        }
        break;

    default:
        break;
    }

    return (LRESULT)FALSE;
}

/// <summary>
/// Returns the ID of the dialog
/// </summary>
/// <returns>ID of dialog</returns>
UINT NuiStreamViewer::GetDlgId()
{
    return IDD_STREAM_VIEW;
}

/// <summary>
/// Message handler of WM_PAINT.
/// </summary>
/// <param name="wParam">Extra message parameter</param>
/// <param name="lParam">Extra message parameter</param>
void NuiStreamViewer::OnPaint(WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = m_pImageRenderer->BeginDraw(m_hWnd);
    if (FAILED(hr))
        return;

    // Get viewer window client rect
    RECT clientRect;
    if (!::GetClientRect(m_hWnd, &clientRect))
    {
        return;
    }

    // Calculate the area the stream image is to streched to fit
    D2D1_RECT_F imageRect = GetImageRect(clientRect);

    // Draw stream images
    DrawImage(imageRect);

    // Draw skeletons
    DrawSkeletons(imageRect);

	// Draw Leg Raising Practice message to screen
	DrawLegRaisMeg(clientRect);

	// Draw Hand Raising Practice message to screen
	DrawHandRaisMeg(clientRect);

	// Draw Events Detection message to screen
	DrawDetectionMeg(clientRect);

	// Draw Hands Movement RIC message to screen
	DrawHandsMovementRICMeg(imageRect,clientRect);

    // Draw image resolution
    DrawResolution(clientRect);

    // Draw FPS
    DrawFPS(clientRect);

    // Draw red edges if skeleton is close to edges of image
    DrawRedEdges(imageRect);

    m_pImageRenderer->EndDraw();
}

/// <summary>
/// Draw the image on screen by D2D
/// </summary>
/// <param name="imageRect">The rect which the color or depth image is streched to fit</param>
void NuiStreamViewer::DrawImage(const D2D1_RECT_F& imageRect)
{
    if (m_pImage && m_pImage->GetBufferSize())
    {
        D2D1_SIZE_U imageSize = D2D1::SizeU(m_pImage->GetWidth(), m_pImage->GetHeight());
        m_pImageRenderer->DrawImage(m_pImage->GetBuffer(), imageSize, imageRect);
    }
}

/// <summary>
/// Draw skeletons
/// </summary>
/// <param name="imageRect">The rect which the color or depth stream image is streched to fit</param>
void NuiStreamViewer::DrawSkeletons(const D2D1_RECT_F& imageRect)
{
    if (m_pSkeletonFrame && !m_pauseSkeleton)
    {
        // Clip the area to avoid drawing outside the image
        m_pImageRenderer->SetClipRect(imageRect);

        for (int i = 0; i < NUI_SKELETON_COUNT; i++)
        {
            m_state = m_pSkeletonFrame->SkeletonData[i].eTrackingState;
            if (NUI_SKELETON_TRACKED == m_state)
            {
                // Draw bones and joints of tracked skeleton
                //DrawSkeleton(m_pSkeletonFrame->SkeletonData[i], imageRect);
				if (m_pInbedAPPs->getIsRunHandsMovementRIC())
				{
					DrawHandsPosition(m_pSkeletonFrame->SkeletonData[i], imageRect);
				}

				// Process the skeleton data for 'Leg Raising' application
				m_pLegRaisExcer->CallProcess(m_pSkeletonFrame->SkeletonData[i]);

				// Process the skeleton data for 'Hand Raising' application
				m_pHandRaisExcer->CallProcess(m_pSkeletonFrame->SkeletonData[i]);

				// Process the skeleton data for 'Fall detection' application
				m_pInbedAPPs->CallProcess(m_pSkeletonFrame->SkeletonData[i], *m_pSkeletonFrame);
				
				//// Save joint positions to file
				//SaveJointPos(m_pSkeletonFrame->SkeletonData[i], *m_pSkeletonFrame); 
            }
            else if (NUI_SKELETON_POSITION_ONLY == m_state)
            {
                //DrawPosition(m_pSkeletonFrame->SkeletonData[i], imageRect);
            }
        }

        m_pImageRenderer->ResetClipRect();
    }
}

/// <summary>
/// Draw skeleton.
/// </summary>
/// <param name="skeletonData">Skeleton coordinates</param>
/// <param name="imageRect">The rect which the color or depth stream image is streched to fit</param>
void NuiStreamViewer::DrawSkeleton(const NUI_SKELETON_DATA& skeletonData, const D2D1_RECT_F& imageRect)
{
    // Torso
    DrawBone(skeletonData, imageRect, NUI_SKELETON_POSITION_HEAD,               NUI_SKELETON_POSITION_SHOULDER_CENTER);
    DrawBone(skeletonData, imageRect, NUI_SKELETON_POSITION_SHOULDER_CENTER,    NUI_SKELETON_POSITION_SHOULDER_LEFT);
    DrawBone(skeletonData, imageRect, NUI_SKELETON_POSITION_SHOULDER_CENTER,    NUI_SKELETON_POSITION_SHOULDER_RIGHT);
    DrawBone(skeletonData, imageRect, NUI_SKELETON_POSITION_SHOULDER_CENTER,    NUI_SKELETON_POSITION_SPINE);
    DrawBone(skeletonData, imageRect, NUI_SKELETON_POSITION_SPINE,              NUI_SKELETON_POSITION_HIP_CENTER);
    DrawBone(skeletonData, imageRect, NUI_SKELETON_POSITION_HIP_CENTER,         NUI_SKELETON_POSITION_HIP_LEFT);
    DrawBone(skeletonData, imageRect, NUI_SKELETON_POSITION_HIP_CENTER,         NUI_SKELETON_POSITION_HIP_RIGHT);

    // Left arm
    DrawBone(skeletonData, imageRect, NUI_SKELETON_POSITION_SHOULDER_LEFT,      NUI_SKELETON_POSITION_ELBOW_LEFT);
    DrawBone(skeletonData, imageRect, NUI_SKELETON_POSITION_ELBOW_LEFT,         NUI_SKELETON_POSITION_WRIST_LEFT);
    DrawBone(skeletonData, imageRect, NUI_SKELETON_POSITION_WRIST_LEFT,         NUI_SKELETON_POSITION_HAND_LEFT);

    // Right arm
    DrawBone(skeletonData, imageRect, NUI_SKELETON_POSITION_SHOULDER_RIGHT,     NUI_SKELETON_POSITION_ELBOW_RIGHT);
    DrawBone(skeletonData, imageRect, NUI_SKELETON_POSITION_ELBOW_RIGHT,        NUI_SKELETON_POSITION_WRIST_RIGHT);
    DrawBone(skeletonData, imageRect, NUI_SKELETON_POSITION_WRIST_RIGHT,        NUI_SKELETON_POSITION_HAND_RIGHT);

    // Left leg
    DrawBone(skeletonData, imageRect, NUI_SKELETON_POSITION_HIP_LEFT,           NUI_SKELETON_POSITION_KNEE_LEFT);
    DrawBone(skeletonData, imageRect, NUI_SKELETON_POSITION_KNEE_LEFT,          NUI_SKELETON_POSITION_ANKLE_LEFT);
    DrawBone(skeletonData, imageRect, NUI_SKELETON_POSITION_ANKLE_LEFT,         NUI_SKELETON_POSITION_FOOT_LEFT);

    // Right leg
    DrawBone(skeletonData, imageRect, NUI_SKELETON_POSITION_HIP_RIGHT,          NUI_SKELETON_POSITION_KNEE_RIGHT);
    DrawBone(skeletonData, imageRect, NUI_SKELETON_POSITION_KNEE_RIGHT,         NUI_SKELETON_POSITION_ANKLE_RIGHT);
    DrawBone(skeletonData, imageRect, NUI_SKELETON_POSITION_ANKLE_RIGHT,        NUI_SKELETON_POSITION_FOOT_RIGHT);

    // Draw joints
    for (int i = 0; i < NUI_SKELETON_POSITION_COUNT; i++)
    {
        DrawJoint(skeletonData, imageRect, (NUI_SKELETON_POSITION_INDEX)i);
    }
}

/// <summary>
/// Draw a circle to indicate a skeleton of which only position info is available
/// </summary>
/// <param name="skeletonData">Skeleton coordinates</param>
/// <param name="imageRect">The rect which the color or depth stream image is streched to fit</param>
void NuiStreamViewer::DrawHandsPosition(const NUI_SKELETON_DATA& skeletonData, const D2D1_RECT_F& imageRect)
{
    //D2D1_POINT_2F center = ToImageRect(skeletonData.Position, imageRect);
	D2D1_POINT_2F LeftHand = ToImageRect(skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_HAND_LEFT], imageRect);
	D2D1_POINT_2F RightHand = ToImageRect(skeletonData.SkeletonPositions[NUI_SKELETON_POSITION_HAND_RIGHT], imageRect);
    m_pImageRenderer->DrawCircle(LeftHand, 10.0f, ImageRendererBrushWhite, 5.0f);
	m_pImageRenderer->DrawCircle(RightHand, 10.0f, ImageRendererBrushWhite, 5.0f);
}

/// <summary>
/// Draw a bone between 2 tracked joint.
/// <summary>
/// <param name="skeletonData">Skeleton coordinates</param>
/// <param name="imageRect">The rect which the color or depth image is streched to fit</param>
/// <param name="joint0">Index for the first joint</param>
/// <param name="joint1">Index for the second joint</param>
void NuiStreamViewer::DrawBone(const NUI_SKELETON_DATA& skeletonData, const D2D1_RECT_F& imageRect, NUI_SKELETON_POSITION_INDEX joint0, NUI_SKELETON_POSITION_INDEX joint1)
{
    NUI_SKELETON_POSITION_TRACKING_STATE state0 = skeletonData.eSkeletonPositionTrackingState[joint0];
    NUI_SKELETON_POSITION_TRACKING_STATE state1 = skeletonData.eSkeletonPositionTrackingState[joint1];

    // Any is not tracked
    if (NUI_SKELETON_POSITION_NOT_TRACKED == state0 || NUI_SKELETON_POSITION_NOT_TRACKED == state1)
    {
        return;
    }

    // Both are inferred
    if (NUI_SKELETON_POSITION_INFERRED == state0 && NUI_SKELETON_POSITION_INFERRED == state1)
    {
        return;
    }

    D2D1_POINT_2F point0 = ToImageRect(skeletonData.SkeletonPositions[joint0], imageRect);
    D2D1_POINT_2F point1 = ToImageRect(skeletonData.SkeletonPositions[joint1], imageRect);

    // We assume all drawn bones are inferred unless BOTH joints are tracked
    if (NUI_SKELETON_POSITION_TRACKED == state0 && NUI_SKELETON_POSITION_TRACKED == state1)
    {
        m_pImageRenderer->DrawLine(point0, point1, ImageRendererBrushBoneTracked, 4.0f);
    }
    else
    {
        m_pImageRenderer->DrawLine(point0, point1, ImageRendererBrushBoneInferred, 4.0f);
    }
}

/// <summary>
/// Draw a joint of the skeleton
/// </summary>
/// <param name="skeletonData">Skeleton coordinates</param>
/// <param name="imageRect">The rect which the color or depth image is streched to fit</param>
/// <param name="joint">Index for the joint to be drawn</param>
void NuiStreamViewer::DrawJoint(const NUI_SKELETON_DATA& skeletonData, const D2D1_RECT_F& imageRect, NUI_SKELETON_POSITION_INDEX joint)
{
    NUI_SKELETON_POSITION_TRACKING_STATE state = skeletonData.eSkeletonPositionTrackingState[joint];

    // Not tracked
    if (NUI_SKELETON_POSITION_NOT_TRACKED == state)
    {
        return;
    }

    D2D1_POINT_2F point = ToImageRect(skeletonData.SkeletonPositions[joint], imageRect);

    if (NUI_SKELETON_POSITION_TRACKED == state)
    {
        m_pImageRenderer->DrawCircle(point, 3.0f, ImageRendererBrushJointTracked, 3.0f);
    }
    else
    {
        if (!IsOutOfImageRect(point, imageRect))
        {
            m_pImageRenderer->DrawCircle(point, 3.0f, ImageRendererBrushJointInferred, 3.0f);
        }

        UpdateDrawEdgeFlags(point, imageRect);
    }
}

/// <summary>
/// Draw frame FPS counter
/// </summary>
/// <param name="clientRect">Client area of viewer's window</param>
void NuiStreamViewer::DrawFPS(const RECT& clientRect)
{
    // Get rectangle position and size
    D2D1_RECT_F rect = D2D1::RectF((FLOAT)clientRect.right - 50.0f, 0.0f, (FLOAT)clientRect.right, (FLOAT)clientRect.top + 50.0f);

    // Fill rectangle
    m_pImageRenderer->FillRectangle(rect, ImageRendererBrushGray);

    // Draw a while circle
    D2D1_POINT_2F center = D2D1::Point2F((rect.right + rect.left) / 2.0f, (rect.bottom + rect.top) / 2.0f);
    m_pImageRenderer->DrawCircle(center, 23.0f, ImageRendererBrushWhite, 4.0f);
    
    // Draw FPS text
    WCHAR text[MaxStringChars];
    swprintf_s(text, sizeof(text) / sizeof(WCHAR) - 1, L"%d", m_fps);
    UINT cch = (UINT)wcsnlen_s(text, MaxStringChars);
    m_pImageRenderer->DrawText(text, cch, rect, ImageRendererBrushWhite, ImageRendererTextFormatFps);
}

/// <summary>
/// Draw image resolution text
/// </summary>
/// <param name="clientRect">Client area of viewer's window</param>
void NuiStreamViewer::DrawResolution(const RECT& clientRect)
{
    if (m_pImage)
    {
        WCHAR buffer[MaxStringChars];
        D2D1_RECT_F rect = D2D1::RectF((FLOAT)clientRect.left, (FLOAT)clientRect.top, (FLOAT)clientRect.right, 10.0f);
        swprintf_s(buffer, sizeof(buffer) / sizeof(WCHAR), L"Resolution: %dx%d", m_pImage->GetWidth(), m_pImage->GetHeight());
        m_pImageRenderer->DrawText(buffer, (UINT)wcsnlen_s(buffer, MaxStringChars), rect, ImageRendererBrushGreen, ImageRendererTextFormatResolution);
    }
}

/// <summary>
/// Draw red edge on image when skeleton is close to or out of the image edge
/// </summary>
/// <param name="imageRect">The rectangle of the image</param>
void NuiStreamViewer::DrawRedEdges(const D2D1_RECT_F& imageRect)
{
    D2D1_RECT_F   rect;
    D2D1_POINT_2F start;
    D2D1_POINT_2F end;

    FLOAT edgeWidth = (imageRect.right - imageRect.left) / 40.0f;

    if (m_drawEdgeFlags & DRAW_EDGE_FLAG_LEFT)
    {
        rect.left   = imageRect.left;
        rect.top    = imageRect.top;
        rect.right  = imageRect.left + edgeWidth;
        rect.bottom = imageRect.bottom;

        start.x = rect.left;
        start.y = (rect.top + rect.bottom) / 2.0f;

        end.x   = rect.left + edgeWidth;
        end.y   = start.y;

        m_pImageRenderer->DrawEdge(rect, start, end);
    }

    if (m_drawEdgeFlags & DRAW_EDGE_FLAG_RIGHT)
    {
        rect.left   = imageRect.right - edgeWidth;
        rect.right  = imageRect.right;
        rect.top    = imageRect.top;
        rect.bottom = imageRect.bottom;

        start.x = rect.right;
        start.y = (rect.top + rect.bottom) / 2.0f;

        end.x   = rect.left;
        end.y   = start.y;

        m_pImageRenderer->DrawEdge(rect, start, end);
    }

    if (m_drawEdgeFlags & DRAW_EDGE_FLAG_TOP)
    {
        rect.left   = imageRect.left;
        rect.top    = imageRect.top;
        rect.right  = imageRect.right;
        rect.bottom = imageRect.top + edgeWidth;

        start.x = (rect.left + rect.right) / 2.0f;
        start.y = rect.top;

        end.x   = start.x;
        end.y   = rect.top + edgeWidth;

        m_pImageRenderer->DrawEdge(rect, start, end);
    }

    if (m_drawEdgeFlags & DRAW_EDGE_FLAG_BOTTOM)
    {
        rect.left   = imageRect.left;
        rect.top    = imageRect.bottom - edgeWidth;
        rect.right  = imageRect.right;
        rect.bottom = imageRect.bottom;

        start.x = (rect.left + rect.right) / 2.0f;
        start.y = rect.bottom;

        end.x   = start.x;
        end.y   = rect.top;

        m_pImageRenderer->DrawEdge(rect, start, end);
    }

    m_drawEdgeFlags = 0;
}

/// <summary>
/// Set the buffer containing the image pixels.
/// </summary>
/// <param name="pImage">The pointer to image buffer object</param>
void NuiStreamViewer::SetImage(const NuiImageBuffer* pImage)
{
    m_pImage = pImage;
    if (m_pImage &&  m_pImage->GetBufferSize() && m_hWnd)
    {
        InvalidateRect(m_hWnd, nullptr, FALSE);

        UpdateFrameRate();
    }
}

/// <summary>
/// Attach skeleton data.
/// </summary>
/// <param name="pFrame">The pointer to skeleton frame</param>
void NuiStreamViewer::SetSkeleton(const NUI_SKELETON_FRAME* pFrame)
{
    if (!m_hWnd)
    {
        return;
    }

    m_pSkeletonFrame = pFrame;

    InvalidateRect(m_hWnd, nullptr, FALSE);
}

/// <summary>
/// Update frame rate
/// </summary>
void NuiStreamViewer::UpdateFrameRate()
{
    m_frameCount++;

    DWORD tickCount = GetTickCount();
    DWORD span      = tickCount - m_lastTick;
    if (span >= 1000)
    {
        m_fps            = (UINT)((double)(m_frameCount - m_lastFrameCount) * 1000.0 / (double)span + 0.5);
        m_lastTick       = tickCount;
        m_lastFrameCount = m_frameCount;
    }
}

/// <summary>
/// Check which red edge should be drawn
/// </summary>
/// <param name="point">Coordinates of a tracked joint</param>
/// <param name="imageRect">Rectangle of the image</param>
void NuiStreamViewer::UpdateDrawEdgeFlags(const D2D1_POINT_2F& point, const D2D1_RECT_F& imageRect)
{
    FLOAT detectWidth = (imageRect.right - imageRect.left) / 40.0f;

    if (point.x < imageRect.left + detectWidth)
    {
        m_drawEdgeFlags |= DRAW_EDGE_FLAG_LEFT;
    }

    if (point.x > imageRect.right - detectWidth)
    {
        m_drawEdgeFlags |= DRAW_EDGE_FLAG_RIGHT;
    }

    if (point.y < imageRect.top + detectWidth)
    {
        m_drawEdgeFlags |= DRAW_EDGE_FLAG_TOP;
    }

    if (point.y > imageRect.bottom - detectWidth)
    {
        m_drawEdgeFlags |= DRAW_EDGE_FLAG_BOTTOM;
    }
}

/// <summary>
/// Decide if skeleton is out of image
/// </summary>
/// <param name="point">Coordinates of a tracked joint</param>
/// <param name="imageRect">Rectangle of the image</param>
bool NuiStreamViewer::IsOutOfImageRect(const D2D1_POINT_2F& point, const D2D1_RECT_F& imageRect)
{
    if (point.x < imageRect.left || point.x > imageRect.right)
    {
        return true;
    }
    
    if (point.y < imageRect.top || point.y > imageRect.bottom)
    {
        return true;
    }

    return false;
}

/// <summary>
/// Calculate the coordinates of the image to be displayed in client area.
/// </summary>
/// <param name="client">Client area of viewer's window</param>
D2D1_RECT_F NuiStreamViewer::GetImageRect(const RECT &client)
{
    D2D1_RECT_F imageRect = D2D1::RectF();
    if (m_pImage && m_pImage->GetBuffer())
    {
        float ratio  = static_cast<float>(m_pImage->GetWidth()) / static_cast<float>(m_pImage->GetHeight());
        float width  = static_cast<float>(client.right);
        float height = width / ratio;

        if (height > client.bottom)
        {
            height = static_cast<float>(client.bottom);
            width  = height * ratio;
        }

        imageRect.left   = (client.right  - width  + 1) / 2.0f;
        imageRect.top    = (client.bottom - height + 1) / 2.0f;
        imageRect.right  = imageRect.left + width;
        imageRect.bottom = imageRect.top  + height;
    }

    return imageRect;
}

/// <summary>
/// Map skeleton point to window coordinate in image rect.
/// </summary>
/// <param name="skeletonPoint">Skeleton point to map.</param>
/// <param name="imageRect">The rectangle of image</param>
/// <returns>Mapped coordinate in client area</returns>
D2D1_POINT_2F NuiStreamViewer::ToImageRect(const Vector4& skeletonPoint, const D2D1_RECT_F& imageRect)
{
    const NUI_IMAGE_RESOLUTION imageResolution = NUI_IMAGE_RESOLUTION_640x480;
    LONG x = 0, y = 0;
    USHORT depthValue = 0;
    NuiTransformSkeletonToDepthImage(skeletonPoint, &x, &y, &depthValue, imageResolution); // Returns coordinates in depth space

    if (NUI_IMAGE_TYPE_COLOR == m_imageType || NUI_IMAGE_TYPE_COLOR_INFRARED == m_imageType
        || NUI_IMAGE_TYPE_COLOR_RAW_BAYER == m_imageType || NUI_IMAGE_TYPE_COLOR_RAW_YUV == m_imageType
        || NUI_IMAGE_TYPE_COLOR_YUV == m_imageType)
    {
        LONG backupX = x, backupY = y;
        if (FAILED(NuiImageGetColorPixelCoordinatesFromDepthPixelAtResolution(imageResolution, imageResolution, nullptr, x, y, depthValue, &x, &y)))
        {
            x = backupX;
            y = backupY;
        }
    }

    DWORD imageWidth, imageHeight;
    NuiImageResolutionToSize(imageResolution, imageWidth, imageHeight);

    FLOAT resultX, resultY;
    resultX = x * (imageRect.right  - imageRect.left + 1.0f) / imageWidth + imageRect.left;
    resultY = y * (imageRect.bottom - imageRect.top  + 1.0f) / imageHeight + imageRect.top;

    return D2D1::Point2F(resultX, resultY);
}

/// <summary>
/// Pause the skeleton
/// </summary>
/// <param name="pause">Pause or resume the skeleton</param>
void NuiStreamViewer::PauseSkeleton(bool pause)
{
    m_pauseSkeleton = pause;
}

void NuiStreamViewer::SaveJointPos(const NUI_SKELETON_DATA& skeletonData, const NUI_SKELETON_FRAME& skeletonFrame)
{
	wchar_t FilePath[MAX_PATH];
	
	swprintf_s(FilePath, MAX_PATH, L"%s\\KinectSkeletonPos\\skeletonPos.txt", knownPath);
	FILE* pfile;
	_wfopen_s(&pfile, FilePath, L"a+");
	for (int k = 0; k < NUI_SKELETON_POSITION_COUNT; k++)
	{
		Vector4 SkeletonPos = skeletonData.SkeletonPositions[k];
		fprintf(pfile, "%f %f %f %f %f %f %f\n", SkeletonPos.x, SkeletonPos.y, SkeletonPos.z, skeletonFrame.vFloorClipPlane.x,
			skeletonFrame.vFloorClipPlane.y, skeletonFrame.vFloorClipPlane.z, skeletonFrame.vFloorClipPlane.w);
	}
	fclose(pfile);
}

void NuiStreamViewer::DrawLegRaisMeg(const RECT& clientRect)
{
	if(m_pLegRaisExcer->isRunningExcer())
	{
        WCHAR buffer[MaxStringChars];
		///Display session and set number
		D2D1_RECT_F rect = D2D1::RectF((FLOAT)clientRect.left, (FLOAT)clientRect.top + 20.0f, (FLOAT)clientRect.right, 50.0f);
		swprintf_s(buffer, sizeof(buffer) / sizeof(WCHAR), L"Session 1 & Set 1");
        m_pImageRenderer->DrawText(buffer, (UINT)wcsnlen_s(buffer, MaxStringChars), rect, ImageRendererBrushHotPink, ImageRendererTextFormatFps);
       ///Display counts and scores
		rect = D2D1::RectF((FLOAT)clientRect.left, (FLOAT)clientRect.top + 80.0f, (FLOAT)clientRect.right, 80.0f);
		swprintf_s(buffer, sizeof(buffer) / sizeof(WCHAR), L"LCount: %d  RCount: %d L/R Scores: %3.1f %3.1f", m_pLegRaisExcer->GetLeftCount(), m_pLegRaisExcer->GetRightCount(), m_pLegRaisExcer->GetLeftScore(), m_pLegRaisExcer->GetRightScore());
        m_pImageRenderer->DrawText(buffer, (UINT)wcsnlen_s(buffer, MaxStringChars), rect, ImageRendererBrushRed, ImageRendererTextFormatFps);
		///Display hold time (Fall message)
		//rect = D2D1::RectF((FLOAT)clientRect.left, (FLOAT)clientRect.top + 140.0f, (FLOAT)clientRect.right, 140.0f);
		//swprintf_s(buffer, sizeof(buffer) / sizeof(WCHAR), L"3 seconds left");
		//swprintf_s(buffer, sizeof(buffer) / sizeof(WCHAR), m_pInbedAPPs->getFallStatus()?L"Fall!!":L"");
       // m_pImageRenderer->DrawText(buffer, (UINT)wcsnlen_s(buffer, MaxStringChars), rect, ImageRendererBrushWhiteSomke, ImageRendererTextFormatFps);
	}
}

void NuiStreamViewer::DrawHandRaisMeg(const RECT& clientRect)
{
	if(m_pHandRaisExcer->isRunningExcer())
	{
        WCHAR buffer[MaxStringChars];
		///Display session and set number
		D2D1_RECT_F rect = D2D1::RectF((FLOAT)clientRect.left, (FLOAT)clientRect.top + 20.0f, (FLOAT)clientRect.right, 50.0f);
		swprintf_s(buffer, sizeof(buffer) / sizeof(WCHAR), L"Session 1 & Set 1");
        m_pImageRenderer->DrawText(buffer, (UINT)wcsnlen_s(buffer, MaxStringChars), rect, ImageRendererBrushHotPink, ImageRendererTextFormatFps);
       ///Display counts and scores
		rect = D2D1::RectF((FLOAT)clientRect.left, (FLOAT)clientRect.top + 80.0f, (FLOAT)clientRect.right, 80.0f);
		swprintf_s(buffer, sizeof(buffer) / sizeof(WCHAR), L"LCount: %d  RCount: %d L/R Scores: %3.1f %3.1f", m_pHandRaisExcer->GetLeftCount(), m_pHandRaisExcer->GetRightCount(),
			m_pHandRaisExcer->GetLeftScore(), m_pHandRaisExcer->GetRightScore());
        m_pImageRenderer->DrawText(buffer, (UINT)wcsnlen_s(buffer, MaxStringChars), rect, ImageRendererBrushRed, ImageRendererTextFormatFps);
	}
}


void NuiStreamViewer::DrawDetectionMeg(const RECT& clientRect)
{
	D2D1_RECT_F rect ;
	WCHAR buffer[MaxStringChars];

	if (m_pInbedAPPs->getIsRunFallDetect())
	{
		///Display fall detection message
		rect = D2D1::RectF((FLOAT)clientRect.left, (FLOAT)clientRect.top + 20.0f, (FLOAT)clientRect.right, 50.0f);
		swprintf_s(buffer, sizeof(buffer) / sizeof(WCHAR), m_pInbedAPPs->getFallStatus()?L"               Fall!!":L"");
        m_pImageRenderer->DrawText(buffer, (UINT)wcsnlen_s(buffer, MaxStringChars), rect, ImageRendererBrushRed, ImageRendererTextFormatFps);
	}

	if (m_pInbedAPPs->getIsRunMovementDetect())
	{
		///Display movement detection message
		rect = D2D1::RectF((FLOAT)clientRect.left, (FLOAT)clientRect.top + 20.0f, (FLOAT)clientRect.right, 50.0f);
		swprintf_s(buffer, sizeof(buffer) / sizeof(WCHAR), m_pInbedAPPs->getMovementStatus()?L"Move!!":L"");
        m_pImageRenderer->DrawText(buffer, (UINT)wcsnlen_s(buffer, MaxStringChars), rect, ImageRendererBrushRed, ImageRendererTextFormatFps);
	}
}

void NuiStreamViewer::DrawHandsMovementRICMeg(const D2D1_RECT_F& imageRect, const RECT& clientRect)
{	  
	if (m_pInbedAPPs->getIsRunHandsMovementRIC())
	{
		FLOAT resultX, resultY;
        resultX = CircleCenterX * (imageRect.right  - imageRect.left + 1.0f) / 640 + imageRect.left;
        resultY = CircleCenterY * (imageRect.bottom - imageRect.top  + 1.0f) / 480 + imageRect.top;
	    D2D1_POINT_2F CenterTarget = {resultX, resultY};

		//Draw central target on the screen 
		m_ImageRendererBrush = ImageRendererBrushRed;
		m_pImageRenderer->DrawCircle(CenterTarget, 10.0f, m_ImageRendererBrush, 10.0f);

		//Refer/Initialize the cetral target
		D2D1_POINT_2F LeftHandPoint = ToImageRect(m_pInbedAPPs->getLeftHandPos(), imageRect);
		D2D1_POINT_2F RightHandPoint = ToImageRect(m_pInbedAPPs->getRightHandPos(), imageRect);
		FLOAT LeftDist = sqrt( pow(LeftHandPoint.x - CenterTarget.x, 2) + pow(LeftHandPoint.y - CenterTarget.y, 2)); 
		FLOAT RightDist = sqrt( pow(RightHandPoint.x - CenterTarget.x, 2) + pow(RightHandPoint.y - CenterTarget.y, 2)); 

		if (LeftDist < TargetDistThr || RightDist < TargetDistThr) 
		{
			m_ImageRendererBrush = ImageRendererBrushGreen;
			m_pImageRenderer->DrawCircle(CenterTarget, 10.0f, m_ImageRendererBrush, 10.0f);

			wsprintf(m_pStrToSpeak, L"%s", L"Start command.");
			m_isHandReinitial = TRUE;
			m_isHitTarget = TRUE;

			if (LeftDist < TargetDistThr) 
			{
				m_pInbedAPPs->setReferHandPos(m_pInbedAPPs->getLeftHandPos());
				m_pInbedAPPs->setLeftHandRaise(TRUE);
				if (m_pInbedAPPs->getRightHandRaise()) m_pInbedAPPs->setRightHandRaise(FALSE);

			}
			if (RightDist < TargetDistThr) 
			{
				m_pInbedAPPs->setReferHandPos(m_pInbedAPPs->getRightHandPos());
				m_pInbedAPPs->setRightHandRaise(TRUE);
				if (m_pInbedAPPs->getLeftHandRaise()) m_pInbedAPPs->setLeftHandRaise(FALSE);
			}
			
		}

		
		///Display hand raising evaluation messages
		WCHAR buffer[MaxStringChars];
		D2D1_RECT_F rect = D2D1::RectF((FLOAT)clientRect.left, (FLOAT)clientRect.top + 20.0f, (FLOAT)clientRect.right, 50.0f);

		wchar_t timeString[MAX_PATH];
	    wchar_t dateString[MAX_PATH];
        GetTimeFormatEx(NULL, TIME_FORCE24HOURFORMAT, NULL, L"hh':'mm", timeString, _countof(timeString));
	    GetDateFormatEx(NULL, 0, NULL, L"MMM'-'dd'-'yyyy'", dateString, _countof(dateString), NULL);

		static int MessageDisplayTimerRU = 0;
		static int MessageDisplayTimerRD = 0;
		static int MessageDisplayTimerRR1 = 0;
		static int MessageDisplayTimerRL1 = 0;
		static int MessageDisplayTimerRR2 = 0;
		static int MessageDisplayTimerRL2 = 0;
		static int MessageDisplayTimerRR3 = 0;
		static int MessageDisplayTimerRL3 = 0;

		switch (m_pInbedAPPs->getHandRaiseEvaluate())
		{
			case RaiseUp:
				if (m_isHandReinitial || MessageDisplayTimerRU > 0)
				{
					if (m_isHandReinitial) {
						MessageDisplayTimerRU = 60;
						MessageDisplayTimerRD = 0;
						MessageDisplayTimerRR1 = 0;
						MessageDisplayTimerRL1 = 0;
						MessageDisplayTimerRR2 = 0;
						MessageDisplayTimerRL2 = 0;
						MessageDisplayTimerRR3 = 0;
						MessageDisplayTimerRL3 = 0;
					}
					swprintf_s(buffer, sizeof(buffer) / sizeof(WCHAR), L"Nurse is coming. Please remain in bed!");
                    m_pImageRenderer->DrawText(buffer, (UINT)wcsnlen_s(buffer, MaxStringChars), rect, ImageRendererBrushRed, ImageRendererTextFormatFps);
					MessageDisplayTimerRU--; 
				}
				wsprintf(m_pStrToSpeak, L"%s", L"Nurse is coming. Please remain in bed.");
				m_isHitTarget = TRUE;
				break;
			case RaiseDown:
				if (m_isHandReinitial || MessageDisplayTimerRD > 0)
				{
					if (m_isHandReinitial) {
						MessageDisplayTimerRD = 60;
						MessageDisplayTimerRU = 0;
						MessageDisplayTimerRR1 = 0;
						MessageDisplayTimerRL1 = 0;
						MessageDisplayTimerRR2 = 0;
						MessageDisplayTimerRL2 = 0;
						MessageDisplayTimerRR3 = 0;
						MessageDisplayTimerRL3 = 0;
					}
				    swprintf_s(buffer, sizeof(buffer) / sizeof(WCHAR), L"%s at %s", dateString, timeString);
                    m_pImageRenderer->DrawText(buffer, (UINT)wcsnlen_s(buffer, MaxStringChars), rect, ImageRendererBrushRed, ImageRendererTextFormatFps);
					MessageDisplayTimerRD--; 
				}
				wsprintf(m_pStrToSpeak, L"%s at %s", dateString, timeString);
				m_isHitTarget = TRUE;
				break;
			case RaiseRight1:
				if (m_isHandReinitial || MessageDisplayTimerRR1 > 0)
				{
					if (m_isHandReinitial) {
						MessageDisplayTimerRR1 = 30;
						MessageDisplayTimerRL1 = 0;
						MessageDisplayTimerRR2 = 0;
						MessageDisplayTimerRL2 = 0;
						MessageDisplayTimerRR3 = 0;
						MessageDisplayTimerRL3 = 0;
						MessageDisplayTimerRD = 0;
						MessageDisplayTimerRU = 0;
					}
					swprintf_s(buffer, sizeof(buffer) / sizeof(WCHAR), L"Right good!");
                    m_pImageRenderer->DrawText(buffer, (UINT)wcsnlen_s(buffer, MaxStringChars), rect, ImageRendererBrushRed, ImageRendererTextFormatFps);
					MessageDisplayTimerRR1--; 
				}
				wsprintf(m_pStrToSpeak, L"%s", L"Right good");
				m_isHitTarget = TRUE;
				break;
			case RaiseRight2:
				if (m_isHandReinitial || MessageDisplayTimerRR2 > 0)
				{
					if (m_isHandReinitial) {
						MessageDisplayTimerRR2 = 30;
						MessageDisplayTimerRL1 = 0;
						MessageDisplayTimerRR1 = 0;
						MessageDisplayTimerRL2 = 0;
						MessageDisplayTimerRR3 = 0;
						MessageDisplayTimerRL3 = 0;
						MessageDisplayTimerRD = 0;
						MessageDisplayTimerRU = 0;
					}
					swprintf_s(buffer, sizeof(buffer) / sizeof(WCHAR), L"Right very good!");
                    m_pImageRenderer->DrawText(buffer, (UINT)wcsnlen_s(buffer, MaxStringChars), rect, ImageRendererBrushRed, ImageRendererTextFormatFps);
					MessageDisplayTimerRR2--; 
				}
				wsprintf(m_pStrToSpeak, L"%s", L"Right very good");
				m_isHitTarget = TRUE;
				break;
			case RaiseRight3:
				if (m_isHandReinitial || MessageDisplayTimerRR3 > 0)
				{
					if (m_isHandReinitial) {
						MessageDisplayTimerRR3 = 30;
						MessageDisplayTimerRL1 = 0;
						MessageDisplayTimerRR1 = 0;
						MessageDisplayTimerRL2 = 0;
						MessageDisplayTimerRR2 = 0;
						MessageDisplayTimerRL3 = 0;
						MessageDisplayTimerRD = 0;
						MessageDisplayTimerRU = 0;
					}
					swprintf_s(buffer, sizeof(buffer) / sizeof(WCHAR), L"Right excellent!");
                    m_pImageRenderer->DrawText(buffer, (UINT)wcsnlen_s(buffer, MaxStringChars), rect, ImageRendererBrushRed, ImageRendererTextFormatFps);
					MessageDisplayTimerRR3--; 
				}
				wsprintf(m_pStrToSpeak, L"%s", L"Right excellent");
				m_isHitTarget = TRUE;
				break;
			case RaiseLeft1:
				if (m_isHandReinitial || MessageDisplayTimerRL1 > 0)
				{
					if (m_isHandReinitial) {
						MessageDisplayTimerRL1 = 30;
						MessageDisplayTimerRR1 = 0;
						MessageDisplayTimerRR2 = 0;
						MessageDisplayTimerRL2 = 0;
						MessageDisplayTimerRR3 = 0;
						MessageDisplayTimerRL3 = 0;
						MessageDisplayTimerRD = 0;
						MessageDisplayTimerRU = 0;
					}
					swprintf_s(buffer, sizeof(buffer) / sizeof(WCHAR), L"Left good!");
                    m_pImageRenderer->DrawText(buffer, (UINT)wcsnlen_s(buffer, MaxStringChars), rect, ImageRendererBrushRed, ImageRendererTextFormatFps);
					MessageDisplayTimerRL1--; 
				}
				wsprintf(m_pStrToSpeak, L"%s", L"Left good");
				m_isHitTarget = TRUE;
				break;
			case RaiseLeft2:
				if (m_isHandReinitial || MessageDisplayTimerRL2 > 0)
				{
					if (m_isHandReinitial) {
						MessageDisplayTimerRL2 = 30;
						MessageDisplayTimerRR1 = 0;
						MessageDisplayTimerRL1 = 0;
						MessageDisplayTimerRR2 = 0;
						MessageDisplayTimerRR3 = 0;
						MessageDisplayTimerRL3 = 0;
						MessageDisplayTimerRD = 0;
						MessageDisplayTimerRU = 0;
					}
					swprintf_s(buffer, sizeof(buffer) / sizeof(WCHAR), L"Left very good!");
                    m_pImageRenderer->DrawText(buffer, (UINT)wcsnlen_s(buffer, MaxStringChars), rect, ImageRendererBrushRed, ImageRendererTextFormatFps);
					MessageDisplayTimerRL2--; 
				}
				wsprintf(m_pStrToSpeak, L"%s", L"Left very good");
				m_isHitTarget = TRUE;
				break;
			case RaiseLeft3:
				if (m_isHandReinitial || MessageDisplayTimerRL3 > 0)
				{
					if (m_isHandReinitial) {
						MessageDisplayTimerRL3 = 30;
						MessageDisplayTimerRR1 = 0;
						MessageDisplayTimerRL1 = 0;
						MessageDisplayTimerRR2 = 0;
						MessageDisplayTimerRL2 = 0;
						MessageDisplayTimerRR3 = 0;
						MessageDisplayTimerRD = 0;
						MessageDisplayTimerRU = 0;
					}
					swprintf_s(buffer, sizeof(buffer) / sizeof(WCHAR), L"Left excellent!");
                    m_pImageRenderer->DrawText(buffer, (UINT)wcsnlen_s(buffer, MaxStringChars), rect, ImageRendererBrushRed, ImageRendererTextFormatFps);
					MessageDisplayTimerRL3--; 
				}
				wsprintf(m_pStrToSpeak, L"%s", L"Left excellent");
				m_isHitTarget = TRUE;
				break;
			default:
				m_isHitTarget = FALSE;
				break;
		}

	}

}

DWORD WINAPI NuiStreamViewer::Txt2SpeechStaticThread(PVOID lpParam)
{
	NuiStreamViewer* context = static_cast<NuiStreamViewer*>(lpParam);
	if (context)
	{
		return context->Txt2SpeechThread();
	}
	return 0;
}

DWORD WINAPI NuiStreamViewer::Txt2SpeechThread()
{
	//if (m_isHandReinitial)
	//{
		//m_isHandReinitial = FALSE;
		
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
				if (m_isHitTarget)
				{
					if (m_isHandReinitial)
					{
					   m_isHandReinitial = FALSE;
					   Sleep(100);   //Refresh m_pStrToSpeak		   
					   cpVoice->Speak(m_pStrToSpeak, SPF_DEFAULT, NULL);		   
					}
				}
			    
			}
		}

		::CoUninitialize();

		if (FAILED(hr)) return FALSE;

	//}

	return TRUE;
}