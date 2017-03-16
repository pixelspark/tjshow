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
#include "../include/tjoscinputplugin.h"
#include "../include/tjoscoutputplugin.h"
#include "../include/tjoscoverudp.h"
#include "../include/tjoscbrowser.h"

using namespace tj::shared;
using namespace tj::show;
using namespace tj::osc;

namespace tj {
	namespace osc {
		namespace intern {
			class OSCDevicesListWnd: public EditableListWnd {
				public:
					OSCDevicesListWnd(strong<OSCInputPlugin> p, ref<PropertyGridProxy> pp);
					virtual ~OSCDevicesListWnd();
					virtual int GetItemCount();
					virtual void PaintItem(int id, graphics::Graphics& g, Area& row, const ColumnInfo& ci);
					virtual ref<Property> GetPropertyForItem(int id, int col);
					virtual ref<OSCDevice> GetSelectedDevice();

				protected:
					virtual void OnEditingDone(int row);
					virtual void OnClickItem(int id, int col, Pixels x, Pixels y);
					virtual void OnRightClickItem(int id, int col);
					strong<OSCInputPlugin> _oip;
					ref<PropertyGridProxy> _pp;

					enum {
						KColID = 1,
						KColName,
						KColType,
						KColDescription,
					};
			};

			class OSCDevicesToolbarWnd: public ToolbarWnd {
				public:
					OSCDevicesToolbarWnd(strong<OSCInputPlugin> p, ref<OSCDevicesListWnd> oidl);
					virtual ~OSCDevicesToolbarWnd();
					virtual void OnCommand(ref<ToolbarItem> item);

				protected:
					void DoAddDevice();

					strong<OSCInputPlugin> _oip;
					weak<OSCDevicesListWnd> _list;

					enum {
						KCAdd = 1,
						KCRemove,
					};
			};

			class OSCDevicesWnd: public ChildWnd {
				public:
					OSCDevicesWnd(strong<OSCInputPlugin> p, ref<PropertyGridProxy> pp);
					virtual ~OSCDevicesWnd();
					virtual void Layout();
					virtual void OnSize(const Area& ns);
					virtual void Paint(graphics::Graphics& g, strong<Theme> theme);

				protected:
					strong<OSCDevicesListWnd> _list;
					strong<OSCDevicesToolbarWnd> _tools;
			};

			class OSCSettingsWnd: public TabWnd {
				public:
					OSCSettingsWnd(ref<OSCInputPlugin> p, ref<PropertyGridProxy> pp);
					virtual ~OSCSettingsWnd();
					virtual std::wstring GetTabTitle() const;

				protected:
					strong<OSCDevicesWnd> _dw;
			};
		}
	}
}

using namespace tj::osc::intern;

/** OSCDevicesListWnd **/
OSCDevicesListWnd::OSCDevicesListWnd(strong<OSCInputPlugin> p,ref<PropertyGridProxy> pp): _oip(p), _pp(pp) {
	//AddColumn(TL(osc_input_device_id), KColID, 0.1f, false);
	AddColumn(TL(osc_input_device_name), KColName, 0.2f, true);
	AddColumn(TL(osc_input_device_type), KColType, 0.1f, true);
	AddColumn(TL(osc_input_device_description), KColDescription, 0.6f, true);
	SetEmptyText(TL(osc_input_no_devices));
}

OSCDevicesListWnd::~OSCDevicesListWnd() {
}

ref<Property> OSCDevicesListWnd::GetPropertyForItem(int id, int col) {
	ref<OSCDevice> od = _oip->GetDeviceByIndex(id);
	if(od) {
		switch(col) {
			case KColName:
				return od->GetPropertyFor(OSCDevice::PropertyName);
		}
	}
	return null;
}

void OSCDevicesListWnd::OnClickItem(int id, int col, Pixels x, Pixels y) {
	SetSelectedRowAndEdit(id);
}

void OSCDevicesListWnd::OnEditingDone(int row) {
	// Reconnect the device
	ref<OSCDevice> od = _oip->GetDeviceByIndex(row);
	if(od) {
		od->Connect(true);
	}
}

void OSCDevicesListWnd::OnRightClickItem(int id, int col) {
	ref<OSCDevice> od = _oip->GetDeviceByIndex(id);
	if(od && od.IsCastableTo<Inspectable>()) {
		ContextMenu cm;
		cm.AddItem(TL(osc_device_properties), 1, true, false);

		Area row = GetRowArea(id);
		Pixels x = Pixels(GetColumnX(col)*row.GetWidth());
		int r = cm.DoContextMenu(this, x, row.GetBottom());
		if(r==1) {
			od->Connect(false);
			ref<PropertyDialogWnd> pdw = GC::Hold(new PropertyDialogWnd(od->GetFriendlyName(), TL(osc_device_properties_question)));
			pdw->GetPropertyGrid()->Inspect(od);
			pdw->SetSize(400,300);
			pdw->DoModal(this);
			od->Connect(true);
		}
	}
}

int OSCDevicesListWnd::GetItemCount() {
	return (int)_oip->_inDevices.size();
}

ref<OSCDevice> OSCDevicesListWnd::GetSelectedDevice() {
	int sel = GetSelectedRow();
	if(sel>=0) {
		return _oip->GetDeviceByIndex(sel);
	}
	return null;
}

void OSCDevicesListWnd::PaintItem(int id, graphics::Graphics& g, Area& row, const ColumnInfo& ci) {
	ref<OSCDevice> oid = _oip->GetDeviceByIndex(id);
	if(oid) {
		bool rowSelected = (GetSelectedRow() == id);
		graphics::StringFormat sf;
		sf.SetFormatFlags(graphics::StringFormatFlagsLineLimit);
		strong<Theme> theme = ThemeManager::GetTheme();
		graphics::SolidBrush tbr(theme->GetColor(Theme::ColorText));
		graphics::SolidBrush dbr(theme->GetColor(rowSelected ? Theme::ColorBackground : Theme::ColorActiveEnd));

		//DrawCellText(g, &sf, &tbr,theme->GetGUIFont(), KColID, row, oid->GetID());
		DrawCellText(g, &sf, &tbr,theme->GetGUIFontBold(), KColName, row, oid->GetFriendlyName());
		DrawCellText(g, &sf, &tbr,theme->GetGUIFont(), KColType, row, oid->GetType());
		DrawCellText(g, &sf, &dbr, theme->GetGUIFont(), KColDescription, row, oid->GetDescription());

		// Draw in/out icons
		OSCDevice::Direction dir = oid->GetDirection();
		if(IsColumnVisible(KColName)) {
			Pixels ncw = Pixels(GetColumnWidth(KColName) * row.GetWidth());
			if(ncw > 48) {
				if((dir & OSCDevice::DirectionOutgoing)!=0) {
					ncw -= 26;
					Area iconArea(ncw+2, row.GetTop()+2, 26-4, row.GetHeight()-6);
					DrawTag(g, iconArea, TL(osc_device_direction_out_tag), theme);
				}
				
				if((dir & OSCDevice::DirectionIncoming)!=0) {
					ncw -= 18;
					Area iconArea(ncw+2, row.GetTop()+2, 18-4, row.GetHeight()-6);
					DrawTag(g, iconArea, TL(osc_device_direction_in_tag), theme);
				}	
			}
		}
	}
}

/** OSCDevicesToolbarWnd **/
OSCDevicesToolbarWnd::OSCDevicesToolbarWnd(strong<OSCInputPlugin> p, ref<OSCDevicesListWnd> iodl): _oip(p), _list(iodl) {
	Add(GC::Hold(new ToolbarItem(KCAdd, L"icons/add.png", TL(osc_input_device_add), false)));
	Add(GC::Hold(new ToolbarItem(KCRemove, L"icons/remove.png", TL(osc_input_device_remove), false)));
}

OSCDevicesToolbarWnd::~OSCDevicesToolbarWnd() {
}

void OSCDevicesToolbarWnd::DoAddDevice() {
	strong<PropertyDialogWnd> pdw = GC::Hold(new PropertyDialogWnd(TL(osc_input_device_add), TL(osc_input_device_add_question)));
	
	class AddDeviceData: public Inspectable {
		public:
			AddDeviceData(): type(L"udp"), direction(OSCDevice::DirectionBoth) {
			}

			virtual ~AddDeviceData() {
			}

			virtual ref<PropertySet> GetProperties() {
				ref<PropertySet> ps = GC::Hold(new PropertySet());
				ps->Add(GC::Hold(new GenericProperty<std::wstring>(TL(osc_input_device_name), this, &name, name)));

				ref<GenericListProperty<std::wstring> > tl = GC::Hold(new GenericListProperty<std::wstring>(TL(osc_input_device_type), this, &type, type));
				tl->AddOption(TL(osc_udp_transport_device), L"udp");
				ps->Add(tl);

				ref< GenericListProperty<OSCDevice::Direction> > dir = GC::Hold(new GenericListProperty<OSCDevice::Direction>(TL(osc_device_direction), this, &direction, direction));
				dir->AddOption(TL(osc_device_direction_out), OSCDevice::DirectionOutgoing);
				dir->AddOption(TL(osc_device_direction_in), OSCDevice::DirectionIncoming);
				dir->AddOption(TL(osc_device_direction_both), OSCDevice::DirectionBoth);
				ps->Add(dir);
				return ps;
			}

			std::wstring name;
			std::wstring type;
			OSCDevice::Direction direction;
	};
	
	pdw->SetSize(300,200);
	ref<AddDeviceData> dwdd = GC::Hold(new AddDeviceData());
	pdw->GetPropertyGrid()->Inspect(dwdd);

	if(pdw->DoModal(this)) {
		// Really add
		std::wstring id = Util::RandomIdentifier(L'D');
		ref<OSCDevice> oid = GC::Hold(new OSCOverUDPDevice(id, dwdd->name, null, dwdd->direction));
		_oip->AddDevice(oid);
		oid->Connect(true);
	}
}

void OSCDevicesToolbarWnd::OnCommand(ref<ToolbarItem> item) {
	if(item) {
		if(item->GetCommand()==KCAdd) {
			DoAddDevice();
		}
		else if(item->GetCommand()==KCRemove) {
			ref<OSCDevicesListWnd> list = _list;
			if(list) {
				if(Alert::ShowYesNo(TL(osc_input_device_remove), TL(osc_input_device_remove_warning), Alert::TypeWarning)) {
					ref<OSCDevice> oid = list->GetSelectedDevice();
					if(oid) {
						_oip->RemoveDevice(oid);
					}
				}
			}
		}
	}
}

/** OSCDevicesWnd **/
OSCDevicesWnd::OSCDevicesWnd(strong<OSCInputPlugin> p, ref<PropertyGridProxy> pp): ChildWnd(false), 
	_list(GC::Hold(new OSCDevicesListWnd(p, pp))), 
	_tools(GC::Hold(new OSCDevicesToolbarWnd(p, _list))) {
		Add(_list);
		Add(_tools);
}

OSCDevicesWnd::~OSCDevicesWnd() {
}

void OSCDevicesWnd::Layout() {
	Area ns = GetClientArea();
	_tools->Fill(LayoutTop, ns);
	_list->Fill(LayoutFill, ns);
}

void OSCDevicesWnd::OnSize(const Area& ns) {
	Layout();
}

void OSCDevicesWnd::Paint(graphics::Graphics& g, strong<Theme> theme) {
}

/** OSCSettingsWnd **/
OSCSettingsWnd::OSCSettingsWnd(ref<OSCInputPlugin> p, ref<PropertyGridProxy> pp): TabWnd(null), _dw(GC::Hold(new OSCDevicesWnd(p,pp))) {
	SetChildStyle(true);
	ref<Pane> dp = GC::Hold(new Pane(TL(osc_devices), _dw, false, false, null));
	AddPane(dp, true);
}

OSCSettingsWnd::~OSCSettingsWnd() {
}

std::wstring OSCSettingsWnd::GetTabTitle() const {
	return L"";
}

/** OSCDevice **/
OSCDevice::OSCDevice(): _wasAutoDiscovered(false) {
}

bool OSCDevice::WasAutomaticallyDiscovered() const {
	return _wasAutoDiscovered;
}

void OSCDevice::SetAutomaticallyDiscovered(bool a) {
	_wasAutoDiscovered = a;
}

OSCDevice::~OSCDevice() {
}

/** OSCPlugin **/
OSCInputPlugin::OSCInputPlugin() {
	// Start up the mDNS service browser
	ref<OSCBrowser> browser = OSCBrowser::Instance();
}

OSCInputPlugin::~OSCInputPlugin() {
}

void OSCInputPlugin::OnCreated() {
	ref<OSCBrowser> browser = OSCBrowser::Instance();
	browser->EventDeviceFound.AddListener(ref<OSCInputPlugin>(this));
}

void OSCInputPlugin::Notify(ref<Object> source, const OSCBrowser::OSCBrowserNotification& data) {
	if(data.device) {
		ref<OSCDevice> dev = data.device;

		if(data.online) {	
			dev->SetAutomaticallyDiscovered(true);
			try {
				dev->Connect(true);
				AddDevice(dev);
			}
			catch(const std::exception& e) {
				Log::Write(L"TJOSC/OSCInputPlugin", L"Could not connect device: "+Wcs(e.what()));
			}
			catch(const Exception& e) {
				Log::Write(L"TJOSC/OSCInputPlugin", L"Could not connect device: "+e.GetMsg());
			}
			catch(...) {
			}
		}
		else {
			if(dev->WasAutomaticallyDiscovered()) {
				dev->Connect(false);
				RemoveDevice(data.device);
			}
		}
	}
}

std::wstring OSCInputPlugin::GetName() const {
	return L"OSC Input";
}

std::wstring OSCInputPlugin::GetFriendlyName() const {
	return TL(osc_input_friendly_name);
}

std::wstring OSCInputPlugin::GetFriendlyCategory() const {
	return TL(osc_category);
}

std::wstring OSCInputPlugin::GetVersion() const {
	std::wostringstream os;
	os << __DATE__ << " @ " << __TIME__;
	#ifdef UNICODE
	os << L" Unicode";
	#endif

	#ifdef NDEBUG
	os << " Release";
	#endif

	return os.str();
}

std::wstring OSCInputPlugin::GetAuthor() const {
	return std::wstring(L"Tommy van der Vorst");
}

std::wstring OSCInputPlugin::GetDescription() const {
	return TL(osc_input_description);
}

void OSCInputPlugin::GetRequiredFeatures(std::list<std::wstring>& fts) const {
	fts.push_back(L"OSC");
}

void OSCInputPlugin::GetDevices(ref<input::Dispatcher> dp, std::vector< ref<Device> >& devs) {
	ThreadLock lock(&_devicesLock);
	std::map<std::wstring, ref<OSCDevice> >::iterator it = _inDevices.begin();
	while(it!=_inDevices.end()) {
		ref<OSCDevice> oid = it->second;
		if(oid) {
			oid->SetDispatcher(dp);
			devs.push_back(ref<Device>(oid));
		}
		++it;
	}
}

void OSCInputPlugin::AddDevice(strong<OSCDevice> oid) {
	ThreadLock lock(&_devicesLock);
	_inDevices[oid->GetID()] = oid;	
}

void OSCInputPlugin::RemoveDevice(strong<OSCDevice> oid) {
	ThreadLock lock(&_devicesLock);
	std::map<std::wstring, ref<OSCDevice> >::iterator it = _inDevices.find(oid->GetID());
	if(it!=_inDevices.end()) {
		_inDevices.erase(it);
	}
}

void OSCInputPlugin::Load(TiXmlElement* you, bool showSpecific) {
	ThreadLock lock(&_devicesLock);
	if(showSpecific) {
		// Remove non-autodiscovered devices from the device list
		std::map< std::wstring, ref<OSCDevice> >::iterator it = _inDevices.begin();
		while(it!=_inDevices.end()) {
			ref<OSCDevice> dev = it->second;
			if(!dev || !dev->WasAutomaticallyDiscovered()) {
				it = _inDevices.erase(it);
			}
			else {
				++it;
			}
		}
		

		TiXmlElement* devices = you->FirstChildElement("devices");
		if(devices!=0) {
			TiXmlElement* device = devices->FirstChildElement("device");
			while(device!=0) {
				std::wstring id = LoadAttributeSmall<std::wstring>(device, "id", L"");
				std::wstring type = LoadAttributeSmall<std::wstring>(device, "type", L"");
				std::wstring name = LoadAttributeSmall<std::wstring>(device, "name", L"");

				std::wstring direction = LoadAttributeSmall<std::wstring>(device, "direction", L"");
				OSCDevice::Direction dir = OSCDevice::DirectionBoth;
				if(direction==L"in") {
					dir = OSCDevice::DirectionIncoming;
				}
				else if(direction==L"out") {
					dir = OSCDevice::DirectionOutgoing;
				}
				else {
					// Both
				}

				if(id.length()>0) {
					ref<OSCDevice> oid = null;
					if(type==L"udp") {
						oid = GC::Hold(new OSCOverUDPDevice(id, name, null, dir));
					}

					if(oid) {
						oid->Load(device);
						oid->Connect(true);
						_inDevices[id] = oid;
					}
				}
				device = device->NextSiblingElement("device");
			}
		}
	}
}

ref<OSCDevice> OSCInputPlugin::GetDeviceByIndex(unsigned int id) {
	ThreadLock lock(&_devicesLock);
	std::map<std::wstring, ref<OSCDevice> >::iterator it = _inDevices.begin();
	while(id>0) {
		++it;
		if(it==_inDevices.end()) {
			return null;
		}
		--id;
	}

	if(it!=_inDevices.end()) {
		return it->second;
	}

	return null;
}

void OSCInputPlugin::Save(TiXmlElement* you, bool showSpecific) {
	ThreadLock lock(&_devicesLock);
	if(showSpecific) {
		if(_inDevices.size()>0) {
			TiXmlElement devices("devices");
			std::map<std::wstring, ref<OSCDevice> >::iterator it = _inDevices.begin();
			while(it!=_inDevices.end()) {
				ref<OSCDevice> oid = it->second;
				if(oid && !oid->WasAutomaticallyDiscovered()) {
					TiXmlElement device("device");
					SaveAttributeSmall(&device, "type", oid->GetType());
					SaveAttributeSmall(&device, "id", oid->GetID());
					SaveAttributeSmall(&device, "name", oid->GetFriendlyName());

					std::wstring directionString = L"both";
					OSCDevice::Direction direction = oid->GetDirection();
					if(direction==OSCDevice::DirectionIncoming) {
						directionString = L"in";
					}
					else if(direction==OSCDevice::DirectionOutgoing) {
						directionString = L"out";
					}

					SaveAttributeSmall(&device, "direction", directionString);
					oid->Save(&device);
					devices.InsertEndChild(device);
				}
				++it;
			}
			you->InsertEndChild(devices);
		}
	}
}

ref<tj::shared::Pane> OSCInputPlugin::GetSettingsWindow(ref<PropertyGridProxy> pg) {
	ref<OSCSettingsWnd> swd = GC::Hold(new OSCSettingsWnd(this, pg));
	return GC::Hold(new Pane(TL(osc_input_settings), swd, false, true, null));
}

extern "C" { 
	__declspec(dllexport) std::vector< ref<Plugin> >* GetPlugins() {
		std::vector< ref<Plugin> >* plugins = new std::vector<ref<Plugin> >();
		plugins->push_back(GC::Hold(new OSCInputPlugin()));
		plugins->push_back(GC::Hold(new OSCOutputPlugin()));
		return plugins;
	}
}