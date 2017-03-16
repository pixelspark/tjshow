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
#include "../../include/internal/view/tjtray.h"
#include <memory.h>
#include <shellapi.h>

using namespace tj::show::view;

TrayIcon::TrayIcon(std::wstring title, HICON icon, UINT msg, HWND parent) {
	_nid = malloc(sizeof(NOTIFYICONDATA));
	memset(_nid, 0, sizeof(NOTIFYICONDATA));

	NOTIFYICONDATA* nid = reinterpret_cast<NOTIFYICONDATA*>(_nid);
	nid->cbSize = sizeof(NOTIFYICONDATA);
	nid->hIcon = icon;
	nid->hWnd = parent;
	nid->uCallbackMessage = msg;
	wcscpy_s(nid->szTip, 127, title.c_str());
	nid->uFlags = NIF_ICON|NIF_TIP|NIF_MESSAGE;
	nid->uID = 0;
	Shell_NotifyIcon(NIM_ADD, nid);
}

TrayIcon::~TrayIcon() {
	Shell_NotifyIcon(NIM_DELETE, reinterpret_cast<NOTIFYICONDATA*>(_nid));
	free(_nid);
}