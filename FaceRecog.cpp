//------------------------------------------------------------------------------
// <copyright file="FaceRecog.cpp" written by Yun Li, 10/30/2013>
//     All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "FaceRecog.h"

const string CSVFileName = "C:\\Users\\yuner\\Desktop\\Work\\FaceRecognition\\at.txt";
const string HaarCascadeFileName = "C:\\opencv\\data\\haarcascades\\haarcascade_frontalface_default.xml";


FaceRecog::FaceRecog(NuiColorStream* pColorStream):
	m_pColorStream(pColorStream),
    m_isRunFaceRecog(TRUE),
    m_imwidth(0),
    m_imheight(0)
	{}

DWORD WINAPI FaceRecog::FaceRecogThread()
{
	vector<Mat> images;
    vector<int> labels;
	vector<string> names;

	// Read in the data (fails if no valid input filename is given, but you'll get an error message):
    try {
        Read_CSV(CSVFileName, images, labels, names);
    } catch (cv::Exception& e) {
        cerr << "Error opening file \"" << CSVFileName << "\". Reason: " << e.msg << endl;
        // nothing more we can do
        exit(1);
    }

	// Get unique names

	getUniqStrs(names, m_uniq_names);
	
	m_imwidth = images[0].cols;
	m_imheight = images[0].rows;

	m_pmodel = createFisherFaceRecognizer();
    m_pmodel->train(images, labels);
    //m_pmodel->load("C:\\Users\\yuner\\Desktop\\Fisher_at.yml");

	m_haar_cascade.load(HaarCascadeFileName);

	//Capture a frame and process it in a loop
	while (1)
	{
		processFrame();
		Sleep(16);
	}

	return 0;
}


void FaceRecog::Read_CSV(const string filename, vector<Mat>& images, vector<int>& labels, vector<string>& names, 
		char separator)
{
	std::ifstream file(filename.c_str(), ifstream::in);
    if (!file) {
        string error_message = "No valid input file was given, please check the given filename.";
        CV_Error(CV_StsBadArg, error_message);
    }
    string line, path, classlabel, name;
    while (getline(file, line)) {
        stringstream liness(line);
        getline(liness, path, separator);
        getline(liness, classlabel, separator);
		getline(liness, name);
        if(!path.empty() && !classlabel.empty()) {
            images.push_back(imread(path, 0));
            labels.push_back(atoi(classlabel.c_str()));
			names.push_back(name);
        }
    }

}

void FaceRecog::getUniqStrs(const vector<string>& names, vector<string>& uniq_names)
{
	string lastUniqStr;
	for (int k=0; k!=names.size(); k++)
	{
		if (lastUniqStr.compare(names[k])!=0)
		{
			lastUniqStr=names[k];
			uniq_names.push_back(lastUniqStr);
		}
	}
}

void FaceRecog::processFrame()
{
	Mat tempIm  =Mat(m_pColorStream->GetCvColorImage(),true);
	m_capImage = tempIm.clone();
	Mat gray;
    cvtColor(m_capImage, gray, CV_BGR2GRAY);

	vector< Rect_<int> > faces;
    m_haar_cascade.detectMultiScale(gray, faces);

	for(size_t i = 0; i < faces.size(); i++) {
		Rect face_i = faces[i];
		Mat face = gray(face_i);
		Mat face_resized;
        resize(face, face_resized, Size(m_imwidth, m_imheight), 1.0, 1.0, INTER_CUBIC);

		int prediction = m_pmodel->predict(face_resized);
		//rectangle(*m_pcapImage, face_i, CV_RGB(0, 255,0), 1);
		string predicted_name=m_uniq_names[prediction];
		//string box_text = format("Prediction = %d, Name = %s", prediction, predicted_name.c_str());
		int pos_x = max(face_i.tl().x - 10, 0);
        int pos_y = max(face_i.tl().y - 10, 0);
	}

}
