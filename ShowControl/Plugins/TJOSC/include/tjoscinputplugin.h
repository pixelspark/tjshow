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
#ifndef _TJOSC_PLUGIN_H
#define _TJOSC_PLUGIN_H

#include "tjosc.h"
#include "tjoscdevice.h"
#include "tjoscbrowser.h"
#include <OSCPack/OscPacketListener.h>
class UdpListeningReceiveSocket;

namespace tj {
	namespace osc {
		class OSCInputPlugin: public tj::show::InputPlugin, public Listener<OSCBrowser::OSCBrowserNotification> {
			friend class intern::OSCDevicesListWnd;

			public:
				OSCInputPlugin();
				virtual ~OSCInputPlugin();
				virtual std::wstring GetName() const;
				virtual std::wstring GetFriendlyName() const;
				virtual std::wstring GetFriendlyCategory() const;
				virtual std::wstring GetVersion() const;
				virtual std::wstring GetAuthor() const;
				virtual std::wstring GetDescription() const;
				virtual void GetRequiredFeatures(std::list<std::wstring>& fts) const;
				virtual void GetDevices(ref<input::Dispatcher> dp, std::vector< ref<Device> >& devs);
				virtual void Load(TiXmlElement* you, bool showSpecific);
				virtual void Save(TiXmlElement* you, bool showSpecific);
				virtual ref<tj::shared::Pane> GetSettingsWindow(ref<PropertyGridProxy> pg);
				
				virtual void OnCreated();
				virtual void Notify(ref<Object> source, const OSCBrowser::OSCBrowserNotification& data);
				virtual void AddDevice(strong<OSCDevice> oid);
				virtual void RemoveDevice(strong<OSCDevice> oid);
				virtual ref<OSCDevice> GetDeviceByIndex(unsigned int id);

			protected:
				CriticalSection _devicesLock;
				std::map<std::wstring, ref<OSCDevice> > _inDevices;
		};
	}
}

#endif