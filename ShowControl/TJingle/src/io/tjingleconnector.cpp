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
#include "../../include/io/tjingleconnector.h"
#include <OSCPack/IpEndpointName.h>
#include <OSCPack/UdpSocket.h>
#include <OSCPack/OscOutboundPacketStream.h>

using namespace tj::jingle::io;
using namespace tj::shared;

namespace tj {
	namespace jingle {
		namespace io {
			class JingleConnectorPrivate {
				public:
					JingleConnectorPrivate() {
					}

					virtual ~JingleConnectorPrivate() {
					}

					ref<UdpTransmitSocket> _oscTransmitSocket;
			};
		}
	}
}

JingleConnector::JingleConnector(strong<Settings> st): _settings(st), _private(GC::Hold(new JingleConnectorPrivate())) {
}

void JingleConnector::SendEvent(const std::wstring& compID, const std::wstring& iid, const std::wstring& eid) {
	if(_private->_oscTransmitSocket) {
		std::ostringstream pattern;
		pattern << '/' << Mbs(compID) << '/' << Mbs(iid) << '/' << Mbs(eid);

		char buffer[4096];
		osc::OutboundPacketStream p(buffer, 4096);
		p << osc::BeginMessage(pattern.str().c_str());
		p << osc::EndMessage;

		_private->_oscTransmitSocket->Send(p.Data(), p.Size()); 
	}
}

void JingleConnector::Initialize() {
	// OSC initialization
	try {
		ref<Settings> oscSettings = _settings->GetNamespace(L"osc");
		if(oscSettings->GetFlag(L"enabled", false)) {
			std::wstring address = oscSettings->GetValue(L"address", L"");
			unsigned short port = StringTo<unsigned short>(oscSettings->GetValue(L"port", L"7000"), 7000);
			Log::Write(L"TJingle/JingleConnector", L"Enabling OSC; endpoint: "+address+L":"+Stringify(port));

			IpEndpointName endpoint;
			if(address.length()>0) {
				std::string mbAddress = Mbs(address);
				endpoint = IpEndpointName(mbAddress.c_str(), port);
			}
			else {
				endpoint = IpEndpointName(IpEndpointName::ANY_ADDRESS, port);
			}

			// Create socket
			_private->_oscTransmitSocket = GC::Hold(new UdpTransmitSocket(endpoint));
		}
	}
	catch(const std::exception& se) {
		Log::Write(L"TJingle/Connector", L"Error in OSC iniitalization:"+Wcs(se.what()));
	}
}

JingleConnector::~JingleConnector() {
}