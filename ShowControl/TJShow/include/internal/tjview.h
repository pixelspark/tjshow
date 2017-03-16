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
#ifndef _TJVIEW_H
#define _TJVIEW_H

namespace tj {
	namespace show {
		namespace view {
			class TrayIcon;
			class MainWnd;
			class ClientMainWnd;
			class SplitTimelineWnd;
			class PreviewWnd;
			class DashboardWnd;

			class View {
				public:
					View(strong<Settings> workspaceSettings, strong<Settings> appSettings);
					virtual ~View();
					virtual void Create(const wchar_t* title, Role role, ref<Model> model, ref<OutputManager> om);
					virtual void AddPlayerWindow(ref<Wnd> pw);
					virtual ref<RootWnd> GetRootWindow();
					virtual void Inspect(ref<Inspectable> isp, ref<Path> p = 0);

					/* These 'event methods' can be called from any thread. View should not make
					any assumptions about that and should only call non-blocking stuff (so,
					no direct UI manipulations except for simple stuff like Repaint */
					virtual void OnAddTrack(ref<TrackWrapper> w);
					virtual void OnCapacityChanged();
					virtual void OnClearShow();
					virtual void OnFileLoaded(ref<Model> model, const std::wstring& fileName);
					virtual void OnFileSaved(ref<Model> model, const std::wstring& fileName);
					virtual void OnFunctionKey(int vk);
					virtual void OnMoveTrackDown(ref<TrackWrapper> tw);
					virtual void OnMoveTrackUp(ref<TrackWrapper> tw);
					virtual void OnRemoveTrack(ref<TrackWrapper> tw);
					virtual void OnThemeChanged();
					virtual void OnVariablesChanged();

					virtual void Command(int c);
					virtual void OpenScriptEditor(const std::wstring& path, ref<tj::script::ScriptContext> sc);
					virtual void OpenBrowser(const std::wstring& url, const std::wstring& title = L"");
					virtual void AddProblemReport(const std::wstring& title, ref<Wnd> plw);
					virtual void AddUtilityWindow(ref<Wnd> wnd, const std::wstring& title, const std::wstring& settingsClass, const std::wstring& iconPath=L"");
					virtual void RevealTimeline(ref<SplitTimelineWnd> sp);
					virtual void ShowPatcher();
					virtual void Show(bool s);
					virtual ref<EventLogger> GetEventLogger();
					virtual ref<PropertyGridProxy> GetPropertyGridProxy();
					virtual void Update();
					virtual ref<DashboardWnd> GetDashboardWindow();

				protected:
					Role _role;
					ref<MainWnd> _mainWindow;
					ref<PreviewWnd> _preview;
					ScreensaverOff _scOff;
					strong<Settings> _appSettings;
					strong<Settings> _workspaceSettings;

					// In client mode only
					ref<ClientMainWnd> _clientWindow;

			};
		}
	}
}

#endif
