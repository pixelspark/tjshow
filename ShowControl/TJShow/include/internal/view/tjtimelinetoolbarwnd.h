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
#ifndef _TJTIMELINETOOLBAR_H
#define _TJTIMELINETOOLBAR_H

namespace tj {
	namespace show {
		namespace view {
			class TimelineWnd;

			class TimelineTimeToolbarItem: public ToolbarItem {
				public:
					TimelineTimeToolbarItem(int command);
					virtual ~TimelineTimeToolbarItem();
					virtual void Paint(graphics::Graphics& g, strong<Theme> theme, bool over, bool down, float alpha);
					virtual void SetTime(const std::wstring& time);

				protected:
					std::wstring _time;
			};

			class TimelineToolbarWnd: public SearchToolbarWnd {
				public:
					TimelineToolbarWnd(TimelineWnd* tw);
					virtual ~TimelineToolbarWnd();
					virtual void Update();
					virtual void UpdateThreaded();
					
					const static int KTimeWidth = 95;

				protected:
					virtual void OnMouse(MouseEvent ev, Pixels x, Pixels y);
					virtual void OnCommand(int c);
					virtual void OnCommand(ref<ToolbarItem> ti);
					virtual LRESULT Message(UINT msg, WPARAM wp, LPARAM lp);
					virtual void OnSearchChange(const std::wstring& q);

					// preserve button order!
					enum Commands {
						CommandNone=0,
						CommandAdd,
						CommandAddFromFile,
						CommandRemove,
						CommandUp,
						CommandDown,
						CommandViewSettings,
						CommandZoom,
						CommandInsertAll,
						CommandInsertOne,
						CommandRunModes,
						CommandProperties,
						CommandPlayStop,
						CommandPlay,
						CommandStop,
						CommandRestart,
						CommandPause,
					};

					TimelineWnd* _timeline;
					ref<ToolbarItem> _zoomItem;
					ref<ToolbarItem> _playItem;
					ref<ToolbarItem> _stopItem;
					ref<ToolbarItem> _restartItem;
					ref<ToolbarItem> _pauseItem;
					ref<TimelineTimeToolbarItem> _timeItem;
					MouseCapture _capture;

					bool _isDraggingZoom;
					int _zoomX, _zoomY;
			};
		}
	}
}

#endif