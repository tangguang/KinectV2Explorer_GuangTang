//------------------------------------------------------------------------------
// <copyright file="FaceRecog.h" written by Yun Li, 10/30/2013>
//     All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#pragma once

#include "NuiColorStream.h"
#include "opencv2/core/core.hpp"
#include "opencv2/contrib/contrib.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/objdetect/objdetect.hpp"


#include <iostream>
#include <fstream>
#include <sstream>

using namespace cv;
using namespace std;


class FaceRecog
{
public:
	FaceRecog(NuiColorStream* pColorStream);
	DWORD WINAPI FaceRecogThread();
	void processFrame();
	~FaceRecog(){m_isRunFaceRecog=false;}

private:
	void Read_CSV(const string filename, vector<Mat>& images, vector<int>& labels, vector<string>& names, 
		char separator = ';');
	void getUniqStrs(const vector<string>& names, vector<string>& uniq_names);

private:
	NuiColorStream*       m_pColorStream;
	Mat                   m_capImage;
	BOOL                  m_isRunFaceRecog;
	CascadeClassifier     m_haar_cascade;
	Ptr<FaceRecognizer>   m_pmodel;
	vector<string>        m_uniq_names;
	INT                   m_imwidth;
	INT                   m_imheight;
};