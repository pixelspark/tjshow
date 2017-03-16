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
#include "../include/internal/tjshow.h" 
// $Rev$
// $Date$
using namespace tj::show; 

const wchar_t* Version::_revisionID = L"$Rev$";
const wchar_t* Version::_revisionDate = L"$Date$";
const wchar_t* Version::_revisionName = L"v4.0 (public beta b1)";

int Version::GetRevisionID() {
	int n = (int)wcslen(_revisionID);
	std::wstring r(_revisionID,6,n-7);
	return StringTo<int>(r,-1);
}

std::wstring Version::GetRevisionDate() {
	int n = (int)wcslen(_revisionDate);
	return std::wstring(_revisionDate, 1, n-2);
}  

std::wstring Version::GetRevisionName() {
	return _revisionName;
}
