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
#ifndef _TJPLUGIN_WRAPPER_H
#define _TJPLUGIN_WRAPPER_H

namespace tj {
	namespace show {
		class TrackWrapper;
		class Network;
		class Instance;

		class PluginWrapper: public virtual Object {
			public:
				PluginWrapper(ref<Plugin> plugin, HMODULE module, const tj::np::PluginHash& hash, const std::wstring& hashName);
				virtual ~PluginWrapper();
				HMODULE GetModule();
				ref<Plugin> GetPlugin();
				ref<TrackWrapper> CreateTrack(ref<Playback> pb, ref<Network> nw, ref<Instance> controller);
				PluginHash GetHash() const;
				std::wstring GetHashName() const;
				std::wstring GetDescription();
				std::wstring GetFriendlyName();
				ref<Pane> GetSettingsWindow();
				bool IsOutputPlugin();
				ref<StreamPlayer> CreateStreamPlayer(ref<Playback> om, ref<Talkback> talk);
				void Message(ref<DataReader> code);
				void Reset();
				static std::wstring GetPluginID(ref<Plugin> p, const std::wstring& moduleFileName);

			protected:
				ref<Plugin> _plugin;
				HMODULE _module;
				tj::np::PluginHash _hash;
				std::wstring _hashName;
		};
	}
}

#endif