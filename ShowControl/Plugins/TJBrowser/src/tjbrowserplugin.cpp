#include "../include/tjbrowser.h"
using namespace tj::browser;

BrowserPlugin::BrowserPlugin() {
}

BrowserPlugin::~BrowserPlugin() {
}

std::wstring BrowserPlugin::GetName() const {
	return L"Browser";
}

std::wstring BrowserPlugin::GetFriendlyName() const {
	return TL(browser_plugin_friendly_name);
}

std::wstring BrowserPlugin::GetFriendlyCategory() const {
	return TL(other_category);
}

std::wstring BrowserPlugin::GetDescription() const {
	return TL(browser_plugin_description);
}

ref<Track> BrowserPlugin::CreateTrack(ref<Playback> playback) {
	return GC::Hold(new BrowserTrack(this));
}

ref<StreamPlayer> BrowserPlugin::CreateStreamPlayer(ref<Playback> playback, ref<Talkback> talk) {
	return GC::Hold(new BrowserStreamPlayer(this));
}

std::wstring BrowserPlugin::GetVersion() const {
	std::wostringstream os;
	os << __DATE__ << " @ " << __TIME__;
	#ifdef UNICODE
	os << L" Unicode";
	#endif

	#ifdef NDEBUG
	os << " Release";
	#endif

	return os.str();
}
std::wstring BrowserPlugin::GetAuthor() const {
	return L"Tommy van der Vorst";
}