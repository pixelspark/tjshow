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
#include "../../include/tjmidi.h"
using namespace tj::show;
using namespace tj::shared;

MSCPlugin::MSCPlugin() {
}

MSCPlugin::~MSCPlugin() {
}

void MSCPlugin::GetRequiredFeatures(std::list<std::wstring>& fts) const {
	fts.push_back(L"MIDI");
}

std::wstring MSCPlugin::GetName() const {
	return L"MSC";
}

std::wstring MSCPlugin::GetFriendlyName() const {
	return TL(midi_msc_plugin_friendly_name);
}

std::wstring MSCPlugin::GetFriendlyCategory() const {
	return TL(midi_category);
}

std::wstring MSCPlugin::GetVersion() const {
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

std::wstring MSCPlugin::GetAuthor() const {
	return std::wstring(L"Tommy van der Vorst");
}

std::wstring MSCPlugin::GetDescription() const {
	return TL(midi_msc_plugin_description);
}

ref<Track> MSCPlugin::CreateTrack(ref<Playback> playback) {
	return GC::Hold(new MSCTrack(playback));
}

ref<StreamPlayer> MSCPlugin::CreateStreamPlayer(ref<Playback> playback, ref<Talkback> talk) {
	return 0;
}