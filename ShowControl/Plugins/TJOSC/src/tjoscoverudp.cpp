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
#include <OSCPack/osctypes.h>
#include <OSCPack/OscReceivedElements.h>
#include <OSCPack/IpEndpointName.h>
#include <OSCPack/UdpSocket.h>
#include <iomanip>
namespace oscp = osc;

#include <limits>
#include "../include/tjoscoverudp.h"

using namespace tj::shared;
using namespace tj::show;
using namespace tj::osc;

Copyright KCROSCPack(L"TJOSC", L"oscpack", L"Copyright (c) 2004-2005 Ross Bencina <rossb@audiomulch.com>");

namespace tj {
	namespace osc {
		namespace intern {
			class OSCOverUDPListenerThread: public Thread {
				friend class tj::osc::OSCOverUDPDevice;

				public:
					OSCOverUDPListenerThread(strong<UdpListeningReceiveSocket> sock, const std::wstring& address, int port);
					virtual ~OSCOverUDPListenerThread();
					virtual void Run();
					virtual void Break();

				protected:
					CriticalSection _sockLock;
					strong<UdpListeningReceiveSocket> _sock;
					std::wstring _address;
					int _port;
			};
		}
	}
}

/** OSCOverUDPListenerThread **/
using namespace tj::osc::intern;

OSCOverUDPListenerThread::OSCOverUDPListenerThread(strong<UdpListeningReceiveSocket> sock, const std::wstring& address, int port): _sock(sock), _address(address), _port(port) {
}

OSCOverUDPListenerThread::~OSCOverUDPListenerThread() {
	Break();
}

void OSCOverUDPListenerThread::Run() {
	Log::Write(L"TJOSC/OSCOverUDPListenerThread", L"Listener thread started (address: "+_address+L":"+Stringify(_port)+L")");
	_sock->Run();
	Log::Write(L"TJOSC/OSCOverUDPListenerThread", L"Listener thread ended");
}

void OSCOverUDPListenerThread::Break() {
	ThreadLock lock(&_sockLock);
	_sock->AsynchronousBreak();
}

/** OSCOverUDPDevice **/
OSCOverUDPDevice::OSCOverUDPDevice(const std::wstring& id, const std::wstring& name, ref<input::Dispatcher> disp, OSCDevice::Direction dir): _transmitSocket(0L), _port(7000), _id(id), _name(name), _disp(disp), _sentBytes(0), _sentPackets(0), _receivedPackets(0), _direction(dir) {
	_icon = GC::Hold(new Icon(L"icons/devices/osc-in.png"));
}

OSCDevice::Direction OSCOverUDPDevice::GetDirection() const {
	return _direction;
}

void OSCOverUDPDevice::Connect(bool t) {
	if(t) {
		ref<UdpListeningReceiveSocket> sock = null;
		Connect(false);

		// Create the address to use
		IpEndpointName address;

		if(_address.length()>0) {
			std::string mbAddress = Mbs(_address);
			address = IpEndpointName(mbAddress.c_str(),_port);
		}
		else {
			address = IpEndpointName(IpEndpointName::ANY_ADDRESS,_port);
		}

		if((_direction & OSCDevice::DirectionIncoming)!=0) {
			try {
				// Create input socket and thread listener
				sock = GC::Hold(new UdpListeningReceiveSocket(address, this));
				_thread = GC::Hold(new intern::OSCOverUDPListenerThread(sock, _address, _port));
				_thread->Start();
			}
			catch(const std::exception& e) {
				Log::Write(L"TJOSC/OSCOverUDPDevice", L"OSC-over-UDP device could not create input socket/thread: "+Wcs(e.what()));
			}
		}

		if((_direction & OSCDevice::DirectionOutgoing)!=0) {
			try {
				// Create output socket
				_connected.Now();
				_transmitSocket = new UdpTransmitSocket(address);
			}
			catch(const std::exception& e) {
				Log::Write(L"TJOSC/OSCOverUDPDevice", L"OSC-over-UDP device could not create output socket: "+Wcs(e.what()));
			}
		}

		_receivedPackets = 0;
		_sentPackets = 0;
		_sentBytes = 0;
		
	}
	else {
		delete _transmitSocket;
		_transmitSocket = 0;

		if(_thread) {
			_thread->Break();
			_thread->WaitForCompletion();
			_thread = null;
		}
	}
}

void OSCOverUDPDevice::Load(TiXmlElement* me) {
	_port = LoadAttributeSmall<unsigned short>(me, "port", _port);
	_address = LoadAttributeSmall<std::wstring>(me, "address", _address);
}

void OSCOverUDPDevice::Save(TiXmlElement* you) {
	SaveAttributeSmall<unsigned short>(you, "port", _port);
	SaveAttributeSmall<std::wstring>(you, "address", _address);
}

void OSCOverUDPDevice::SetDispatcher(ref<tj::show::input::Dispatcher> disp) {
	_disp = disp;
}

std::wstring OSCOverUDPDevice::GetType() const {
	return L"udp";
}

std::wstring OSCOverUDPDevice::GetID() const {
	return _id;
}

void OSCOverUDPDevice::SetAddress(const std::wstring& addr) {
	_address = addr;
}

void OSCOverUDPDevice::SetPort(unsigned short port) {
	_port = port;
}

std::wstring OSCOverUDPDevice::GetAddress() const {
	return _address;
}

unsigned short OSCOverUDPDevice::GetPort() const {
	return _port;
}

ref<PropertySet> OSCOverUDPDevice::GetProperties() {
	ref<PropertySet> ps = GC::Hold(new PropertySet());
	ps->Add(GC::Hold(new GenericProperty<std::wstring>(TL(osc_udp_address), this, &_address, L"")));
	ps->Add(GC::Hold(new GenericProperty<unsigned short>(TL(osc_udp_port), this, &_port, _port)));

	ref< GenericListProperty<OSCDevice::Direction> > dir = GC::Hold(new GenericListProperty<OSCDevice::Direction>(TL(osc_device_direction), this, &_direction, _direction));
	dir->AddOption(TL(osc_device_direction_out), OSCDevice::DirectionOutgoing);
	dir->AddOption(TL(osc_device_direction_in), OSCDevice::DirectionIncoming);
	dir->AddOption(TL(osc_device_direction_both), OSCDevice::DirectionBoth);
	ps->Add(dir);

	return ps;
}

void OSCOverUDPDevice::Send(const char* data, size_t size) {
	if(_transmitSocket==0) {
		Connect(true);
	}

	if(_transmitSocket!=0) {
		++_sentPackets;
		_sentBytes += (Bytes)size;
		_transmitSocket->Send(data, (int)size);
	}
}

std::wstring OSCOverUDPDevice::GetDescription() const {
	std::wostringstream wos;
	if(_address.length()>0) {
		wos << TL(osc_udp_address) << L": " << _address  << L", ";
	}
	else {
		wos << TL(osc_udp_address_any) << L", ";
	}
	
	wos << TL(osc_udp_port) << L": " << _port;

	if(WasAutomaticallyDiscovered()) {
		wos << L" (auto)";
	}

	if(_thread || _transmitSocket) {
		long double diff = _connected.Difference(Timestamp(true)).ToMilliSeconds() / 1000.0;
		long double sentBytesPerSec = _sentBytes / diff;
		long double sentPacketsPerSec = _sentPackets / diff;
		long double receivedPacketsPerSec = _receivedPackets / diff;

		wos << L" (tx: " << Util::GetSizeString((Bytes)sentBytesPerSec) << L"/s, " << std::setprecision(1) << sentPacketsPerSec << L" p/s; rx: ";
		wos << std::setprecision(1) << receivedPacketsPerSec << L" p/s)";
	}

	return wos.str();
}

ref<Property> OSCOverUDPDevice::GetPropertyFor(OSCDeviceProperty p) {
	switch(p) {
		case OSCDevice::PropertyName:
			return GC::Hold(new GenericProperty<std::wstring>(TL(osc_device_name), this, &_name, _name));

		default:
			return null;
	}
}

bool OSCOverUDPDevice::OSCArgumentToAny(const oscp::ReceivedMessageArgument& arg, Any& any) {
	const char type = arg.TypeTag();
	switch(type) {
		case oscp::TRUE_TYPE_TAG:
			any = Any(true);
			return true;

		case oscp::FALSE_TYPE_TAG:
			any = Any(false);
			return true;

		case oscp::NIL_TYPE_TAG:
			any = Any();
			return true;

		case oscp::INFINITUM_TYPE_TAG:
			any = Any(std::numeric_limits<double>::infinity());
			return true;

		case oscp::INT32_TYPE_TAG:
			any = Any((int)arg.AsInt32());
			return true;

		case oscp::FLOAT_TYPE_TAG:
			any = Any((float)arg.AsFloat());
			return true;

		case oscp::CHAR_TYPE_TAG:
			any = Any(Stringify(arg.AsChar()));
			return true;

		case oscp::RGBA_COLOR_TYPE_TAG: {
			strong<Tuple> data = GC::Hold(new Tuple(4));
			unsigned int color = arg.AsRgbaColor();

			data->Set(0, Any(double(int((color >> 24) & 0xFF)) / 255.0));
			data->Set(1, Any(double(int((color >> 16) & 0xFF)) / 255.0));
			data->Set(2, Any(double(int((color >> 8) & 0xFF)) / 255.0));
			data->Set(3, Any(double(int(color & 0xFF)) / 255.0));
			any = Any(data);
			return true;								
		}

		case oscp::MIDI_MESSAGE_TYPE_TAG: {
			strong<Tuple> data = GC::Hold(new Tuple(4));
			unsigned int msg = arg.AsMidiMessage();

			data->Set(0, Any(double(int((msg >> 24) & 0xFF)) / 255.0));
			data->Set(1, Any(double(int((msg >> 16) & 0xFF)) / 255.0));
			data->Set(2, Any(double(int((msg >> 8) & 0xFF)) / 255.0));
			data->Set(3, Any(double(int(msg & 0xFF)) / 255.0));
			any = Any(data);
			return true;											  
		}

		case oscp::DOUBLE_TYPE_TAG:
			any = Any(arg.AsDouble());
			return true;

		case oscp::STRING_TYPE_TAG:
			any = Any(Wcs(std::string(arg.AsString())));
			return true;

		case oscp::SYMBOL_TYPE_TAG:
			any = Any(Wcs(std::string(arg.AsSymbol())));
			return true;

		case oscp::BLOB_TYPE_TAG:
			// TODO: implement this in some way... We cannot convert blob data to a string,
			// since it can contain \0 characters. It is thus not very suited for Any...

		case oscp::INT64_TYPE_TAG:
			// TODO: add Any::TypeInt64 or something like that...

		case oscp::TIME_TAG_TYPE_TAG:
			// TODO: conversion to Time type (and addition of Any::TypeTime?)

		default:
			return false;
	}
} 

// Called from listener thread
void OSCOverUDPDevice::ProcessMessage(const ::osc::ReceivedMessage& m, const ::IpEndpointName& remoteEndpoint) {
	++_receivedPackets;
	if(_disp) {
		try {
			unsigned long ac = m.ArgumentCount();
			Any value;
			if(ac==0) {
				// message without arguments, 'null' will be sent as value
				value = Any(Any::TypeNull);
			}
			else {
				oscp::ReceivedMessageArgumentIterator ait = m.ArgumentsBegin();

				if(ac==1) {
					// Exactly one argument; value will be the OSC value converted to Any, where possible
					OSCArgumentToAny(*ait, value);
				}
				else {
					strong<Tuple> tuple = GC::Hold(new Tuple(ac));
					for(unsigned long a=0;a<ac;a++) {
						Any val;
						OSCArgumentToAny(*ait, val);
						tuple->Set(a, val);
						++ait;
					}
					value = Any(tuple);
				}
			}

			_disp->Dispatch(this, Wcs(std::string(m.AddressPattern())), value);
		}
		catch(const std::exception& e) {
			Log::Write(L"TJOSC/OSCOverUDPDevice", L"Error in ProcessMessage: "+Wcs(e.what()));
		}
	}
}

OSCOverUDPDevice::~OSCOverUDPDevice() {
	Connect(false);
}

std::wstring OSCOverUDPDevice::GetFriendlyName() const {
	return _name;	
}

DeviceIdentifier OSCOverUDPDevice::GetIdentifier() const {
	return L"@tjosc:"+_id;
}

ref<tj::shared::Icon> OSCOverUDPDevice::GetIcon() {
	return _icon;
}

bool OSCOverUDPDevice::IsMuted() const {
	return false;
}

void OSCOverUDPDevice::SetMuted(bool t) const {
}