//------------------------------------------------------------------------------
// <copyright file="DepthInbedAPPs.cpp" written by Yun Li, 12/20/2013>
//     All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#include "DepthInbedAPPs.h"
#include "NuiAudioStream.h"
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/background_segm.hpp>
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <sstream>
#include <math.h>


using namespace cv;
using namespace std;

static const int Thr = 64; //Threshold for edge detection
static const size_t Maxlines = 30; //Maximu number of detected lines
static const int dilation_size = 2;
static const int y_delta = 5;
static const int x_delta = 10;
//static const FLOAT BedHeight = 0.7f;
static const FLOAT BedBackLength = 0.6f;

static UINT BufferLength = 20;
static UINT SlidingLength = 10;  //0.5 overlapping rate
static UINT NormalVectUpdate = 700;
static UINT HSLPoint1Update = 100;
static UINT ModelUpdateInterval = 40;

//Mat fgMaskMOG2, 
Mat fgMaskMOG2, dilate_dst, grad, cdst;
Mat avgFrame = Mat::zeros(480,640,0);
Mat dst = Mat::zeros(480,640,0);
//Ptr<BackgroundSubtractor> pMOG2 = new BackgroundSubtractorMOG2();
Mat element = getStructuringElement(MORPH_RECT,
                                    Size( 2*dilation_size + 1, 2*dilation_size+1 ),
                                    Point(dilation_size, dilation_size));
static FLOAT NorVect[3] = {0.0f, 0.0f, 0.0f};
static FLOAT HSLp1[3] = {0.0f, 0.0f, 0.0f};
static FLOAT HSLp1Updated[3] = {0.0f, 0.0f, 0.0f};

DepthInbedAPPs::DepthInbedAPPs():
m_DetectLinesIndex(0),
m_DetectLinesIndex2(0),
m_DetectLinesIndex3(0),
m_DetectLinesIndex4(0),
m_FrameCount(0),
m_HLDist(0.0f),
m_AvgHeight(0.0f),
m_LyAngle(0.0f),
m_BedHeight(0.0f),
m_IsRunLyAngleDetect(FALSE)
{
	//m_pWriter = cvCreateVideoWriter("C:\\Users\\yuner\\Desktop\\KinectGUI\\DataLog\\Room1\\KinectDepth\\SBMog2Inbed_test.avi",
	//CV_FOURCC('L', 'A', 'G', 'S'), 25, cvSize(640, 480));
	m_pdepthImage = cvCreateImage(cvSize(640, 480), 8, 3);
	//fopen_s(&m_pFile, "C:\\Users\\yuner\\Desktop\\KinectGUI\\DataLog\\Room1\\BedHeight_test.txt", "a+");

	m_pHeightBuffer = new FLOAT[BufferLength];
	memset(m_pHeightBuffer, 0, sizeof(FLOAT)*BufferLength);
	m_pAngleBuffer = new FLOAT[BufferLength/2]; 
	memset(m_pAngleBuffer, 0, sizeof(FLOAT)*BufferLength/2);
	//memset(m_NorVect, 0, sizeof(FLOAT)*3);
	//memset(m_HSLp1, 0, sizeof(FLOAT)*3);
	memset(m_SideBoundPlanes, 0, sizeof(FLOAT)*5);
};

DepthInbedAPPs::~DepthInbedAPPs()
{
	//if (m_pWriter) cvReleaseVideoWriter(&m_pWriter);
	//if (m_pFile) fclose(m_pFile);
	SafeDelete(m_pHeightBuffer);
	SafeDelete(m_pAngleBuffer);
}


void DepthInbedAPPs::processFrame(Mat CapturedFrame, const BYTE* source, CvSize imageRes, const NUI_SKELETON_FRAME* pSkeletonFrame)
{
	if (m_IsRunLyAngleDetect)
	{
		m_CapturedFrame = CapturedFrame.clone(); 
		// Background substraction
		//pMOG2->set("detectShadows",false);
		//pMOG2->operator()(m_CapturedFrame, fgMaskMOG2);

		//running average
	    addWeighted(m_CapturedFrame,0.3,avgFrame,0.7,0,dst);
	    avgFrame = dst.clone();

		//Edge detection
		//SobelEdgeDetector(fgMaskMOG2, grad);
		SobelEdgeDetector(dst, grad);

		//Dilated
		dilate(grad, dilate_dst, element);

		//Lines detect Hough transform
		cvtColor(dilate_dst, cdst, CV_GRAY2BGR);
		vector<Vec4i> lines;
		HoughLinesP(dilate_dst, lines, 1, CV_PI/180, 10, 150, 20);
		size_t sizeLines = lines.size();
		if (sizeLines > Maxlines) sizeLines = Maxlines;

		//// Get estimated floor coefficients
		FLOAT FloorClipPlaneCoeffs[4];
		FloorClipPlaneCoeffs[0] = pSkeletonFrame->vFloorClipPlane.x;
		FloorClipPlaneCoeffs[1] = pSkeletonFrame->vFloorClipPlane.y;
		FloorClipPlaneCoeffs[2] = pSkeletonFrame->vFloorClipPlane.z;
		FloorClipPlaneCoeffs[3] = pSkeletonFrame->vFloorClipPlane.w;

		processLines(lines, sizeLines, source, imageRes, FloorClipPlaneCoeffs);
	}
}

void DepthInbedAPPs::SobelEdgeDetector(Mat src, Mat& des)
{
  Mat grad, src_gray;
  int scale = 1;
  int delta = 0;
  int ddepth = CV_16S;

  GaussianBlur( src, src, Size(3,3), 0, 0, BORDER_DEFAULT );
	/// Convert it to gray
  //cvtColor( src, src_gray, CV_RGB2GRAY );
  src_gray = src;
  /// Generate grad_x and grad_y
  Mat grad_x, grad_y;
  Mat abs_grad_x, abs_grad_y;

  /// Gradient X
  //Scharr( src_gray, grad_x, ddepth, 1, 0, scale, delta, BORDER_DEFAULT );
  Sobel( src_gray, grad_x, ddepth, 1, 0, 3, scale, delta, BORDER_DEFAULT );
  convertScaleAbs( grad_x, abs_grad_x );

  /// Gradient Y
  //Scharr( src_gray, grad_y, ddepth, 0, 1, scale, delta, BORDER_DEFAULT );
  Sobel( src_gray, grad_y, ddepth, 0, 1, 3, scale, delta, BORDER_DEFAULT );
  convertScaleAbs( grad_y, abs_grad_y );

  /// Total Gradient (approximate)
  addWeighted( abs_grad_x, 0.5, abs_grad_y, 0.5, 0, grad );
 
  des = grad.clone() > Thr;
}

void DepthInbedAPPs::processLines(vector<Vec4i> lines, size_t sizeLines, const BYTE* source, CvSize imageRes, FLOAT* FloorClipPlaneCoeffs)
{
	BedRotateStatus RS = InvalidStatus;
	float Dist_HL = 0.0f;
	Point Q = Point(0, 0);

	//// Detection of HL
	for( size_t i = 0; i < sizeLines; i++ )
	{
		Vec4i l = lines[i];

		float Slop = (float)(l[1] - l[3])/(l[0] - l[2]);
		float Theta = (float)CV_PI/2 - atan(-Slop);
		float Dist = l[0]*cos(Theta) + l[1]*sin(Theta);
		Theta *= 180.0f/(float)CV_PI;

		if (Dist>30.0f && Dist<200.0f)
		{
			if (Theta>70 && Theta<90)
			{
				RS = RotateLeft;
				Dist_HL = Dist;
				Q = Point(l[2], l[3]);
				getHeightFromLine(l, source, imageRes, FloorClipPlaneCoeffs);
				line(cdst, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,0,255), 3, CV_AA);	            
			}
			if (Theta>=90 && Theta<110)
			{
				RS = RotateRight;
				Dist_HL = Dist;
				Q = Point(l[0], l[1]);
				getHeightFromLine(l, source, imageRes, FloorClipPlaneCoeffs);
				line(cdst, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,0,255), 3, CV_AA);
			}
		}
	}

	if (RS == InvalidStatus)
	{
		string box_text = format("No head line is detected!");
		putText(cdst, box_text, Point(30, 60), FONT_HERSHEY_PLAIN, 2.0, CV_RGB(0,0,255), 3);
	}
	//// Detection of HSL
	else
	{
		float DistOQ_square = (float) Q.x * Q.x + Q.y * Q.y;
		float Dist_HSL = sqrt(DistOQ_square - pow(Dist_HL,2));

		for( size_t i = 0; i < sizeLines; i++ )
		{
			Vec4i l = lines[i];

			float Slop = (float)(l[1] - l[3])/(l[0] - l[2]);
			float Theta = (float)CV_PI/2 - atan(-Slop);
			float Dist = l[0]*cos(Theta) + l[1]*sin(Theta);
			Theta *= 180.0f/(float)CV_PI;

			if (RS == RotateLeft)
			{
				Dist_HSL = -Dist_HSL;
				//// Compute distance between Q and Point(l[0],l[1])
				float QPdist_square = (float)(l[0] - Q.x) * (l[0] - Q.x) + (l[1] - Q.y) * (l[1] - Q.y);
				float QPdist = sqrt(QPdist_square);

				if ((Theta>120 && Theta<180) && (Dist>(Dist_HSL - 100) && Dist<(Dist_HSL + 30)) 
					&& (QPdist < 50))
				{
					line(cdst, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,255,0), 3, CV_AA);
					//circle(cdst, Point(l[2], l[3]), 10, Scalar(0,255,0), 5);

					getBLA_BH_FromLine(l,source,imageRes,FloorClipPlaneCoeffs);
 				}
			}
			if (RS == RotateRight)
			{
				//// Compute distance between Q and Point(l[0],l[1])
				float QPdist_square = (float)(l[0] - Q.x) * (l[0] - Q.x) + (l[1] - Q.y) * (l[1] - Q.y);
				float QPdist = sqrt(QPdist_square);

				if ((Theta>=0 && Theta<40) && (Dist>(Dist_HSL - 100) && Dist<(Dist_HSL + 30))
					&& (QPdist < 50))
				{
					line(cdst, Point(l[0], l[1]), Point(l[2], l[3]), Scalar(0,255,0), 3, CV_AA);
					//circle(cdst, Point(l[2], l[3]), 10, Scalar(0,255,0), 5);
					getBLA_BH_FromLine(l,source,imageRes,FloorClipPlaneCoeffs);
 				}
			}
		}
		string box_text = format("H: %2.2f A: %2.1f", m_BedHeight, m_LyAngle);
		string box_text1 = format("A %2.3f, B %2.3f, C %2.3f, D1 %2.3f, D2 %2.3f, HL %2.1f", m_SideBoundPlanes[0], m_SideBoundPlanes[1],
			m_SideBoundPlanes[2], m_SideBoundPlanes[3], m_SideBoundPlanes[4], m_HLDist);
		//string box_text = format("rows %d, cols %d, channels %d, type %d",
			//m_CapturedFrame.rows, m_CapturedFrame.cols, m_CapturedFrame.channels(), m_CapturedFrame.type());
		putText(cdst, box_text, Point(30, 60), FONT_HERSHEY_PLAIN, 2.0, CV_RGB(0,255,0), 3);
		putText(cdst, box_text1, Point(10, 90), FONT_HERSHEY_PLAIN, 1.0, CV_RGB(0,255,0), 2);
		//*m_pdepthImage = cdst;
		//cvWriteFrame(m_pWriter, m_pdepthImage);
		//namedWindow("Frame");
		namedWindow("Detected lines");
		//imshow("Frame", fgMaskMOG2);
		imshow("Detected lines",cdst);

		//Reinitialize the model
		//m_FrameCount %= ModelUpdateInterval;
		//if (m_FrameCount++ % ModelUpdateInterval == 0)
		//pMOG2 = new BackgroundSubtractorMOG2();
	}
}

void DepthInbedAPPs::getHeightFromLine(Vec4i line, const BYTE* source, CvSize imageRes, FLOAT* FloorClipPlaneCoeffs)
{
	NUI_DEPTH_IMAGE_PIXEL* pBufferRun = (NUI_DEPTH_IMAGE_PIXEL*)source;
	///Adjustment of the points for right projection
	Point Pos1(line[0] + (line[2] - line[0]) / 5, line[1] + (line[3] - line[1]) / 5);
	Point Pos2(line[2] - (line[2] - line[0]) / 5, line[3] - (line[3] - line[1]) / 5);
	Pos1.y += y_delta;
	Pos2.y += y_delta;
	Point MidPos((Pos1.x + Pos2.x)/2, (Pos1.y + Pos2.y)/2);

	//circle(cdst, MidPos, 10, Scalar(255,0,0), 5); 
	INT IncreMent= MidPos.y * imageRes.width + MidPos.x;

	//Get cloud point
	Vector4 CldPos = NuiTransformDepthImageToSkeleton(MidPos.x, MidPos.y, ((pBufferRun+IncreMent)->depth)<<3, NUI_IMAGE_RESOLUTION_640x480);
	Vector4 CldPos1 = NuiTransformDepthImageToSkeleton(Pos1.x, Pos1.y, ((pBufferRun+IncreMent)->depth)<<3, NUI_IMAGE_RESOLUTION_640x480);
	Vector4 CldPos2 = NuiTransformDepthImageToSkeleton(Pos2.x, Pos2.y, ((pBufferRun+IncreMent)->depth)<<3, NUI_IMAGE_RESOLUTION_640x480);

	wchar_t FilePath1[MAX_PATH];
	swprintf_s(FilePath1, MAX_PATH, L"%s\\KinectSkeletonPos\\HLPos.txt", knownPath);
	FILE* pFile1;
	_wfopen_s(&pFile1, FilePath1, L"a+");
	fwprintf(pFile1, L"%f %f %f %f %f %f\n", CldPos1.x, CldPos1.y, CldPos1.z, CldPos2.x, CldPos2.y, CldPos2.z);
	fclose(pFile1);

	//Normal vector updating
	if (m_DetectLinesIndex3++ < NormalVectUpdate)
	{
		NorVect[0] += ((CldPos1.x - CldPos2.x) / NormalVectUpdate);
		NorVect[1] += ((CldPos1.y - CldPos2.y) / NormalVectUpdate);
		NorVect[2] += ((CldPos1.z - CldPos2.z) / NormalVectUpdate);
	}
	else
	{
		//Normalization
		FLOAT NormScale = sqrt(NorVect[0] * NorVect[0] + NorVect[1] * NorVect[1] + NorVect[2] * NorVect[2]); 
		m_HLDist = 5.0f/3 * NormScale;
		m_SideBoundPlanes[0] = NorVect[0] / NormScale; 
		m_SideBoundPlanes[1] = NorVect[1] / NormScale; 
		m_SideBoundPlanes[2] = NorVect[2] / NormScale;
		m_SideBoundPlanes[3] = -(m_SideBoundPlanes[0] * HSLp1Updated[0] + m_SideBoundPlanes[1] * HSLp1Updated[1] + 
			m_SideBoundPlanes[2] * HSLp1Updated[2]);
		m_SideBoundPlanes[4] = m_SideBoundPlanes[3] - m_HLDist;
        //Initial normal vector
		NorVect[0] = 0.0f;
		NorVect[1] = 0.0f;
		NorVect[2] = 0.0f;
		m_DetectLinesIndex3 = 0;
	}
	

	//// Get estimated floor coefficients
	FLOAT A = FloorClipPlaneCoeffs[0];
	FLOAT B = FloorClipPlaneCoeffs[1];
	FLOAT C = FloorClipPlaneCoeffs[2];
	FLOAT D = FloorClipPlaneCoeffs[3];

	//// Get estimated height H
	FLOAT distNumer = A * CldPos.x + B * CldPos.y + C* CldPos.z + D ;
	FLOAT distDenomer = sqrtf (pow(A, 2) + pow(B, 2) + pow(C, 2)) ;
	FLOAT Height;
	if (distDenomer)
		Height = abs(distNumer/distDenomer);
	else 
		Height = m_AvgHeight;

	//Push height into buffer and compute average
	m_DetectLinesIndex %= BufferLength;
	m_pHeightBuffer[m_DetectLinesIndex++] = Height;

	if (m_DetectLinesIndex % SlidingLength == 0)
	{
		//Compute average
		FLOAT Sum = 0.0f;
		for (UINT k = 0; k < BufferLength; k++)
		{
			Sum += m_pHeightBuffer[k];
		}
		m_AvgHeight = Sum / BufferLength;
	}	
}

void DepthInbedAPPs::getBLA_BH_FromLine(Vec4i line, const BYTE* source, CvSize imageRes, FLOAT* FloorClipPlaneCoeffs)
{
	NUI_DEPTH_IMAGE_PIXEL* pBufferRun = (NUI_DEPTH_IMAGE_PIXEL*)source;

	Point Pos1(line[0] + (line[2] - line[0]) / 4, line[1] + (line[3] - line[1]) / 4 );
	Point Pos2(line[2] - (line[2] - line[0]) / 4, line[3] - (line[3] - line[1]) / 4);
	///Adjustment of the points for right projection
	Pos1.x -= x_delta;
	Pos2.x -= x_delta;

	//circle(cdst, MidPos, 10, Scalar(255,0,0), 5); 
	INT IncreMent1= Pos1.y * imageRes.width + Pos1.x;
	INT IncreMent2= Pos2.y * imageRes.width + Pos2.x;

	//Get cloud points
	Vector4 CldPos1 = NuiTransformDepthImageToSkeleton(Pos1.x, Pos1.y, ((pBufferRun+IncreMent1)->depth)<<3, NUI_IMAGE_RESOLUTION_640x480);
	Vector4 CldPos2 = NuiTransformDepthImageToSkeleton(Pos2.x, Pos2.y, ((pBufferRun+IncreMent2)->depth)<<3, NUI_IMAGE_RESOLUTION_640x480);

	wchar_t FilePath2[MAX_PATH], FilePath3[MAX_PATH];
	swprintf_s(FilePath2, MAX_PATH, L"%s\\KinectSkeletonPos\\HSLPos.txt", knownPath);
	swprintf_s(FilePath3, MAX_PATH, L"%s\\KinectSkeletonPos\\Floor.txt", knownPath);
	FILE* pFile2, *pFile3;
	_wfopen_s(&pFile2, FilePath2, L"a+");
	_wfopen_s(&pFile3, FilePath3, L"a+");
	fwprintf(pFile2, L"%f %f %f %f %f %f\n", CldPos1.x, CldPos1.y, CldPos1.z, CldPos2.x, CldPos2.y, CldPos2.z);
	fclose(pFile2);

	//// Get estimated floor coefficients
	FLOAT A = FloorClipPlaneCoeffs[0];
	FLOAT B = FloorClipPlaneCoeffs[1];
	FLOAT C = FloorClipPlaneCoeffs[2];
	FLOAT D = FloorClipPlaneCoeffs[3];

	fwprintf(pFile3, L"%f %f %f %f\n", A, B, C, D);
	fclose(pFile3);

	//// Averaging HSL end point 1
	if (m_DetectLinesIndex4++ < HSLPoint1Update)
	{
		HSLp1[0] += (CldPos1.x / HSLPoint1Update);
		HSLp1[1] += (CldPos1.y / HSLPoint1Update);
		HSLp1[2] += (CldPos1.z / HSLPoint1Update);

	}
	else
	{
		HSLp1Updated[0] = HSLp1[0];
		HSLp1Updated[1] = HSLp1[1];
		HSLp1Updated[2] = HSLp1[2];
	    HSLp1[0] = 0.0f;
		HSLp1[1] = 0.0f;
		HSLp1[2] = 0.0f;
		m_DetectLinesIndex4 = 0;
	}


	//// Get directional vector of the line s={m, n, p}
	float m = CldPos1.x - CldPos2.x; 
	float n = CldPos1.y - CldPos2.y; 
	float p = CldPos1.z - CldPos2.z; 

	//// Get BLA (line w.s.p the plane)
	FLOAT LyAngle = asin(abs(A*m + B*n + C*p)/(sqrt(m*m + n*n + p*p) * sqrt(A*A + B*B + C*C))) * 180 / (FLOAT)CV_PI;
	//LyAngle = 3 * LyAngle + 5.0f;
	//Push height into buffer and compute average
	m_DetectLinesIndex2 %= (BufferLength/2);
	m_pAngleBuffer[m_DetectLinesIndex2++] = LyAngle;
	if (m_DetectLinesIndex2 % (SlidingLength/2) == 0)
	{   //Compute average
		FLOAT Sum = 0.0f;
		for (UINT k = 0; k < BufferLength/2; k++)
		{
			Sum += m_pAngleBuffer[k];
		}
		m_LyAngle = Sum / (BufferLength/2);
	}

	//// Get BH
	m_BedHeight = m_AvgHeight - BedBackLength * sin(m_LyAngle * (FLOAT)CV_PI / 180);
}


void DepthInbedAPPs::SaveAngleTime()
{
	wchar_t FilePath[MAX_PATH];
	swprintf_s(FilePath, MAX_PATH, L"%s\\KinectSkeletonPos\\LyAngleRecords\\LyAngleRec.txt", knownPath);
	FILE* pFile;
	_wfopen_s(&pFile, FilePath, L"a+");

	// Get the time
    wchar_t timeString[MAX_PATH];
	wchar_t dateString[MAX_PATH];
    GetTimeFormatEx(NULL, TIME_FORCE24HOURFORMAT, NULL, L"hh':'mm':'ss", timeString, _countof(timeString));
	GetDateFormatEx(NULL, 0, NULL, L"yyyy'-'MMM'-'dd", dateString, _countof(dateString), NULL);
	
	//print the time
	fwprintf(pFile, L"%f %s %s\n", m_LyAngle, dateString, timeString);
	fclose(pFile);
}
