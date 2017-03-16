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
#ifndef _TJPLUGINMGR_H
#define _TJPLUGINMGR_H

namespace tj { 
	namespace show {
		namespace view {
			class PluginSettingsWnd;
		}

		typedef std::wstring EndpointID;
		typedef std::wstring EndpointCategoryID;

		class EndpointCategory: public virtual Object {
			public:
				EndpointCategory(const EndpointCategoryID& id);
				virtual EndpointCategoryID GetID() const;
				virtual ~EndpointCategory();
				virtual ref<Endpoint> GetEndpointById(const EndpointID& eid) = 0;

			private:
				EndpointCategoryID _cat;
		};

		class PluginManager {
			friend class view::PluginSettingsWnd;

			protected:
				PluginManager();
				static ref<PluginManager> _instance;
				
			public:
				virtual ~PluginManager();

				/* Path ends with a slash if you want it to search for files in that directory, otherwise
				it will search for files that match the exact name */
				void Discover(const std::wstring& path);
				ref<PluginWrapper> GetPluginByHash(PluginHash h);
				ref<PluginWrapper> GetNullPlugin();

				std::map<PluginHash, ref<PluginWrapper> >* GetPluginsByHash();
				std::map< DeviceIdentifier, ref<Device> >* GetDevices();
				void AddInternalPlugin(ref<Plugin> plug);

				// Endpoints
				void AddEndpointCategory(strong<EndpointCategory> cat);
				ref<EndpointCategory> GetEndpointCategoryById(const EndpointCategoryID& cid);

				// Plug-in settings stuff
				void SaveSettings(const std::wstring& path);
				void LoadSettings(const std::wstring& path);
				void SaveShowSpecificSettings(TiXmlElement* parent);
				void LoadShowSpecificSettings(TiXmlElement* parent);

				// We're a singleton
				static strong<PluginManager> Instance();
				void ResetPlugins();

				// Device management
				virtual ref<Device> GetDeviceByIdentifier(const DeviceIdentifier& di);
				virtual void RediscoverDevices(std::vector< ref<Device> >& alsoAdd);
				virtual void AddDevice(ref<Device> dev);
				virtual void AddDevices(std::vector< ref<Device> >& devs);
				virtual void RemoveDevice(DeviceIdentifier di);
				virtual void RemoveDevice(ref<Device> dev);
				
			protected:
				CriticalSection _devicesLock;
				std::map<PluginHash, ref<PluginWrapper> > _pluginsByHash;
				std::map<DeviceIdentifier, ref<Device> > _devicesByIdentifier;
				std::map<EndpointCategoryID, ref<EndpointCategory> > _endpointCategories;
				strong<input::Dispatcher> _modelDispatcher;
		};
	}
}

#endif
