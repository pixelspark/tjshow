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
#ifndef _TJOSC_BROWSER_H
#define _TJOSC_BROWSER_H

#include <TJShared/include/tjshared.h>
#include <TJScout/include/tjscout.h>

#include "tjoscdevice.h"

namespace tj {
	namespace osc {
		using namespace tj::shared;

		class OSCBrowser: public virtual Object, public Listener<tj::scout::ResolveRequest::ServiceNotification> {
			public:
				static ref<OSCBrowser> Instance();

				OSCBrowser();
				virtual ~OSCBrowser();
				virtual void Notify(ref<Object> src, const tj::scout::ResolveRequest::ServiceNotification& sn);

				struct OSCBrowserNotification {
					ref<OSCDevice> device;
					bool online;
				};

				Listenable<OSCBrowserNotification> EventDeviceFound;

			protected:
				CriticalSection _lock;
				virtual void OnCreated();

				std::map< std::wstring, ref<OSCDevice> > _devices;
				static ref<OSCBrowser> _instance;
				ref<tj::scout::ResolveRequest> _request;
		};
	}
}

#endif