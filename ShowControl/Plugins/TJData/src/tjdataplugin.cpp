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
#include "../include/tjdataplugin.h"

using namespace tj::show;
using namespace tj::shared;

DataPlugin::DataPlugin() {
}

DataPlugin::~DataPlugin() {
}

std::wstring DataPlugin::GetName() const {
	return L"Data";
}

std::wstring DataPlugin::GetFriendlyName() const {
	return TL(data_plugin_friendly_name);
}

void DataPlugin::GetRequiredFeatures(std::list<std::wstring>& fts) const {
	fts.push_back(L"DATA");
}

std::wstring DataPlugin::GetFriendlyCategory() const {
	return TL(data_category);
}

std::wstring DataPlugin::GetVersion() const {
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

std::wstring DataPlugin::GetAuthor() const {
	return std::wstring(L"Tommy van der Vorst");
}

std::wstring DataPlugin::GetDescription() const {
	return TL(data_plugin_description);
}

ref<Track> DataPlugin::CreateTrack(ref<Playback> playback) {
	return GC::Hold(new DataTrack(playback));
}

ref<StreamPlayer> DataPlugin::CreateStreamPlayer(ref<Playback> playback, ref<Talkback> talk) {
	return null;
}

extern "C" { 
	__declspec(dllexport) std::vector<ref<Plugin> >* GetPlugins() {
		std::vector<ref<Plugin> >* plugins = new std::vector<ref<Plugin> >();
		plugins->push_back(GC::Hold(new DataPlugin()));
		return plugins;
	}
}