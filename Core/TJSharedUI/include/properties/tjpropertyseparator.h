/* This file is part of TJShow. TJShow is free software: you 
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
 
 #ifndef _TJPROPERTYSEPARATOR_H
#define _TJPROPERTYSEPARATOR_H

namespace tj {
	namespace shared {
		class EXPORTED PropertySeparator: public Property {
			public:
				PropertySeparator(const String& group, bool collapsed = false);
				virtual ~PropertySeparator();
				virtual ref<Wnd> GetWindow();
				virtual void Update();
				virtual int GetHeight();
				virtual void SetCollapsed(bool c);
				virtual bool IsCollapsed() const;

			protected:
				bool _collapsed;
		};
	}
}

#endif