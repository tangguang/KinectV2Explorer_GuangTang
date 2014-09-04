//------------------------------------------------------------------------------
// <copyright file="DepthInbedAPPs.h" written by Yun Li, 10/30/2013>
//     All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

//opencv
#include <opencv2/highgui/highgui.hpp>
#include "Utility.h"

#include <NuiApi.h>

using namespace cv;

#pragma once

typedef enum _BedRotateStatus {
	RotateRight = 0,
	RotateLeft,
	InvalidStatus,
	NumOfStatus
} BedRotateStatus;

class DepthInbedAPPs
{
public:
	DepthInbedAPPs();
	~DepthInbedAPPs();
	void processFrame(Mat CapturedFrame, const BYTE* source, CvSize imageRes, const NUI_SKELETON_FRAME* pSkeletonFrame);
	FLOAT getLyAngle() const {return m_LyAngle;}
	BOOL getIsRunLyAngleDetect() const {return m_IsRunLyAngleDetect;}
	void setIsRunLyAngleDetect(BOOL status) {m_IsRunLyAngleDetect = status;}

private:
	void SobelEdgeDetector(Mat src, Mat& des);
	void processLines(vector<Vec4i> lines, size_t sizeLines, const BYTE* source, CvSize imageRes, FLOAT* FloorClipPlaneCoeffs);
	void getHeightFromLine(Vec4i line, const BYTE* source, CvSize imageRes, FLOAT* FloorClipPlaneCoeffs);
	void getBLA_BH_FromLine(Vec4i line, const BYTE* source, CvSize imageRes, FLOAT* FloorClipPlaneCoeffs);
	void SaveAngleTime();

private:
	UINT            m_DetectLinesIndex;
	UINT            m_DetectLinesIndex2;
	UINT            m_DetectLinesIndex3;
	UINT            m_DetectLinesIndex4;
	UINT            m_FrameCount;
	FLOAT*          m_pHeightBuffer;
	FLOAT*          m_pAngleBuffer;
	//FLOAT           m_NorVect[3];
	//FLOAT           m_HSLp1[3];
	FLOAT           m_HLDist;
	Mat             m_CapturedFrame;
	IplImage*       m_pdepthImage;
	CvVideoWriter*  m_pWriter;
	FILE*           m_pFile;
	FLOAT           m_AvgHeight;
	FLOAT           m_LyAngle;
	FLOAT           m_BedHeight;
	FLOAT           m_SideBoundPlanes[5];   //[A,B,C,D1,D2], normal vector A, B, C and D1 / D2 (two parallel planes)
	//FLOAT           m_HSLp1Updated[3];
	BOOL            m_IsRunLyAngleDetect;
};