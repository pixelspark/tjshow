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
#include <TJZip/include/tjzip.h>

#include "../include/internal/tjshow.h"
#include "../include/internal/tjnetwork.h"
#include "../include/internal/tjfileserver.h"
#include "../include/internal/tjdashboardserver.h"

#include "../include/internal/view/tjactions.h"
#include "../include/internal/view/tjtimelinewnd.h"
#include "../include/internal/tjscriptapi.h"
#include "../include/internal/view/tjplayerwnd.h"
#include "../include/internal/view/tjsplittimelinewnd.h"
#include "../include/internal/view/tjcapacitywnd.h"
#include "../include/internal/view/tjeventlogwnd.h"

#include <time.h>
#include <shellapi.h>

using namespace tj::zip;
using namespace tj::show::view;

ref<Application> Application::_instance;

Application::Application(): 
	_network(GC::Hold(new Network())), 
	_outputManager(GC::Hold(new OutputManager())),
	_settings(GC::Hold(new SettingsStorage())),
	_workspaceSettings(GC::Hold(new SettingsStorage())) {

	_model = GC::Hold(new Model(_network));
	_instances = GC::Hold(new Instances(_model, _network));
	LoadDefaultSettings();

	/* Get application path, useful for knowing where to look for plugins and stuff */
	wchar_t* buf = new wchar_t[MAX_PATH];
	memset(buf,0,sizeof(wchar_t)*MAX_PATH);
	GetModuleFileName(GetModuleHandle(NULL), buf, MAX_PATH);	
	_programDirectory = File::GetDirectory(std::wstring(buf));
	delete[] buf;	
}

Application::~Application() {
	if(_network->GetRole()==RoleMaster) {
		_settings->SaveFile(GetSettingsPath(L"settings"));
		_workspaceSettings->SaveFile(GetSettingsPath(L"workspace"));
		ref<SettingsStorage> st = ThemeManager::GetLayoutSettings();
		if(st) {
			st->SaveFile(GetSettingsPath(L"layout"));
		}
	}

	PluginManager::Instance()->SaveSettings(GetSettingsPath(L"plugins"));
}

void Application::OnClearShow() {
	UndoStack::Instance()->Clear();
	_instances->GetRootInstance()->SetPlaybackStateRecursive(PlaybackStop);

	ref<View> view = GetView();
	if(view) {
		view->OnClearShow();
	}

	ref<PluginManager> pmgr = PluginManager::Instance();
	pmgr->ResetPlugins();

	ref<Network> nw = GetNetwork();
	if(nw) {
		nw->Clear();
	}
}

void Application::OnClientLeft(ref<Client> cl) {
	ref<EventLogger> ew = GetEventLogger();
	ew->AddEvent(TL(client_disconnected)+cl->GetIP(), ExceptionTypeMessage, false);
}

void Application::Initialize(ref<Arguments> args, ref<SplashThread> splash, bool asService) {
	// Load and apply settings
	if(!args->IsSet(L"nosettings")) {
		_settings->LoadFile(GetSettingsPath(L"settings"));
		_workspaceSettings->LoadFile(GetSettingsPath(L"workspace"));
		ref<SettingsStorage> layoutSettings = ThemeManager::GetLayoutSettings();
		if(layoutSettings) {
			layoutSettings->LoadFile(GetSettingsPath(L"layout"));
		}
	}

	LoadLocale(_settings->GetValue(L"locale"));

	// Enable or disable animations
	Animation::SetAnimationsEnabled(_settings->GetFlag(L"view.enable-animations"));

	// Initialize COM
	if(FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED))) {
		Log::Write(L"TJShow/Application/Initialize", TL(com_initialization_failed));
	}

	if(args->IsSet(L"help")) {
		Alert::Show(TL(command_line_help_title), TL(command_line_help), Alert::TypeInformation);
	}

	// Decide on a role
	// process command line arguments:
	// - If 'server' is specified, run as server
	// - If 'client' is specified or we are a service (asService), run as client
	// - All other cases: run as master
	std::wstring fileToLoad;
	if(args->IsSet(L"server")) {
		_network->SetRole(RoleMaster);

		if(args->GetOptions()->size()>0) {
			fileToLoad = *(args->GetOptions()->rbegin());
		}
	}
	else if(asService || args->IsSet(L"client")) {
		if(args->GetOptions()->size()>0) {
			_network->GetFilter()->Parse(*(args->GetOptions()->rbegin()));
		}
		_network->SetRole(RoleClient);
	}
	else {
		_network->SetRole(RoleMaster);
	}
	
	if(_network->GetRole()==RoleNone) {
		return;
	}

	// start up networking stuff
	_network->Connect(_settings);

	// Create UI
	ThemeManager::SelectTheme(StringTo<int>(_settings->GetValue(L"view.theme"),0));
	_view = GC::Hold(new View(_workspaceSettings->GetNamespace(L"view"), _settings));
	_view->Create(TL(application_name), _network->GetRole(), _model, _outputManager);

	// If client, always use the standard black theme
	if(_network->GetRole()==RoleClient) {
		ThemeManager::SelectTheme(0);
	}

	// if needed, start the webserver
	int webPort = StringTo<int>(_settings->GetValue(L"net.web.port"),0);
	if(webPort>0) {
		_fileServer = GC::Hold(new tj::np::WebServer(webPort));
	}

	// Make sure the file server can serve resources and dashboard info
	if(_fileServer) {
		ref<tj::np::WebItem> rrr = GC::Hold(new ResourcesRequestResolver());
		std::wstring prefix = _settings->GetValue(L"net.web.resources.path", L"/_resources");
		_fileServer->AddResolver(prefix, rrr);

		ref<tj::np::WebItem> drr = GC::Hold(new DashboardWebItem(_model));
		prefix = _settings->GetValue(L"net.web.dashboard.path", L"/_dashboard");
		_fileServer->AddResolver(prefix, drr);
	}

	// let the network know we're there
	_network->Announce();

	// load the accelerators from the resource file
	_accelerators = LoadAccelerators(GetInstance(), MAKEINTRESOURCE(IDR_MAIN_ACCELERATOR));
	if(!_accelerators) {
		Log::Write(L"TJShow/Application", L"Could not load accelerators; they won't work!");	
	}

	// change to the right directory
	std::wstring path = GetApplicationPath();
	_wchdir(path.c_str());

	// Discover plug-ins and load their settings
	ref<PluginManager> pgm = PluginManager::Instance();
	if(pgm) {
		pgm->Discover(GetApplicationPath()+std::wstring(L"plugins\\"));
		pgm->LoadSettings(GetSettingsPath(L"plugins"));

		std::vector< ref<Device> > alsoAdd;
		_outputManager->ListDevices(alsoAdd);
		pgm->RediscoverDevices(alsoAdd);

		// Add endpoint categories to plugin manager
		ref<CueEndpointCategory> ccat = GC::Hold(new CueEndpointCategory());
		pgm->AddEndpointCategory(strong<CueEndpointCategory>(ccat));

		ref<LiveControlEndpointCategory> lccat = GC::Hold(new LiveControlEndpointCategory());
		pgm->AddEndpointCategory(strong<LiveControlEndpointCategory>(lccat));

		ref<VariableEndpointCategory> vec = GC::Hold(new VariableEndpointCategory());
		pgm->AddEndpointCategory(strong<VariableEndpointCategory>(vec));
	}

	// if a file is specified to load at the commandline, load it here
	if(fileToLoad.length()>0) {
		SetCurrentDirectory(fileToLoad.c_str());
		_wchdir(fileToLoad.c_str());
	}

	if(fileToLoad.length()>0) {
		ExecuteAction(GC::Hold(new OpenFileAction(this, fileToLoad, _model)));
	}

	if(splash) {
		Sleep(1000);
	}

	// showtime!
	_view->Show(true);

	// Show the splash screen just a split second more
	Sleep(200);
	if(splash) {
		splash->Hide();
	}
	
	Log::Write(L"TJShow/Application", L"Engage!");
}

/** NOTE: If this is ever going to change, do change it in TJCrashReporter too, as it relies
on settings files to find out what the locale for TJShow is (and probably other programs in the 
future do that too... **/
std::wstring Application::GetSettingsPath(const std::wstring& file) {
	return SettingsStorage::GetSettingsPath(L"TJ", L"TJShow", file);
}

void Application::RediscoverDevices() {
	strong<PluginManager> pgm = PluginManager::Instance();
	std::vector< ref<Device> > alsoAdd;
	_outputManager->ListDevices(alsoAdd);
	pgm->RediscoverDevices(alsoAdd);
}

Application* Application::Instance() {
	return InstanceReference().GetPointer();
}

ref<Application> Application::InstanceReference() {
	if(!_instance) {
		_instance = GC::Hold(new Application());
	}

	return _instance;
}

strong<Instances> Application::GetInstances() {
	return _instances;
}

strong<EventLogger> Application::GetEventLogger() {
	Role role = _network->GetRole();
	if(role==RoleMaster && _view) {
		if(_view) {
			ref<EventLogger> ew = _view->GetEventLogger();
			if(ew) return ew;
		}
	}
	else if(role==RoleClient) {
		return _network;
	}

	// If there is no UI or server for logging, just return the default event logger
	// TODO: on clients, provide a custom EventLogger that writes to the server
	return Log::GetEventLogger();
}

void Application::Close() {
	_instance = 0;
}

strong<Settings> Application::GetSettings() {
	return _settings;
}

void Application::ExecuteAction(ref<Action> c) {
	c->Execute();
}

void Application::LoadLocale(const std::wstring& id) {
	Language::Clear();
	Log::Write(L"TJShow/Application", L"LoadLocale dir="+_programDirectory+L"locale; id="+id);
	Language::LoadDirectory(_programDirectory+L"\\locale", id);
}

std::wstring Application::GetWebRootURL() {
	return L"http://localhost:"+(GetSettings()->GetValue(L"net.web.port"))+ L"/";
}

void Application::Message(MSG& msg) {
	try {
		// announce timer
		if(_view && _view->GetRootWindow() && TranslateAccelerator(_view->GetRootWindow()->GetWindow(), _accelerators, &msg)) {
		}
		else if(msg.message==WM_KEYDOWN) {
			if(msg.wParam >= VK_F1 && msg.wParam <= VK_F24 && Wnd::IsKeyDown(KeyControl)) {
				_view->OnFunctionKey((int)msg.wParam);
			}
			else {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	catch(Exception& e) {
		if(e.GetType()!=ExceptionTypeMessage) {
			std::wstring msg = e.ToString();
			Log::Write(L"TJShow/Application", std::wstring(TL(error))+L" "+msg);

			// Show an error message on masters
			if(_network->GetRole()==RoleMaster) {
				MessageBox(0L, msg.c_str(), TL(error), MB_TASKMODAL|MB_OK|(e.GetType()==ExceptionTypeWarning?MB_ICONWARNING:MB_ICONERROR));
			}

			// Always report the error on the network, even if we're a master (there might be multiple masters)
			_network->ReportError(FeaturesUnknown, e.GetType(), msg);

			// Quit if this is a very severe error
			if(e.GetType()==ExceptionTypeSevere) {
				PostQuitMessage(0);
			}
		}
	}
	catch(...) {
		Log::Write(L"TJShow/Application", L"An unknown error has occurred. Please restart");
		if(_network->GetRole()==RoleMaster) {
			ref<EventLogger> ew = Application::Instance()->GetEventLogger();
			if(ew) {
				ew->AddEvent(TL(player_thread_error), ExceptionTypeError, false);
			}
		}
		_network->ReportError(FeaturesUnknown, ExceptionTypeSevere, TL(error_unknown));
	}
}

strong<Network> Application::GetNetwork() {
	return _network;
}

strong<OutputManager> Application::GetOutputManager() {
	return _outputManager;
}

void Application::OnThemeChange() {
	if(_view) {
		_view->OnThemeChanged();
	}

	// Update settings
	_settings->SetValue(L"view.theme", Stringify(ThemeManager::GetThemeId()));
}

void Application::Update() {
	if(_view) {
		_view->Update();
	}
}

std::wstring Application::GetVersion() {
	std::wostringstream os;
	os << L"TJShow [" << __DATE__ << " @ " << __TIME__ << "] ";
	#ifdef UNICODE
	os << L"Unicode; ";
	#endif

	#ifdef NDEBUG
	os << "Release; ";
	#endif

	return os.str();
}

HINSTANCE Application::GetInstance() {
	return GetModuleHandle(NULL);
}

void Application::OpenWebClient() {
	if(!_fileServer) {
		Alert::Show(TL(error), TL(webserver_disabled_error), Alert::TypeError);
		return;
	}

	std::string url = std::string("http://localhost:") + Mbs(_settings->GetValue(L"net.web.port"))+"/";
	ShellExecuteA(0, "open", url.c_str(), "", 0L, SW_SHOWNORMAL);
}

/** Warning: copy all changes made here to Timeline::Load also, because tracks are created there too */
ref<TrackWrapper> Application::CreateTrack(PluginHash pt, ref<tj::show::Instance> tl) {
	try {
		ref<PluginWrapper> plug = PluginManager::Instance()->GetPluginByHash(pt);
		if(!plug || !plug->IsOutputPlugin()) {
			Log::Write(L"TJShow/Application", L"CreateTrack could not find plugin with hash "+StringifyHex(pt));
			return 0;
		}

		ref<TrackWrapper> track = plug->CreateTrack(tl->GetPlayback(), _network, tl);
		if(!track) {
			Log::Write(L"TJShow/Application", L"CreateTrack: Plugin->CreateTrack failed for reasons unknown");
			return 0;
		}

		tl->GetTimeline()->AddTrack(track);
		_view->OnAddTrack(track);

		Update();
		return track;
	}
	catch(const Exception& e) {
		Log::Write(L"TJShow/Application", L"Exception occurred when trying to create track: "+e.ToString());
	}
	catch(...) {
		Log::Write(L"TJShow/Application", L"Unknown exception occurred when trying to create track");
	}
	return null;
}

ref<Scriptable> Application::GetScriptable() {
	return GC::Hold(new tj::show::script::ApplicationScriptable());
}

ref<ScriptContext> Application::GetScriptContext() {
	if(!_scriptContext) {
		_scriptContext = GC::Hold(new ScriptContext(GetScriptable()));
	}
	return _scriptContext;
}

namespace tj {
	namespace show {
		class ApplicationCrumb: public Crumb {
			public:
				ApplicationCrumb(): Crumb(TL(application_name), L"icons/computer.png") {
				}

				virtual ~ApplicationCrumb() {
				}

				virtual ref<Inspectable> GetSubject() {
					return Application::InstanceReference();
				}

				virtual void GetChildren(std::vector< ref<Crumb> >& crs) {
					crs.push_back(GC::Hold(new BasicCrumb(TL(client_settings_title), L"icons/settings.png", Application::InstanceReference())));
				}
		};
	}
}

ref<Crumb> Application::CreateCrumb() {
	return GC::Hold(new ApplicationCrumb());
}

ref<tj::np::WebServer> Application::GetFileServer() {
	return _fileServer;
}