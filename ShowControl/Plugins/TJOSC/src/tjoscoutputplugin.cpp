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
#include "../include/tjosc.h"
#include "../include/tjoscoutputplugin.h"
#include "../include/tjosccuetrack.h"

using namespace tj::show;
using namespace tj::shared;
using namespace tj::osc;

OSCOutputPlugin::OSCOutputPlugin() {
}

OSCOutputPlugin::~OSCOutputPlugin() {
}

void OSCOutputPlugin::GetRequiredFeatures(std::list<std::wstring>& fts) const {
	fts.push_back(L"OSC");
}

std::wstring OSCOutputPlugin::GetName() const {
	return L"OSC Output";
}

std::wstring OSCOutputPlugin::GetFriendlyName() const {
	return TL(osc_output_plugin_friendly_name);
}

std::wstring OSCOutputPlugin::GetFriendlyCategory() const {
	return TL(osc_category);
}

std::wstring OSCOutputPlugin::GetVersion() const {
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

std::wstring OSCOutputPlugin::GetAuthor() const {
	return std::wstring(L"Tommy van der Vorst");
}

std::wstring OSCOutputPlugin::GetDescription() const {
	return TL(osc_output_plugin_description);
}

ref<Track> OSCOutputPlugin::CreateTrack(ref<Playback> playback) {
	return GC::Hold(new OSCOutputCueTrack(playback));
}

ref<StreamPlayer> OSCOutputPlugin::CreateStreamPlayer(ref<Playback> playback, ref<Talkback> talk) {
	return 0;
}