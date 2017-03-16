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
#include "../../include/internal/tjshow.h"
#include "../../include/internal/tjcontroller.h"
#include "../../include/internal/view/tjmainwnd.h"
#include "../../include/internal/view/tjaboutwnd.h"
#include "../../include/internal/view/tjclientswnd.h"
#include "../../include/internal/view/tjcapacitywnd.h"
#include "../../include/internal/view/tjpluginsettingswnd.h"
#include "../../include/internal/view/tjvariablewnd.h"
#include "../../include/internal/view/tjeventlogwnd.h"
#include "../../include/internal/view/tjtimelinewnd.h"
#include "../../include/internal/view/tjfilewnd.h"
#include "../../include/internal/view/tjsettingswnd.h"
#include "../../include/internal/view/tjsplittimelinewnd.h"
#include "../../include/internal/view/tjtray.h"
#include "../../include/internal/view/tjscripteditorwnd.h"
#include "../../include/internal/view/tjplayerwnd.h"
#include "../../include/internal/view/tjtoolbar.h"
#include "../../include/internal/view/tjlivewnd.h"
#include "../../include/internal/tjsubtimeline.h"
#include "../../include/internal/view/tjactions.h"
#include "../../include/internal/tjchecker.h"
#include "../../include/internal/tjnetwork.h"
#include "../../include/internal/view/tjtimelinetreewnd.h"
#include "../../include/internal/view/tjdashboardwnd.h"

#include <direct.h>
using namespace tj::shared::graphics;
using namespace tj::show;
using namespace tj::show::view;

class LiveControlWrapperWnd: public ChildWnd {
	public:
		LiveControlWrapperWnd(strong<Wnd> lc, ref<TrackWrapper> tw);
		virtual ~LiveControlWrapperWnd();
		virtual void OnSize(const Area& ns);
		virtual void Paint(graphics::Graphics& g, strong<Theme> theme);
		virtual void Layout();
		virtual void OnFocus(bool focus);
		virtual std::wstring GetTabTitle() const;

	private:
		strong<Wnd> _lc;
		weak<TrackWrapper> _track;
};

/** LiveControlWrapperWnd **/
LiveControlWrapperWnd::LiveControlWrapperWnd(strong<Wnd> lc, ref<TrackWrapper> tw): ChildWnd(false), _lc(lc), _track(tw) {
	Add(_lc);
}

LiveControlWrapperWnd::~LiveControlWrapperWnd() {
}

void LiveControlWrapperWnd::OnSize(const Area& ns) {
	Layout();
}

void LiveControlWrapperWnd::Paint(graphics::Graphics& g, strong<Theme> theme) {
}

void LiveControlWrapperWnd::Layout() {
	Area rc = GetClientArea();
	_lc->Fill(LayoutFill, rc);
}

void LiveControlWrapperWnd::OnFocus(bool focus) {
	if(focus) {
		_lc->Focus();
	}
}

std::wstring LiveControlWrapperWnd::GetTabTitle() const {
	ref<TrackWrapper> tw = _track;
	if(tw) {
		return tw->GetInstanceName();
	}
	return L"";
}

/** MainWnd **/
MainWnd::MainWnd(const wchar_t* title): RootWnd(title) {
}

MainWnd::~MainWnd() {
}

class PropertyGridProxyImpl: public PropertyGridProxy {
	public:
		PropertyGridProxyImpl(ref<MainWnd> pgw);
		virtual ~PropertyGridProxyImpl();
		virtual void SetProperties(ref<Inspectable> ps);

	protected:
		weak<MainWnd> _view;
};

void MainWnd::Create(const wchar_t* title, Role r, ref<Model> model, strong<Settings> wsp) {
	_model = model;
	SetSize(1024, 576);
	
	/* Create tab windows */
	_liveTab = GC::Hold(new TabWnd((RootWnd*)this, L"livecontrols"));
	AddTabWindow(_liveTab);
	
	_timelines = GC::Hold(new TabWnd((RootWnd*)this, L"timelines"));
	AddTabWindow(_timelines);
	
	_tab = GC::Hold(new TabWnd((RootWnd*)this, L"properties"));
	AddTabWindow(_tab);

	/* Toolbar */
	_tools = GC::Hold(new ApplicationToolbarWnd());
	Add(_tools);

	// These windows usually end up on some tab pane; added with AddOtherPane once settings are loaded
	_eventLogWnd = GC::Hold(new EventLogWnd());
	_capsWnd = GC::Hold(new CapacitiesWnd(model->GetCapacities()));
	_variableWnd = GC::Hold(new VariableWnd(_model->GetVariables()));
	_time = GC::Hold(new SplitTimelineWnd(Application::Instance()->GetInstances()->GetRootInstance()));
	_properties = GC::Hold(new PropertyGridWnd());
	_pg = GC::Hold(new PropertyGridProxyImpl(this));
	_fileWnd = GC::Hold(new FileWnd(model));
	_dashboardWnd = GC::Hold(new DashboardWnd(_model->GetDashboard()));

	/* The tree window **/
	_tree = GC::Hold(new TimelineTreeWnd(model, Application::Instance()->GetInstances()->GetRootInstance(), true));

	/* Splitters to layout all tabpanes */
	_topSplitter = GC::Hold(new SplitterWnd(OrientationVertical));
	_topSplitter->SetFirst(_liveTab);
	_topSplitter->SetSecond(_tab);
	_topSplitter->SetRatio(0.75f);

	_splitter = GC::Hold(new SplitterWnd(OrientationHorizontal));
	_splitter->SetFirst(_topSplitter);
	_splitter->SetSecond(_timelines);
	_splitter->SetRatio(0.5f);
	
	_treeSplitter = GC::Hold(new SplitterWnd(OrientationVertical));
	_treeSplitter->SetFirst(GC::Hold(new SidebarWnd(_tree)));
	_treeSplitter->SetSecond(_splitter);
	_treeSplitter->SetRatio(0.2f);

	Add(_treeSplitter);
	Layout();

	/* Window settings */
	SetStyle(WS_OVERLAPPEDWINDOW);
	SetQuitOnClose(true);
	SetDropTarget(true);

	SetMenu(GetWindow(), LoadMenu(GetModuleHandle(NULL),MAKEINTRESOURCE(IDR_MAIN_MENU)));
	Language::Translate(GetWindow());
	SetSettings(wsp);

	StartTimer(StringTo<int>(Application::Instance()->GetSettings()->GetValue(L"controller.tick-length"),0), KTickTimerID);
}

void MainWnd::AddPlayerWindow(ref<Wnd> pw, ref<Settings> st) {
	Placement livePlacement = _liveTab->GetPlacement();
	ref<Pane> player = GC::Hold(new Pane(TL(player), pw, false, false, st, livePlacement, L"icons/tabs/movie.png"));
	AddPane(player,true);
}

void MainWnd::OnTimer(unsigned int id) {
	if(id==KTickTimerID) {
		ref<Instance> root = Application::Instance()->GetInstances()->GetRootInstance();
		if(root && root.IsCastableTo<Controller>()) {
			ref<Controller>(root)->Timer();
		}

		/* Update live controls; this calls Update() on each LiveGroupWnd we have. LiveGroupWnd will
		call LiveControl::Update. Live controls should decide for themselves if they have to repaint
		or not. 
		
		For standalone live controls: it's probably best if they use their own timer...
		*/
		std::map< std::wstring, ref<view::LiveGroupWnd> >::iterator it = _controlGroups.begin();
		while(it!=_controlGroups.end()) {
			ref<LiveGroupWnd> lgw = it->second;
			if(lgw) {
				lgw->Update();
			}
			++it;
		}
	}
}

void MainWnd::GetAccelerators(std::vector<Accelerator>& accels) {
	accels.push_back(Accelerator(KeyF2, L"F2", TL(toolbar_global_play)));
	accels.push_back(Accelerator(KeyF3, L"F3", TL(toolbar_global_pause)));
	accels.push_back(Accelerator(KeyF3, L"F3", TL(toolbar_global_resume), false, KeyControl));
	accels.push_back(Accelerator(KeyF4, L"F4", TL(toolbar_global_stop)));
}

void MainWnd::AddOtherPanes() {
	Placement livePlacement = _liveTab->GetPlacement();
	Placement rightPlacement = _tab->GetPlacement();

	ref<Settings> wsp = GetSettings();

	// TODO: add icon for dashboard tab
	AddPane(GC::Hold(new Pane(TL(dashboard), _dashboardWnd, false, false, wsp->GetNamespace(L"dashboard-wnd"), livePlacement, L"icons/tabs/dashboard.png")), true);
	AddPane(GC::Hold(new Pane(TL(event_log), _eventLogWnd, false, false, wsp->GetNamespace(L"event-log-wnd"))));
	AddPane(GC::Hold(new Pane(TL(capacities), _capsWnd, false, false, wsp->GetNamespace(L"capacities-wnd"), Placement(), L"icons/tabs/capacity.png")));
	AddPane(GC::Hold(new Pane(TL(variables), _variableWnd, false, false, wsp->GetNamespace(L"variables-wnd"), Placement(), L"icons/tabs/variables.png")));
	AddPane(GC::Hold(new Pane(L"", _time, false, false, wsp->GetNamespace(L"timeline-wnd"), _timelines->GetPlacement())));
	AddPane(GC::Hold(new Pane(TL(properties), _properties, false, false, wsp->GetNamespace(L"property-grid-wnd"), rightPlacement)));
	AddPane(GC::Hold(new Pane(TL(resources), _fileWnd, false, false, wsp->GetNamespace(L"file-wnd"), rightPlacement, L"icons/tabs/files.png")));
	_liveTab->Layout();
}

void MainWnd::OnSettingsChanged() {
	ref<Settings> st = GetSettings();
	if(st) {
		if(_treeSplitter) {
			_treeSplitter->SetSettings(st->GetNamespace(L"main-tree-splitter"));
		}

		ref<SettingsWnd> sw = _settingsWnd;
		if(sw) {
			sw->SetSettings(st->GetNamespace(L"settings-wnd"));
		}

		if(_fileWnd) {
			_fileWnd->SetSettings(st->GetNamespace(L"file-wnd"));
		}

		if(_properties) {
			_properties->SetSettings(st->GetNamespace(L"properties-wnd"));
		}

		if(_variableWnd) {
			_variableWnd->SetSettings(st->GetNamespace(L"variable-wnd"));
		}

		if(_eventLogWnd) _eventLogWnd->SetSettings(st->GetNamespace(L"event-log-wnd"));
	}

	_splitter->SetResizeMode(SplitterWnd::ResizeModeLeftOrTop);
	_treeSplitter->SetResizeMode(SplitterWnd::ResizeModeRightOrBottom);
	_topSplitter->SetResizeMode(SplitterWnd::ResizeModeLeftOrTop);

	TopWnd::OnSettingsChanged();
}

ref<TabWnd> MainWnd::GetLiveTabWindow() {
	return _liveTab;
}

void MainWnd::SetFileName(std::wstring fn) {
	if(fn.length()>0) {
		SetText(TL(application_name)+std::wstring(L" - ")+fn);
	}
	else {
		SetText(TL(application_name));
	}
}

void MainWnd::Paint(Graphics& g, ref<Theme> theme) {
	RootWnd::Paint(g,theme);
}

ref<CapacitiesWnd> MainWnd::GetCapacitiesWindow() {
	return _capsWnd;
}

ref<SplitTimelineWnd> MainWnd::GetSplitTimelineWindow() {
	return _time;
}

ref<DashboardWnd> MainWnd::GetDashboardWindow() {
	return _dashboardWnd;
}

bool MainWnd::IsFullscreen() {
	LONG l = GetWindowLong(GetWindow(),GWL_STYLE);
	return (l&WS_POPUP)>0;
}

void MainWnd::ToggleFullscreen() {
	SetFullScreen(!IsFullScreen());
}

void MainWnd::SetShowToolbar(bool t) {
	Application::Instance()->GetSettings()->SetFlag(L"view.toolbar.show",t);
	if(_tools) {
		_tools->Show(t);
		CheckMenuItem(GetMenu(GetWindow()), ID_SHOW_TOOLBAR, MF_BYCOMMAND|(t?MF_CHECKED:MF_UNCHECKED));
		Layout();
	}
}

void MainWnd::Layout() {
	Area rect = GetClientArea();

	if(_tools && _tools->IsShown()) {
		_tools->Fill(LayoutTop, rect);
	}

	if(_treeSplitter) {
		_treeSplitter->Fill(LayoutFill, rect);
	}
}

void MainWnd::SetFullScreen(bool fs) {
	TopWnd::SetFullScreen(fs);
	HMENU men = GetMenu(GetWindow());
	CheckMenuItem(men, ID_FULLSCREEN, MF_BYCOMMAND|(fs?MF_CHECKED:MF_UNCHECKED));
}

void MainWnd::OnSize(const Area& s) {
	Layout();
	RootWnd::OnSize(s);
}

void MainWnd::OnDropFiles(const std::vector< std::wstring >& files) {
	std::vector<std::wstring>::const_iterator it = files.begin();
	while(it!=files.end()) {
		FileReader fr;
		fr.Read(Mbs(*it), _model);		
		++it;
	}

	Application::Instance()->Update();
}

bool MainWnd::IsAnythingStillRunning() {
	ref<Settings> st = Application::Instance()->GetSettings();
	ref<Instance> mainController = Application::Instance()->GetInstances()->GetRootInstance();
	if(st && st->GetFlag(L"view.deny-exit-when-playing") && mainController && mainController->IsPlayingOrPausedRecursive()) {
		if(Alert::ShowYesNo(TL(application_name), TL(exit_still_playing), Alert::TypeWarning)) {
			mainController->SetPlaybackStateRecursive(PlaybackStop);
			return false;
		}
		return true;
	}
	return false;
}

LRESULT MainWnd::Message(UINT msg, WPARAM wp, LPARAM lp) {
	if(msg==WM_CLOSE) {
		//check if there's anything playing and prevent exit if there are tracks playing
		if(Application::Instance()->GetNetwork()->GetRole()==RoleMaster) {
			if(IsAnythingStillRunning()) {
				return 0L;
			}

			// 'Do you want to save...'-prompt
			ref<Settings> st = Application::Instance()->GetSettings();
			if(!st || st->GetFlag(L"view.save-prompt")) {
				Application::Instance()->ExecuteAction(GC::Hold(new SaveFileAction(Application::Instance(), _model, SaveFileAction::TypeSaveOnExit)));
			}
		}

		/* No DestroyWindow here, because PostQuitMessage will exit the message loop which will cause the application/view objects to
		be deleted -> DestroyWindow is called from Wnd::~Wnd */
		PostQuitMessage(0);
		return 0;
	}
	else if(msg==WM_COMMAND||msg==WM_SYSCOMMAND) {
		Command(LOWORD(wp));
	}
	else if(msg==WM_APPCOMMAND) {
		switch(GET_APPCOMMAND_LPARAM(lp)) {
			case APPCOMMAND_OPEN:
				Command(ID_OPEN);
				return TRUE;

			case APPCOMMAND_SAVE:
				Command(ID_SAVE);
				return TRUE;

			case APPCOMMAND_NEW:
				Command(ID_NEW);
				return TRUE;

			default:
				/* Let DefWindowProc handle it; according to the documentation, it will send the message
				to parent windows, and when there are none (i.e. in this case) it will call some shell hook. */
				break;
		};
	}
	else if(msg==WM_KEYDOWN) {
		if(wp==VK_F2) {
			Command(ID_PLAY);
		}
		else if(wp==VK_F3) {
			Command(ID_PAUSE);
		}
		return 0;
	}
	else if(msg==WM_POWERBROADCAST) {
		if(wp==PBT_APMBATTERYLOW) {
			Application::Instance()->GetEventLogger()->AddEvent(TL(battery_low), ExceptionTypeWarning);
		}
	}

	return RootWnd::Message(msg,wp,lp);
}

ref<EventLogWnd> MainWnd::GetEventLogWindow() {
	return _eventLogWnd;
}

void MainWnd::Update() {
	if(_time) {
		_time->Update();
	}

	if(_liveTab) {
		_liveTab->Update();
	}

	if(_tools) {
		_tools->Update();
	}

	if(_properties) {
		_properties->Update();
	}

	ref<Model> model = _model;
	std::wstring title = TL(application_name);
	std::wstring filename = model->GetFileName();
	if(filename.length()>0) {
		filename = PathFindFileName(filename.c_str());
		title += L" - ";
		title += filename;
	}
	SetWindowText(GetWindow(), title.c_str());
}

void MainWnd::Show(bool t) {
	Wnd::Show(t);
	if(t) {
		BringToFront();
	}
}

void MainWnd::GetMinimumSize(Pixels& w, Pixels& h) {
	w = 640;
	h = 480;
}

void MainWnd::ShowPatcher() {
	Command(ID_SETTINGS);
	ref<SettingsWnd> swd = _settingsWnd;
	if(swd) {
		swd->ShowPatcher();
	}
}

/* TODO: move to some controller class */
void MainWnd::Command(WPARAM wp) {
	Role role = Application::Instance()->GetNetwork()->GetRole();

	/** No about window on clients, as there is no tab window to put it in **/
	if(wp==ID_ABOUT && role==RoleMaster) {
		ref<Settings> st = GetSettings();
		if(st) {
			ref<AboutWnd> aboutWnd = GC::Hold(new AboutWnd());
			AddPane(GC::Hold(new Pane(TL(application_name), aboutWnd, false, true, st->GetNamespace(L"about-wnd"), _liveTab->GetPlacement(), L"")),true);
		}
	}
	/** No settings tab on clients, since that is pointless and there's no tab window to put it in */
	else if(wp==ID_SETTINGS && role!=RoleClient) {
		ref<SettingsWnd> swd = _settingsWnd;
		if(!swd) {
			swd = GC::Hold(new SettingsWnd());

			// Give the window some settings
			ref<Settings> st = GetSettings();
			if(st) {
				swd->SetSettings(st->GetNamespace(L"settings-wnd"));
			}
			AddPane(GC::Hold(new Pane(TL(settings), swd, false, true, GetSettings()->GetNamespace(L"settings-wnd"),_liveTab->GetPlacement(), L"icons/tabs/settings.png")));
			_settingsWnd = swd;
		}
		RootWnd::RevealWindow(swd, _liveTab);
	}
	else if(wp==ID_PREFERENCES && role!=RoleClient) {
		ref<PreferencesInspectable> pi = GC::Hold(new PreferencesInspectable(Application::Instance()->GetSettings()));
		ref<PropertyDialogWnd> dwdd = GC::Hold(new PropertyDialogWnd(TL(preferences), TL(preferences_question)));
		dwdd->GetPropertyGrid()->Inspect(pi);
		dwdd->SetSize(400, 400);
		if(!dwdd->DoModal(this)) {
			// TODO: implement cancel function
		}
		if(pi->ChangesNeedRestart()) {
			Alert::Show(TL(preferences), TL(preferences_changes_need_restart), Alert::TypeInformation);
		}
	}
	/** Resource checking with user interface is not wanted and necessary at the client **/
	else if(wp==ID_BUNDLE_RESOURCES && role!=RoleClient) {
		Application::Instance()->ExecuteAction(GC::Hold(new BundleResourcesAction(Application::Instance())));
	}
	else if(wp==ID_CHECK_SHOW && role!=RoleClient) {
		Application::Instance()->ExecuteAction(GC::Hold(new checker::CheckShowAction(_model, Application::InstanceReference(), false)));
	}
	else if(wp==ID_DEPLOY_SHOW && role!=RoleClient) {
		Application::Instance()->ExecuteAction(GC::Hold(new checker::CheckShowAction(_model, Application::InstanceReference(), true)));
	}
	else if(wp==ID_SHOW_TREE) {
		ref<TimelineTreeWnd> ttw = GC::Hold(new TimelineTreeWnd(Application::Instance()->GetModel(), Application::Instance()->GetInstances()->GetRootInstance()));
		Application::Instance()->GetView()->AddUtilityWindow(ttw, TL(timeline_tree), L"timeline-tree");
	}
	else if(wp==ID_CAPACITIES && _capsWnd) {
		RootWnd::RevealWindow(_capsWnd, _liveTab);
	}
	/** No splitters on the client */
	else if(wp==ID_COLLAPSE && role!=RoleClient) {
		if(!_splitter->IsCollapsed()) {
			_splitter->Collapse();
			_topSplitter->Collapse();
		}
		else {
			_splitter->Expand();
			_topSplitter->Expand();
		}
	}
	else if(wp==ID_EXIT) {
		SendMessage(GetWindow(), WM_CLOSE,0,0);
	}
	else if(wp==ID_EVENT_LOG) {
		RevealWindow(_eventLogWnd, _liveTab);
	}
	else if(wp==ID_FULLSCREEN) {
		ToggleFullscreen();
	}
	else if(wp==ID_GLOBAL_STOP) {
		Application::Instance()->GetInstances()->GetRootInstance()->SetPlaybackStateRecursive(PlaybackStop);
		_model->GetCapacities()->Reset();
		_model->GetVariables()->ResetAll();
	}
	else if(wp==ID_GLOBAL_PAUSE) {
		Application::Instance()->GetInstances()->GetRootInstance()->SetPlaybackStateRecursive(PlaybackPause, PlaybackPlay);
	}
	else if(wp==ID_GLOBAL_RESUME) {
		Application::Instance()->GetInstances()->GetRootInstance()->SetPlaybackStateRecursive(PlaybackPlay, PlaybackPause);
	}
	else if(wp==ID_GLOBAL_PLAY) {
		Application::Instance()->GetInstances()->GetRootInstance()->SetPlaybackState(PlaybackPlay);
	}
	else if(wp==ID_MODEL_PROPERTIES && role==RoleMaster) {
		ref<Path> p = GC::Hold(new Path());
		p->Add(_model->CreateModelCrumb());
		p->Add(TL(model_properties), L"icons/toolbar/properties.png", _model);
		Inspect(_model,p);
	}
	else if(wp==ID_NEW && role==RoleMaster) {
		if(!IsAnythingStillRunning()) {
			if(Alert::ShowYesNo(TL(application_name), TL(confirm_new), Alert::TypeQuestion)) {
				// Model::New will stop all playing tracks and call Application::OnClearShow for us
				_model->New();

				ref<Path> p = GC::Hold(new Path());
				p->Add(_model->CreateModelCrumb());
				p->Add(TL(model_properties), L"icons/toolbar/properties.png", _model);
				Inspect(_model, p);
				
				Application::Instance()->Update();
			}
		}
	}
	else if(wp==ID_OPEN && role==RoleMaster) {
		if(!IsAnythingStillRunning()) {
			std::wstring fn = Dialog::AskForOpenFile(this, TL(open_file_select), L"TJShow (*.tsx)\0*.tsx\0\0", L"tsx");
			Application::Instance()->ExecuteAction(GC::Hold(new OpenFileAction(Application::Instance(), fn, _model,false)));
		}
	}
	else if(wp==ID_OPEN_WEBCLIENT) {
		if(_liveTab) {
			ref<BrowserWnd> wnd = GC::Hold(new BrowserWnd(TL(browser_web_client)));
			wnd->SetShowToolbar(true);

			ref<Settings> st = GetSettings();
			if(st) {
				AddPane(GC::Hold(new Pane(TL(browser_web_client), wnd,false,true, st->GetNamespace(L"browser-wnd"), _liveTab->GetPlacement(), L"icons/tabs/browser.png")), true);
				wnd->Navigate(Application::Instance()->GetWebRootURL());
			}
		}
	}
	else if(wp==ID_IMPORT && role==RoleMaster) {
		if(!IsAnythingStillRunning()) {
			if(Alert::ShowYesNo(TL(application_name), TL(import_warning), Alert::TypeWarning)) {
				std::wstring fn = Dialog::AskForOpenFile(this, TL(open_file_select), L"TJShow (*.tsx)\0*.tsx\0\0", L"tsx");
				Application::Instance()->ExecuteAction(GC::Hold(new OpenFileAction(Application::Instance(), fn, _model, true)));
			}
		}
	}
	else if(wp==ID_PLAY) {
		Application::Instance()->GetInstances()->GetRootInstance()->SetPlaybackState(Application::Instance()->GetInstances()->GetRootInstance()->GetPlaybackState()==PlaybackPlay ? PlaybackStop : PlaybackPlay);
	}
	else if(wp==ID_PLUGINS) {
		ref<Settings> st = GetSettings();
		if(_liveTab && st) {
			AddPane(GC::Hold(new Pane(TL(plugins), GC::Hold(new PluginSettingsWnd(PluginManager::Instance())), false, true, st->GetNamespace(L"plugins-wnd"), _liveTab->GetPlacement())), true);
		}
	}
	else if(wp==ID_PAUSE) {
		Application::Instance()->GetInstances()->GetRootInstance()->SetPlaybackState(PlaybackPause);
	}
	else if(wp==ID_SAVE) {
		Application::Instance()->ExecuteAction(GC::Hold(new SaveFileAction(Application::Instance(), _model, SaveFileAction::TypeSave)));
	}
	else if(wp==ID_SAVEAS) {
		Application::Instance()->ExecuteAction(GC::Hold(new SaveFileAction(Application::Instance(), _model, SaveFileAction::TypeSaveAs)));
	}
	else if(wp==ID_SCRIPT) {
		if(_liveTab) {
			// TODO use editor settings like filewnd does
			ref<ScriptEditorWnd> wnd = GC::Hold(new ScriptEditorWnd(Application::Instance()->GetScriptContext()));
			ref<Settings> st = GetSettings()->GetNamespace(L"script-editor-wnd");
			if(st) {
				AddPane(GC::Hold(new Pane(TL(editor), wnd, false, true, st, _liveTab->GetPlacement(), L"icons/tabs/editor.png")), true);
			}
		}
	}
	else if(wp==ID_SHOW_LOG) {
		Core::Instance()->ShowLogWindow(true);
	}
	else if(wp==ID_SHOW_TOOLBAR && role==RoleMaster) {
		SetShowToolbar(!_tools->IsShown());
	}
	else if(wp==ID_DEBUG_INFO) {
		Application::Instance()->ExecuteAction(GC::Hold(new InfoAction(Application::InstanceReference())));
	}
	else if(wp==ID_VARIABLES) {
		RootWnd::RevealWindow(_variableWnd, _liveTab);
	}
}

ref<PropertyGridWnd> MainWnd::GetPropertyGrid() {
	return _properties;
}

ref<TimelineWnd> MainWnd::GetTimeWindow() {
	if(_time) {
		return _time->GetTimelineWindow();
	}
	return 0;
}

ref<TabWnd> MainWnd::GetTabWindow() {
	return _tab;
}

void MainWnd::Inspect(ref<Inspectable> isp, ref<Path> p) {
	if(isp) {
		_properties->Inspect(isp,p);
		RootWnd::RevealWindow(_properties);
	}

	Update();
	_properties->Update();
}

void MainWnd::OnFunctionKey(WPARAM key) {
}

void MainWnd::RevealTimeline(ref<SplitTimelineWnd> w) {
	RevealWindow(w, _timelines);
}

void MainWnd::OnAddTrack(ref<TrackWrapper> w) {
	ref<LiveControl> live = w->GetLiveControl();
	if(live) {
		live->SetPropertyGrid(_pg);

		if(live->IsSeparateTab()) {
			ref<Wnd> wnd = live->GetWindow();
			if(wnd) {
				if(live->IsInitiallyVisible()) {
					/** Sub timeline controls are a bit of a special case; they automatically get added to the timelines tab
					window. We do not have to wrap them in a LiveControlWrapperWnd, since SplitTimelineWnd is known to
					implement GetTabTitle/GetTabIcon for itself. **/
					if(wnd.IsCastableTo<SplitTimelineWnd>()) {
						_standaloneControls[w] = wnd;
						_timelines->AddPane(GC::Hold(new Pane(L"", wnd, false, false, 0, Placement())));
					}
					else {
						/* The LiveControlWrapperWnd is there to implement GetTabTitle, so the live control tab will aways
						have the up-to-date instance name of the track. */
						ref<LiveControlWrapperWnd> lww = GC::Hold(new LiveControlWrapperWnd(wnd, w));
						_standaloneControls[w] = ref<Wnd>(lww);

						/* Use the group name from the LiveControl to determine which settings we should use; this is probably
						the most in line with non-tab controls (they also use one settings namespace for all controls with the
						same group name). Default placement for these tab-control-panes is in the live tabs window, but the 
						settings may move it elsewhere. */
						ref<Settings> st = GetSettings()->GetNamespace(L"live-tabs")->GetNamespace(live->GetGroupName());
						AddPane(GC::Hold(new Pane(L"", lww, false, false, st, _liveTab->GetPlacement(), L"icons/tabs/slider.png")));
					}
				}
				else {
					AddOrphanPane(GC::Hold(new Pane(L"", wnd, false, false,0)));
				}
			}
		}
		else {
			std::wstring groupName = live->GetGroupName();
			ref<LiveGroupWnd> group; 

			std::map< std::wstring, ref<LiveGroupWnd> >::iterator it = _controlGroups.find(groupName);

			if(it==_controlGroups.end() || !((*it).second)) {
				group = GC::Hold(new LiveGroupWnd());
				
				ref<Settings> st = GetSettings()->GetNamespace(L"live-tabs")->GetNamespace(groupName);
				AddPane(GC::Hold(new Pane(groupName, group, false, false, st, _liveTab->GetPlacement(), L"icons/tabs/slider.png")));
				_controlGroups[groupName] = group;
			}
			else {
				group = (*it).second;
			}

			if(group) {
				group->AddControl(w, live);
			}
		}
	}
}

void MainWnd::OnRemoveTrack(ref<TrackWrapper> w) {
	if(_properties) {
		_properties->Inspect(0);
	}

	ref<LiveControl> live = w->GetLiveControl();
	if(live) {
		if(live->IsSeparateTab()) {
			weak<TrackWrapper> ww = w;
			std::map< weak<TrackWrapper>, weak<Wnd> >::iterator it = _standaloneControls.find(ww);
			if(it!=_standaloneControls.end()) {
				RemoveWindow(it->second);
				_standaloneControls.erase(it);
			}
		}
		else {
			ref<LiveGroupWnd> group = _controlGroups[live->GetGroupName()];
			if(group) {
				group->RemoveControl(w);

				if(group->GetCount()<1) {
					_controlGroups.erase(live->GetGroupName());
					RemoveWindow(group);
				}
			}	
		}
	}
}

void MainWnd::OnRemoveAllTracks() {
	// remove standalones
	std::map< weak<TrackWrapper>, weak<Wnd> >::iterator it = _standaloneControls.begin();
	while(it!=_standaloneControls.end()) {
		ref<Wnd> ctrl = it->second;
		if(ctrl) {
			RemoveWindow(ctrl);
		}
		++it;
	}
	_standaloneControls.clear();
	if(_dashboardWnd) _dashboardWnd->RemoveAllWidgets();

	// remove all group windows
	std::map< std::wstring, ref<LiveGroupWnd> >::iterator itt = _controlGroups.begin();
	while(itt!=_controlGroups.end()) {
		std::pair< std::wstring, ref<LiveGroupWnd> > data = *itt;
		RemoveWindow(data.second);
		itt++;
	}
	_controlGroups.clear();
}

void MainWnd::OnVariablesChanged() {
	if(_variableWnd) _variableWnd->Update();
}

void MainWnd::OnMoveTrackUp(ref<TrackWrapper> w) {
	std::map< std::wstring, ref<LiveGroupWnd> >::iterator itt = _controlGroups.begin();
	while(itt!=_controlGroups.end()) {
		std::pair< std::wstring, ref<LiveGroupWnd> > data = *itt;
		if(data.second) {
			data.second->MoveUp(w);
		}
		itt++;
	}
}

void MainWnd::OnMoveTrackDown(ref<TrackWrapper> w) {
	std::map< std::wstring, ref<LiveGroupWnd> >::iterator itt = _controlGroups.begin();
	while(itt!=_controlGroups.end()) {
		std::pair< std::wstring, ref<LiveGroupWnd> > data = *itt;
		if(data.second) {
			data.second->MoveDown(w);
		}
		itt++;
	}
}

bool MainWnd::IsToolbarShown() const {
	return (_tools && _tools->IsShown());
}

void MainWnd::ShowCommandWindow() {
	if(_tab) {
		_tab->SelectPane(3);
	}
}

ref<PropertyGridProxy> MainWnd::GetPropertyGridProxy() {
	return _pg;
}

/** PropertyGridProxyImpl **/
PropertyGridProxyImpl::PropertyGridProxyImpl(ref<MainWnd> pgw) {
	_view = pgw;
}

PropertyGridProxyImpl::~PropertyGridProxyImpl() {
}

void PropertyGridProxyImpl::SetProperties(ref<Inspectable> properties) {
	ref<MainWnd> mw = _view;
	if(mw) {
		mw->Inspect(properties);
	}
}
