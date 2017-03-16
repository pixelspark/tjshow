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
#include "../include/internal/tjshow.h"
#include "../include/internal/tjsubtimeline.h"

PluginWrapper::PluginWrapper(ref<Plugin> plugin, HMODULE module, const tj::np::PluginHash& hash, const std::wstring& hashName) {
	assert(plugin);
	_plugin = plugin;
	_module = module;
	_hash = hash;
	_hashName = hashName;
}

void PluginWrapper::Reset() {
	try {
		_plugin->Reset();
	}
	catch(Exception& e) {
		Log::Write(L"TJShow/PluginWrapper/Reset", e.GetMsg());
	}
	catch(...) {
	}
}

PluginWrapper::~PluginWrapper() {
}

PluginHash PluginWrapper::GetHash() const {
	return _hash;
}

std::wstring PluginWrapper::GetHashName() const {
	return _hashName;
}

HMODULE PluginWrapper::GetModule() {
	return _module;
}

ref<Plugin> PluginWrapper::GetPlugin() {
	return _plugin;
}

std::wstring PluginWrapper::GetFriendlyName() {
	return _plugin->GetFriendlyName();
}

std::wstring PluginWrapper::GetDescription() {
	return _plugin->GetDescription();
}

ref<Pane> PluginWrapper::GetSettingsWindow() {
	try {
		return _plugin->GetSettingsWindow(Application::Instance()->GetView()->GetPropertyGridProxy());
	}
	catch(Exception& e) {
		Log::Write(L"TJShow/PluginWrapper/GetSettingsWindow", e.GetMsg());
		return 0;
	}
	catch(...) {
		return 0;
	}
}

bool PluginWrapper::IsOutputPlugin() {
	return _plugin.IsCastableTo<OutputPlugin>();
}

std::wstring PluginWrapper::GetPluginID(ref<Plugin> plug, const std::wstring& id) {
	return id + L"." + plug->GetName();
}

ref<StreamPlayer> PluginWrapper::CreateStreamPlayer(ref<Playback> om, ref<Talkback> t) {
	try {
		if(IsOutputPlugin()) {
			return ref<OutputPlugin>(_plugin)->CreateStreamPlayer(om,t);
		}
	}
	catch(Exception& e) {
		Log::Write(L"TJShow/PluginWrapper/GetSettingsWindow", e.GetMsg());
		return 0;
	}
	catch(...) {
		return 0;
	}

	return 0;
}

void PluginWrapper::Message(ref<DataReader> code) {
	if(IsOutputPlugin()) {
		ref<OutputPlugin> op = _plugin;
		op->Message(code);
	}
}

ref<TrackWrapper> PluginWrapper::CreateTrack(ref<Playback> pb, ref<Network> nw, ref<Instance> instance) {
	if(_plugin.IsCastableTo<OutputPlugin>()) {
		ref<OutputPlugin> op = _plugin;
		ref<Track> track = op->CreateTrack(pb);

		if(track) {
			TrackWrapper* trackWrapper = new TrackWrapper(track, ref<PluginWrapper>(this), nw, instance);
			return GC::Hold(trackWrapper);
		}
	}
	return null;
}