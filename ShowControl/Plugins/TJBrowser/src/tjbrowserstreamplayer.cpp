#include "../include/tjbrowser.h"
using namespace tj::browser;

/* BrowserStreamPlayer */
BrowserStreamPlayer::BrowserStreamPlayer(ref<BrowserPlugin> p) {
	_plugin = p;
}

BrowserStreamPlayer::~BrowserStreamPlayer() {
}

ref<Plugin> BrowserStreamPlayer::GetPlugin() {
	return _plugin;
}

void BrowserStreamPlayer::Message(ref<Code> msg, ref<Playback> pb) {
	// TODO: implement all this using the fancy new surface browser renderer
	unsigned int pos = 0;
	BrowserAction action = msg->Get<BrowserAction>(pos);
	switch(action) {
		case BrowserActionHide:
			///_browser->Show(false);
			break;

		case BrowserActionShow: {
			///_browser->Show(true);
			break;
		}

		case BrowserActionNavigate: {
			std::wstring url = msg->Get<std::wstring>(pos);
			Log::Write(L"TJBrowser/StreamPlayer", L"Navigate to "+url);
			///_browser->GetBrowser()->Navigate(url);
			break;
		}
		
		case BrowserActionNone:
		default:
			break;
	}
}