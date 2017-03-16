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
#ifndef _TJMAINWND_H
#define _TJMAINWND_H

namespace tj {
	namespace show {
		namespace view {
			class TimelineWnd;
			class TrayIcon;
			class NetworkWnd;
			class VariableWnd;
			class EventLogWnd;
			class SplitTimelineWnd;
			class FileWnd;
			class SettingsWnd;
			class LiveGroupWnd;
			class PlayerWnd;
			class CapacitiesWnd;
			class CapacitiesTabWnd;
			class ApplicationToolbarWnd;
			class TimelineTreeWnd;
			class DashboardWnd;

			class MainWnd: public RootWnd {
				public:
					MainWnd(const wchar_t* title);
					void Create(const wchar_t* title, Role role, ref<Model> model, strong<Settings> workspaceSettings);
					virtual ~MainWnd();
					ref<view::TimelineWnd> GetTimeWindow();
					ref<view::SplitTimelineWnd> GetSplitTimelineWindow();
					ref<PropertyGridWnd> GetPropertyGrid();
					ref<TabWnd> GetTabWindow();
					ref<TabWnd> GetLiveTabWindow();
					ref<view::CapacitiesWnd> GetCapacitiesWindow();
					ref<PropertyGridProxy> GetPropertyGridProxy();
					ref<DashboardWnd> GetDashboardWindow();

					void Command(WPARAM cmd);
					void OnFunctionKey(WPARAM key);
					virtual void SetFullScreen(bool fs);
					virtual void Layout();
					void ToggleFullscreen();
					bool IsFullscreen();
					virtual void Update();

					virtual LRESULT Message(UINT msg, WPARAM wp, LPARAM lp);
					virtual void Show(bool t);
					virtual void Paint(tj::shared::graphics::Graphics& g, ref<Theme> theme);
					
					void ShowCommandWindow();

					void Inspect(ref<Inspectable> isp, ref<Path> p = 0);
					void SetShowToolbar(bool t);
					bool IsToolbarShown() const;
					ref<view::EventLogWnd> GetEventLogWindow();
					void AddPlayerWindow(ref<Wnd> pw, ref<Settings> settings);

					// call when tracks are added or removed, so View can add/remove live controls
					void OnAddTrack(ref<TrackWrapper> w);
					void OnRemoveTrack(ref<TrackWrapper> w);
					void OnRemoveAllTracks();
					void OnVariablesChanged();
					void OnMoveTrackUp(ref<TrackWrapper> w);
					void OnMoveTrackDown(ref<TrackWrapper> w);
					void SetFileName(std::wstring fn);
					void RevealTimeline(ref<view::SplitTimelineWnd> w);
					void ShowPatcher();
					void AddOtherPanes();

				protected:
					virtual void GetMinimumSize(Pixels& w, Pixels& h);
					virtual void OnSettingsChanged();
					virtual void OnDropFiles(const std::vector< std::wstring >& files);
					virtual void OnTimer(unsigned int id);
					virtual void GetAccelerators(std::vector<Accelerator>& accels);
					bool IsAnythingStillRunning();

				private:
					ref<SplitterWnd> _treeSplitter;
					ref<SplitterWnd> _splitter;
					ref<SplitterWnd> _topSplitter;
					ref<SplitTimelineWnd> _time;
					ref<TimelineTreeWnd> _tree;
					ref<TabWnd> _tab;
					ref<TabWnd> _timelines;
					ref<PropertyGridWnd> _properties;
					ref<PropertyGridProxy> _pg;
					ref<TabWnd> _liveTab;
					ref<FileWnd> _fileWnd;
					ref<DashboardWnd> _dashboardWnd;
					ref<EventLogWnd> _eventLogWnd;
					ref<Model> _model;
					ref<ApplicationToolbarWnd> _tools;
					ref<CapacitiesWnd> _capsWnd;
					ref<VariableWnd> _variableWnd;
					weak<SettingsWnd> _settingsWnd;

					// live controls
					std::map< weak<TrackWrapper>,  weak<Wnd> > _standaloneControls;
					std::map< std::wstring, ref<view::LiveGroupWnd> > _controlGroups;
					std::wstring _lastScript;

					virtual void OnSize(const Area& ns);

					enum {MinSizeX = 640, MinSizeY = 480};

				private:
					const static int KTickTimerID = 1;
			};
		
		}
	}
}

#endif
