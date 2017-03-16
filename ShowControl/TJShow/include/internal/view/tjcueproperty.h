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
#ifndef _TJ_CUEPROPERTY_H
#define _TJ_CUEPROPERTY_H

namespace tj {
	namespace show {
		namespace view {
			class DistantCuePropertyWnd;

			class DistantCueProperty: public Property {
				public:
					DistantCueProperty(const std::wstring& name, ref<CueList> list, std::wstring* tlid, std::wstring* cid);
					virtual ~DistantCueProperty();
					virtual ref<Wnd> GetWindow();
					virtual void Update();
					virtual Pixels GetHeight();

				protected:
					ref<CueList> _list;
					std::wstring *_tlid, *_cid;
					ref<DistantCuePropertyWnd> _wnd;
			};
		}
	}
}

#endif