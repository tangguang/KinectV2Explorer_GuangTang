//------------------------------------------------------------------------------
// <copyright file="stdafx.h" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

// include file for standard system and project includes

#pragma once

//#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#endif


// Windows Header Files
#include <windows.h>
#include <ShellAPI.h>
#include <Shlobj.h>

// Direct2D Header Files
#include <d2d1.h>

// Assert Header Files
#include <assert.h>

// Kinect Header files
#include <Kinect.h>

#pragma comment (lib, "d2d1.lib")
#pragma comment(lib, "Dwrite.lib")
#pragma comment (lib, "kinect20.lib")
#pragma comment(lib,"strmiids.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "Dmoguids.lib")
#pragma comment(lib, "Msdmo.lib")

// The user defined message
#define WM_UPDATEMAINWINDOW             WM_USER + 1
#define WM_CLOSEKINECTWINDOW            WM_USER + 2
#define WM_STREAMEVENT                  WM_USER + 3
#define WM_TIMEREVENT                   WM_USER + 4
#define WM_SHOWKINECTWINDOW             WM_USER + 5

static const UINT MaxStringChars = 256;

#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif