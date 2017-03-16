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
#ifndef _TJEVENTLOGWND_H
#define _TJEVENTLOGWND_H

namespace tj {
	namespace show {
		namespace view {
			class EventLogWnd: public ListWnd, public virtual EventLogger {
				public:
					EventLogWnd();
					virtual ~EventLogWnd();
					virtual int GetItemCount();
					virtual void PaintItem(int id, tj::shared::graphics::Graphics& g, Area& row, const ColumnInfo& ci);
					virtual void AddEvent(const std::wstring& message, ExceptionType e, bool read);
					virtual void OnClickItem(int id, int col, Pixels x, Pixels y);
					virtual bool HasUnreadMessages();

				protected:
					enum {
						KColRead=1,
						KColType,
						KColDate,
						KColMessage,
					};

					struct EventMessage {
						inline EventMessage(const std::wstring& msg, const std::wstring& date, bool read, ExceptionType et) {
							_message = msg;
							_read = read;
							_date = date;
							_severity = et;
						}

						std::wstring _message;
						bool _read;
						std::wstring _date;
						ExceptionType _severity;
					};

					CriticalSection _lock;
					Icon _readIcon, _unreadIcon;
					std::deque<EventMessage> _messages;
					Icon _notifyIcon, _messageIcon, _errorIcon, _fatalIcon;
			};
		}
	}
}

#endif