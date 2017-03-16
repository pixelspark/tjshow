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
 
 #ifndef _TJDMXDEVICE_H
#define _TJDMXDEVICE_H

#include "tjdmxinternal.h"

namespace tj {
	namespace dmx {
		using namespace tj::shared;

		class DMXController;

		#ifdef TJ_DMX_HAS_TJSHAREDUI
			class DMX_EXPORTED DMXDevice: public Thread, public virtual Serializable, public Inspectable {
		#else
			class DMX_EXPORTED DMXDevice: public Thread, public virtual Serializable {
		#endif
				
			public:
				DMXDevice();
				virtual ~DMXDevice();
				virtual std::wstring GetPort() = 0;
				virtual std::wstring GetDeviceID() = 0;
				virtual std::wstring GetDeviceName() const = 0;
				virtual std::wstring GetDeviceInfo() = 0;
				virtual std::wstring GetDeviceSerial() = 0;
				virtual unsigned int GetSupportedUniversesCount() = 0;

				virtual void Run();
				virtual void Stop();
				void SetController(ref<DMXController> controller);
				void Transmit(); // signals thread to transmit
				void SetRetransmitEvery(Time ms);
				
				virtual void Load(TiXmlElement* you);
				virtual void Save(TiXmlElement* you);

			protected:
				virtual void OnTransmit(ref<DMXController> controller) = 0;
				virtual void Connect() = 0;
				CriticalSection* GetLock();

				Event _update;
				volatile bool _running;
				weak<DMXController> _controller;
				CriticalSection _lock;
				Time _retransmitTime;
		};

		/** A DMXDeviceClass manages a class of devices. For instance, it may support a few
		devices from vendor Foo. When GetAvailableDevices is called, it should return a list
		of Foo-devices connected (/available) to the computer. The class may decide for itself on
		how to do the discovery (either using methods provided by Windows, a polling thread or simple
		only when the class is constructed the first time). It should make sure that no two DMXDevice-instances
		exist for the same device. **/
		class DMX_EXPORTED DMXDeviceClass {
			public:
				virtual ~DMXDeviceClass();
				virtual void GetAvailableDevices(std::vector< ref<DMXDevice> >& devs) = 0;
		};
	}
}

#endif