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
#include "../include/tjmedia.h"
using namespace tj::media::master;
using namespace tj::media;

MediaPlugin::~MediaPlugin() {
}

MediaPlugin::MediaPlugin(strong<master::MediaMasters> ms): _keyingSupported(false), _masters(ms) {
}

std::wstring MediaPlugin::GetName() const {
	return std::wstring(L"Media");
}

void MediaPlugin::GetRequiredFeatures(std::list<std::wstring>& fts) const {
	fts.push_back(L"MEDIA");
}

ref<Track> MediaPlugin::CreateTrack(ref<Playback> pb) {
	_keyingSupported = pb->IsKeyingSupported();
	return GC::Hold(new MediaTrack(pb, this, true));
}

bool MediaPlugin::IsKeyingSupported() const {
	return _keyingSupported;
}

ref<StreamPlayer> MediaPlugin::CreateStreamPlayer( ref<Playback> pb, ref<Talkback> talk) {
	return GC::Hold(new MediaStreamPlayer(this, pb));
}

strong<master::MediaMasters> MediaPlugin::GetMasters() {
	return _masters;
}

std::wstring MediaPlugin::GetFriendlyName() const {
	return TL(media_plugin_friendly_name);
}

std::wstring MediaPlugin::GetFriendlyCategory() const {
	return TL(media_category);
}

std::wstring MediaPlugin::GetDescription() const {
	return TL(media_plugin_description);
}

std::wstring MediaPlugin::GetVersion() const {
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

std::wstring MediaPlugin::GetAuthor() const {
	return std::wstring(L"Tommy van der Vorst");
}

void MediaPlugin::Reset() {
	_masters->Reset();
}

/** MediaAudioPlugin **/
// MediaAudioPlugin acts as a 'proxy' to MediaPlugin. The only thing it does is create MediaTrack with isVideo set to
// 'false', so its video features are disabled
MediaAudioPlugin::MediaAudioPlugin(ref<MediaPlugin> mp): _mp(mp) {
}

MediaAudioPlugin::~MediaAudioPlugin() {
}

ref<Track> MediaAudioPlugin::CreateTrack(ref<Playback> playback) {
	return GC::Hold(new MediaTrack(playback, _mp, false));
}

ref<StreamPlayer> MediaAudioPlugin::CreateStreamPlayer(ref<Playback> playback, ref<Talkback> talk) {
	return _mp->CreateStreamPlayer(playback,talk);
}

std::wstring MediaAudioPlugin::GetName() const {
	return L"Audio";
}

std::wstring MediaAudioPlugin::GetFriendlyName() const {
	return TL(media_audio_plugin_friendly_name);
}

std::wstring MediaAudioPlugin::GetFriendlyCategory() const {
	return _mp->GetFriendlyCategory();
}

void MediaAudioPlugin::GetRequiredFeatures(std::list<std::wstring>& fts) const {
	fts.push_back(L"MEDIA");
}

std::wstring MediaAudioPlugin::GetVersion() const {
	return _mp->GetVersion();
}

std::wstring MediaAudioPlugin::GetAuthor() const {
	return _mp->GetAuthor();
}

std::wstring MediaAudioPlugin::GetDescription() const {
	return TL(media_audio_plugin_description);
}

extern "C" { 
	__declspec(dllexport) std::vector<ref<Plugin> >* GetPlugins() {
		std::vector<ref<Plugin> >* plugins = new std::vector<ref<Plugin> >();

		strong<master::MediaMasters> ms = GC::Hold(new master::MediaMasters());

		ref<MediaPlugin> mp = GC::Hold(new MediaPlugin(ms));
		plugins->push_back(mp);
		plugins->push_back(GC::Hold(new MediaAudioPlugin(mp)));
		plugins->push_back(GC::Hold(new MediaMasterPlugin(ms)));
		plugins->push_back(GC::Hold(new ImagePlugin(ms)));
		plugins->push_back(GC::Hold(new TextImagePlugin(ms)));
		return plugins;
	}
}