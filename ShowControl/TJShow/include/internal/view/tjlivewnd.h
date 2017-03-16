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
#ifndef _TJLIVEWND_H
#define _TJLIVEWND_H

namespace tj {
	namespace show {
		namespace view {
			class LiveGroupWnd: public ChildWnd {	
				friend class LiveWnd;

				public:
					LiveGroupWnd();
					virtual ~LiveGroupWnd();
					virtual void Layout();

					void AddControl(ref<TrackWrapper> track, ref<LiveControl> control);
					void RemoveControl(ref<TrackWrapper> track);
					void Clear();
					virtual void Update();
					void SetFocus(int control);
					void MoveUp(ref<TrackWrapper> tr);
					void MoveDown(ref<TrackWrapper> tr);
					int GetCount();
					virtual void Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme);
					virtual void OnSize(const Area& ns);
					virtual void OnMouse(MouseEvent ev, Pixels x, Pixels y);
					virtual void OnScroll(ScrollDirection dir);
					virtual void SetFilter(const std::wstring& f);

				protected:
					void Layout(bool alsoSetScrollInfo);
					std::pair< ref<TrackWrapper>, ref<LiveControl> > GetControlAt(Pixels x, Pixels y);
					bool MatchesFilter(ref<TrackWrapper> tw, ref<LiveControl> lc) const;

					std::vector< std::pair< ref<TrackWrapper>, ref<LiveControl> > > _controls;
					ref<SearchToolbarWnd> _tools;
					std::wstring _filter;

					const static int KDefaultControlWidth = 30;
					const static int KDefaultHeaderHeight = 16;
					const static int KDefaultMarginH = 2;
					const static int KMaximalControlHeight = 255;
			};
		}
	}
}
#endif