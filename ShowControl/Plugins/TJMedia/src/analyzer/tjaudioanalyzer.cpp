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
#include "../../include/analyzer/tjaudioanalyzer.h"
#include <TJShared/include/tjshared.h>
#include <TJSharedUI/include/tjsharedui.h>
#undef DLLEXPORT
#include <DirectShow/Streams/streams.h>
#include <initguid.h>
#pragma include_alias( "dxtrans.h", "qedit.h" )
#define __IDxtCompositor_INTERFACE_DEFINED__
#define __IDxtAlphaSetter_INTERFACE_DEFINED__
#define __IDxtJpeg_INTERFACE_DEFINED__
#define __IDxtKey_INTERFACE_DEFINED__
#include <qedit.h>
#include <atlbase.h>

using namespace tj::shared;
using namespace tj::media::analyzer;
using namespace tj::shared::graphics;

ref<AudioAnalyzer> AudioAnalyzer::_instance;
const int AudioAnalysis::KResolutionMS = 40; // 25Hz

// If there's more than an hour of media, stop after an hour. This already is ~400 kB
const Time AudioAnalysis::KMaximumLength(3600*1000); 

namespace tj {
	namespace media {
		namespace analyzer {
			class AudioAnalysisTask: public Task {
				public:
					AudioAnalysisTask(strong<AudioAnalysis> aa);
					virtual ~AudioAnalysisTask();
					virtual void Run();
					virtual void Analyze();

				protected:
					strong<AudioAnalysis> _analysis;
					const static Bytes KMaxCacheDirectorySize;
					String _cacheDirectory;
			};

			const Bytes AudioAnalysisTask::KMaxCacheDirectorySize = 1024 * 1024 * 10; // 10MB
		}
	}
}

class CSampleGrabberCB : public ISampleGrabberCB {
	public:
		CSampleGrabberCB(const Time& duration): _duration(duration.ToInt()), _data(0) {
			const static int maxPeaks = AudioAnalysis::KMaximumLength.ToInt() / AudioAnalysis::KResolutionMS;
			_dataSize = min(maxPeaks, (_duration / AudioAnalysis::KResolutionMS) + AudioAnalysis::KResolutionMS);
			_data = new unsigned char[_dataSize];
		}

		virtual ~CSampleGrabberCB() {
			// _data is transferred to the AudioAnalysis object, and should not be deleted here
		}

		STDMETHODIMP_(ULONG) AddRef() {
			return 2;
		}

		STDMETHODIMP_(ULONG) Release() {
			return 1;
		}

		STDMETHODIMP QueryInterface(REFIID riid, void ** ppv) {
			CheckPointer(ppv,E_POINTER);

			if(riid == IID_ISampleGrabberCB || riid == IID_IUnknown)  {
				*ppv = (void *)static_cast<ISampleGrabberCB*>(this);
				return NOERROR;
			}
			return E_NOINTERFACE;
		}

		STDMETHODIMP SampleCB(double SampleTime, IMediaSample * pSample) {
			return 0;
		}

		STDMETHODIMP BufferCB(double sampleTime, BYTE * pBuffer, long BufferSize) {
			unsigned int pos = (unsigned int)floor((double(sampleTime) * 1000.0) / AudioAnalysis::KResolutionMS);
			if(pos < _dataSize) {
				// calculate peak
				int* buffer = reinterpret_cast<int*>(pBuffer);
				float maxValue = 0.0;
				for(long a=0;a<BufferSize/4;a++) {
					maxValue = max(maxValue, (float)buffer[a]);
				}

				//maxValue = log(maxValue);				
				_data[pos] = (unsigned char)(maxValue / (UINT_MAX / UCHAR_MAX));
			}
			return 0;
		}

		int _duration;
		unsigned char* _data;
		unsigned int _dataSize;
};

/** AudioAnalysisTask **/
AudioAnalysisTask::AudioAnalysisTask(strong<AudioAnalysis> as): _analysis(as) {
}

AudioAnalysisTask::~AudioAnalysisTask() {
}

void AudioAnalysisTask::Run() {
	// Create directory for cached peak data
	wchar_t tp[MAX_PATH+2];
	::GetTempPath(MAX_PATH, tp);
	_cacheDirectory = std::wstring(tp) + L"TJShow\\PeakData\\";
	::CreateDirectory(_cacheDirectory.c_str(), NULL);

	// Check if our directory with cached peak files isn't getting too big;
	// if it is, do something about it
	Bytes cacheSize = File::GetDirectorySize(_cacheDirectory);
	if(cacheSize > KMaxCacheDirectorySize) {
		Log::Write(L"TJMedia/Analyzer", L"Peak file cache directory got too big, erasing all cache files");
		File::DeleteFiles(_cacheDirectory, L"*.peaks.tj");
	}

	Analyze();
}

HRESULT GetPin(IBaseFilter * pFilter, PIN_DIRECTION dirrequired, int iNum, IPin **ppPin) {
	CComPtr< IEnumPins > pEnum;
	*ppPin = NULL;

	HRESULT hr = pFilter->EnumPins(&pEnum);
	if(FAILED(hr)) return hr;

	ULONG ulFound;
	IPin *pPin;
	hr = E_FAIL;

	while (S_OK == pEnum->Next(1, &pPin, &ulFound)) {
		PIN_DIRECTION pindir = (PIN_DIRECTION)3;
		pPin->QueryDirection(&pindir);
		if (pindir == dirrequired) {
			if (iNum == 0) {
				*ppPin = pPin;  // Return the pin's interface
				hr = S_OK;	  // Found requested pin, so clear error
				break;
			}
			iNum--;
		} 
		pPin->Release();
	}

	return hr;
}

IPin* GetInPin(IBaseFilter * pFilter, int nPin) {
	CComPtr<IPin> pComPin = 0;
	GetPin(pFilter, PINDIR_INPUT, nPin, &pComPin);
	return pComPin;
}

IPin* GetOutPin(IBaseFilter * pFilter, int nPin) {
	CComPtr<IPin> pComPin = 0;
	GetPin(pFilter, PINDIR_OUTPUT, nPin, &pComPin);
	return pComPin;
}

void AudioAnalysisTask::Analyze() {
	std::wstring path = _analysis->GetFilePath();
	Log::Write(L"TJMedia/Analyzer", L"Threaded analysis started for file "+path);

	Time duration;
	// Calculate duration first
	try {
		duration = MediaUtil::GetDuration(path);

		{
			ThreadLock lock(&(_analysis->_lock));
			_analysis->_duration = duration;
		}
	}
	catch(const Exception& e) {
		Log::Write(L"TJMedia/Analyzer", L"Error occurred in threaded analysis for file "+path+L": "+e.GetMsg());
	}

	// Try to find a cache peak data file, so we don't have to calculate all peaks
	// Calculate a hash first to identify the cache file that belongs to this file
	Hash hasher;
	std::wostringstream wos;
	WIN32_FILE_ATTRIBUTE_DATA fatd;
	
	if(!GetFileAttributesEx(path.c_str(), GetFileExInfoStandard, &fatd)) {
		Throw(L"Could not get file attributes for file to be analyzed; not analyzing peaks!", ExceptionTypeError);
	}
	wos << _cacheDirectory << L'\\' << std::hex << hasher.Calculate(path) << L'-' << std::hex << fatd.nFileSizeHigh << std::hex << fatd.nFileSizeLow << std::hex << fatd.ftLastWriteTime.dwHighDateTime << std::hex << fatd.ftLastWriteTime.dwLowDateTime << L".peaks.tj";
	std::wstring cacheFilePath = wos.str();
	Log::Write(L"TJMedia/AudioAnalyzer", L"Cache file path is "+cacheFilePath);
	
	if(GetFileAttributes(cacheFilePath.c_str())!=INVALID_FILE_ATTRIBUTES) {
		// A cache file exists, load it!
		if(_analysis->LoadPeaksFromFile(cacheFilePath)) {
			return; // Analysis done
		}
		else {
			Log::Write(L"TJMedia/AudioAnalyzer", L"Peak data file cannot be used; deleting it!");
			DeleteFile(cacheFilePath.c_str());
		}
	}

	// Calculate peaks
	try {
		CComPtr<IGraphBuilder> gb;
		if(SUCCEEDED(gb.CoCreateInstance(CLSID_FilterGraph))) {
			CComPtr<IBaseFilter> nullRenderer;
			if(SUCCEEDED(nullRenderer.CoCreateInstance(CLSID_NullRenderer))) {
				gb->AddFilter(nullRenderer, L"NR");
				
				HRESULT hr = 0;
				CComPtr<ISampleGrabber> sampleGrabber;
				if(FAILED(sampleGrabber.CoCreateInstance(CLSID_SampleGrabber))) {
					Throw(L"Could not create sample grabber", ExceptionTypeError);
				}

				CMediaType grabType;
				grabType.SetType(&MEDIATYPE_Audio);
				grabType.SetSubtype(&MEDIASUBTYPE_PCM);
				grabType.SetFormatType(&FORMAT_WaveFormatEx);

				WAVEFORMATEX wex;
				wex.wFormatTag = WAVE_FORMAT_PCM;
				wex.cbSize = 0;
				wex.nAvgBytesPerSec = 44100 * 4;
				wex.nBlockAlign = 4;
				wex.nChannels = 2;
				wex.nSamplesPerSec = 44100;
				wex.wBitsPerSample = 16;
				grabType.SetFormat((BYTE*)&wex, sizeof(WAVEFORMATEX));

				if(FAILED(sampleGrabber->SetMediaType(&grabType))) {
					Throw(L"Could not set grabber media type", ExceptionTypeError);
				}

				CComQIPtr<IBaseFilter> samplerBase(sampleGrabber);
				gb->AddFilter(samplerBase, L"GRABBER");

				CComPtr<IBaseFilter> sourceFilter;
				if(FAILED(gb->AddSourceFilter(path.c_str(), NULL, &sourceFilter))) {
					Throw(L"Could not add source filter", ExceptionTypeError);
				}

				// Connect some pins
				CComPtr<IPin> sourceOutPin = GetOutPin(sourceFilter, 0);
				CComPtr<IPin> grabInPin = GetInPin(samplerBase, 0);
				CComPtr<IPin> grabOutPin = GetOutPin(samplerBase, 0);
				CComPtr<IPin> nullInPin = GetInPin(nullRenderer, 0);

				if(FAILED(gb->Connect(sourceOutPin, grabInPin))) {
					Throw(L"Could not connect pins", ExceptionTypeError);
				}
				
				if(FAILED(gb->Connect(grabOutPin, nullInPin))) {
					Throw(L"Could not connect pins (2)", ExceptionTypeError);
				}
				
				sampleGrabber->SetBufferSamples(FALSE);
				sampleGrabber->SetOneShot(FALSE);

				// Set the callback, so we can grab the one sample
				CSampleGrabberCB cb(duration);
				sampleGrabber->SetCallback(&cb, 1);

				// Keep a useless clock from being instantiated....
				CComQIPtr<IMediaFilter, &IID_IMediaFilter> mediaFilter(gb);
				if(FAILED(mediaFilter->SetSyncSource(NULL))) {
					Throw(L"Failed to set sync source", ExceptionTypeError);
				}

				CComQIPtr<IVideoWindow, &IID_IVideoWindow> videoWindow(gb);
				if (videoWindow) {
					videoWindow->put_AutoShow(OAFALSE);
				}

				// activate the threads		
				CComQIPtr<IMediaControl, &IID_IMediaControl> mediaControl(gb);
				mediaControl->Run();
				
				CComQIPtr<IMediaEvent> ev(gb);
				if(ev) {
					long evc = 0;
					ev->WaitForCompletion(INFINITE, &evc);
				}

				// Move peaks buffer into AudioAnalysis
				{
					ThreadLock lock(&(_analysis->_lock));
					_analysis->SetPeakData(cb._data, cb._dataSize);
				}
			}
			else {
				Throw(L"Couldn't create null renderer", ExceptionTypeError);
			}
		}
		else {
			Throw(L"Couldn't create graph builder", ExceptionTypeError);
		}
	}
	catch(const Exception& e) {
		Log::Write(L"TJMedia/Analyzer", L"Error occurred in threaded analysis for file "+path+L": "+e.GetMsg());
	}

	_analysis->SavePeaksToFile(cacheFilePath);
	Log::Write(L"TJMedia/Analyzer", L"Threaded analysis finished for file "+path);
}

/** AudioAnalysis **/
AudioAnalysis::~AudioAnalysis() {
}

AudioAnalysis::AudioAnalysis(const std::wstring& fp): _file(fp), _peaksSize(0), _peaks(0) {
}

bool AudioAnalysis::HasPeaks() const {
	return _peaks != 0 && _peaksSize > 0;
}

void AudioAnalysis::SetPeakData(unsigned char* data, unsigned int sz) {
	ThreadLock lock(&_lock);

	if(_peaks!=0) {
		delete[] _peaks;
	}

	// We can 'take over' the buffer, since CSamplerGrabberCB doesn't delete[] it
	_peaks = data;
	_peaksSize = sz;
}

void AudioAnalysis::SavePeaksToFile(const std::wstring& realPath) {
	if(HasPeaks()) {
		HANDLE file = ::CreateFile(realPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, NULL, NULL);
		if(file!=INVALID_HANDLE_VALUE) {
			DWORD written = 0;

			// Write version tag (first four bytes of file)
			char buffer[4] = {'T', 'J', 'P', '1'};
			if(!WriteFile(file, buffer, 4, &written, NULL)) {
				Log::Write(L"TJMedia/Analyzer", L"Could not write version tag to peak data file");
			}
			else {
				if(!WriteFile(file, _peaks, _peaksSize, &written, NULL)) {
					Log::Write(L"TJMedia/AudioAnalyzer", L"Could not save peak data to file (WriteFile failed)");
				}
				if(written!=_peaksSize) {
					Log::Write(L"TJMedia/AudioAnalyzer", L"Could not save all peak data (file may be incomplete)");
				}
			}
			CloseHandle(file);
		}
		else {
			Log::Write(L"TJMedia/AudioAnalyzer", L"Could not save peak data to file (CreateFile failed)");
		}
	}
}

bool AudioAnalysis::LoadPeaksFromFile(const std::wstring& realPath) {
	const static unsigned long maxPeaks = KMaximumLength.ToInt() / KResolutionMS;

	HANDLE file = ::CreateFile(realPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if(file!=INVALID_HANDLE_VALUE) {
		DWORD size = GetFileSize(file, NULL);
		if(size!=INVALID_FILE_SIZE) {
			DWORD read = 0;
			char versionBuffer[4];
			if(!ReadFile(file, versionBuffer, 4, &read, NULL)) {
				Log::Write(L"TJMedia/AudioAnalyzer", L"Could not load file version in peak data file");
				return false;
			}
			else {
				if(versionBuffer[0]!='T' || versionBuffer[1]!='J' || versionBuffer[2]!='P' || versionBuffer[3]!='1') {
					// Wrong version
					return false;
				}
				else {
					int bytesToRead = min(maxPeaks, size);
					unsigned char* buffer = new unsigned char[bytesToRead];
					
					if(!ReadFile(file, buffer, bytesToRead, &read, NULL) || read==bytesToRead) {
						Log::Write(L"TJMedia/AudioAnalyzer", L"Could not load all peak data from file (ReadFile failed or was only partial)");
						return false;
					}
					SetPeakData(buffer, read);
				}
			}
			CloseHandle(file);
		}
		else {
			Log::Write(L"TJMedia/AudioAnalyzer", L"Could not load peak data from file (GetFileSize failed)");
			return false;
		}
	}
	else {
		Log::Write(L"TJMedia/AudioAnalyzer", L"Could not load peak data from file (CreateFile failed)");
		return false;
	}
	return true;
}

Time AudioAnalysis::GetDuration() {
	ThreadLock lock(&_lock);
	return _duration;
}

std::wstring AudioAnalysis::GetFilePath() const {
	return _file;
}

/** AudioAnalyzer **/
strong<AudioAnalyzer> AudioAnalyzer::Instance() {
	if(!_instance) {
		_instance = GC::Hold(new AudioAnalyzer());
	}
	return _instance;
}

AudioAnalyzer::AudioAnalyzer() {
}

AudioAnalyzer::~AudioAnalyzer() {
}

ref<AudioAnalysis> AudioAnalyzer::_Analyze(const std::wstring& realPath) {
	// Don't even bother if the file does not exist
	if(GetFileAttributes(realPath.c_str())==INVALID_FILE_ATTRIBUTES) {
		return null;
	}

	ref<AudioAnalysis> aa = GC::Hold(new AudioAnalysis(realPath));
	ref<AudioAnalysisTask> at = GC::Hold(new AudioAnalysisTask(aa));
	Dispatcher::CurrentOrDefaultInstance()->Dispatch(ref<Task>(at));
	return aa;
}

ref<AudioAnalysis> AudioAnalyzer::Analyze(const std::wstring& realPath) {
	strong<AudioAnalyzer> aa = Instance();
	return aa->_Analyze(realPath);
}