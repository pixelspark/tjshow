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
#include "../include/tjingle.h"
#include "../resource.h"
#include <shlwapi.h>
using namespace tj::shared;
using namespace tj::jingle;

namespace tj {
	namespace jingle {
		class JingleSettings: public virtual Object, public Inspectable {
			public:
				JingleSettings(strong<Settings> st): _st(st), 
					_smEffects(st,L"effects"),
					_smDevice(st, L"device"),
					_smSingleJingle(st, L"single-jingle"),
					_smConnectorEnabled(st, L"connector.enabled"),
					_smOSCEnabled(st, L"connector.osc.enabled"),
					_smOSCAddress(st, L"connector.osc.address"),
					_smOSCPort(st, L"connector.osc.port") {

					_locale = st->GetValue(L"locale");
					_keyboardLayout = st->GetValue(L"keyboard.layout");
				}

				virtual ~JingleSettings() {
					if(_smOSCEnabled.IsChanged() || _smOSCAddress.IsChanged() || _smOSCPort.IsChanged() || _locale!=_st->GetValue(L"locale") || _smDevice.IsChanged() || _smEffects.IsChanged()) {
						Alert::Show(TL(jingle_settings), TL(jingle_settings_restart_warning), Alert::TypeInformation);
					}
					_st->SetValue(L"locale", _locale);
					_st->SetValue(L"keyboard.layout", _keyboardLayout);
				}

				virtual ref<PropertySet> GetProperties() {
					ref<PropertySet> ps = GC::Hold(new PropertySet());
					ps->Add(Properties::CreateLanguageProperty(TL(locale), this, &_locale));
					ps->Add(_smDevice.CreateProperty(TL(jingle_device), this));
					ps->Add(_smConnectorEnabled.CreateProperty(TL(jingle_connector_enabled), this));
					ps->Add(GC::Hold(new PropertySeparator(TL(jingle_user_interface))));
					ps->Add(_smSingleJingle.CreateProperty(TL(jingle_single), this));
					ps->Add(_smEffects.CreateProperty(TL(jingle_effects), this));

					// Property for keyboard layout
					ref< GenericListProperty<std::wstring> > klp = GC::Hold(new GenericListProperty<std::wstring>(TL(jingle_keyboard_layout), this, &_keyboardLayout, _keyboardLayout));
					// TODO: load these options from the list inside JingleKeyboardLayoutFactory
					klp->AddOption(TL(jingle_keyboard_layout_qwerty), L"qwerty");
					klp->AddOption(TL(jingle_keyboard_layout_azerty), L"azerty");
					klp->AddOption(TL(jingle_keyboard_layout_qwertz), L"qwertz");
					klp->AddOption(TL(jingle_keyboard_layout_dvorak), L"dvorak");
					ps->Add(klp);

					ps->Add(GC::Hold(new PropertySeparator(TL(jingle_connector_osc))));
					ps->Add(_smOSCEnabled.CreateProperty(TL(jingle_connector_osc_enabled), this));
					ps->Add(_smOSCAddress.CreateProperty(TL(jingle_connector_osc_address), this));
					ps->Add(_smOSCPort.CreateProperty(TL(jingle_connector_osc_port), this));

					return ps;
				}

				strong<Settings> _st;
				LocaleIdentifier _locale;
				std::wstring _keyboardLayout;
				SettingsMarshal<int> _smDevice;
				SettingsMarshal<bool> _smEffects;
				SettingsMarshal<bool> _smSingleJingle;
				SettingsMarshal<bool> _smConnectorEnabled;
				SettingsMarshal<bool> _smOSCEnabled;
				SettingsMarshal<std::wstring> _smOSCAddress;
				SettingsMarshal<unsigned short> _smOSCPort;
		};
	}
}

ref<JingleApplication> JingleApplication::_instance;

JingleApplication::JingleApplication() { 
	/* Get application path, useful for knowing where to look for plugins and stuff */
	wchar_t* buf = new wchar_t[MAX_PATH];
	memset(buf,0,sizeof(wchar_t)*MAX_PATH);
	GetModuleFileName(GetModuleHandle(NULL), buf, MAX_PATH);
	PathRemoveFileSpec(buf);
	_programDirectory = buf;
	delete[] buf;	

	_settings = GC::Hold(new SettingsStorage());
	
	// Default settings go here
	_settings->SetValue(L"locale", L"nl");
	_settings->SetValue(L"device", L"");
	_settings->SetValue(L"effects", L"yes");
	_settings->SetValue(L"single-jingle", L"no");
	_settings->SetValue(L"theme", Stringify(ThemeManager::GetThemeId()));
	_settings->SetFlag(L"connector.enabled", true);
	_settings->SetFlag(L"connector.osc.enabled", false);
	_settings->SetValue(L"connector.osc.address", L"224.0.0.2");
	_settings->SetValue(L"connector.osc.port" , L"7000");
	_settings->SetValue(L"keyboard.layout", L"qwerty");
}

ref<JingleKeyboardLayout> JingleApplication::GetKeyboardLayout() {
	// TODO: the JingleKeyboardLayout object should be cached somewhere, and only recreated if the setting has changed
	std::wstring id = _settings->GetValue(L"keyboard.layout");
	return JingleKeyboardLayoutFactory::Instance()->CreateObjectOfType(id);
}

JingleApplication::~JingleApplication() {
	BASS_Free();
	_settings->SetValue(L"theme", Stringify(ThemeManager::GetThemeId()));
	_settings->SaveFile(SettingsStorage::GetSettingsPath(L"TJ", L"TJingle", L"settings"));
}

ref<JingleView> JingleApplication::GetView() {
	return _view;
}

std::wstring JingleApplication::GetProgramDirectory() const {
	return _programDirectory;
}

void JingleApplication::Message(MSG& msg) {
	if((msg.message==WM_KEYDOWN||msg.message==WM_SYSKEYDOWN) && LOWORD(msg.wParam)>=VK_F1 && LOWORD(msg.wParam)<=VK_F12) {
		msg.hwnd = _view->GetWindow();
	}
	TranslateMessage(&msg);
	DispatchMessage(&msg);
}

void JingleApplication::OnJingleEvent(const std::wstring& jingleID, const JingleEvent& ev) {
	if(_connector) {
		std::wstring evCode = L"";
		switch(ev) {
			case JinglePlay:
				evCode = L"play";
				break;

			case JingleStop:
				evCode = L"stop";
				break;

			case JingleFadeIn:
				evCode = L"fadeIn";
				break;

			case JingleFadeOut:
				evCode = L"fadeOut";
				break;
		}

		_connector->SendEvent(L"jingle", jingleID, evCode);
	}
}


void JingleApplication::Initialize(ref<Arguments> args) {
	// Load settings from file
	if(!args->IsSet(L"nosettings")) {
		_settings->LoadFile(SettingsStorage::GetSettingsPath(L"TJ", L"TJingle", L"settings"));
	}

	Animation::SetAnimationsEnabled(StringTo<bool>(_settings->GetValue(L"effects",L"yes"), true));
	Log::Write(L"TJingle/Initialize", L"Animations enabled:"+Stringify(Animation::IsAnimationsEnabled()));

	// Load preferences (locale, theme etc)
	Language::LoadDirectory(_programDirectory + L"\\locale", _settings->GetValue(L"locale"));
	ThemeManager::SelectTheme(StringTo<int>(_settings->GetValue(L"theme"), ThemeManager::GetThemeId()));

	// Initialize the JingleConnector, if we have to
	if(_settings->GetFlag(L"connector.enabled", true)) {
		_connector = GC::Hold(new io::JingleConnector(_settings->GetNamespace(L"connector")));
		_connector->Initialize();
	}

	// initialize BASS
	std::wostringstream wos;
	wos << _programDirectory << L"\\icons\\splash\\tjingle.png";
	Log::Write(L"TJingleApplication/SplashInit", wos.str());
	ref<SplashThread> st = GC::Hold(new SplashThread(wos.str(), 400, 300));
	st->Start();

	_view = GC::Hold(new JingleView());
	_view->Create();
	int device = StringTo<int>(_settings->GetValue(L"device", L"-1"),-1);

	// Device 0 means the 'no sound device'. Hmm, who would initialize an audio library to produce *no* sound?
	// Change it to -1 (meaning 'the default sound device')
	if(device==0) {
		device = -1;
	}
	if(!BASS_Init(device, 44100, BASS_DEVICE_LATENCY, _view->GetWindow(), 0)) {
		Log::Write(L"TJingle/Init", L"Could not initialize BASS, trying again with default device");
		BASS_Init(-1, 44100, BASS_DEVICE_LATENCY, _view->GetWindow(), 0);
	}

	BASS_SetVolume(100);

	// Load specified file, if it exists
	if(args->GetOptions()->size()>1) {
		std::wstring file = args->GetOptions()->at(1);
		Log::Write(L"TJingle/Application/Load", file);
		TiXmlDocument doc(Mbs(file));
		doc.LoadFile();
		TiXmlElement* root = doc.FirstChildElement("jingle");
		if(root==0) {
			Log::Write(L"TJingle/Load", L"Root element == 0");
		}
		else {
			_view->Load(root);
		}
	}

	Sleep(1000);
	st->Hide();
	_view->Show(true);
	_view->BringToFront();
}

void JingleApplication::OnBeforeJingleStart() {
	bool ctrlDown = Wnd::IsKeyDown(KeyControl);
	bool single = _settings->GetFlag(L"single-jingle", false);
	bool stopAll = (ctrlDown && !single) || (single && !ctrlDown);
	if(stopAll && _view) {
		_view->StopAll();
	}
}

void JingleApplication::FindJinglesByName(const std::wstring& q, std::deque< ref<Jingle> >& results) {
	if(_view) {
		_view->FindJinglesByName(q,results);
	}
}

void JingleApplication::Command(int c) {
	if(c==JID_EXIT) {
		SendMessage(_view->GetWindow(), WM_CLOSE, 0, 0);
	}
	else if(c==JID_SHOW_LOG) {
		Core::Instance()->ShowLogWindow(true);
	}
	else if(c==JID_ABOUT) {
		_view->ShowAboutInformation();
	}
	else if(c==JID_NEW) {
		std::map< int, ref<JinglePane> >::iterator it = _view->_panes.begin();
		while(it!=_view->_panes.end()) {
			std::pair<int, ref<JinglePane> > data = *it;
			ref<JinglePane> pane = data.second;
			if(pane) {
				pane->GetJingleCollection()->Clear();
			}
			++it;
		}
		_view->Update();
	}
	else if(c==JID_OPEN) {
		std::wstring file = Dialog::AskForOpenFile(_view, TL(jingle_open_set), L"Jingle (*.jingle)\0*.jingle\0\0", L"jingle");
		TiXmlDocument doc(Mbs(file));
		doc.LoadFile();
		TiXmlElement* root = doc.FirstChildElement("jingle");
		if(root==0) {
			Log::Write(L"TJingle/Load", L"Root element == 0");
		}
		else {
			_view->Load(root);
		}
	}
	else if(c==JID_SAVE) {
		std::wstring file = Dialog::AskForSaveFile(_view, TL(jingle_open_set), L"Jingle (*.jingle)\0*.jingle\0\0", L"jingle");
		TiXmlDocument doc(Mbs(file));
		TiXmlDeclaration decl("1.0", "ISO-8859-1", "no");
		doc.InsertEndChild(decl);
		TiXmlElement jingle("jingle");
		_view->Save(&jingle);
		doc.InsertEndChild(jingle);
		doc.SaveFile();
	}
	else if(c==JID_STOP_ALL) {
		_view->StopAll();
	}
	else if(c==JID_SETTINGS) {
		{
			ref<JingleSettings> st = GC::Hold(new JingleSettings(ref<Settings>(_settings)));
			_view->Inspect(st);
		}
		// let's hope the JingleSettings are deleted by now, so the settings are marshaled
		if(!BASS_SetDevice(StringTo<int>(_settings->GetValue(L"device", L"-1"),-1))) {
			Log::Write(L"TJingle/SettingsChanged", L"Could not set BASS device to "+_settings->GetValue(L"device", L"-1"));
			BASS_SetDevice(-1);
		}

		Animation::SetAnimationsEnabled(StringTo<bool>(_settings->GetValue(L"effects",L"yes"), true));
	}
}

void JingleApplication::OnThemeChanged() {
	if(_view) {
		_view->OnThemeChanged();
	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR cmd, int nShow) {
	SharedDispatcher sd;
	ref<Core> core = Core::Instance();
	ref<Arguments> args = GC::Hold(new Arguments(GetCommandLine()));
	ref<JingleApplication> application = JingleApplication::Instance();
	{
		application->Initialize(args);
	}

	// Start the updater
	std::wstring appPath = application->GetProgramDirectory();
	std::wstring updaterExecutable = appPath + L"\\tjupdater.exe";
	if(GetFileAttributes(updaterExecutable.c_str())!=INVALID_FILE_ATTRIBUTES) {
		STARTUPINFO si;
		memset(&si, 0, sizeof(si));
		si.cb = sizeof(si);

		PROCESS_INFORMATION pi;
		memset(&pi, 0, sizeof(pi));
		if(CreateProcess(updaterExecutable.c_str(), NULL, NULL, NULL, FALSE, 0L, NULL, appPath.c_str(), &si, &pi)==TRUE) {
			Log::Write(L"TJShow/Main", L"Started the updater process");
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
	}

	core->Run(dynamic_cast<RunnableApplication*>(application.GetPointer()), args);

	return 0;
}
