//------------------------------------------------------------------------------
// <copyright file="FaceTracker.h" written by Yun Li, 9/27/2013>
//     All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#pragma once

#include "NuiDepthStream.h"
#include "NuiColorStream.h"
#include "NuiSkeletonStream.h"

class FaceTracker
{
private:
	NuiColorStream*        m_pColorStream;             // Pointer to color stream
    NuiDepthStream*        m_pDepthStream;             // Pointer to depth stream
    NuiSkeletonStream*     m_pSkeletonStream;          // Pointer to skeleton stream

	WCHAR*                 m_instanceName;
	IFTFaceTracker*        m_pFaceTracker;
	IFTResult*             m_pFTResult;
    IFTImage*              m_pFTcolorImage;
    IFTImage*              m_pFTdepthImage;
	FT_CAMERA_CONFIG       m_ColorCameraConfig;
	FT_CAMERA_CONFIG       m_DepthCameraConfig;
	bool                   m_ApplicationIsRunning;
	POINT                  m_ViewOffSet;
	float                  m_ZoomFactor;
	bool                   m_LastTrackSucceeded;
	FT_VECTOR3D            m_hint3D[2];
	bool                   m_3DModelSave;
	UINT                   m_AUSUcounts;
	FILE*                  m_pAUSUfile;

	//GLFWwindow*            m_hWindow;
	//GLuint                 m_ProgramID;
	//GLuint                 m_MatrixID;
	//GLuint                 m_Texture;

private:
	void CheckCameraInput();
	float GetZoomFactor() const {return m_ZoomFactor;};
	POINT GetViewOffSet() const {return m_ViewOffSet;};
	HRESULT Stop();
	bool SubmitFraceTrackingResult(IFTResult* pResult);
	HRESULT Save3DModel(IFTModel* model, 
                float* pSUs, uint32_t suCount,
                float* pAUs, uint32_t auCount,
                float scale, float rotationXYZ[3], float translationXYZ[3]);
	HRESULT Get3DModelFileName(wchar_t *FileName, wchar_t *TimeString, UINT FileNameSize, UINT TimeStringSize, WCHAR* instanceName, UINT flag);
	//int CreateMeshViewer();
	//int myDrawMesh(std::vector<glm::vec3> & out_vertices);

public:
    FaceTracker(NuiColorStream* pColorStream, NuiDepthStream* pDepthStream, NuiSkeletonStream* pSkeletonStream, PCWSTR instanceName);
	~FaceTracker();
	DWORD WINAPI FaceTrackingThread();
	bool GetFTRecordingStatus() const {return m_3DModelSave;};
	void SetFTRecordingStatus(bool status) {m_3DModelSave=status;};
	void ResetAUSUcounts() {m_AUSUcounts=0;};
	void CloseAUSUfile();
};