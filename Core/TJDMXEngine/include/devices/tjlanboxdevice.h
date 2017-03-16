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
 
 #ifndef _TJLANBOXDEVICE_H
#define _TJLANBOXDEVICE_H

#include "../tjdmxinternal.h"
#include "../tjdmxdevice.h"
#include <TJNP/include/tjsocket.h>

namespace tj {
	namespace dmx {
		namespace devices {
			using namespace tj::shared;
			using namespace tj::dmx;
			using namespace tj::np;

			class DMX_EXPORTED DMXLanboxDevice: public DMXDevice {
				friend class DMXController;

				public:
					DMXLanboxDevice();
					virtual ~DMXLanboxDevice();
					virtual std::wstring GetDeviceID();
					virtual std::wstring GetPort();
					virtual std::wstring GetDeviceInfo();
					virtual std::wstring GetDeviceSerial();
					virtual std::wstring GetDeviceName() const;
					virtual unsigned int GetSupportedUniversesCount();

					#ifdef TJ_DMX_HAS_TJSHAREDUI
						virtual ref<PropertySet> GetProperties();
					#endif
					
					virtual void Save(TiXmlElement* you);
					virtual void Load(TiXmlElement* you);

				protected:
					virtual void Connect();
					virtual void OnTransmit(ref<DMXController> controller);

					CriticalSection _lock;
					ref<Socket> _socket;
					unsigned int _universe;
					std::wstring _address;
					int _port;
					unsigned int _outBytes;
					unsigned short _sequence;
					Timestamp _connected;

					const static std::wstring KDefaultAddress;
					const static int KDefaultPort;
			};

			class DMX_EXPORTED DMXLanboxDeviceClass: public DMXDeviceClass {
				public:
					DMXLanboxDeviceClass();
					virtual ~DMXLanboxDeviceClass();
					virtual void GetAvailableDevices(std::vector< ref<DMXDevice> >& devs);

				protected:
					ref<DMXLanboxDevice> _device;
			};
		}
	}
}

#endif