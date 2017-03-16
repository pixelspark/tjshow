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
#ifndef _TJCUELISTWND_H
#define _TJCUELISTWND_H

namespace tj {
	namespace show {
		namespace view {
			class CueListWnd;
			class CueListToolbarWnd;

			class CueWnd: public ChildWnd, public virtual Listener<ButtonWnd::NotificationClicked> {
				public:
					CueWnd(strong<Instance> c);
					virtual ~CueWnd();
					virtual void Paint(graphics::Graphics& g, strong<Theme> theme);
					virtual void OnSize(const Area& ns);
					virtual void Layout();
					virtual void Update();
					virtual ref<CueListWnd> GetCueListWindow();
					virtual void Notify(ref<Object> source, const ButtonWnd::NotificationClicked& evt);
					virtual void OnCreated();

				protected:
					ref<CueListToolbarWnd> _toolbar;
					ref<CueListWnd> _list;
					ref<ButtonWnd> _next;
					strong<Instance> _controller;
			};

			class CueListToolbarWnd: public SearchToolbarWnd {
				public:
					CueListToolbarWnd(CueListWnd* cw);
					virtual ~CueListToolbarWnd();

				protected:
					virtual void OnCommand(ref<ToolbarItem> ti);
					virtual void OnCommand(int c);
					virtual void OnSearchChange(const std::wstring& q);

					enum Commands {
						KNone=0,
						KAddCue,
						KViewSettings,
					};

					CueListWnd* _cw;
			};

			class CueListWnd: public ChildWnd {
				friend class CueListToolbarWnd;

				public:
					CueListWnd(strong<Instance> inst);
					virtual ~CueListWnd();
					virtual void Update();
					virtual void Layout();
					virtual void Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme);

					void SetShowNameless(bool t);
					bool GetShowNameless() const;

					TimeFormat GetTimeFormat() const;
					void SetTimeFormat(TimeFormat f);

					void SetShowPlayCuesOnly(bool t);
					bool GetShowPlayCuesOnly() const;

					void SetFilter(const std::wstring& q);
					static void DoCueMenu(ref<Wnd> wnd, ref<Cue> cue, ref<CueList> list, Pixels x, Pixels y);

				protected:
					bool IsCueVisible(ref<Cue> c);
					void UpdateScrollBars();
					void DoCueMenu(ref<Cue> cue, Pixels x, Pixels y);
					ref<Cue> GetCueAt(Pixels h);
					virtual void OnMouse(MouseEvent ev, Pixels x, Pixels y);
					virtual void OnSettingsChanged();
					virtual void OnSize(const Area& ns);
					virtual void OnContextMenu(Pixels x, Pixels y);
					int GetItemHeightInPixels() const;
					virtual void OnCopy();
					virtual void OnPaste();
					virtual void OnCut();

					strong<CueList> _cues;
					strong<Instance> _controller;
					ref<Cue> _over;
					ref<CueListToolbarWnd> _toolbar;
					bool _in;
					bool _showNameless;
					bool _onlyPlayCues;
					TimeFormat _format;
					std::wstring _filter;

					Icon _actionPlayIcon, _actionPauseIcon, _actionStopIcon, _triggerIcon, _guardIcon, _conditionIcon, _assignmentIcon;

					const static int KTimeColumnWidth = 70;
			};
		}
	}
}

#endif