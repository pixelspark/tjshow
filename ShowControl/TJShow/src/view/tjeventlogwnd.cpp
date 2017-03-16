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
#include "../../include/internal/tjshow.h"
#include "../../include/internal/view/tjeventlogwnd.h"
using namespace tj::show::view;
using namespace tj::show;
using namespace tj::shared::graphics;

EventLogWnd::EventLogWnd(): ListWnd(), 
_readIcon(L"icons/notifications/read.png"), 
_unreadIcon(L"icons/notifications/unread.png"),
_notifyIcon(Icons::GetIconPath(Icons::IconExceptionNotify)),
_fatalIcon(Icons::GetIconPath(Icons::IconExceptionFatal)),
_errorIcon(Icons::GetIconPath(Icons::IconExceptionError)),
_messageIcon(Icons::GetIconPath(Icons::IconExceptionMessage))
{
	AddColumn(TL(event_read), KColRead, 0.1f);
	AddColumn(TL(event_type), KColType, 0.1f);
	AddColumn(TL(event_date), KColDate, 0.1f);
	AddColumn(TL(event_message), KColMessage, 0.7f);
	SetEmptyText(TL(event_list_empty_text));
}

EventLogWnd::~EventLogWnd() {

}

int EventLogWnd::GetItemCount() {
	return (int)_messages.size();
}

bool EventLogWnd::HasUnreadMessages() {
	ThreadLock lock(&_lock);
	std::deque<EventMessage>::iterator it = _messages.begin();
	while(it!=_messages.end()) {
		EventMessage& em = *it;
		if(!em._read && em._severity!=ExceptionTypeMessage) return true;
		++it;
	}
	return false;
}

void EventLogWnd::AddEvent(const std::wstring& msg, ExceptionType e, bool read) {
	Date date;

	{
		ThreadLock lock(&_lock);
		_messages.push_front(EventMessage(msg, date.ToFriendlyString(), read, e));
	}
	Repaint();
}

void EventLogWnd::PaintItem(int id, Graphics &g, tj::shared::Area &row, const ColumnInfo& ci) {
	ThreadLock lock(&_lock);

	EventMessage& em = _messages.at(id);
	strong<Theme> theme = ThemeManager::GetTheme();

	StringFormat sf;
	sf.SetTrimming(StringTrimmingEllipsisCharacter);
	SolidBrush br(theme->GetColor(Theme::ColorText));

	switch(em._severity) {
		case ExceptionTypeError:
			DrawCellIcon(g, KColType, row, _errorIcon);
			break;

		case ExceptionTypeSevere:
			DrawCellIcon(g, KColType, row, _fatalIcon);
			break;

		case ExceptionTypeMessage:
			DrawCellIcon(g, KColType, row, _messageIcon);
			break;

		case ExceptionTypeWarning:
			DrawCellIcon(g, KColType, row, _notifyIcon);
	}

	if(!em._read) {
		DrawCellIcon(g, KColRead, row, _unreadIcon);
	}
	DrawCellText(g, &sf, &br, theme->GetGUIFont(), KColMessage, row, em._message);
	DrawCellText(g, &sf, &br, theme->GetGUIFont(), KColDate, row, em._date);
}

void EventLogWnd::OnClickItem(int id, int col, Pixels x, Pixels y) {
	ThreadLock lock(&_lock);

	if(int(_messages.size())>id) {
		EventMessage& em = _messages.at(id);
		em._read = true;
	}
	ListWnd::SetSelectedRow(id);
}