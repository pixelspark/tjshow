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
#ifndef _TJINGLECONNECTOR_H
#define _TJINGLECONNECTOR_H

#include <TJShared/include/tjshared.h>

namespace tj {
	namespace jingle {
		namespace io {
			class JingleConnectorPrivate;

			/** JingleConnector provides connectivity to other applications, through protocols such as
			OSC (OpenSound Control). **/
			class JingleConnector: public virtual tj::shared::Object {
				public:
					JingleConnector(tj::shared::strong<tj::shared::Settings> settings);
					virtual ~JingleConnector();
					virtual void Initialize(); // Loads configuration from settings
					virtual void SendEvent(const std::wstring& componentID, const std::wstring& instanceID, const std::wstring& eventID);

				protected:
					tj::shared::strong<JingleConnectorPrivate> _private;
					tj::shared::strong<tj::shared::Settings> _settings;
			};
		}
	}
}

#endif