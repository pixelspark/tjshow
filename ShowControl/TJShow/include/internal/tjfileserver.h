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
#ifndef _TJFILESERVER_H
#define _TJFILESERVER_H

#include <TJNP/include/tjauthorizer.h>
#include <TJNP/include/tjwebserver.h>

namespace tj {
	namespace show {
		class ResourcesRequestResolver: public tj::np::WebItemResource {
			public:
				ResourcesRequestResolver();
				virtual ~ResourcesRequestResolver();
				virtual tj::np::Resolution Get(ref<tj::np::WebRequest> wrp, String& error, char** data, Bytes& length);
		};
	}
}

#endif
