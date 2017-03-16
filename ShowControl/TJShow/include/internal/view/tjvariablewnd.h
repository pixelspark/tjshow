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
#ifndef _TJVARIABLEWND_H
#define _TJVARIABLEWND_H

namespace tj {
	namespace show {	
		namespace view {
			class VariableToolbarWnd;
			class VariableListWnd;

			class VariableWnd: public ChildWnd {
				friend class VariableToolbarWnd;

				public:
					VariableWnd(ref<Variables> vars);
					virtual ~VariableWnd();
					virtual void Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme);
					virtual void Layout();
					virtual void OnSize(const Area& ns);
					virtual void OnSettingsChanged();
					virtual void Update();

				protected:
					ref<Variables> _variables;
					ref<VariableToolbarWnd> _toolbar;
					ref<VariableListWnd> _list;
			};
		}
	}
}

#endif