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
#ifndef _TJDASHBOARD_H
#define _TJDASHBOARD_H

#include "../tjdashboard.h"

namespace tj {
	namespace show {
		namespace view {
			class DashboardWnd: public ChildWnd {
				public:
					DashboardWnd(ref<Dashboard> db);
					virtual ~DashboardWnd();
					virtual void SetDashboard(ref<Dashboard> db);
					
					virtual void Paint(graphics::Graphics& g, strong<Theme> theme);
					virtual void Layout();
					virtual void SetDesignMode(bool d);
					virtual bool IsInDesignMode() const;
					virtual void RemoveWidget(ref<Widget> wi);
					virtual void RemoveAllWidgets();
					virtual void AddWidget(ref<Widget> wi);
					virtual void GetAccelerators(std::vector<Accelerator>& accs);

				protected:
					virtual void OnSize(const Area& ns);
					virtual void OnMouse(MouseEvent e, Pixels x, Pixels y);
					virtual void OnKey(Key k, wchar_t ch, bool down, bool isAccelerator);
					virtual void OnContextMenu(Pixels x, Pixels y);
					virtual void OnCreated();
					virtual void OnTimer(unsigned int id);
					virtual void RepaintIfNecessary();

					bool _designMode;
					ref<ToolbarWnd> _toolbar;
					ref<Dashboard> _dashboard;
					ref<Widget> _dragging;
					Area _draggingOldArea;
					ref<Widget> _focus;
					Pixels _dragOffsetX, _dragOffsetY;
					bool _isDraggingSize;
					Icon _grabberIcon;

					const static Pixels KResizeAreaSize = 16;
			};

			class DashboardToolbarWnd: public ToolbarWnd {
				public:
					DashboardToolbarWnd(ref<DashboardWnd> dw);
					virtual ~DashboardToolbarWnd();
					virtual void OnCommand(ref<ToolbarItem> ti);

				protected:
					ref<DashboardWnd> _dw;
					ref<ToolbarItem> _designItem;
			};
		}
	}
}

#endif