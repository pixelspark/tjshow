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
#include "../include/tjdmx.h"

DMXPositionPlugin::DMXPositionPlugin(ref<DMXPlugin> plugin) {
	_plugin = plugin;
}

DMXPositionPlugin::~DMXPositionPlugin() {

}
std::wstring DMXPositionPlugin::GetName() const {
	return L"DMX Position";
}

void DMXPositionPlugin::GetRequiredFeatures(std::list<std::wstring>& fts) const {
	fts.push_back(L"DMX");
}

ref<Track> DMXPositionPlugin::CreateTrack(ref<Playback> pb) {
	if(pb->IsFeatureAvailable(L"DMX")) {
		return GC::Hold(new DMXPositionTrack(_plugin));
	}
	return null;
}

ref<StreamPlayer> DMXPositionPlugin::CreateStreamPlayer(ref<Playback> pb, ref<Talkback> talk) {
	return 0;
}

std::wstring DMXPositionPlugin::GetFriendlyName() const {
	return TL(dmx_position_friendly_name);
}

std::wstring DMXPositionPlugin::GetFriendlyCategory() const {
	return TL(dmx_category);
}


std::wstring DMXPositionPlugin::GetDescription() const {
	return TL(dmx_position_description);
}

std::wstring DMXPositionPlugin::GetVersion() const {
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

std::wstring DMXPositionPlugin::GetAuthor() const {
	return L"Tommy van der Vorst";
}