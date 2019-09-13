/** @file

MODULE				: VirtualCam

FILE NAME			: VirtualCam.h

DESCRIPTION			: VirtualCam source header

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
#pragma once

#define DECLARE_PTR(type, ptr, expr) type* ptr = (type*)(expr);

// fwd
class CVCamStream;

// {8E14549A-DB61-4309-AFA1-3578E927E933}
static const GUID CLSID_VPP_VirtualCam =
{ 0x8e14549a, 0xdb61, 0x4309, { 0xaf, 0xa1, 0x35, 0x78, 0xe9, 0x27, 0xe9, 0x33 } };

class CVCam : public CSource
{
public:
  //////////////////////////////////////////////////////////////////////////
  //  IUnknown
  //////////////////////////////////////////////////////////////////////////
  static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
  STDMETHODIMP QueryInterface(REFIID riid, void **ppv);

  IFilterGraph *GetGraph() { return m_pGraph; }

private:
  CVCam(LPUNKNOWN lpunk, HRESULT *phr);
};

class CVCamStream : public CSourceStream, public IAMStreamConfig, public IKsPropertySet
{
public:

  //////////////////////////////////////////////////////////////////////////
  //  IUnknown
  //////////////////////////////////////////////////////////////////////////
  STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
  STDMETHODIMP_(ULONG) AddRef() { return GetOwner()->AddRef(); }                                                          \
    STDMETHODIMP_(ULONG) Release() { return GetOwner()->Release(); }

  //////////////////////////////////////////////////////////////////////////
  //  IQualityControl
  //////////////////////////////////////////////////////////////////////////
  STDMETHODIMP Notify(IBaseFilter * pSender, Quality q);

  //////////////////////////////////////////////////////////////////////////
  //  IAMStreamConfig
  //////////////////////////////////////////////////////////////////////////
  HRESULT STDMETHODCALLTYPE SetFormat(AM_MEDIA_TYPE *pmt);
  HRESULT STDMETHODCALLTYPE GetFormat(AM_MEDIA_TYPE **ppmt);
  HRESULT STDMETHODCALLTYPE GetNumberOfCapabilities(int *piCount, int *piSize);
  HRESULT STDMETHODCALLTYPE GetStreamCaps(int iIndex, AM_MEDIA_TYPE **pmt, BYTE *pSCC);

  //////////////////////////////////////////////////////////////////////////
  //  IKsPropertySet
  //////////////////////////////////////////////////////////////////////////
  HRESULT STDMETHODCALLTYPE Set(REFGUID guidPropSet, DWORD dwID, void *pInstanceData, DWORD cbInstanceData, void *pPropData, DWORD cbPropData);
  HRESULT STDMETHODCALLTYPE Get(REFGUID guidPropSet, DWORD dwPropID, void *pInstanceData, DWORD cbInstanceData, void *pPropData, DWORD cbPropData, DWORD *pcbReturned);
  HRESULT STDMETHODCALLTYPE QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport);

  //////////////////////////////////////////////////////////////////////////
  //  CSourceStream
  //////////////////////////////////////////////////////////////////////////
  CVCamStream(HRESULT *phr, CVCam *pParent, LPCWSTR pPinName);
  ~CVCamStream();

  HRESULT FillBuffer(IMediaSample *pms);
  HRESULT DecideBufferSize(IMemAllocator *pIMemAlloc, ALLOCATOR_PROPERTIES *pProperties);
  HRESULT CheckMediaType(const CMediaType *pMediaType);
  HRESULT GetMediaType(int iPosition, CMediaType *pmt);
  HRESULT SetMediaType(const CMediaType *pmt);
  HRESULT OnThreadCreate(void);

private:
  CVCam *m_pParent;
  REFERENCE_TIME m_rtLastTime;
  HBITMAP m_hLogoBmp;
  CCritSec m_cSharedState;
  IReferenceClock *m_pClock;

  unsigned m_uiThickness;
  unsigned m_uiCurrentColorIndex;
  unsigned m_uiFrameCount;
  unsigned m_uiChangeRate;
};


