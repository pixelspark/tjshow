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
#ifndef _TJOSCARGUMENTS_H
#define _TJOSCARGUMENTS_H

#include "tjosc.h"
#include <OSCPack/OscTypes.h>

namespace oscp = osc;

namespace tj {
	namespace osc {
		namespace intern {
			class OSCArgumentWnd;
		}

		struct OSCArgument {
			OSCArgument();
			static std::wstring GetTypeName(const oscp::TypeTagValues& tv);

			oscp::TypeTagValues _type;
			std::wstring _value;
			std::wstring _description;
		};

		class OSCArgumentList {
			public:
				OSCArgumentList();
				virtual ~OSCArgumentList();
				std::vector<OSCArgument> _arguments;
		};

		class OSCArgumentProperty: public Property {
			public:
				OSCArgumentProperty(const std::wstring& name, strong<OSCArgumentList> al, ref<Playback> pb);
				virtual ~OSCArgumentProperty();
				virtual ref<Wnd> GetWindow();
				virtual void Update();

			private:
				strong<OSCArgumentList> _args;
				ref<intern::OSCArgumentWnd> _wnd;
				ref<Playback> _pb;
		};
	}
}

#endif