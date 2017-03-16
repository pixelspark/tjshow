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
#ifndef _TJOSCOVERUDP_H
#define _TJOSCOVERUDP_H

#include "tjoscinputplugin.h"

class UdpTransmitSocket;

namespace osc {
	class ReceivedMessageArgument;
}

namespace tj {
	namespace osc {
		namespace intern {
			class OSCOverUDPListenerThread;
		}

		class OSCOverUDPDevice: public OSCDevice, public ::osc::OscPacketListener, public Inspectable {
			public:
				OSCOverUDPDevice(const std::wstring& id, const std::wstring& name, ref<input::Dispatcher> disp, Direction d);
				virtual ~OSCOverUDPDevice();
				virtual std::wstring GetFriendlyName() const;
				virtual DeviceIdentifier GetIdentifier() const;
				virtual ref<tj::shared::Icon> GetIcon();
				virtual bool IsMuted() const;
				virtual void SetMuted(bool t) const;
				virtual void ProcessMessage(const ::osc::ReceivedMessage& m, const ::IpEndpointName& remoteEndpoint);
				virtual void Connect(bool t);
				virtual void Load(TiXmlElement* me);
				virtual void Save(TiXmlElement* you);
				virtual void SetDispatcher(ref<tj::show::input::Dispatcher> disp);
				virtual std::wstring GetType() const;
				virtual std::wstring GetID() const;
				virtual std::wstring GetDescription() const;
				virtual Direction GetDirection() const;

				virtual void SetAddress(const std::wstring& addr);
				virtual void SetPort(unsigned short port);
				virtual std::wstring GetAddress() const;
				virtual unsigned short GetPort() const;
				
				virtual ref<Property> GetPropertyFor(OSCDeviceProperty p);
				virtual ref<PropertySet> GetProperties();

				virtual void Send(const char* data, size_t size);

			protected:
				static bool OSCArgumentToAny(const ::osc::ReceivedMessageArgument& arg, Any& any);

				Direction _direction;
				ref<input::Dispatcher> _disp;
				std::wstring _name;
				std::wstring _id;
				unsigned short _port;
				std::wstring _address;
				ref<Icon> _icon;
				ref<intern::OSCOverUDPListenerThread> _thread;
				::UdpTransmitSocket* _transmitSocket;
				Timestamp _connected;
				Bytes _sentBytes;
				unsigned int _sentPackets, _receivedPackets;
		};
	}
}

#endif