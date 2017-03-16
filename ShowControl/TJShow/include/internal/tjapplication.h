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
#ifndef _TJAPPLICATION_H
#define _TJAPPLICATION_H

#include "tjnetwork.h"
#include <TJNP/include/tjwebserver.h>

namespace tj {
	namespace show {
		class Client;

		namespace view {
			class Dialogs;
		}

		class Application: public virtual Object, public RunnableApplication {
			friend class view::Dialogs;
			friend class Controller;
			friend class Network;

			public:
				Application();
				virtual ~Application();
				static Application* Instance();
				static ref<Application> InstanceReference();
				static void Close();

				void ExecuteAction(ref<Action> command);
				virtual void Message(MSG& msg);
				void Initialize(ref<Arguments> args, ref<SplashThread> st, bool asService = false);
				ref<Crumb> CreateCrumb();
				std::wstring GetWebRootURL();

				// scripting
				virtual ref<Scriptable> GetScriptable();
				ref<ScriptContext> GetScriptContext();

				HINSTANCE GetInstance();
				std::wstring GetApplicationPath() { return _programDirectory + L"\\"; }
				inline ref<view::View> GetView() { return _view; }
				inline strong<Model> GetModel() { return _model; }
				std::wstring GetVersion();
				void Update();
				void OnTimer();
				void RediscoverDevices();
				
				strong<EventLogger> GetEventLogger();
				strong<Instances> GetInstances();
				strong<Network> GetNetwork();
				strong<OutputManager> GetOutputManager();
				ref<tj::np::WebServer> GetFileServer();

				ref<TrackWrapper> CreateTrack(PluginHash pt, ref<tj::show::Instance> tl);
				void OpenWebClient();
				strong<Settings> GetSettings();
				
				void OnThemeChange();
				void OnClientLeft(ref<Client> cl);
				void OnClearShow();

			private:
				void LoadLocale(const std::wstring& id);
				std::wstring GetSettingsPath(const std::wstring& file);
				void LoadDefaultSettings();

				static ref<Application> _instance;
				ref<Model> _model;
				strong<Network> _network;
				ref<Instances> _instances;
				strong<SettingsStorage> _settings;
				strong<SettingsStorage> _workspaceSettings;
				strong<OutputManager> _outputManager;
				
				ref<view::View> _view;
				ref<tj::np::WebServer> _fileServer;
				ref<ScriptContext> _scriptContext;
		
				HACCEL _accelerators;
				std::wstring _programDirectory;
				static const int KTimerID = 1339;
		};
	}
}

#endif
