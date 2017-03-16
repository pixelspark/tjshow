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
#ifndef _TJCLIENTSWND_H
#define _TJCLIENTSWND_H

namespace tj {
	namespace show {
		namespace view {
			class GroupsListWnd;
			class ClientListWnd;
			
			class ClientsWnd: public ChildWnd {
				public:
					ClientsWnd(strong<Network> net);
					virtual ~ClientsWnd();
					virtual void Paint(graphics::Graphics& g, strong<Theme> theme);
					virtual void Layout();
					virtual void OnSize(const Area& ns);
					virtual void OnSettingsChanged();

				protected:
					ref<ToolbarWnd> _tools;
					ref<ClientListWnd> _list;

			};

			class GroupsWnd: public ChildWnd {
				public:
					GroupsWnd(strong<Groups> groups);
					virtual ~GroupsWnd();
					virtual void Paint(graphics::Graphics& g, strong<Theme> theme);
					virtual void Layout();
					virtual void OnSize(const Area& ns);
					virtual void OnSettingsChanged();

				private:
					ref<ToolbarWnd> _tools;
					ref<GroupsListWnd> _list;
			};
		}
	}
}

#endif