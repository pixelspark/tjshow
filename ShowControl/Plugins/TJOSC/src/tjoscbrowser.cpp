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
#include "../include/tjoscbrowser.h"
#include "../include/tjoscoverudp.h"
#include <TJScout/include/tjscout.h>

using namespace tj::shared;
using namespace tj::osc;
using namespace tj::scout;

/** OSCBrowser **/
ref<OSCBrowser> OSCBrowser::Instance() {
	if(!_instance) {
		_instance = GC::Hold(new OSCBrowser());
	}
	return _instance;
}

OSCBrowser::OSCBrowser() {
	ServiceDescription sd;
	sd.AddType(ServiceDiscoveryDNSSD, L"_osc._udp");
	_request = GC::Hold(new ResolveRequest(sd));
}

void OSCBrowser::OnCreated() {
	_request->EventService.AddListener(this);
	strong<Scout> scout = Scout::Instance();
	scout->Resolve(_request);
}

OSCBrowser::~OSCBrowser() {
}

void OSCBrowser::Notify(ref<Object> source, const ResolveRequest::ServiceNotification& sn) {
	ThreadLock lock(&_lock);
	if(sn.online) {
		ref<OSCOverUDPDevice> dev = GC::Hold(new OSCOverUDPDevice(sn.service->GetID(), sn.service->GetFriendlyName(), null, OSCDevice::DirectionOutgoing));
		dev->SetAddress(sn.service->GetAddress());
		dev->SetPort(sn.service->GetPort());
		_devices[sn.service->GetID()] = dev;

		OSCBrowserNotification on;
		on.device = dev;
		on.online = true;
		EventDeviceFound.Fire(ref<OSCBrowser>(this), on);
	}
	else {
		// TODO: send a notification to remove the OSCDevice with the ID of the service that disappeared
		std::map<std::wstring, ref<OSCDevice> >::iterator it = _devices.find(sn.service->GetID());
		if(it!=_devices.end()) {
			OSCBrowserNotification on;
			on.device = it->second;
			on.online = false;
			EventDeviceFound.Fire(ref<OSCBrowser>(this), on);
			_devices.erase(it);
		}
	}
}

ref<OSCBrowser> OSCBrowser::_instance;