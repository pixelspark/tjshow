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
#ifndef _TJINGLETOOLBAR_H
#define _TJINGLETOOLBAR_H

namespace tj {
	namespace jingle {
		class JingleSearchPopupWnd;

		class JingleToolbarWnd: public tj::shared::SearchToolbarWnd {
			public:
				JingleToolbarWnd();
				virtual ~JingleToolbarWnd();
				virtual void Paint(tj::shared::graphics::Graphics& g, tj::shared::ref<tj::shared::Theme> theme);
				virtual void CancelSearch();
				virtual void StartSearch();

			protected:
				virtual void OnSearchChange(const tj::shared::String& q);
				virtual void OnSearchCommit();
				virtual void OnCommand(int c);
				virtual void OnCommand(tj::shared::ref<tj::shared::ToolbarItem> ti);
				virtual void OnSize(const tj::shared::Area& ns);

				tj::shared::Icon _logo;
				tj::shared::ref<JingleSearchPopupWnd> _searchWnd;
		};
	}
}

#endif