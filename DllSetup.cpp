/** @file

MODULE				: VirtualCam

FILE NAME			: VirtualCam.h

DESCRIPTION			: VirtualCam DLLSetup source

LICENSE: Software License Agreement (BSD License)

Copyright (c) 2015, CSIR
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
* Neither the name of the CSIR nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

===========================================================================
NOTE: The original code was written by Vivek.
Adapted from http://tmhare.mvps.org/downloads/vcam.zip
===========================================================================
*/
//////////////////////////////////////////////////////////////////////////
//  This file contains routines to register / Unregister the 
//  Directshow filter 'Virtual Cam'
//  We do not use the inbuilt BaseClasses routines as we need to register as
//  a capture source
//////////////////////////////////////////////////////////////////////////
#pragma comment(lib, "kernel32")
#pragma comment(lib, "user32")
#pragma comment(lib, "gdi32")
#pragma comment(lib, "advapi32")
#pragma comment(lib, "winmm")
#pragma comment(lib, "ole32")
#pragma comment(lib, "oleaut32")
#include <streams.h>
#include <olectl.h>
#include <initguid.h>
#include <dllsetup.h>
#include "VirtualCam.h"

#define CreateComObject(clsid, iid, var) CoCreateInstance( clsid, NULL, CLSCTX_INPROC_SERVER, iid, (void **)&var);

STDAPI AMovieSetupRegisterServer(CLSID   clsServer, LPCWSTR szDescription, LPCWSTR szFileName, LPCWSTR szThreadingModel = L"Both", LPCWSTR szServerType = L"InprocServer32");
STDAPI AMovieSetupUnregisterServer(CLSID clsServer);

const AMOVIESETUP_MEDIATYPE AMSMediaTypesVCam =
{
  &MEDIATYPE_Video,
  &MEDIASUBTYPE_NULL
};

const AMOVIESETUP_PIN AMSPinVCam =
{
  L"Output",             // Pin string name
  FALSE,                 // Is it rendered
  TRUE,                  // Is it an output
  FALSE,                 // Can we have none
  FALSE,                 // Can we have many
  &CLSID_NULL,           // Connects to filter
  NULL,                  // Connects to pin
  1,                     // Number of types
  &AMSMediaTypesVCam     // Pin Media types
};

const AMOVIESETUP_FILTER AMSFilterVCam =
{
  &CLSID_VPP_VirtualCam,    // Filter CLSID
  L"CSIR VPP Virtual Cam",  // String name
  MERIT_DO_NOT_USE,         // Filter merit
  1,                        // Number pins
  &AMSPinVCam               // Pin details
};

CFactoryTemplate g_Templates[] =
{
  {
    L"CSIR VPP Virtual Cam",
    &CLSID_VPP_VirtualCam,
    CVCam::CreateInstance,
    NULL,
    &AMSFilterVCam
  },

};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

STDAPI RegisterFilters(BOOL bRegister)
{
  HRESULT hr = NOERROR;
  WCHAR achFileName[MAX_PATH];
  char achTemp[MAX_PATH];
  ASSERT(g_hInst != 0);

  if (0 == GetModuleFileNameA(g_hInst, achTemp, sizeof(achTemp)))
    return AmHresultFromWin32(GetLastError());

  MultiByteToWideChar(CP_ACP, 0L, achTemp, lstrlenA(achTemp) + 1,
    achFileName, NUMELMS(achFileName));

  hr = CoInitialize(0);
  if (bRegister)
  {
    hr = AMovieSetupRegisterServer(CLSID_VPP_VirtualCam, L"CSIR VPP Virtual Cam", achFileName, L"Both", L"InprocServer32");
  }

  if (SUCCEEDED(hr))
  {
    IFilterMapper2 *fm = 0;
    hr = CreateComObject(CLSID_FilterMapper2, IID_IFilterMapper2, fm);
    if (SUCCEEDED(hr))
    {
      if (bRegister)
      {
        IMoniker *pMoniker = 0;
        REGFILTER2 rf2;
        rf2.dwVersion = 1;
        rf2.dwMerit = MERIT_DO_NOT_USE;
        rf2.cPins = 1;
        rf2.rgPins = &AMSPinVCam;
        hr = fm->RegisterFilter(CLSID_VPP_VirtualCam, L"CSIR VPP Virtual Cam", &pMoniker, &CLSID_VideoInputDeviceCategory, NULL, &rf2);
      }
      else
      {
        hr = fm->UnregisterFilter(&CLSID_VideoInputDeviceCategory, 0, CLSID_VPP_VirtualCam);
      }
    }

    // release interface
    //
    if (fm)
      fm->Release();
  }

  if (SUCCEEDED(hr) && !bRegister)
    hr = AMovieSetupUnregisterServer(CLSID_VPP_VirtualCam);

  CoFreeUnusedLibraries();
  CoUninitialize();
  return hr;
}

STDAPI DllRegisterServer()
{
  return RegisterFilters(TRUE);
}

STDAPI DllUnregisterServer()
{
  return RegisterFilters(FALSE);
}

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  dwReason, LPVOID lpReserved)
{
  return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}
