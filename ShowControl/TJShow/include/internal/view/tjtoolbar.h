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
#ifndef _TJTOOLBAR_H
#define _TJTOOLBAR_H

#include "../tjnetwork.h"

namespace tj {
	namespace show {
		namespace view {
			class ApplicationToolbarWnd: public ToolbarWnd, public Listener<Network::Notification>, public Listener<UndoStack::UndoNotification> {
				public:
					ApplicationToolbarWnd();
					virtual ~ApplicationToolbarWnd();
					virtual void Update();
					virtual void Notify(ref<Object> source, const Network::Notification& notification);
					virtual void Notify(ref<Object> source, const UndoStack::UndoNotification& notification);
					
				protected:
					virtual void OnCommand(ref<ToolbarItem> ti);
					virtual void OnCommand(int c);
					virtual void OnTimer(unsigned int id);
					virtual void OnCreated();

					void DoPromoteMenu();

					Icon _primaryIcon, _promotingIcon;
					ref<ToolbarItem> _promoteItem;
					ref<ToolbarItem> _messagesItem;
					ref<ToolbarItem> _globalPlayItem;
					ref<ToolbarItem> _undoItem, _redoItem;
			};
		}
	}
}

#endif