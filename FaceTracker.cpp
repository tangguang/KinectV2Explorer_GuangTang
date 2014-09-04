//------------------------------------------------------------------------------
// <copyright file="FaceTracker.cpp" written by Yun Li, 9/27/2013>
//     All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#include "stdafx.h"
#include <vector>
#include "FaceTracker.h"
#include "Utility.h"
#include <strsafe.h>
#include "NuiAudioStream.h"


const UINT TimersPerAUSUFile = 5000;


FaceTracker::FaceTracker(NuiColorStream* pColorStream, NuiDepthStream* pDepthStream, NuiSkeletonStream* pSkeletonStream, PCWSTR instanceName)
	: m_pColorStream(pColorStream)
	, m_pDepthStream(pDepthStream)
	, m_pSkeletonStream(pSkeletonStream)
	, m_instanceName((WCHAR*)instanceName)
	, m_pFaceTracker(nullptr)
	, m_pFTResult(nullptr)
	, m_pFTcolorImage(nullptr)
	, m_pFTdepthImage(nullptr)
	, m_ApplicationIsRunning(true)
	, m_ZoomFactor(1.0f)
	, m_LastTrackSucceeded(false)
	, m_3DModelSave(false)
	, m_AUSUcounts(0)
	, m_pAUSUfile(nullptr)
{
	m_ColorCameraConfig.Width=640;
	m_ColorCameraConfig.Height=480;
	m_ColorCameraConfig.FocalLength=NUI_CAMERA_COLOR_NOMINAL_FOCAL_LENGTH_IN_PIXELS;
	m_DepthCameraConfig.Width=320;
	m_DepthCameraConfig.Height=240;
	m_DepthCameraConfig.FocalLength=NUI_CAMERA_DEPTH_NOMINAL_FOCAL_LENGTH_IN_PIXELS;

	m_ViewOffSet.x=0;
	m_ViewOffSet.y=0;

	m_hint3D[0]=m_hint3D[1]=FT_VECTOR3D(0, 0, 0);
}

FaceTracker::~FaceTracker()
{
	m_ApplicationIsRunning=false;
	if (m_pAUSUfile) fclose(m_pAUSUfile);
}

DWORD WINAPI FaceTracker::FaceTrackingThread()
{
	///Image buffer initialize
	m_pColorStream->FTcolorInit(m_ColorCameraConfig);
	m_pDepthStream->FTdepthInit(m_DepthCameraConfig);

	m_pFaceTracker = FTCreateFaceTracker(NULL);
    if (!m_pFaceTracker) return E_OUTOFMEMORY;

    HRESULT hr = m_pFaceTracker->Initialize(&m_ColorCameraConfig, &m_DepthCameraConfig, NULL, NULL);
    if (FAILED(hr)) return hr;

    hr = m_pFaceTracker->CreateFTResult(&m_pFTResult);
	if (FAILED(hr)||!m_pFTResult) return hr;

	m_pFTcolorImage=FTCreateImage();
	if (!m_pFTcolorImage||FAILED(hr = m_pFTcolorImage->Allocate(m_ColorCameraConfig.Width, m_ColorCameraConfig.Height, FTIMAGEFORMAT_UINT8_B8G8R8X8))) return E_OUTOFMEMORY;

	m_pFTdepthImage=FTCreateImage();
	if (!m_pFTdepthImage||FAILED(hr = m_pFTdepthImage->Allocate(m_DepthCameraConfig.Width, m_DepthCameraConfig.Height, FTIMAGEFORMAT_UINT16_D13P3))) return E_OUTOFMEMORY;
	
	m_LastTrackSucceeded=false;

	//CreateMeshViewer();

	while (m_ApplicationIsRunning)
    {
        CheckCameraInput();
        Sleep(16);
    }

	m_pFaceTracker->Release();
    m_pFaceTracker = NULL;

    if(m_pFTcolorImage)
    {
        m_pFTcolorImage->Release();
        m_pFTcolorImage = NULL;
    }

    if(m_pFTdepthImage) 
    {
        m_pFTdepthImage->Release();
        m_pFTdepthImage = NULL;
    }

    if(m_pFTResult)
    {
        m_pFTResult->Release();
        m_pFTResult = NULL;
    }
	return 0;
}

void FaceTracker::CheckCameraInput()
{
	HRESULT hrFT = E_FAIL;

    if (m_pColorStream->GetColorBuffer())
    {
        HRESULT hrCopy = m_pColorStream->GetColorBuffer()->CopyTo(m_pFTcolorImage, NULL, 0, 0);
        if (SUCCEEDED(hrCopy) && m_pDepthStream->GetDepthBuffer())
        {
            hrCopy = m_pDepthStream->GetDepthBuffer()->CopyTo(m_pFTdepthImage, NULL, 0, 0);
        }
        // Do face tracking
        if (SUCCEEDED(hrCopy))
        {
            FT_SENSOR_DATA sensorData(m_pFTcolorImage, m_pFTdepthImage, GetZoomFactor(), &GetViewOffSet());

            FT_VECTOR3D* hint = NULL;

			HRESULT hr=m_pSkeletonStream->GetClosestHint(m_hint3D);
			if (SUCCEEDED(hr))
            {
                hint = m_hint3D;
            }
            if (m_LastTrackSucceeded)
            {
                hrFT = m_pFaceTracker->ContinueTracking(&sensorData, hint, m_pFTResult);
            }
            else
            {
                hrFT = m_pFaceTracker->StartTracking(&sensorData, NULL, hint, m_pFTResult);
            }
        }
    }
	HRESULT hrSt=m_pFTResult->GetStatus();


	m_LastTrackSucceeded = SUCCEEDED(hrFT) && SUCCEEDED(hrSt);

	if (m_LastTrackSucceeded)
	{
		SubmitFraceTrackingResult(m_pFTResult);
	}
	else
	{
		m_pFTResult->Reset();
	}
}

bool FaceTracker::SubmitFraceTrackingResult(IFTResult* pResult)
{
	HRESULT hrSt=pResult->GetStatus();
	if (pResult != NULL && SUCCEEDED(hrSt))
    {
		bool DrawMask=true;
        if (DrawMask)
        {
            FLOAT* pSU = NULL;
            UINT numSU;
            BOOL suConverged;
            m_pFaceTracker->GetShapeUnits(NULL, &pSU, &numSU, &suConverged);
            IFTModel* ftModel;
            HRESULT hr = m_pFaceTracker->GetFaceModel(&ftModel);
            if (SUCCEEDED(hr))
            {
				 UINT vertexCount =ftModel->GetVertexCount();
				 FT_VECTOR3D* pPts3D = reinterpret_cast<FT_VECTOR3D*>(_malloca(sizeof(FT_VECTOR3D) * vertexCount));
				 if (pPts3D)
                 {
					 FLOAT *pAUs;
					 UINT auCount;
					 hr = pResult->GetAUCoefficients(&pAUs, &auCount);
					 if (SUCCEEDED(hr))
					 {
						 FLOAT scale, rotationXYZ[3], translationXYZ[3];
						 hr = pResult->Get3DPose(&scale, rotationXYZ, translationXYZ);
						 if (SUCCEEDED(hr))
					     {

                        ///For visualization mesh model, currently not work
                        // hr = VisualizeFaceModel(m_pFTcolorImage, ftModel, &m_ColorCameraConfig, pSU, GetZoomFactor(), GetViewOffSet(), pResult, 0x00FFFF00);
           
				         ///Save 3D model in obj file
						     Save3DModel(ftModel, pSU, numSU, pAUs, auCount, scale, rotationXYZ, translationXYZ);
						 }
						 
					 }
				 }
                ftModel->Release();
            }
        }
    }
    return TRUE;
}


HRESULT FaceTracker::Save3DModel(IFTModel* model, 
                float* pSUs, UINT suCount,
                float* pAUs, UINT auCount,
                float scale, float rotationXYZ[3], float translationXYZ[3])
{
	HRESULT hr;
	UINT vertexCount = model->GetVertexCount();
    FT_VECTOR3D* pVertices = new FT_VECTOR3D[vertexCount];
    model->Get3DShape(pSUs, suCount, pAUs, auCount, scale, rotationXYZ, translationXYZ, pVertices, vertexCount);

    UINT triangleCount = 0;
    FT_TRIANGLE* pTriangles = NULL;
    model->GetTriangles(&pTriangles, &triangleCount);

	if (m_3DModelSave)
	{
		WCHAR szFilename[MAX_PATH] = { 0 };
		WCHAR szTimeString[MAX_PATH] = { 0 };
		if ((m_AUSUcounts++)%TimersPerAUSUFile==0)
		{
			m_AUSUcounts%=TimersPerAUSUFile;
			if (SUCCEEDED(hr=Get3DModelFileName(szFilename,szTimeString,_countof(szFilename), _countof(szTimeString), m_instanceName, 2)))
				_wfopen_s(&m_pAUSUfile,szFilename,L"a+");
			else return hr;
		}

		if (SUCCEEDED(hr=Get3DModelFileName(szFilename,szTimeString,_countof(szFilename), _countof(szTimeString), m_instanceName, 1)))
		{
			////AUs/SUs
			//fwprintf_s(m_pAUSUfile,L"%s\r\nSUs ",szTimeString);
			//for (UINT k=0; k<suCount; k++)
			//{
			//	fprintf(m_pAUSUfile,"%f ", pSUs[k]);
			//}
			//fwprintf_s(m_pAUSUfile,L"\r\nAUs ");
			fwprintf_s(m_pAUSUfile,L"%s AUs ",szTimeString);
			for (UINT k=0; k<auCount; k++)
			{
				fprintf(m_pAUSUfile, "%f ", pAUs[k]);
			}
			fwprintf_s(m_pAUSUfile,L"\r\n");

			////Initial vertices vector for displaying
			//vector<glm::vec3> temp_vertices;
			//glm::vec3 temp_vertice;
			//vector<INT> temp_indices;
			//vector<glm::vec3> out_vertices;

			//////Facial model in obj files
			//FILE* pfile;
			//_wfopen_s(&pfile,szFilename,L"w");
			//fprintf_s(pfile, "# %u vertices, # %u faces\n", vertexCount, triangleCount);
			//for (UINT vi = 0; vi < vertexCount; ++vi) 
			//{
			//	fprintf_s(pfile, "v %f %f %f\n", pVertices[vi].x, pVertices[vi].y, pVertices[vi].z);
			//	//temp_vertice.x = pVertices[vi].x;
			//	//temp_vertice.y = pVertices[vi].y;
			//	//temp_vertice.z = pVertices[vi].z;
			//	//temp_vertices.push_back(temp_vertice);
			//}
			//for (UINT ti = 0; ti < triangleCount; ++ti) 
			//{
			//	fprintf_s(pfile, "f %d %d %d\n", pTriangles[ti].i+1, pTriangles[ti].j+1, pTriangles[ti].k+1);
			//	//temp_indices.push_back(pTriangles[ti].i);
			//	//temp_indices.push_back(pTriangles[ti].j);
			//	//temp_indices.push_back(pTriangles[ti].k);
			//}
		
			//fflush(pfile);
			//fclose(pfile);

			////Allign vertices along the indices
			//for (size_t tv = 0; tv < temp_indices.size(); tv++)
			//{
			//	out_vertices.push_back(temp_vertices[temp_indices[tv]]);
			//}

			//myDrawMesh(out_vertices);
		}
		
		if (m_AUSUcounts%TimersPerAUSUFile==0)
		{
			CloseAUSUfile();
		}
	}

	//if (!m_3DModelSave)
	//{
	//	if (m_pAUSUfile) 
	//	{
	//		fclose(m_pAUSUfile);
	//		m_pAUSUfile=nullptr;
	//	}
	//}

    delete[] pVertices;

    return hr;
}

HRESULT FaceTracker::Get3DModelFileName(wchar_t *FileName, wchar_t *TimeString, UINT FileNameSize, UINT TimeStringSize, WCHAR* instanceName, UINT flag)
{
	wchar_t DevIdOut[20];

	GetDevIdforFile(instanceName, DevIdOut);

    // Get the time
	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);

    HRESULT hr=StringCchPrintfW(TimeString, TimeStringSize, L"%d-%d-%d-%d", sysTime.wHour, sysTime.wMinute, sysTime.wSecond, sysTime.wMilliseconds);

	if (SUCCEEDED(hr))
	{
		switch (flag)
		{
		case 1:
			hr=StringCchPrintfW(FileName, FileNameSize, L"%s\\KinectFaceTracking\\FacialModelObj\\3DModel-%s-%d-%d-%d-%s.obj", knownPath, DevIdOut, sysTime.wYear, sysTime.wMonth, sysTime.wDay, TimeString);
			break;
		case 2:
			hr=StringCchPrintfW(FileName, FileNameSize, L"%s\\KinectFaceTracking\\AUSUs\\Training\\AUSUs-%s-%d-%d-%d-%s.txt", knownPath, DevIdOut, sysTime.wYear, sysTime.wMonth, sysTime.wDay, TimeString);
			break;
		default:
			hr=E_INVALIDARG;
			break;
		}
	}
	return hr;
}

void FaceTracker::CloseAUSUfile()
{
	if (m_pAUSUfile) 
	{
		fclose(m_pAUSUfile);
		m_pAUSUfile=nullptr;
	}
}
