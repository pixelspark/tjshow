#include "../include/tjbrowser.h"
#include "../../../Libraries/Awesomium/include/WebView.h"
#include "../../../Libraries/Awesomium/include/WebCore.h"
using namespace tj::browser;
using namespace Awesomium;
using namespace tj::shared::graphics;

Copyright KCRAwesomium(L"TJBrowser", L"Awesomium", L"© 2008-2009 Adam J. Simmons");

namespace tj {
	namespace browser {
		class BrowserWebViewListener: public Awesomium::WebViewListener {
			public:
				BrowserWebViewListener(ref<BrowserSurfacePlayer> bsp);
				~BrowserWebViewListener();
				void onBeginNavigation(const std::string& url, const std::wstring& frameName);
				void onCallback(const std::string& name, const Awesomium::JSArguments& args);
				void onReceiveTitle(const std::wstring& title, const std::wstring& frameName);
				void onChangeTooltip(const std::wstring& tooltip);
				void onChangeKeyboardFocus(bool isFocused);
				void onChangeTargetURL(const std::string& url);

				void onBeginLoading(const std::string& url, const std::wstring& frameName, int statusCode, const std::wstring& mimeType);
				void onFinishLoading();
				void onReceiveTitle(const std::wstring& title);

				#ifdef TJ_OS_WIN
					void onChangeCursor(const HCURSOR& cursor);
				#endif

			protected:	
				weak<BrowserSurfacePlayer> _bsp;
		};
	}
}

BrowserWebViewListener::BrowserWebViewListener(ref<BrowserSurfacePlayer> bsp): _bsp(bsp) {
}

BrowserWebViewListener::~BrowserWebViewListener() {
	Log::Write(L"TJBrowser/BWVL", L"Destroyed; end of session!");
}

void BrowserWebViewListener::onBeginLoading(const std::string& url, const std::wstring& frameName, int statusCode, const std::wstring& mimeType) {
	Log::Write(L"TJBrowser/BWVL", L"Begin loading");
}

void BrowserWebViewListener::onFinishLoading() {
	Log::Write(L"TJBrowser/BWVL", L"Finish loading");
	ref<BrowserSurfacePlayer> bsp = _bsp;
	if(bsp) {
		bsp->RepaintBrowser();
	}
}

void BrowserWebViewListener::onBeginNavigation(const std::string& url, const std::wstring& frameName) {
	Log::Write(L"TJBrowser/BrowserWebViewListener", L"begin nav: "+Wcs(url));
}

void BrowserWebViewListener::onCallback(const std::string& name, const Awesomium::JSArguments& args) {
}

void BrowserWebViewListener::onReceiveTitle(const std::wstring& title, const std::wstring& frameName) {
	Log::Write(L"TJBrowser/BrowserWebViewListener", L"Received title: "+title);
}

void BrowserWebViewListener::onChangeTooltip(const std::wstring& tooltip) {
}

void BrowserWebViewListener::onChangeKeyboardFocus(bool isFocused) {
}

void BrowserWebViewListener::onChangeTargetURL(const std::string& url) {
	Log::Write(L"TJBrowser/BrowserWebViewListener", L"Received url: "+Wcs(url));
}

void BrowserWebViewListener::onReceiveTitle(const std::wstring& title) {
	Log::Write(L"TJBrowser/BrowserWebViewListener", L"Received title: "+title);
}

void BrowserWebViewListener::onChangeCursor(const HCURSOR& cursor) {
}

BrowserSurfacePlayer::BrowserSurfacePlayer(ref<BrowserTrack> tr, ref<Stream> str): _width(512), _height(512) {
	_track = tr;
	_stream = str;
	_output = false;
	_core = new WebCore();
	_view = 0;
}

BrowserSurfacePlayer::~BrowserSurfacePlayer() {
	_view->destroy(); // this actually does 'delete this' on _view
	delete _core;
	delete _listener;
}

ref<Track> BrowserSurfacePlayer::GetTrack() {
	return _track;
}

void BrowserSurfacePlayer::Stop() {
	if(_output) {
		if(_stream) {
			ref<Message> msg = _stream->Create();
			msg->Add(BrowserActionHide);
			_stream->Send(msg);
		}
	}

	_surface = null;

	ThreadLock lock(&_browserLock);
	_view->setListener(0);
	delete _listener;
	_listener = 0;
	_view->destroy();
	_view = 0;
}

void BrowserSurfacePlayer::Start(Time pos, ref<Playback> playback, float speed) {
	if(_stream) {
		ref<Message> msg = _stream->Create();
		msg->Add(BrowserActionShow);
		_stream->Send(msg);
	}
	
	_surface = playback->CreateSurface(_width, _height);

	ThreadLock lock(&_browserLock);
	_surface->SetCanFocus(true);
	_surface->EventMouse.AddListener(this);
	_surface->EventKey.AddListener(this);
	_view = _core->createWebView(_width, _height);
	_listener = new BrowserWebViewListener(this);
	_surface->SetScale(Vector((float)_width,(float)_height,1.0f));
}

void BrowserSurfacePlayer::Notify(ref<Object> source, const Interactive::MouseNotification& mn) {
	ThreadLock lock(&_browserLock);
	if(_view!=0) {
		_view->injectMouseMove(int(mn._x*_width), int(mn._y*_height));
		if(mn._event==MouseEventLDown) {
			_view->injectMouseDown(LEFT_MOUSE_BTN);
		}
		else if(mn._event==MouseEventRDown) {
			_view->injectMouseDown(RIGHT_MOUSE_BTN);
		}
		else if(mn._event==MouseEventLUp) {
			_view->injectMouseUp(LEFT_MOUSE_BTN);
		}
		else if(mn._event==MouseEventRUp) {
			_view->injectMouseUp(RIGHT_MOUSE_BTN);
		}
	}
}

void BrowserSurfacePlayer::Notify(ref<Object> source, const Interactive::KeyNotification& mn) {
	ThreadLock lock(&_browserLock);
	if(_view!=0 && mn._down) {
		_view->injectKeyboardEvent(NULL, WM_CHAR, mn._code, 0);
	}
}

void BrowserSurfacePlayer::Navigate(const std::wstring& n) {
	if(_stream) {
		ref<Message> msg = _stream->Create();
		msg->Add(BrowserActionNavigate);
		msg->Add(n);
		_stream->Send(msg);
	}

	ThreadLock lock(&_browserLock);

	if(_view!=0) {
		_view->setListener(_listener);
		_view->loadURL(Mbs(n));
		_view->setListener(_listener);
		_surface->SetVisible(true);
	}
}

void BrowserSurfacePlayer::RepaintBrowser() {
	// This can be called from any thread (it is called from the BrowserWebViewListener, so that depends on the WebView implementation
	// So, we lock stuff to be sure
	ThreadLock lock(&_browserLock);

	if(_view!=0 && _view->isDirty()) {
		ref<SurfacePainter> sp = _surface->GetPainter();
		if(sp) {
			graphics::Bitmap* bmp = sp->GetBitmap();
			if(bmp!=0) {
				graphics::Rect rc(0, 0, bmp->GetWidth(), bmp->GetHeight());
				BitmapData bd;
				bmp->LockBits(rc, true, &bd);
				_view->render(bd.GetPointer(), bd.GetStride(), 4, 0);
				bmp->UnlockBits(&bd);
			}
		}
	}
}

void BrowserSurfacePlayer::Pause(Time pos) {
}

void BrowserSurfacePlayer::Tick(Time currentPosition) {
	ref<BrowserCue> cur = _track->GetCueBefore(currentPosition);
	if(cur!=_current) {
		_current = cur;
		if(cur) {
			Navigate(cur->GetURL());
		}
	}

	RepaintBrowser();
}

void BrowserSurfacePlayer::Jump(Time t, bool pause) {
}

Time BrowserSurfacePlayer::GetNextEvent(Time t) {
	ref<BrowserCue> cue = _track->GetCueAfter(t);
	if(cue) {
		return cue->GetTime();
	}

	/* This is for repainting the browser. It seems like the WebViewListener doesn't do anything
	 yet, so as a temporary workaround, we repaint every 100ms (if the view is dirty, which is a
	 cheap test I believe) 
	 
	 TODO: remove this when Awesomium WebViewListener works like it should
	 */
	return t+Time(100);
}

void BrowserSurfacePlayer::SetPlaybackSpeed(Time t, float c) {
}

void BrowserSurfacePlayer::SetOutput(bool enable) {
	_output = enable;
}