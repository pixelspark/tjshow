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
#include "../include/internal/tjshow.h"
#include "../include/internal/tjnetwork.h"

/** Note about hashing: PluginManager chooses the hashes for the plugins when they're loaded into TJShow. The
hash is a FNV-1 32bit hash of the string Module.dll.PluginName.type (so for example "TJMedia.dll.MediaPlugin.1"). Internal plugins
like Stats will use 'internal' as module name ("internal.Stats.12"). **/

ref<PluginManager> PluginManager::_instance;

class ModelDispatcher: public input::Dispatcher {
	public:
		ModelDispatcher() {
		}

		virtual ~ModelDispatcher() {
		}

		virtual void Dispatch(ref<Device> device, const InputID& path, const Any& value) {
			try {
				ref<Network> net = Application::Instance()->GetNetwork();
				if(net->GetRole()==RoleMaster) {
					ref<Model> model = Application::Instance()->GetModel();
					if(model) {
						ref<input::Rules> rules = model->GetInputRules();
						if(rules) {
							rules->Dispatch(device, path, value);
						}
					}
				}
				else if(net->GetRole()==RoleClient) {
					PatchIdentifier pi = Application::Instance()->GetOutputManager()->GetPatchByDevice(device);
					net->SendInput(pi, path, value);
				}
			}
			catch(Exception& e) {
				ref<EventLogger> el = Application::Instance()->GetEventLogger();
				el->AddEvent(e.GetMsg(), e.GetType(), false);
			}
		}
};

/* Plugin manager */
PluginManager::PluginManager(): _modelDispatcher(GC::Hold(new ModelDispatcher())) {
}

PluginManager::~PluginManager() {
}

strong<PluginManager> PluginManager::Instance() {
	if(!_instance) {
		_instance = GC::Hold(new PluginManager());
	}
	return _instance;
}

std::map<PluginHash, ref<PluginWrapper> >* PluginManager::GetPluginsByHash() {
	return &_pluginsByHash;
}

ref<Device> PluginManager::GetDeviceByIdentifier(const DeviceIdentifier& di) {
	ThreadLock lock(&_devicesLock);

	std::map<DeviceIdentifier, ref<Device> >::iterator it = _devicesByIdentifier.find(di);
	if(it!=_devicesByIdentifier.end()) {
		return it->second;
	}
	return 0;
}

std::map< DeviceIdentifier, ref<Device> >* PluginManager::GetDevices() {
	return &_devicesByIdentifier; 
}

void PluginManager::AddDevice(ref<Device> dev) {
	if(dev) {
		ThreadLock lock(&_devicesLock);
		_devicesByIdentifier[dev->GetIdentifier()] = dev;
	}
}

void PluginManager::RemoveDevice(DeviceIdentifier di) {
	ThreadLock lock(&_devicesLock);
	
	std::map<DeviceIdentifier, ref<Device> >::iterator it = _devicesByIdentifier.find(di);
	if(it!=_devicesByIdentifier.end()) {
		_devicesByIdentifier.erase(it);
	}
}

void PluginManager::RemoveDevice(ref<Device> d) {
	ThreadLock lock(&_devicesLock);
	
	std::map<DeviceIdentifier, ref<Device> >::iterator it = _devicesByIdentifier.begin();
	while(it!=_devicesByIdentifier.end()) {
		ref<Device> dev = it->second;
		if(dev==d) {
			it = _devicesByIdentifier.erase(it);
		}
		else {
			++it;
		}
	}
}

void PluginManager::RediscoverDevices(std::vector< ref<Device> >& addAnyway) {
	ThreadLock lock(&_devicesLock);
	_devicesByIdentifier.clear();

	if(addAnyway.size()>0) {
		AddDevices(addAnyway);
	}

	std::map<PluginHash, ref<PluginWrapper> >::iterator it = _pluginsByHash.begin();
	while(it!=_pluginsByHash.end()) {
		ref<PluginWrapper> pw = it->second;
		if(pw) {
			try {
				ref<Plugin> p = pw->GetPlugin();
				if(p) {
					
					// Get output devices
					if(p.IsCastableTo<OutputPlugin>()) {
						ref<OutputPlugin> op = p;
						std::vector< ref<Device> > devs;
						op->GetDevices(devs);
						if(devs.size()>0) {
							std::vector< ref<Device> >::iterator dit = devs.begin();
							while(dit!=devs.end()) {
								ref<Device> device = *dit;
								if(device) {
									_devicesByIdentifier[device->GetIdentifier()] = device;
								}
								++dit;
							}
						}
					}

					// Get input devices
					if(p.IsCastableTo<InputPlugin>()) {
						ref<InputPlugin> ip = p;
						std::vector< ref<Device> > devs;
						ip->GetDevices(devs, _modelDispatcher);
						if(devs.size()>0) {
							std::vector< ref<Device> >::iterator dit = devs.begin();
							while(dit!=devs.end()) {
								ref<Device> device = *dit;
								if(device) {
									_devicesByIdentifier[device->GetIdentifier()] = device;
								}
								++dit;
							}
						}
					}
				}
			}
			catch(Exception& e) {
				Log::Write(L"TJShow/PluginManager", L"Could not get device list from plug-in '"+pw->GetFriendlyName()+L"'");
				Log::Write(L"TJShow/PluginManager", L"Error: "+e.GetMsg()+L" at "+Wcs(e.GetFile())+L":"+Stringify(e.GetLine()));
			}
			catch(...) {
				Log::Write(L"TJShow/PluginManager", L"Could not get device list from plug-in '"+pw->GetFriendlyName()+L"'");
			}
		}
		++it;
	}

	Log::Write(L"TJShow/PluginManager", L"Discovered "+Stringify(_devicesByIdentifier.size())+L" devices");
}

void PluginManager::LoadSettings(const std::wstring& path) {
	TiXmlDocument doc;

	if(doc.LoadFile(Mbs(path))) {
		TiXmlElement* root = doc.FirstChildElement("settings");
		if(root!=0) {
			TiXmlElement* plugin = root->FirstChildElement("plugin");
			while(plugin!=0) {
				PluginHash ph = LoadAttributeSmall<PluginHash>(plugin, "id", 0);
				ref<PluginWrapper> pluginWrapper = GetPluginByHash(ph);
				if(pluginWrapper) {
					ref<Plugin> pl = pluginWrapper->GetPlugin();
					if(pl) {
						pl->Load(plugin, false);
					}
				}
				plugin = plugin->NextSiblingElement("plugin");
			}
		}
	}
}

void PluginManager::SaveSettings(const std::wstring& path) {
	TiXmlDocument doc;
	TiXmlElement root("settings");

	std::map<PluginHash, ref<PluginWrapper> >::iterator it = _pluginsByHash.begin();
	while(it!=_pluginsByHash.end()) {
		ref<PluginWrapper> pw = it->second;
		TiXmlElement plugin("plugin");
		SaveAttributeSmall<PluginHash>(&plugin, "id", it->first);
		pw->GetPlugin()->Save(&plugin, false);
		root.InsertEndChild(plugin);
		++it;
	}

	doc.InsertEndChild(root);
	doc.SaveFile(Mbs(path));
}

void PluginManager::SaveShowSpecificSettings(TiXmlElement* root) {
	std::map<PluginHash, ref<PluginWrapper> >::iterator it = _pluginsByHash.begin();
	while(it!=_pluginsByHash.end()) {
		ref<PluginWrapper> pw = it->second;
		TiXmlElement plugin("plugin");
		pw->GetPlugin()->Save(&plugin, true);
		
		// Only put the plug-in settings tag in here if the plug-in has something to say
		// Otherwise our show files will contain useless stuff
		if(plugin.FirstChild()!=0) {
			SaveAttributeSmall<PluginHash>(&plugin, "id", it->first);
			root->InsertEndChild(plugin);
		}
		++it;
	}
}

void PluginManager::LoadShowSpecificSettings(TiXmlElement* root) {
	TiXmlElement* plugin = root->FirstChildElement("plugin");
	while(plugin!=0) {
		PluginHash ph = LoadAttributeSmall<PluginHash>(plugin, "id", 0);
		ref<PluginWrapper> pluginWrapper = GetPluginByHash(ph);
		if(pluginWrapper) {
			ref<Plugin> pl = pluginWrapper->GetPlugin();
			if(pl) {
				pl->Load(plugin, true);
			}
		}
		plugin = plugin->NextSiblingElement("plugin");
	}

	std::vector< ref<Device> > alsoAdd;
	RediscoverDevices(alsoAdd);
}

void PluginManager::Discover(const std::wstring& path) {
	WIN32_FIND_DATAW d;
	ZeroMemory(&d,sizeof(d));

	std::wstring pathfilter = path;
	pathfilter += L"*.dll";

	HANDLE hsr = FindFirstFile(pathfilter.c_str(), &d);
	wchar_t buf[MAX_PATH+1];

	do {
		if(hsr==INVALID_HANDLE_VALUE) {
			continue;
		}

		std::wstring naam = path;
		naam += d.cFileName;

		if(GetFullPathName(naam.c_str(), MAX_PATH,buf,0)==0) {
			continue;
		}

		Hash hashCalc;
		HMODULE mod = LoadLibrary((wchar_t*)buf);
		if(mod) {
			PluginEntry entry = (PluginEntry)GetProcAddress(mod, PluginEntryName);
			if(entry) {
				try {
					std::vector< ref<Plugin> >* plugins = entry();
					if(plugins!=0) {
						std::vector< ref<Plugin> >::iterator it = plugins->begin();
						while(it!=plugins->end()) {
							ref<Plugin> plug = *it;
							if(plug) {
								std::wstring id = PluginWrapper::GetPluginID(plug, d.cFileName);
								PluginHash hash = hashCalc.Calculate(id);

								// check if the same hash isn't already used
								std::map<PluginHash, ref<PluginWrapper> >::iterator it = _pluginsByHash.find(hash);
								if(it!=_pluginsByHash.end()) {
									Throw(L"Another plugin already exists with the same plugin hash.. Only one of the plugins will be loaded this time.", ExceptionTypeWarning);
								}

								ref<PluginWrapper> wrapper = GC::Hold(new PluginWrapper(plug,mod,hash, id));
								_pluginsByHash.insert(std::pair<PluginHash, ref<PluginWrapper> >(hash, wrapper));
							}
							else {
								Throw(L"Plugin didn't initialize. Probably there's something missing or an error occurred when the plugin was initializing", ExceptionTypeError);
							}
							++it;
						}

						delete plugins;
					}
				}
				catch(Exception& e) {
					MessageBox(0L, e.GetMsg().c_str(), L"Plugin Error", MB_OK|MB_ICONERROR);
				}
			}
		}
		else {
			MessageBox(0L, L"A plugin could not be loaded. The plugin might be damaged or have a dependency error. ", L"Plugin Error", MB_OK);
		}

	} while(FindNextFile(hsr, &d));

	FindClose(hsr);
}

void PluginManager::AddInternalPlugin(ref<Plugin> plug) {
	assert(plug);
	Hash hashCalculator;

	// calculate internal plugin hash
	std::wstring id = L"internal.";
	id += plug->GetName();
	PluginHash hash = hashCalculator.Calculate(id);

	// check if the hash already exists
	std::map<PluginHash, ref<PluginWrapper> >::iterator it = _pluginsByHash.find(hash);
	if(it!=_pluginsByHash.end()) {
		Throw(L"Another plugin already exists with the same plugin hash.. Only one of the plugins will be loaded this time.", ExceptionTypeWarning);
	}

	ref<PluginWrapper> pw = GC::Hold(new PluginWrapper(plug, GetModuleHandle(NULL), hash, id));
	_pluginsByHash[hash] = pw;
}

ref<PluginWrapper> PluginManager::GetPluginByHash(PluginHash t) {
	std::map<PluginHash, ref<PluginWrapper> >::iterator it = _pluginsByHash.find(t);
	if(it!=_pluginsByHash.end()) {
		return (*it).second;
	}
	
	return 0;
}

void PluginManager::ResetPlugins() {
	std::map< PluginHash, ref<PluginWrapper> >::iterator it = _pluginsByHash.begin();
	while(it!=_pluginsByHash.end()) {
		ref<PluginWrapper> pw = it->second;
		if(pw) {
			pw->Reset();
		}
		++it;
	}
}

void PluginManager::AddDevices(std::vector< ref<Device> >& devs) {
	if(devs.size()==0) return;

	std::vector< ref<Device> >::iterator it = devs.begin();
	while(it!=devs.end()) {
		ref<Device> dev = *it;
		if(dev) {
			AddDevice(dev);
		}
		++it;
	}
}

ref<EndpointCategory> PluginManager::GetEndpointCategoryById(const EndpointCategoryID& id) {
	std::map<EndpointCategoryID, ref<EndpointCategory> >::iterator it = _endpointCategories.find(id);
	if(it!=_endpointCategories.end()) {
		return it->second;
	}
	return 0;
}

void PluginManager::AddEndpointCategory(strong<EndpointCategory> cat) {
	_endpointCategories[cat->GetID()] = cat;
}

/* EndpointCategory */
EndpointCategory::EndpointCategory(const EndpointCategoryID& id): _cat(id) {
}

EndpointCategory::~EndpointCategory() {
}

EndpointCategoryID EndpointCategory::GetID() const {
	return _cat;
}
