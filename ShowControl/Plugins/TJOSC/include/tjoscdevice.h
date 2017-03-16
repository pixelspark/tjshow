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
#ifndef _TJOSCDEVICE_H
#define _TJOSCDEVICE_H

#include <TJShow/include/tjshow.h>

namespace tj {
	namespace osc {
		using namespace tj::shared;
		using namespace tj::show;

		namespace intern {
			class OSCDevicesListWnd;
		}		

		class OSCDevice: public virtual tj::shared::Object, public tj::show::Device {
			public:
				enum Direction {
					DirectionOutgoing = 1,
					DirectionIncoming = 2,
					DirectionBoth = 3,
				};

				OSCDevice();
				virtual ~OSCDevice();

				virtual bool WasAutomaticallyDiscovered() const;
				virtual void SetAutomaticallyDiscovered(bool a);

				virtual void Connect(bool t) = 0;
				virtual void Load(TiXmlElement* me) = 0;
				virtual void Save(TiXmlElement* you) = 0;
				virtual void SetDispatcher(ref<tj::show::input::Dispatcher> disp) = 0;
				virtual std::wstring GetType() const = 0;
				virtual std::wstring GetID() const = 0;
				virtual std::wstring GetDescription() const = 0;
				virtual Direction GetDirection() const = 0;

				enum OSCDeviceProperty {
					PropertyName = 1,
				};

				virtual ref<Property> GetPropertyFor(OSCDeviceProperty p) = 0;
				virtual void Send(const char* data, size_t length) = 0;

			private:
				bool _wasAutoDiscovered;
		};
	}
}

#endif