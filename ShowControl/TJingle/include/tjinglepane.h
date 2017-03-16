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
#ifndef _TJINGLEPANE_H
#define _TJINGLEPANE_H

namespace tj {
	namespace jingle {
		class JinglePaneToolbarWnd;

		class JinglePane: public tj::shared::ChildWnd {
			friend class JinglePaneToolbarWnd;
			friend class JingleView;

			public:
				JinglePane(HWND parent, tj::shared::ref<JingleCollection> collection);
				virtual ~JinglePane();
				virtual void Layout();
				virtual LRESULT Message(UINT msg, WPARAM wp, LPARAM lp);
				virtual void Paint(tj::shared::graphics::Graphics& g, tj::shared::strong<tj::shared::Theme> theme);
				virtual std::wstring GetTabTitle() const;
				virtual void GetAccelerators(std::vector<tj::shared::Accelerator>& alist);
				virtual void Update();
				virtual tj::shared::ref<JingleCollection> GetJingleCollection();
				virtual void OnThemeChanged();
				virtual tj::shared::ref<Jingle> GetJingleAt(tj::shared::Pixels x, tj::shared::Pixels y);
				
			protected:
				void DoContextMenu(tj::shared::Pixels x, tj::shared::Pixels y, tj::shared::ref<Jingle> jingle);
				virtual void OnMouse(tj::shared::MouseEvent ev, tj::shared::Pixels x, tj::shared::Pixels y);
				virtual void OnSize(const tj::shared::Area& ns);
				virtual void OnTimer(unsigned int id);
				virtual void OnFocus(bool focus);
				virtual LRESULT PreMessage(UINT msg, WPARAM wp, LPARAM lp);

				tj::shared::ref<JinglePaneToolbarWnd> _toolbar;
				std::wstring _name;
				tj::shared::ref<JingleCollection> _collection;
				tj::shared::Icon _loadedIcon;
		};
	}
}

#endif