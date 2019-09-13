/** @file

MODULE				: VirtualCam

FILE NAME			: VirtualCam.cpp

DESCRIPTION			: VirtualCam source

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
#pragma warning(disable:4244)
#pragma warning(disable:4711)

#pragma warning(push)     // disable for this header only
#pragma warning(disable:4312) 
// DirectShow
#include <streams.h>
#pragma warning(pop)      // restore original warning level
#include <stdio.h>
#include <olectl.h>
#include <dvdmedia.h>
#include "VirtualCam.h"

const unsigned MAX_COLORS = 8;
const unsigned THICKNESS = 20;
RGBTRIPLE red = { 0, 0, 255 };
RGBTRIPLE blue = { 255, 0, 0 };
RGBTRIPLE green = { 0, 255, 0 };
RGBTRIPLE yellow = { 0, 255, 255 };
RGBTRIPLE magenta = { 255, 0, 255 };
RGBTRIPLE cyan = { 255, 255, 0 };
RGBTRIPLE black = { 0, 0, 0 };
RGBTRIPLE white = { 255, 255, 255 };
RGBTRIPLE lineColors[MAX_COLORS] = { white, yellow, cyan, green, magenta, red, blue, black };

//////////////////////////////////////////////////////////////////////////
//  CVCam is the source filter which masquerades as a capture device
//////////////////////////////////////////////////////////////////////////
CUnknown * WINAPI CVCam::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
  ASSERT(phr);
  CUnknown *punk = new CVCam(lpunk, phr);
  return punk;
}

CVCam::CVCam(LPUNKNOWN lpunk, HRESULT *phr)
  :CSource(NAME("CSIR VPP Virtual Cam"), lpunk, CLSID_VPP_VirtualCam)
{
  ASSERT(phr);
  CAutoLock cAutoLock(&m_cStateLock);
  // Create the one and only output pin
  m_paStreams = (CSourceStream **) new CVCamStream*[1];
  m_paStreams[0] = new CVCamStream(phr, this, L"CSIR VPP Virtual Cam");
}

HRESULT CVCam::QueryInterface(REFIID riid, void **ppv)
{
  //Forward request for IAMStreamConfig & IKsPropertySet to the pin
  if (riid == _uuidof(IAMStreamConfig) || riid == _uuidof(IKsPropertySet))
    return m_paStreams[0]->QueryInterface(riid, ppv);
  else
    return CSource::QueryInterface(riid, ppv);
}

//////////////////////////////////////////////////////////////////////////
// CVCamStream is the one and only output pin of CVCam which handles 
// all the stuff.
//////////////////////////////////////////////////////////////////////////
CVCamStream::CVCamStream(HRESULT *phr, CVCam *pParent, LPCWSTR pPinName) :
CSourceStream(NAME("CSIR VPP Virtual Cam"), phr, pParent, pPinName), m_pParent(pParent)
{
  // Set the default media type as 320x240x24@15
  GetMediaType(4, &m_mt);

  lineColors[0] = white;
  lineColors[1] = yellow;
  lineColors[2] = cyan;
  lineColors[3] = green;
  lineColors[4] = magenta;
  lineColors[5] = red;
  lineColors[6] = blue;
  lineColors[7] = black;

  m_uiThickness = THICKNESS;
  m_uiCurrentColorIndex = 0;
  m_uiChangeRate = 1;
  m_uiFrameCount = 0;
}

CVCamStream::~CVCamStream()
{
}

HRESULT CVCamStream::QueryInterface(REFIID riid, void **ppv)
{
  // Standard OLE stuff
  if (riid == _uuidof(IAMStreamConfig))
    *ppv = (IAMStreamConfig*)this;
  else if (riid == _uuidof(IKsPropertySet))
    *ppv = (IKsPropertySet*)this;
  else
    return CSourceStream::QueryInterface(riid, ppv);

  AddRef();
  return S_OK;
}


//////////////////////////////////////////////////////////////////////////
//  This is the routine where we create the data being output by the Virtual
//  Camera device.
//////////////////////////////////////////////////////////////////////////

HRESULT CVCamStream::FillBuffer(IMediaSample *pms)
{
  REFERENCE_TIME rtNow;

  VIDEOINFOHEADER* pVih = (VIDEOINFOHEADER*)m_mt.pbFormat;
  REFERENCE_TIME avgFrameTime = ((VIDEOINFOHEADER*)m_mt.pbFormat)->AvgTimePerFrame;

  unsigned uiWidth = pVih->bmiHeader.biWidth;
  unsigned uiHeight = pVih->bmiHeader.biHeight;

  rtNow = m_rtLastTime;
  m_rtLastTime += avgFrameTime;
  pms->SetTime(&rtNow, &m_rtLastTime);
  pms->SetSyncPoint(TRUE);

  BYTE *pData;
  long lDataLen;
  pms->GetPointer(&pData);
  lDataLen = pms->GetSize();

#if 1
  unsigned uiOffset = 0;

  BYTE* pRow = new BYTE[uiWidth * 3];

  unsigned uiRowsCompleted = 0;
  unsigned uiLeftOfCol = m_uiThickness;
  unsigned uiCurrentColorIndex = m_uiCurrentColorIndex;
  while (uiRowsCompleted < uiHeight)
  {
    // Current color
    RGBTRIPLE* pColor = &lineColors[uiCurrentColorIndex];
    //create a single row and copy it uiThickness times
    for (size_t j = 0; j < uiWidth * 3; j += 3)
    {
      memcpy(pRow + j, pColor, sizeof(RGBTRIPLE));
    }

    while ((uiLeftOfCol > 0) && (uiRowsCompleted < uiHeight))
    {
      memcpy(pData + uiOffset, pRow, uiWidth * 3);
      uiOffset += uiWidth * 3;
      ++uiRowsCompleted;

      --uiLeftOfCol;
    }

    // Next thickness
    uiLeftOfCol = THICKNESS;
    // Next color
    uiCurrentColorIndex = (++uiCurrentColorIndex) % MAX_COLORS;
  }

  ++m_uiFrameCount;
  // Update thickness for rolling effect

#if 1
  if (m_uiFrameCount%m_uiChangeRate == 0)
  {
    if (m_uiThickness == 1)
      //if (m_uiThickness == 2)
    {
      // Next color
      m_uiCurrentColorIndex = (++m_uiCurrentColorIndex) % MAX_COLORS;
      m_uiThickness = THICKNESS;
    }
    else --m_uiThickness;
    //else m_uiThickness -= 2;

  }
#endif
#if 0
  unsigned uiCurrentColorIndex = 0;
  unsigned uiColorRows = uiHeight/THICKNESS;

  for (size_t iRow = 0; iRow < uiColorRows; ++iRow)
  {
    RGBTRIPLE* pColor = &lineColors[uiCurrentColorIndex];
    //create a single row and copy it uiThickness times
    for (size_t j = 0; j < uiWidth*3; j = j + 3)
    {
      memcpy(pRow + j, pColor, sizeof(RGBTRIPLE));
    }

    for (size_t k = 0; k < uiThickness; ++k)
    {
      memcpy(pData + uiOffset, pRow, uiWidth*3);
      uiOffset += uiWidth*3;
    }
    // get next color
    uiCurrentColorIndex = (uiCurrentColorIndex + 1)%MAX_COLORS;
  }
#endif
  delete pRow;
#endif
#if 0
  for(int i = 0; i < lDataLen; ++i)
    pData[i] = rand();
#endif

  return NOERROR;
} // FillBuffer


//
// Notify
// Ignore quality management messages sent from the downstream filter
STDMETHODIMP CVCamStream::Notify(IBaseFilter * pSender, Quality q)
{
  return E_NOTIMPL;
} // Notify

//////////////////////////////////////////////////////////////////////////
// This is called when the output format has been negotiated
//////////////////////////////////////////////////////////////////////////
HRESULT CVCamStream::SetMediaType(const CMediaType *pmt)
{
  DECLARE_PTR(VIDEOINFOHEADER, pvi, pmt->Format());
  HRESULT hr = CSourceStream::SetMediaType(pmt);
  return hr;
}

// See Directshow help topic for IAMStreamConfig for details on this method
HRESULT CVCamStream::GetMediaType(int iPosition, CMediaType *pmt)
{
  if (iPosition < 0) return E_INVALIDARG;
  if (iPosition > 8) return VFW_S_NO_MORE_ITEMS;

  if (iPosition == 0)
  {
    *pmt = m_mt;
    return S_OK;
  }

  DECLARE_PTR(VIDEOINFOHEADER, pvi, pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER)));
  ZeroMemory(pvi, sizeof(VIDEOINFOHEADER));

  pvi->bmiHeader.biCompression = BI_RGB;
  pvi->bmiHeader.biBitCount = 24;
  pvi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  pvi->bmiHeader.biWidth = 80 * iPosition;
  pvi->bmiHeader.biHeight = 60 * iPosition;
  pvi->bmiHeader.biPlanes = 1;
  pvi->bmiHeader.biSizeImage = GetBitmapSize(&pvi->bmiHeader);
  pvi->bmiHeader.biClrImportant = 0;

  pvi->AvgTimePerFrame = 400000; // 25fps

  SetRectEmpty(&(pvi->rcSource)); // we want the whole image area rendered.
  SetRectEmpty(&(pvi->rcTarget)); // no particular destination rectangle

  pmt->SetType(&MEDIATYPE_Video);
  pmt->SetFormatType(&FORMAT_VideoInfo);
  pmt->SetTemporalCompression(FALSE);

  // Work out the GUID for the subtype from the header info.
  const GUID SubTypeGUID = GetBitmapSubtype(&pvi->bmiHeader);
  pmt->SetSubtype(&SubTypeGUID);
  pmt->SetSampleSize(pvi->bmiHeader.biSizeImage);

  return NOERROR;

} // GetMediaType

// This method is called to see if a given output format is supported
HRESULT CVCamStream::CheckMediaType(const CMediaType *pMediaType)
{
  VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)(pMediaType->Format());
  if (*pMediaType != m_mt)
    return E_INVALIDARG;
  return S_OK;
} // CheckMediaType

// This method is called after the pins are connected to allocate buffers to stream data
HRESULT CVCamStream::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties)
{
  CAutoLock cAutoLock(m_pFilter->pStateLock());
  HRESULT hr = NOERROR;

  VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)m_mt.Format();
  pProperties->cBuffers = 1;
  pProperties->cbBuffer = pvi->bmiHeader.biSizeImage;

  ALLOCATOR_PROPERTIES Actual;
  hr = pAlloc->SetProperties(pProperties, &Actual);

  if (FAILED(hr)) return hr;
  if (Actual.cbBuffer < pProperties->cbBuffer) return E_FAIL;

  return NOERROR;
} // DecideBufferSize

// Called when graph is run
HRESULT CVCamStream::OnThreadCreate()
{
  m_rtLastTime = 0;
  return NOERROR;
} // OnThreadCreate


//////////////////////////////////////////////////////////////////////////
//  IAMStreamConfig
//////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE CVCamStream::SetFormat(AM_MEDIA_TYPE *pmt)
{
  DECLARE_PTR(VIDEOINFOHEADER, pvi, m_mt.pbFormat);
  m_mt = *pmt;
  IPin* pin;
  ConnectedTo(&pin);
  if (pin)
  {
    IFilterGraph *pGraph = m_pParent->GetGraph();
    pGraph->Reconnect(this);
  }
  return S_OK;
}

HRESULT STDMETHODCALLTYPE CVCamStream::GetFormat(AM_MEDIA_TYPE **ppmt)
{
  *ppmt = CreateMediaType(&m_mt);
  return S_OK;
}

HRESULT STDMETHODCALLTYPE CVCamStream::GetNumberOfCapabilities(int *piCount, int *piSize)
{
  *piCount = 8;
  *piSize = sizeof(VIDEO_STREAM_CONFIG_CAPS);
  return S_OK;
}

HRESULT STDMETHODCALLTYPE CVCamStream::GetStreamCaps(int iIndex, AM_MEDIA_TYPE **pmt, BYTE *pSCC)
{
  *pmt = CreateMediaType(&m_mt);
  DECLARE_PTR(VIDEOINFOHEADER, pvi, (*pmt)->pbFormat);

  if (iIndex == 0) iIndex = 4;

  pvi->bmiHeader.biCompression = BI_RGB;
  pvi->bmiHeader.biBitCount = 24;
  pvi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  pvi->bmiHeader.biWidth = 80 * iIndex;
  pvi->bmiHeader.biHeight = 60 * iIndex;
  pvi->bmiHeader.biPlanes = 1;
  pvi->bmiHeader.biSizeImage = GetBitmapSize(&pvi->bmiHeader);
  pvi->bmiHeader.biClrImportant = 0;

  SetRectEmpty(&(pvi->rcSource)); // we want the whole image area rendered.
  SetRectEmpty(&(pvi->rcTarget)); // no particular destination rectangle

  (*pmt)->majortype = MEDIATYPE_Video;
  (*pmt)->subtype = MEDIASUBTYPE_RGB24;
  (*pmt)->formattype = FORMAT_VideoInfo;
  (*pmt)->bTemporalCompression = FALSE;
  (*pmt)->bFixedSizeSamples = FALSE;
  (*pmt)->lSampleSize = pvi->bmiHeader.biSizeImage;
  (*pmt)->cbFormat = sizeof(VIDEOINFOHEADER);

  DECLARE_PTR(VIDEO_STREAM_CONFIG_CAPS, pvscc, pSCC);

  pvscc->guid = FORMAT_VideoInfo;
  pvscc->VideoStandard = AnalogVideo_None;
  pvscc->InputSize.cx = 640;
  pvscc->InputSize.cy = 480;
  pvscc->MinCroppingSize.cx = 80;
  pvscc->MinCroppingSize.cy = 60;
  pvscc->MaxCroppingSize.cx = 640;
  pvscc->MaxCroppingSize.cy = 480;
  pvscc->CropGranularityX = 80;
  pvscc->CropGranularityY = 60;
  pvscc->CropAlignX = 0;
  pvscc->CropAlignY = 0;

  pvscc->MinOutputSize.cx = 80;
  pvscc->MinOutputSize.cy = 60;
  pvscc->MaxOutputSize.cx = 640;
  pvscc->MaxOutputSize.cy = 480;
  pvscc->OutputGranularityX = 0;
  pvscc->OutputGranularityY = 0;
  pvscc->StretchTapsX = 0;
  pvscc->StretchTapsY = 0;
  pvscc->ShrinkTapsX = 0;
  pvscc->ShrinkTapsY = 0;
  pvscc->MinFrameInterval = 200000;   //50 fps
  pvscc->MaxFrameInterval = 50000000; // 0.2 fps
  pvscc->MinBitsPerSecond = (80 * 60 * 3 * 8) / 5;
  pvscc->MaxBitsPerSecond = 640 * 480 * 3 * 8 * 50;

  return S_OK;
}

//////////////////////////////////////////////////////////////////////////
// IKsPropertySet
//////////////////////////////////////////////////////////////////////////


HRESULT CVCamStream::Set(REFGUID guidPropSet, DWORD dwID, void *pInstanceData,
  DWORD cbInstanceData, void *pPropData, DWORD cbPropData)
{// Set: Cannot set any properties.
  return E_NOTIMPL;
}

// Get: Return the pin category (our only property). 
HRESULT CVCamStream::Get(
  REFGUID guidPropSet,   // Which property set.
  DWORD dwPropID,        // Which property in that set.
  void *pInstanceData,   // Instance data (ignore).
  DWORD cbInstanceData,  // Size of the instance data (ignore).
  void *pPropData,       // Buffer to receive the property data.
  DWORD cbPropData,      // Size of the buffer.
  DWORD *pcbReturned     // Return the size of the property.
  )
{
  if (guidPropSet != AMPROPSETID_Pin)             return E_PROP_SET_UNSUPPORTED;
  if (dwPropID != AMPROPERTY_PIN_CATEGORY)        return E_PROP_ID_UNSUPPORTED;
  if (pPropData == NULL && pcbReturned == NULL)   return E_POINTER;

  if (pcbReturned) *pcbReturned = sizeof(GUID);
  if (pPropData == NULL)          return S_OK; // Caller just wants to know the size. 
  if (cbPropData < sizeof(GUID))  return E_UNEXPECTED;// The buffer is too small.

  *(GUID *)pPropData = PIN_CATEGORY_CAPTURE;
  return S_OK;
}

// QuerySupported: Query whether the pin supports the specified property.
HRESULT CVCamStream::QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport)
{
  if (guidPropSet != AMPROPSETID_Pin) return E_PROP_SET_UNSUPPORTED;
  if (dwPropID != AMPROPERTY_PIN_CATEGORY) return E_PROP_ID_UNSUPPORTED;
  // We support getting this property, but not setting it.
  if (pTypeSupport) *pTypeSupport = KSPROPERTY_SUPPORT_GET;
  return S_OK;
}
