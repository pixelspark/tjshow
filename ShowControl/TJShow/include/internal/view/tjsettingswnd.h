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
#ifndef _TJSETTINGSWND_H
#define _TJSETTINGSWND_H

#include "tjclientswnd.h"

namespace tj {
	namespace show {
		namespace view {
			/** The naming here is a bit confusing. 'Preferences' are application settings (stored in settings.xml). 'Settings'
			are show-specific settings (like patching, input rules, plug-in settings screens) and their data is stored in the tsx **/
			class PreferencesInspectable: public Inspectable {
				public:
					PreferencesInspectable(ref<Settings> st);
					virtual ~PreferencesInspectable();
					virtual ref<PropertySet> GetProperties();
					bool ChangesNeedRestart();

				private:
					SettingsMarshal<int> _netPort;
					SettingsMarshal<std::wstring> _netAddress;
					SettingsMarshal<bool> _enableEPEndpoint;
					SettingsMarshal<bool> _advertiseFiles;
					SettingsMarshal<int> _tickLength;
					SettingsMarshal<int> _minTickLength;
					SettingsMarshal<bool> _tooltips;
					SettingsMarshal<bool> _resourceWarnings;
					SettingsMarshal<bool> _noClientWarnings;
					SettingsMarshal<bool> _debug;
					SettingsMarshal<int> _defaultTrackHeight;
					SettingsMarshal<bool> _enableAnimations;
					SettingsMarshal<bool> _stickyFaders;
					ref<Settings> _st;
					LocaleIdentifier _locale;
			};

			class InputSettingsWnd;
			class PatchWnd;
			class PatchListWnd;


			class SettingsWnd: public ChildWnd {
				public:
					SettingsWnd();
					virtual ~SettingsWnd();
					virtual void Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme);
					virtual void Layout();
					virtual void Update();
					virtual void OnSize(const Area& newSize);
					void ShowPatcher();

				protected:
					virtual void OnSettingsChanged();

					ref<TabWnd> _tabs;
					std::map< PluginHash, ref<Pane> > _pluginSettingsWindows;
					ref<InputSettingsWnd> _inputSettings;
					ref<ClientsWnd> _clientsWnd;
					ref<GroupsWnd> _groupsWnd;
					ref<PatchWnd> _patchWnd;
			};

			class PatchWnd: public ChildWnd {
				public:
					PatchWnd(ref<Patches> p);
					virtual ~PatchWnd();
					virtual void Layout();
					virtual void OnSize(const Area& ns);
					virtual void Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme);
					virtual PatchIdentifier GetSelectedPatch();
					virtual ref<Patches> GetPatches();
					virtual void Update();
					virtual void OnSettingsChanged();
					virtual void SetClient(ref<Client> c);
					virtual ref<Client> GetClient();
					
				protected:
					ref<PatchListWnd> _list;
					ref<ToolbarWnd> _tools;
					ref<Patches> _patches;
			};
		}
	}
}

#endif