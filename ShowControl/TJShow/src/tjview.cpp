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
#include "../include/internal/tjnetwork.h"

#include "../include/internal/view/tjmainwnd.h"
#include "../include/internal/view/tjtray.h"
#include "../include/internal/view/tjcapacitywnd.h"
#include "../include/internal/view/tjscripteditorwnd.h"
#include "../include/internal/view/tjtimelinewnd.h"
#include "../include/internal/view/tjcuelistwnd.h"
#include "../include/internal/view/tjplayerwnd.h"
#include "../include/internal/view/tjpreviewwnd.h"
#include "../include/internal/view/tjeventlogwnd.h"
#include "../include/internal/view/tjsplittimelinewnd.h"
#include "../include/internal/view/tjdashboardwnd.h"

using namespace tj::show::view;
using namespace tj::shared::graphics;

/* ClientMainWnd */
namespace tj {
	namespace show {
		namespace view {
			class ClientMainWnd: public DialogWnd {
				public:
					ClientMainWnd();
					virtual ~ClientMainWnd();
					virtual void Layout();
					virtual void OnSize(const Area& ns);
					virtual void OnCreated();
					virtual void Paint(tj::shared::graphics::Graphics& g, ref<Theme> theme);
					virtual void Notify(ref<Object> source, const ButtonWnd::NotificationClicked& evt);
					virtual LRESULT Message(UINT msg, WPARAM wp, LPARAM lp);

				protected:
					void DoClientMenu();
					const static int KTrayNotifyMessage = (WM_USER+1337);
					ref<ButtonWnd> _btnExit;
					ref<ButtonWnd> _btnLog;
					ref<TrayIcon> _tray;
			};
		}
	}
}

ClientMainWnd::ClientMainWnd(): DialogWnd(TL(application_name)) {
	_tray = GC::Hold(new TrayIcon(TL(application_name), LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_TJ)), KTrayNotifyMessage, GetWindow()));
	SetSize(280,280);
	
	_btnExit = GC::Hold(new ButtonWnd(L"icons/client/exit.png", TL(client_exit)));
	_btnLog = GC::Hold(new ButtonWnd(L"icons/client/log.png", TL(client_show_log)));
	Add(_btnExit, true);
	Add(_btnLog, true);
}

void ClientMainWnd::OnCreated() {
	_btnExit->EventClicked.AddListener(this);
	_btnLog->EventClicked.AddListener(this);
	DialogWnd::OnCreated();
}

void ClientMainWnd::Notify(ref<Object> source, const ButtonWnd::NotificationClicked& evt) {
	if(source==_btnExit) {
		PostQuitMessage(0);	
		EndModal(true);
	}
	else if(source==_btnLog) {
		Core::Instance()->ShowLogWindow(true);
		EndModal(true);
	}
	
	DialogWnd::Notify(source, evt);
}

void ClientMainWnd::DoClientMenu() {
	Mouse::Instance()->SetCursorHidden(false);
	DoModal(GetParent());
	Mouse::Instance()->SetCursorHidden(true);
}

ClientMainWnd::~ClientMainWnd() {
}

void ClientMainWnd::OnSize(const Area& ns) {
	Layout();
	DialogWnd::OnSize(ns);
}

void ClientMainWnd::Layout() {
	Area rc = GetClientArea();
	rc.Narrow(5,5,5,5);
	if(_btnExit) {
		Area row = rc;
		row.SetHeight(24);
		_btnExit->Move(row.GetLeft(), row.GetTop(), row.GetWidth(), row.GetHeight());

		if(_btnLog) {
			row.Translate(0,30);
			_btnLog->Move(row.GetLeft(), row.GetTop(), row.GetWidth(), row.GetHeight());
		}
	}
	DialogWnd::Layout();
}

void ClientMainWnd::Paint(tj::shared::graphics::Graphics& g, ref<Theme> theme) {
	Area rc = GetClientArea();
	SolidBrush backBr(theme->GetColor(Theme::ColorBackground));
	g.FillRectangle(&backBr, rc);
	
	Area text = rc;
	text.SetHeight(24);
	text.Translate(0, rc.GetTop()+60);

	StringFormat sf;
	sf.SetAlignment(StringAlignmentCenter);
	sf.SetLineAlignment(StringAlignmentCenter);

	SolidBrush tbr(theme->GetColor(Theme::ColorText));
	std::wstring info = TL(application_name) + std::wstring(L" ") + Version::GetRevisionName() + L" / " + StringifyHex(Application::Instance()->GetNetwork()->GetInstanceID());
	g.DrawString(info.c_str(), (int)info.length(), theme->GetGUIFontBold(), text, &sf, &tbr);

	DialogWnd::Paint(g,theme);
}

LRESULT ClientMainWnd::Message(UINT msg, WPARAM wp, LPARAM lp) {
	if(msg==KTrayNotifyMessage && lp==WM_LBUTTONDBLCLK) {
		DoClientMenu();
		return 0;
	}
	return DialogWnd::Message(msg,wp,lp);
}

/* View */
View::View(strong<Settings> workspaceSettings, strong<Settings> appSettings): _workspaceSettings(workspaceSettings), _appSettings(appSettings) {
}

View::~View() {
}

void View::Create(const wchar_t* title, Role role, ref<Model> model, ref<OutputManager> om) {
	_role = role;

	if(role==RoleClient) {
		std::vector< ref<PlayerWnd> > players;
		om->AddAllScreens(players);
		std::vector< ref<PlayerWnd> >::iterator it = players.begin();
		while(it!=players.end()) {
			ref<PlayerWnd> player = *it;
			player->Show(true);
			++it;
		}

		ref<PlayerWnd> mainPlayerWnd = players.at(0);
		_clientWindow = GC::Hold(new ClientMainWnd());
		mainPlayerWnd->Add(_clientWindow,false);
	}
	else {
		_mainWindow = GC::Hold(new MainWnd(title));
		_mainWindow->Create(title, role, model,  _workspaceSettings);

		// If we're in demo, show some nice advertising
		strong<ScreenDefinition> sd = Application::Instance()->GetModel()->GetPreviewScreenDefinition(_appSettings->GetNamespace(L"view.video.default-screen-definition"));
		_preview = GC::Hold(new PreviewWnd(om->AddScreen(TL(screen_preview), sd)));
		AddPlayerWindow(_preview);

		_mainWindow->AddOtherPanes();
	}
}

void View::AddPlayerWindow(ref<Wnd> pw) {
	if(_mainWindow && pw) {
		_mainWindow->AddPlayerWindow(pw, _workspaceSettings->GetNamespace(L"player-wnd"));
	}
}

ref<PropertyGridProxy> View::GetPropertyGridProxy() {
	if(_mainWindow) {
		return _mainWindow->GetPropertyGridProxy();
	}
	return 0;
}

void View::AddProblemReport(const std::wstring& title, ref<Wnd> plw) {
	AddUtilityWindow(plw, title, L"problem-report-wnd", L"icons/tabs/problems.png");
}

void View::AddUtilityWindow(ref<Wnd> wnd, const std::wstring& title, const std::wstring& settingsClass, const std::wstring& iconPath) {
	if(wnd) {
		ref<Settings> utilitySettings = _workspaceSettings->GetNamespace(settingsClass);
		wnd->SetSettings(utilitySettings);

		if(_mainWindow) {
			ref<Pane> pane = GC::Hold(new Pane(title, wnd, false, true, utilitySettings->GetNamespace(L"pane"), _mainWindow->GetLiveTabWindow()->GetPlacement(), iconPath));
			_mainWindow->AddPane(pane, true);
		}
	}
}

void View::Inspect(ref<Inspectable> isp, ref<Path> p) {
	if(_mainWindow) {
		_mainWindow->Inspect(isp, p);
	}
}

void View::Update() {
	if(_mainWindow) {
		_mainWindow->Update();
	}
}

void View::OnCapacityChanged() {
	if(_mainWindow) {
		ref<CapacitiesWnd> cw = _mainWindow->GetCapacitiesWindow();
		if(cw) {
			cw->Update();
		}
	}
}

void View::OnThemeChanged() {
	if(_mainWindow) {
		_mainWindow->Layout();
		_mainWindow->FullRepaint();
	}
}

ref<RootWnd> View::GetRootWindow() {
	return _mainWindow;
}

ref<DashboardWnd> View::GetDashboardWindow() {
	if(_mainWindow) return _mainWindow->GetDashboardWindow();
	return null;
}

void View::ShowPatcher() {
	if(_mainWindow) {
		_mainWindow->ShowPatcher();
	}
}

void View::OnVariablesChanged() {
	if(_mainWindow) {
		_mainWindow->OnVariablesChanged();
	}
}

void View::OnFunctionKey(int vk) {
	if(_mainWindow) {
		_mainWindow->OnFunctionKey(vk);
	}
}

void View::OnRemoveTrack(ref<TrackWrapper> tw) {
	if(_mainWindow) {
		_mainWindow->OnRemoveTrack(tw);
	}
}

ref<EventLogger> View::GetEventLogger() {
	if(_mainWindow) {
		return _mainWindow->GetEventLogWindow();
	}
	return 0;
}

void View::Command(int c) {
	if(_mainWindow) {
		_mainWindow->Command(c);
	}
}

void View::Show(bool s) {
	if(_mainWindow) {
		_mainWindow->Show(s);
	}
}

void View::RevealTimeline(ref<SplitTimelineWnd> sp) {
	if(_mainWindow) {
		_mainWindow->RevealTimeline(sp);
	}
}

void View::OpenBrowser(const std::wstring& url, const std::wstring& title) {
	if(_mainWindow) {
		ref<TabWnd> tw = _mainWindow->GetLiveTabWindow();
		if(tw) {
			ref<BrowserWnd> browser = GC::Hold(new BrowserWnd(L""));
			browser->Navigate(url);
			tw->AddPane(GC::Hold(new Pane(title.length()>0 ? title : url, browser, true, true,0)));
		}
	}
}

void View::OpenScriptEditor(const std::wstring& path, ref<tj::script::ScriptContext> sc) {
	if(_mainWindow) {
		ref<ScriptEditorWnd> wnd = GC::Hold(new ScriptEditorWnd(sc));
		wnd->Open(path);
		ref<Pane> pane = GC::Hold(new Pane(L"", wnd, false, true, _workspaceSettings->GetNamespace(L"script-editor-wnd"), _mainWindow->GetLiveTabWindow()->GetPlacement(), L"icons/tabs/editor.png"));
		_mainWindow->AddPane(pane,true);
	}
}

void View::OnClearShow() {
	if(_mainWindow) {
		ref<PropertyGridWnd> pg = _mainWindow->GetPropertyGrid();
		if(pg) {
			pg->Clear();
		}
		_mainWindow->OnRemoveAllTracks();
		_mainWindow->SetFileName(L"");
	}

	if(_preview) {
		_preview->SetDefinition(Application::Instance()->GetModel()->GetPreviewScreenDefinition(_appSettings->GetNamespace(L"view.video.default-screen-definition")));
	}
}

void View::OnFileLoaded(ref<Model> model, const std::wstring& file) {
	if(_mainWindow) {
		ref<Path> p = GC::Hold(new Path());
		p->Add(model->CreateModelCrumb());
		Inspect(model, p);
		_mainWindow->SetFileName(file);

		ref<TimelineWnd> tw = _mainWindow->GetTimeWindow();
		if(tw) {
			tw->SetTimeline(model->GetTimeline());
			tw->SetCueList(model->GetCueList());
		}
	}

	if(_preview) {
		_preview->SetDefinition(Application::Instance()->GetModel()->GetPreviewScreenDefinition(_appSettings->GetNamespace(L"view.video.default-screen-definition")));
	}
}

void View::OnFileSaved(ref<Model> model, const std::wstring& fn) {
	if(_mainWindow) {
		_mainWindow->SetFileName(fn);
	}
}

void View::OnAddTrack(ref<TrackWrapper> tw) {
	if(_mainWindow) {
		_mainWindow->OnAddTrack(tw);
	}
}

void View::OnMoveTrackUp(ref<TrackWrapper> tw) {
	if(_mainWindow) {
		_mainWindow->OnMoveTrackUp(tw);
	}
}

void View::OnMoveTrackDown(ref<TrackWrapper> tw) {
	if(_mainWindow) {
		_mainWindow->OnMoveTrackDown(tw);
	}
}