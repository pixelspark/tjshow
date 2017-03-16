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
 
 #ifndef _TJARTNETDEVICE_H
#define _TJARTNETDEVICE_H

#include "../tjdmxinternal.h"
#include "../tjdmxdevice.h"
#include <TJNP/include/tjsocket.h>

namespace tj {
	namespace dmx {
		namespace devices {
			using namespace tj::shared;
			using namespace tj::dmx;
			using namespace tj::np;

			class DMX_EXPORTED DMXArtNetDevice: public DMXDevice {
				friend class DMXController;

				public:
					DMXArtNetDevice();
					virtual ~DMXArtNetDevice();
					virtual std::wstring GetPort();
					virtual std::wstring GetDeviceID();
					virtual std::wstring GetDeviceInfo();
					virtual std::wstring GetDeviceSerial();
					virtual std::wstring GetDeviceName() const;

					#ifdef TJ_DMX_HAS_TJSHAREDUI
						virtual ref<PropertySet> GetProperties();
					#endif
					virtual unsigned int GetSupportedUniversesCount();

					virtual void Save(TiXmlElement* you);
					virtual void Load(TiXmlElement* you);
					
				protected:
					virtual void Connect();
					virtual void OnTransmit(ref<DMXController> controller);

					ref<Socket> _socket;
					int _inUniverse, _outUniverse, _universeCount;
					std::wstring _bcastAddress;
					int _port;
					static const wchar_t* KBroadcastAddress;
					static const int KBroadcastPort;
					Bytes _outBytes;
					Timestamp _connected;
			};

			class DMX_EXPORTED DMXArtNetDeviceClass: public DMXDeviceClass {
				public:
					DMXArtNetDeviceClass();
					virtual ~DMXArtNetDeviceClass();
					virtual void GetAvailableDevices(std::vector< ref<DMXDevice> >& devs);

				protected:
					ref<DMXArtNetDevice> _device;
			};
		}
	}
}

#endif