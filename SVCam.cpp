// SVCam.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "SVCam.h"

#include <stdio.h>
#include <VideoProc.h>

//////////////////////////////////////////////////////////////////////////
//  CSimpleVirtualCamFilter is the source filter which masquerades as a capture device
//////////////////////////////////////////////////////////////////////////
CUnknown * WINAPI CSimpleVirtualCamFilter::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
	ASSERT(phr);
	CUnknown *punk = new CSimpleVirtualCamFilter(lpunk, phr);
	return punk;
}

CSimpleVirtualCamFilter::CSimpleVirtualCamFilter(LPUNKNOWN lpunk, HRESULT *phr) :
CSource(NAME("Simple Virtual Cam"), lpunk, CLSID_SimpleVirtualCamFilter)
{
	ASSERT(phr);
	CAutoLock cAutoLock(&m_cStateLock);

	// Create the one and only output pin
	m_paStreams = (CSourceStream **) new CSimpleVirtualCamFilterStream*[1];
	m_paStreams[0] = new CSimpleVirtualCamFilterStream(phr, this, L"Simple Virtual Cam");
}

HRESULT CSimpleVirtualCamFilter::QueryInterface(REFIID riid, void **ppv)
{
	//Forward request for IAMStreamConfig & IKsPropertySet to the pin
	if (riid == _uuidof(IAMStreamConfig) || riid == _uuidof(IKsPropertySet))
		return m_paStreams[0]->QueryInterface(riid, ppv);
	else
		return CSource::QueryInterface(riid, ppv);
}



//////////////////////////////////////////////////////////////////////////
// CKCamStream is the one and only output pin of CKCam which handles 
// all the stuff.
//////////////////////////////////////////////////////////////////////////
CSimpleVirtualCamFilterStream::CSimpleVirtualCamFilterStream(HRESULT *phr, CSimpleVirtualCamFilter *pParent, LPCWSTR pPinName) :
CSourceStream(NAME("Simple Virtual Cam"), phr, pParent, pPinName), m_pParent(pParent)
{
	VideoInit(WINDOW_WIDTH, WINDOW_HEIGHT);

	pvi = (VIDEOINFOHEADER*)(m_mt.AllocFormatBuffer(sizeof(VIDEOINFOHEADER)));
	ZeroMemory(pvi, sizeof(VIDEOINFOHEADER));
	pvi->bmiHeader.biCompression = BI_RGB;
	pvi->bmiHeader.biBitCount = BITCOUNT;
	pvi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pvi->bmiHeader.biWidth = WINDOW_WIDTH;
	pvi->bmiHeader.biHeight = WINDOW_HEIGHT;
	pvi->bmiHeader.biPlanes = 1;
	pvi->bmiHeader.biSizeImage = GetBitmapSize(&pvi->bmiHeader);
	pvi->bmiHeader.biClrImportant = 0;
	pvi->AvgTimePerFrame = m_rtFrameLength;

	m_BmpData = new DWORD[WINDOW_WIDTH * WINDOW_HEIGHT];
	memset(m_BmpData, 0, sizeof(m_BmpData));
	HDC dwhdc = GetDC(GetDesktopWindow());
	m_Bitmap = CreateDIBitmap(dwhdc, &pvi->bmiHeader, CBM_INIT, m_BmpData, (BITMAPINFO*)(&pvi->bmiHeader), DIB_RGB_COLORS);
	if (m_Hdc)
	{
		DeleteDC(m_Hdc);
		m_Hdc = NULL;
	}
	m_Hdc = CreateCompatibleDC(dwhdc);
	SelectObject(m_Hdc, m_Bitmap);
	ReleaseDC(GetDesktopWindow(), dwhdc);

	GetMediaType(8, &m_mt);
}

CSimpleVirtualCamFilterStream::~CSimpleVirtualCamFilterStream()
{
	VideoTerminate();
	if (m_Bitmap) DeleteObject(m_Bitmap);
	if (m_Hdc) DeleteDC(m_Hdc);
	if (m_BmpData) delete m_BmpData;
}

ULONG CSimpleVirtualCamFilterStream::Release(){
	return GetOwner()->Release();
}
ULONG CSimpleVirtualCamFilterStream::AddRef(){

	return GetOwner()->AddRef();
}

HRESULT CSimpleVirtualCamFilterStream::QueryInterface(REFIID riid, void **ppv)
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

///////////////////////////////////////////////////////////
// This is where the magic happens!
///////////////////////////////////////////////////////////
HRESULT CSimpleVirtualCamFilterStream::FillBuffer(IMediaSample *pms)
{
	HRESULT hr = E_FAIL;
	CheckPointer(pms, E_POINTER);
	// ダウンストリームフィルタが
	// フォーマットを動的に変えていないかチェック
	ASSERT(m_mt.formattype == FORMAT_VideoInfo);
	ASSERT(m_mt.cbFormat >= sizeof(VIDEOINFOHEADER));
	// フレームに書き込み
	LPBYTE pSampleData = NULL;
	const long size = pms->GetSize();
	pms->GetPointer(&pSampleData);

	CRefTime ref;
	m_pFilter->StreamTime(ref);

	//IReferenceClock *clock;
	//m_pFilter->GetSyncSource(&clock);
	//REFERENCE_TIME stime;
	//clock->GetTime(&stime);

	/*_snwprintf_s(buffer, _countof(buffer), _TRUNCATE, TEXT("%09d"), ref.Millisecs());
	TextOut(m_Hdc, 0, 0, buffer, lstrlen(buffer));
	clock->Release();

	VIDEOINFO *pvi = (VIDEOINFO *)m_mt.Format();

	GetDIBits(m_Hdc, m_Bitmap, 0, WINDOW_HEIGHT,
		pSampleData, (BITMAPINFO*)&pvi->bmiHeader, DIB_RGB_COLORS);
	*/
	VideoProc(m_Hdc);
	//clock->Release();
	VIDEOINFO *pvi = (VIDEOINFO *)m_mt.Format();
	GetDIBits(m_Hdc, m_Bitmap, 0, WINDOW_HEIGHT,
		pSampleData, (BITMAPINFO*)&pvi->bmiHeader, DIB_RGB_COLORS);

	const REFERENCE_TIME delta = m_rtFrameLength;
	REFERENCE_TIME start_time = ref;
	FILTER_STATE state;
	m_pFilter->GetState(0, &state);
	if (state == State_Paused)
		start_time = 0;
	REFERENCE_TIME end_time = (start_time + delta);
	pms->SetTime(&start_time, &end_time);
	pms->SetActualDataLength(size);
	pms->SetSyncPoint(TRUE);

	return S_OK;
}

//
// Notify
// Ignore quality management messages sent from the downstream filter
STDMETHODIMP CSimpleVirtualCamFilterStream::Notify(IBaseFilter * pSender, Quality q)
{
	return E_NOTIMPL;
} // Notify


//////////////////////////////////////////////////////////////////////////
// This is called when the output format has been negotiated
//////////////////////////////////////////////////////////////////////////
HRESULT CSimpleVirtualCamFilterStream::SetMediaType(const CMediaType *pmt)
{
	DECLARE_PTR(VIDEOINFOHEADER, pvi, pmt->Format());
	HRESULT hr = CSourceStream::SetMediaType(pmt);
	return hr;
}

// See Directshow help topic for IAMStreamConfig for details on this method
HRESULT CSimpleVirtualCamFilterStream::GetMediaType(int iPosition, CMediaType *pmt)
{
	if (iPosition < 0) return E_INVALIDARG;
	if (iPosition > 8) return VFW_S_NO_MORE_ITEMS;

	if (iPosition == 0)
	{
		*pmt = m_mt;
		return S_OK;
	}

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
HRESULT CSimpleVirtualCamFilterStream::CheckMediaType(const CMediaType *pMediaType)
{
	VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)(pMediaType->Format());
	if (*pMediaType != m_mt)
		return E_INVALIDARG;
	return S_OK;
} // CheckMediaType

// This method is called after the pins are connected to allocate buffers to stream data
HRESULT CSimpleVirtualCamFilterStream::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties)
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
HRESULT CSimpleVirtualCamFilterStream::OnThreadCreate()
{
	m_rtLastTime = 0;
	return NOERROR;
} // OnThreadCreate

//////////////////////////////////////////////////////////////////////////
//  IAMStreamConfig
//////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE CSimpleVirtualCamFilterStream::SetFormat(AM_MEDIA_TYPE *pmt)
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

HRESULT STDMETHODCALLTYPE CSimpleVirtualCamFilterStream::GetFormat(AM_MEDIA_TYPE **ppmt)
{
	*ppmt = CreateMediaType(&m_mt);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CSimpleVirtualCamFilterStream::GetNumberOfCapabilities(int *piCount, int *piSize)
{
	*piCount = 8;
	*piSize = sizeof(VIDEO_STREAM_CONFIG_CAPS);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CSimpleVirtualCamFilterStream::GetStreamCaps(int iIndex, AM_MEDIA_TYPE **pmt, BYTE *pSCC)
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


HRESULT CSimpleVirtualCamFilterStream::Set(REFGUID guidPropSet, DWORD dwID, void *pInstanceData,
	DWORD cbInstanceData, void *pPropData, DWORD cbPropData)
{// Set: Cannot set any properties.
	return E_NOTIMPL;
}

// Get: Return the pin category (our only property). 
HRESULT CSimpleVirtualCamFilterStream::Get(
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
HRESULT CSimpleVirtualCamFilterStream::QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport)
{
	if (guidPropSet != AMPROPSETID_Pin) return E_PROP_SET_UNSUPPORTED;
	if (dwPropID != AMPROPERTY_PIN_CATEGORY) return E_PROP_ID_UNSUPPORTED;
	// We support getting this property, but not setting it.
	if (pTypeSupport) *pTypeSupport = KSPROPERTY_SUPPORT_GET;
	return S_OK;
}

// Set misc flag that this is truly a live source
ULONG CSimpleVirtualCamFilterStream::GetMiscFlags(void)
{
	return AM_FILTER_MISC_FLAGS_IS_SOURCE;
}