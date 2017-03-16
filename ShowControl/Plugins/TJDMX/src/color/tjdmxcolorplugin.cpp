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
#include "../../include/tjdmx.h"
#include "../../include/color/tjdmxcolor.h"
using namespace tj::dmx::color;

DMXColorPlugin::DMXColorPlugin(ref<DMXPlugin> plug): _plugin(plug) {
}

DMXColorPlugin::~DMXColorPlugin() {
}

std::wstring DMXColorPlugin::GetFriendlyName() const {
	return TL(dmx_color_plugin_friendly_name);
}

std::wstring DMXColorPlugin::GetVersion() const {
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

std::wstring DMXColorPlugin::GetAuthor() const {
	return L"Tommy van der Vorst";
}

std::wstring DMXColorPlugin::GetFriendlyCategory() const {
	return TL(dmx_category);
}

void DMXColorPlugin::GetRequiredFeatures(std::list<std::wstring>& fts) const {
	fts.push_back(L"DMX");
}

std::wstring DMXColorPlugin::GetDescription() const {
	return TL(dmx_color_plugin_description);
}

std::wstring DMXColorPlugin::GetName() const {
	return L"DMXColor";
}

ref<Track> DMXColorPlugin::CreateTrack(ref<Playback> pb) {
	if(pb->IsFeatureAvailable(L"DMX")) {
		return GC::Hold(new DMXColorTrack(_plugin));
	}
	return null;
}

ref<StreamPlayer> DMXColorPlugin::CreateStreamPlayer(ref<Playback> playback, ref<Talkback> talk) {
	return 0;
}