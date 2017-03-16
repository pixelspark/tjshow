/* TJShow (C) Tommy van der Vorst, Pixelspark, 2005-2017.
 * 
 * This file is part of TJShow. TJShow is free software: you 
 * can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later 
 * version.
 * 
 * TJShow is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with TJShow.  If not, see <http://www.gnu.org/licenses/>. */
#include "../include/internal/tjshow.h"
#include "../include/internal/tjdirectx.h"
#include "view/tjtexturerenderer.h"
using namespace tj::show::media;

CComPtr<IPin> DXGetPin(CComPtr<IBaseFilter> filter, PIN_DIRECTION dir) {
    bool found = false;
    CComPtr<IEnumPins> pinEnum;
    CComPtr<IPin> pin;

    // Begin by enumerating all the pins on a filter
	if(FAILED(filter->EnumPins(&pinEnum))) {
		return NULL;
	}

    // Now look for a pin that matches the direction characteristic.
    // When we've found it, we'll return with it.
    while(pinEnum->Next(1, &pin, 0)==S_OK) {
        PIN_DIRECTION cdir;
        pin->QueryDirection(&cdir);

		if(dir==cdir) {
			found = true;
			break;
		}
    }

	return found?pin:NULL;
}

HRESULT FindCaptureDevice(IBaseFilter ** ppSrcFilter, int index) {
    HRESULT hr;
    IBaseFilter * pSrc = NULL;
    CComPtr<IMoniker> pMoniker =NULL;
    ULONG cFetched;

    if (!ppSrcFilter)
        return E_POINTER;
   
    // Create the system device enumerator
    CComPtr<ICreateDevEnum> pDevEnum =NULL;

    hr = CoCreateInstance (CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC, IID_ICreateDevEnum, (void **) &pDevEnum);
    if(FAILED(hr)) {
        return hr;
    }

    // Create an enumerator for the video capture devices
    CComPtr<IEnumMoniker> pClassEnum = NULL;

    hr = pDevEnum->CreateClassEnumerator (CLSID_VideoInputDeviceCategory, &pClassEnum, 0);
    if(FAILED(hr)) {
        return hr;
    }

    // If there are no enumerators for the requested type, then 
    // CreateClassEnumerator will succeed, but pClassEnum will be NULL.
    if(pClassEnum == NULL) {
		Log::Write(L"TJShow/Deck", L"No video capture hardware detected");
        return E_FAIL;
    }

    // Use the first video capture device on the device list.
    // Note that if the Next() call succeeds but there are no monikers,
    // it will return S_FALSE (which is not a failure).  Therefore, we
    // check that the return code is S_OK instead of using SUCCEEDED() macro.
	pClassEnum->Skip(index-1);

    if(SUCCEEDED(pClassEnum->Next(1, &pMoniker, &cFetched))) {
        // Bind Moniker to a filter object
        hr = pMoniker->BindToObject(0,0,IID_IBaseFilter, (void**)&pSrc);
        if (FAILED(hr)) {
            return hr;
        }
    }
    else {
        return E_FAIL;
    }

    *ppSrcFilter = pSrc;
    return hr;
}

HRESULT DXLoadGraphFile(CComPtr<IGraphBuilder> pGraph, std::wstring name) {
    IStorage *pStorage = 0;

    if (S_OK != StgIsStorageFile(name.c_str())) {
        return E_FAIL;
    }

    HRESULT hr = StgOpenStorage(name.c_str(), 0,STGM_TRANSACTED | STGM_READ | STGM_SHARE_DENY_WRITE, 0, 0, &pStorage);

    if (FAILED(hr)) {
        return hr;
    }

    IPersistStream *pPersistStream = 0;
    hr = pGraph->QueryInterface(IID_IPersistStream, reinterpret_cast<void**>(&pPersistStream));

    if (SUCCEEDED(hr)) {
        IStream *pStream = 0;
        pStorage->OpenStream(L"ActiveMovieGraph", 0,STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &pStream);

        if(SUCCEEDED(hr)) {
            hr = pPersistStream->Load(pStream);
            pStream->Release();
        }
        pPersistStream->Release();
    }
    pStorage->Release();
    return hr;
}

VideoDeck::VideoDeck(CComPtr<TextureRenderer> renderer, bool visible): _keyingEnabled(false), _keyingTolerance(0.1f), _keyingColor(0.0f, 1.0f, 0.0f) {
	assert(renderer);
	_volume = 1.0f;
	_alpha = 1.0f;
	_muted = false;
	_videoOutput = true;
	_loaded = false;
	_visible = visible;

	// keep the soul alive...
	_pm = Application::Instance()->GetOutputManager();
	_playing = PlaybackStop;
	_renderer = renderer;
	_renderer->AddRef();
}

VideoDeck::~VideoDeck() {
	Stop();
	_renderer->Release();
}

bool VideoDeck::IsKeyingEnabled() const {
	return _keyingEnabled;
}

void VideoDeck::SetKeyingEnabled(bool t) {
	ThreadLock lock(&_lock);
	_keyingEnabled = t;
}

float VideoDeck::GetKeyingTolerance() const {
	return _keyingTolerance;
}

void VideoDeck::SetKeyingTolerance(float t) {
	ThreadLock lock(&_lock);
	_keyingTolerance = t;
}

RGBColor VideoDeck::GetKeyColor() const {
	return _keyingColor;
}

void VideoDeck::SetKeyColor(const RGBColor& col) {
	ThreadLock lock(&_lock);
	_keyingColor = col;
}

IDirect3DTexture9* VideoDeck::GetTexture() {
	if(HasVideo() && (IsPlaying() || IsPaused())) {
		IDirect3DTexture9* tex = _renderer->GetTexture();
		if(tex!=0) {
			tex->AddRef();
			return tex;
		}
	}
	return 0;
}

volatile float VideoDeck::GetCurrentAudioLevel() const {
	/***if(IsPlaying()) {
		return _sampler->GetLastAverage();
	}***/
	return 0.0f;
}

void VideoDeck::Load(const std::wstring& fn, ref<Device> card) {
	Load(fn,0,card);
}

void VideoDeck::LoadLive(ref<Device> source, ref<Device> card) {
	Load(L"", source, card);
}

bool VideoDeck::IsHorizontallyFlipped() const {
	return true;
}

bool VideoDeck::IsVerticallyFlipped() const {
	return false;
}

void VideoDeck::Load(const std::wstring& fn, ref<Device> live, ref<Device> card) {
	ThreadLock lock(&_lock);
	
	if(!live) {
		if(fn.length()==0) return;
		if(fn==_file && IsLoaded()) {
			Log::Write(L"TJShow/VideoDeck", L"File already loaded: "+fn);
			return;
		}
	}
	Stop();

	// create graph
	if(FAILED(CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&_graph))) {
		Throw(L"Couldn't create filter graph", ExceptionTypeError);
	}

	// if we're trying to play a .grf-file, load it in a special way
	if(fn.length()>4 && fn.substr(fn.length()-4, 4)==L".grf") {
		if(FAILED(DXLoadGraphFile(_graph, fn.c_str()))) {
			Throw(L"Couldn't load GRF graph file", ExceptionTypeError);
		}
	}
	else {
		// Create and add texture renderer to graph
		if(FAILED(_graph->AddFilter(_renderer, L"TJ Texture Renderer"))) {
			Log::Write(L"TJShow/VideoDeck", L"Could not add texture renderer to graph");
		}

		// Add audio sampler filter to graph
		/*** if(FAILED(_graph->AddFilter(_sampler, L"TJ Audio Sampler"))) {
			Log::Write(L"TJShow/VideoDeck", L"Could not add audio sampler to graph");
		} ***/
		
		// select the sound card (if 0, let DShow choose for us)
		if(card) {
			if(!card.IsCastableTo<MediaDevice>()) {
				Log::Write(L"TJShow/VideoDeck", L"Wrong device for output sound card patched, using default!");
			}
			else {
				ref<MediaDevice> sd = card;
				if(sd) {
					_graph->AddFilter(sd->GetFilter(), L"TJShow Sound Output");
				}
			}
		}

		if(FAILED(_graph->QueryInterface(IID_IMediaControl, (void**)&_mc))) {
			Throw(L"Failed to create MediaControl", ExceptionTypeError);
		}

		if(FAILED(_graph->QueryInterface(IID_IMediaSeeking, (void**)&_ms))) {
			//Throw(L"Failed to create MediaSeeking", ExceptionTypeError);
			_ms = 0;
		}

		if(FAILED(_graph->QueryInterface(IID_IBasicAudio, (void**)&_ba))) {
			// Picture or video without audio
			_ba = 0;
		}

		CComPtr<IBaseFilter> inputFilter;
		HRESULT renderResult = E_UNEXPECTED;

		if(live) {
			if(live.IsCastableTo<MediaDevice>()) {
				CComPtr<IBaseFilter> filter = ref<MediaDevice>(live)->GetFilter();
				_graph->AddFilter(filter, L"TJ Live Video Source");
				CComPtr<IPin> inputOutPin = DXGetPin(filter, PINDIR_OUTPUT);
				renderResult = _graph->Render(inputOutPin);
			}
			else {
				Log::Write(L"TJShow/VideoDeck", L"Wrong device type for live input!");
			}
		}
		else {
			if(FAILED( _graph->AddSourceFilter(fn.c_str(), L"File", &inputFilter))) {
				// Probably a picture or stream; Try to render without file source filter
				renderResult = _graph->RenderFile(fn.c_str(),NULL);
			}
			else {
				CComPtr<IPin> inputOutPin = DXGetPin(inputFilter, PINDIR_OUTPUT);
				renderResult = _graph->Render(inputOutPin);
			}
		}

		if(renderResult==VFW_S_VIDEO_NOT_RENDERED||renderResult==VFW_S_PARTIAL_RENDER) {
			Log::Write(L"TJShow/VideoDeck", L"Partial render, not all parts of the video or audio may be working");
		}
		else if(renderResult==VFW_E_CANNOT_RENDER) {
			Throw(L"Couldn't render file (VFW_E_CANNOT_RENDER)", ExceptionTypeError);
		}
		else if(renderResult==VFW_E_UNKNOWN_FILE_TYPE) {
			Throw(L"Couldn't render file (VFW_E_UNKNOWN_FILE_TYPE)", ExceptionTypeError);
		}
		else if(FAILED(renderResult)) {
			Throw(L"Couldn't render file", ExceptionTypeError);
		}

		IVideoWindow* vw = 0;
		if(SUCCEEDED(_graph->QueryInterface(IID_IVideoWindow, (void**)&vw))) {
			vw->Release();
			Log::Write(L"TJShow/VideoDeck", L"Rendering to a video window outside the 3D renderer");
		}
	}

	_loaded = true;
	_file = fn;
}

bool VideoDeck::IsLoaded() const {
	ThreadLock lock(&_lock);
	return _loaded;
}

void VideoDeck::Stop() {
	ThreadLock lock(&_lock);
	if(_mc) {
		_mc->Stop();
	}

	// unload all stuff
	_graph = 0;
	_mc = 0;
	_ms = 0;
	_ba = 0;	

	_loaded = false;
	_playing = PlaybackStop;
	_file = L"";
}

void VideoDeck::Play() {
	ThreadLock lock(&_lock);
	if(!IsLoaded()) return;

	if(_mc) {
		if(!FAILED(_mc->Run())) {
			_playing = PlaybackPlay;
		}
	}
}

void VideoDeck::Pause() {
	ThreadLock lock(&_lock);
	if(!IsLoaded()) return;

	if(_mc) {
		_mc->Pause();
	}

	_playing = PlaybackPause;
}

void VideoDeck::Jump(Time t) {
	ThreadLock lock(&_lock);
	if(!IsLoaded()) return;

	if(_ms) {				
		LONGLONG cur = (LONGLONG)int(t)*10000;
		_ms->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME);
		_ms->SetPositions(&cur,AM_SEEKING_AbsolutePositioning, 0,0);
	}
}

float VideoDeck::GetAspectRatio() const {
	return 1.0f;
}

volatile bool VideoDeck::IsVisible() const {
	return _visible;
}

void VideoDeck::SetVisible(bool v) {
	ThreadLock lock(&_lock);
	_visible = v;
}

void VideoDeck::SetVolume(float v) {
	ThreadLock lock(&_lock);
	_volume = v;
	UpdateVolume();
}

void VideoDeck::SetBalance(int v) {
	ThreadLock lock(&_lock);

	if(!IsLoaded()) return;
	if(_ba) {
		_ba->put_Balance(v);
	}
}

void VideoDeck::SetOpacity(float v) {
	ThreadLock lock(&_lock);
	_alpha = v;
}

float VideoDeck::GetOpacity() const {
	ThreadLock lock(&_lock);
	return _alphaMixer.GetMultiplyMixValue(_alpha);
}

void VideoDeck::SetAudioOutput(bool m) {
	ThreadLock lock(&_lock);
	_muted = !m;
	UpdateVolume();
}

void VideoDeck::SetVideoOutput(bool v) {
	ThreadLock lock(&_lock);
	_videoOutput = v;
}

std::wstring VideoDeck::GetFileName() const {
	ThreadLock lock(&_lock);
	return _file;
}

void VideoDeck::SetPlaybackSpeed(float c) {
	ThreadLock lock(&_lock);
	if(!IsLoaded()) return;
	if(_ms) {
		_ms->SetRate(c);
	}
}

void VideoDeck::UpdateVolume() {
	ThreadLock lock(&_lock);
	if(!IsLoaded()) return;

	// DirectShow pretends to be logarithmic, but isn't really (at least on my PC); by squaring the volume
	// we decrease the sensitivity at the higher volume range.
	int vol = int(pow(1.0f-GetVolume(), 2)*-10000.0f);

	if(_ba) {
		_ba->put_Volume(_muted?-10000:vol);
	}
}

float VideoDeck::GetVolume() const {
	ThreadLock lock(&_lock);
	return _volumeMixer.GetMultiplyMixValue(_volume);
}

void VideoDeck::SetMixVolume(const std::wstring& ident, float v) {
	if(v<0.0f) v = 0.0f;
	if(v>1.0f) v = 1.0f;

	{
		ThreadLock lock(&_lock);
		_volumeMixer.SetMixValue(ident,v);
	}
	UpdateVolume();
}

float VideoDeck::GetMixVolume(const std::wstring& ident) {
	ThreadLock lock(&_lock);
	return _volumeMixer.GetMixValue(ident, 1.0f);
}

float VideoDeck::GetMixAlpha(const std::wstring& ident) {
	ThreadLock lock(&_lock);
	return _alphaMixer.GetMixValue(ident, 1.0f);
}

void VideoDeck::SetMixAlpha(const std::wstring& ident, float v) {
	if(v<0.0f) v = 0.0f;
	if(v>1.0f) v = 1.0f;

	ThreadLock lock(&_lock);
	_alphaMixer.SetMixValue(ident,v);
}

bool VideoDeck::HasVideo() {
	ThreadLock lock(&_lock);
	return (_renderer != 0) && _renderer->HasVideo();
}

int VideoDeck::GetLength() {
	ThreadLock lock(&_lock);
	if(!IsLoaded()) return 0;

	LONGLONG p = -1;
	_ms->GetDuration(&p);
	return int(p/10000);
}

int VideoDeck::GetPosition() {
	ThreadLock lock(&_lock);
	if(!IsLoaded()) return 0;

	if(_ms) {
		LONGLONG p, x;
		_ms->GetPositions(&p, &x);
		return int(p/10000);
	}
	return 0;
}

void VideoDeck::ListDevices(const IID& cat, std::vector< ref<Device> >& devlist, bool isSource, std::map<std::wstring, ref<Device> >& existing) {
	// Create system device enumerator
	ICreateDevEnum* pSysDevEnum = 0;
	if(FAILED(CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void **)&pSysDevEnum)) || pSysDevEnum==0) {
		return;
	}

	// Create class enumerator, we want to enumerate audio devices
	IEnumMoniker *pEnumCat = 0;
	if(FAILED(pSysDevEnum->CreateClassEnumerator(cat, &pEnumCat, 0)) || pEnumCat == 0) {
		return;
	}

	IMoniker *pMoniker = 0;
	IMalloc* ma = 0;
	if(FAILED(CoGetMalloc(1,&ma))) {
		return;
	}

	// Enumerate all monikers
	while(pEnumCat->Next(1, &pMoniker, 0L) == S_OK) {
		IBaseFilter* filter = 0;
		IPropertyBag *properties = 0;

		if(pMoniker!=0) {
			pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&properties);
			pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&filter);

			// Try to get friendly name, identifier and filter pointer and create a MediaDevice for it
			wchar_t* name = 0;
			pMoniker->GetDisplayName(0, 0, &name);
			
			VARIANT varName;
			VariantInit(&varName);

			if(SUCCEEDED(properties->Read(L"FriendlyName", &varName, 0)) && filter!=0) {
				if(existing.find(std::wstring(name))!=existing.end()) {
					devlist.push_back(existing[std::wstring(name)]);
				}
				else {
					devlist.push_back(GC::Hold(new MediaDevice(std::wstring(name), std::wstring(varName.bstrVal), filter, isSource)));
				}
				filter->Release();
			}
			else {
				Log::Write(L"TJShow/VideoDeck", L"Could not read friendly name for device");
			}

			VariantClear(&varName);
			ma->Free(name);
			properties->Release();
			pMoniker->Release();
		}
	}

	pEnumCat->Release();
	pSysDevEnum->Release();
}

void VideoDeck::ListDevices(std::vector< ref<Device> >& devlist, std::map<std::wstring, ref<Device> >& existing) {
	ListDevices(CLSID_AudioRendererCategory, devlist, false, existing);
	ListDevices(CLSID_VideoInputDeviceCategory, devlist, true, existing);
}

IBaseFilter* VideoDeck::GetSoundCard(unsigned char index) {
	// Create the System Device Enumerator.
	HRESULT hr;
	ICreateDevEnum *pSysDevEnum = NULL;
	hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void **)&pSysDevEnum);
	if(FAILED(hr)) {
		return 0;
	}

	IEnumMoniker *pEnumCat = NULL;
	hr = pSysDevEnum->CreateClassEnumerator(CLSID_AudioRendererCategory, &pEnumCat, 0);
	if(FAILED(hr)) {
		pSysDevEnum->Release();
		return 0;
	}

	IBaseFilter *pFilter;
	// Enumerate the monikers.
	IMoniker *pMoniker = NULL;
	ULONG cFetched;
	if(index>1) {
		if(pEnumCat->Skip(index-1)!=S_OK) {
			pSysDevEnum->Release();
			pEnumCat->Release();
			return 0;
		}
	}

	if(pEnumCat->Next(1, &pMoniker, &cFetched) == S_OK) {
		pMoniker->BindToObject(NULL, NULL, IID_IBaseFilter,(void**)&pFilter);
		pEnumCat->Release();
		pSysDevEnum->Release();
		pMoniker->Release();
		return pFilter;
	}

	return 0;
}

bool VideoDeck::IsPlaying() const {
	return _playing==PlaybackPlay;
}

bool VideoDeck::IsPaused() const {
	return _playing==PlaybackPause;
}

/* TextureDeck */
TextureDeck::~TextureDeck() {
}

TextureDeck::TextureDeck(): _translate(0.0f, 0.0f, 0.0f), _scale(1.0f, 1.0f, 1.0f), _rotate(0.0f, 0.0f, 0.0f) {
}

void TextureDeck::SetTranslate(const Vector& v) {
	_translate = v;
}

void TextureDeck::SetScale(const Vector& v) {
	_scale = v;
}

void TextureDeck::SetRotate(const Vector& v) {
	_rotate = v;
}

const Vector& TextureDeck::GetRotate() const {
	return _rotate;
}

const Vector& TextureDeck::GetScale() const {
	return _scale;
}

const Vector& TextureDeck::GetTranslate() const {
	return _translate;
}

float TextureDeck::GetTextureWidth() const {
	return 1.0f;
}

float TextureDeck::GetTextureHeight() const {
	return 1.0f;
}

/** MediaDevice */
MediaDevice::MediaDevice(const std::wstring& id, const std::wstring& friendly, IBaseFilter* filter, bool isSource): _friendlyName(friendly), _id(id), _filter(filter) {
	assert(_filter!=0);
	filter->AddRef();
	_icon = GC::Hold(new Icon(isSource ? L"icons/devices/video.png" : L"icons/devices/media.png"));
}

MediaDevice::~MediaDevice() {
	if(_filter!=0) {
		_filter->Release();
		_filter = 0;
	}
}

std::wstring MediaDevice::GetFriendlyName() const {
	return _friendlyName;
}

DeviceIdentifier MediaDevice::GetIdentifier() const {
	return _id;
}

ref<tj::shared::Icon> MediaDevice::GetIcon() {
	return _icon;
}

bool MediaDevice::IsMuted() const {
	return false;
}

void MediaDevice::SetMuted(bool t) const {
}

CComPtr<IBaseFilter> MediaDevice::GetFilter() {
	return _filter;
}