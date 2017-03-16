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
#include "../include/tjdmx.h"
using namespace tj::shared::graphics;
using namespace tj::shared;

DMXPatchListWnd::DMXPatchListWnd(ref<DMXPlugin> plg): _plugin(plg), _resetIcon(L"icons/check.png") {
	AddColumn(TL(dmx_patch_col_tnp), KColTNPAddress, 0.2f, false);
	AddColumn(TL(dmx_patch_col_dmx), KColDMXAddress, 0.2f);
	AddColumn(TL(dmx_patch_col_name), KColName, 0.3f);
	AddColumn(TL(dmx_patch_col_type), KColType, 0.1f);
	AddColumn(TL(dmx_patch_col_reset), KColReset, 0.1f);

	SetColumnWidth(KColTNPAddress, 0.075f);
	SetColumnWidth(KColReset, 0.075f);
	SetEmptyText(TL(dmx_patch_empty_text));
	plg->SortTrackList();
}

DMXPatchListWnd::~DMXPatchListWnd() {
}

ref<Property> DMXPatchListWnd::GetPropertyForItem(int id, int col) {
	ref<DMXPlugin> plug = _plugin;
	if(plug && id>=0 && id < int(plug->_tracks.size())) {
		ref<DMXPatchable> track = plug->_tracks.at(id);
		if(track) {
			switch(col) {
				case KColDMXAddress:
					return track->GetPropertyFor(DMXPatchable::PropertyAddress);

				case KColReset:
					return track->GetPropertyFor(DMXPatchable::PropertyResetOnStop);
			}
		}
	}

	return 0;
}

void DMXPatchListWnd::PaintItem(int id, Graphics& g, Area& r, const ColumnInfo& ci) {
	ref<DMXPlugin> plug = _plugin;
	if(plug && int(plug->_tracks.size())>id) {
		ref<DMXPatchable> track = plug->_tracks.at(id);

		if(track) {
			strong<Theme> theme = ThemeManager::GetTheme();
			StringFormat sf;
			sf.SetLineAlignment(StringAlignmentCenter);
			sf.SetAlignment(StringAlignmentNear);
			sf.SetTrimming(StringTrimmingEllipsisCharacter);

			// draw stuff
			int mtype = track->GetSliderType();
			SolidBrush textBr(mtype==Theme::SliderNormal?theme->GetColor(Theme::ColorText):theme->GetSliderColorStart((Theme::SliderType)mtype));

			DrawCellText(g, &sf, &textBr, theme->GetGUIFont(), KColDMXAddress, r, track->GetDMXAddress());
			DrawCellText(g, &sf, &textBr, theme->GetGUIFont(), KColType, r, track->GetTypeName());
			DrawCellText(g, &sf, &textBr, theme->GetGUIFont(), KColName, r, track->GetInstanceName());
			DrawCellText(g, &sf, &textBr, theme->GetGUIFont(), KColTNPAddress, r, Stringify(track->GetID()));

			if(track->GetResetOnStop()) {
				DrawCellIcon(g, KColReset, r, _resetIcon);
			}
		}	
	}
}

int DMXPatchListWnd::GetItemCount() {
	ref<DMXPlugin> plug = _plugin;
	if(plug) {
		return int(plug->_tracks.size());
	}
	return 0;
}

void DMXPatchListWnd::OnClickItem(int id, int col, Pixels x, Pixels y) {
	SetSelectedRowAndEdit(id);
}

void DMXPatchListWnd::OnDoubleClickItem(int id, int col) {
	if(col==KColReset) {
		ref<DMXPatchable> current = 0;

		if(id>=0 && id<GetItemCount()) {
			ref<DMXPlugin> plug = _plugin;
			if(plug) {
				current = plug->_tracks.at(id);
			}
		}
	
		if(current) {
			if(col==KColReset) {
				current->SetResetOnStop(!current->GetResetOnStop());
			}
			Repaint();
		}
	}
}

class DMXSettingsToolbarWnd: public ToolbarWnd {
	public:	
		DMXSettingsToolbarWnd(ref<DMXPlugin> plugin, ref<PropertyGridProxy> pg) {
			// Patching buttons
			Add(GC::Hold(new ToolbarItem(KCRemovePatches, L"icons/reset.png", TL(dmx_patch_remove_all))));
			ref<ToolbarItem> ap = GC::Hold(new ToolbarItem(KCAutoPatch, L"icons/patch-auto.png", TL(dmx_patch_auto)));
			ap->SetSeparator(true);
			Add(ap);

			// Controller settings, such as universe management; a pretty god-like feature ;-)
			Add(GC::Hold(new ToolbarItem(KCControllerSettings, L"icons/universes.png", TL(dmx_change_controller_settings), true)));

			// Patchset saving/loading
			Add(GC::Hold(new ToolbarItem(KCSavePatches, L"icons/dmx/set-save.png", TL(dmx_patchset_save), false)));
			Add(GC::Hold(new ToolbarItem(KCLoadPatches, L"icons/dmx/set-load.png", TL(dmx_patchset_load), false)));


			_plugin = plugin;
			_pg = pg;
		}

		virtual ~DMXSettingsToolbarWnd() {
		}

		virtual void OnCommand(ref<ToolbarItem> ti) {
			OnCommand(ti->GetCommand());
		}

		virtual void OnCommand(int c) {
			// Remove patch
			if(c==KCRemovePatches) {
				ref<DMXPlugin> plugin(_plugin);
				if(plugin) {
					std::vector<weak<DMXPatchable> >::iterator it = plugin->_tracks.begin();
					while(it!=plugin->_tracks.end()) {
						ref<DMXPatchable> track = *it;
						if(track) {
							track->SetDMXAddress(L"");
						}
						++it;
					}
				}
			}
			// Auto patch
			else if(c==KCAutoPatch) {
				ref<DMXPlugin> plugin(_plugin);
				if(plugin) {
					int channel = 1;

					std::vector<weak<DMXPatchable> >::iterator it = plugin->_tracks.begin();
					while(it!=plugin->_tracks.end()) {
						ref<DMXPatchable> track = *it;
						if(track) {
							track->SetDMXAddress(Stringify(channel));
						}
						++it;
						++channel;
					}
				}
			}
			else if(c==KCControllerSettings) {
				class ControllerSettingsData: public Inspectable {
					public:
						ControllerSettingsData(strong<DMXController> dc): _dc(dc) {
							_n = dc->GetUniverseCount();
							std::wostringstream wos;
							std::set<DMXSlot> switching;
							dc->GetSwitching(switching);

							std::set<DMXSlot>::const_iterator it = switching.begin();
							while(it!=switching.end()) {
								wos << *it;
								++it;
								if(it!=switching.end()) {
									wos << L',' << L' ';
								}
							}
							_switchingChannels = wos.str();
						}

						virtual ~ControllerSettingsData() {
						}
						
						virtual ref<PropertySet> GetProperties() {
							ref<PropertySet> ps = GC::Hold(new PropertySet());
							ps->Add(GC::Hold(new GenericProperty<int>(TL(dmx_universe_count), this, &_n, _n)));
							ps->Add(GC::Hold(new TextProperty(TL(dmx_switching_channels), this, &_switchingChannels)));
							return ps;
						}

						virtual void Apply() {
							if(_n>1) {
								_dc->SetUniverseCount(_n);
							}

							std::set<DMXSlot> switching;
							std::vector<std::wstring> parts = Explode<std::wstring>(_switchingChannels, std::wstring(L","));
							std::vector<std::wstring>::const_iterator it = parts.begin();
							while(it!=parts.end()) {
								int slot = StringTo<int>(*it, -1);
								if(slot>=0) {
									switching.insert((DMXSlot)slot);
								}
								++it;
							}
							_dc->SetSwitching(switching);
						}

						int _n;
						std::wstring _switchingChannels;
						strong<DMXController> _dc;
				};

				ref<DMXPlugin> plugin(_plugin);
				if(plugin) {
					ref<DMXController> controller = plugin->GetController();
					if(controller) {
						ref<ControllerSettingsData> data = GC::Hold(new ControllerSettingsData(controller));
						ref<PropertyDialogWnd> dw = GC::Hold(new PropertyDialogWnd(TL(dmx_controller_settings), TL(dmx_controller_settings_question)));
						dw->SetSize(400,300);
						dw->GetPropertyGrid()->Inspect(data);
						if(dw->DoModal(this)) {
							data->Apply();
						}
					}
				}
			}
			else if(c==KCSavePatches) {
				ref<DMXPlugin> plugin(_plugin);
				if(plugin) {
					std::wstring fn = Dialog::AskForSaveFile(this, TL(dmx_patchset_file), L"DMX Patch-set (*.tdp)\0*.tdp\0\0", L"tdp");
					if(fn.length()>0) {
						plugin->SavePatchSet(fn);
					}
				}
			}
			else if(c==KCLoadPatches) {
				ref<DMXPlugin> plugin(_plugin);
				if(plugin) {
					std::wstring fn = Dialog::AskForOpenFile(this, TL(dmx_patchset_file), L"DMX Patch-set (*.tdp)\0*.tdp\0\0", L"tdp");
					if(fn.length()>0) {
						plugin->LoadPatchSet(fn);
					}
				}
			}
		}

	protected:
		enum {
			KCRemovePatches = 1,
			KCAutoPatch,
			KCEnableDisableDevice,
			KCDeviceProperties,
			KCControllerSettings,
			KCSavePatches,
			KCLoadPatches,
		};

		weak<DMXPlugin> _plugin;
		ref<PropertyGridProxy> _pg;
};

DMXPatchWnd::DMXPatchWnd(ref<DMXPlugin> plugin, ref<PropertyGridProxy> pg): ChildWnd(L"") {
	_tools = GC::Hold(new DMXSettingsToolbarWnd(plugin,pg));
	Add(_tools);

	_list = GC::Hold(new DMXPatchListWnd(plugin));
	Add(_list);	
}

DMXPatchWnd::~DMXPatchWnd() {
}

void DMXPatchWnd::Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme) {
}

void DMXPatchWnd::OnSize(const Area& ns) {
	Layout();
}

void DMXPatchWnd::Layout() {
	Area rc = GetClientArea();
	_tools->Fill(LayoutTop, rc);
	_list->Fill(LayoutFill, rc);
}

void DMXPatchWnd::OnSettingsChanged() {
	ref<Settings> st = GetSettings();
	if(st) {
		_tools->SetSettings(st->GetNamespace(L"toolbar"));
		_list->SetSettings(st->GetNamespace(L"list"));
	}
}

/* DMXDeviceListWnd */
DMXDeviceListWnd::DMXDeviceListWnd(ref<DMXPlugin> p, ref<PropertyGridProxy> pg): _plugin(p), _pg(pg), 
	_checkedIcon(Icons::GetIconPath(Icons::IconChecked)),
	_uncheckedIcon(Icons::GetIconPath(Icons::IconUnchecked)) {
	AddColumn(TL(dmx_device_name), KColName, 0.3f, true);
	AddColumn(TL(dmx_device_enabled), KColEnabled, 0.1f, true);
	AddColumn(TL(dmx_device_universe_count), KColUniverses, 0.1f, true);
	AddColumn(TL(dmx_device_port), KColPort, 0.1f, false);
	AddColumn(TL(dmx_device_info), KColInfo, 0.5f, true);
	AddColumn(TL(dmx_device_serial), KColSerial, 0.2f, false);
}

DMXDeviceListWnd::~DMXDeviceListWnd() {
}

int DMXDeviceListWnd::GetItemCount() {
	ref<DMXController> controller = _plugin->GetController();
	if(controller) {
		int n = 0;

		std::set< ref<DMXDeviceClass> >& classes = controller->GetDeviceClasses();
		std::set< ref<DMXDeviceClass> >::iterator it = classes.begin();
		while(it!=classes.end()) {
			std::vector< ref<DMXDevice> > devices;
			(*it)->GetAvailableDevices(devices);
			n += int(devices.size());
			++it;
		}
		return n;
	}
	return 0;
}

ref<DMXDevice> DMXDeviceListWnd::GetDeviceByIndex(int i) {
	ref<DMXController> controller = _plugin->GetController();
	if(controller) {
		int n = 0;

		std::set< ref<DMXDeviceClass> >& classes = controller->GetDeviceClasses();
		std::set< ref<DMXDeviceClass> >::iterator it = classes.begin();
		while(it!=classes.end()) {
			std::vector< ref<DMXDevice> > devices;
			(*it)->GetAvailableDevices(devices);

			int size = int(devices.size());
			if(i < (n+size)) {
				return devices.at(i - n);
			}

			n += size;
			++it;
		}
	}
	return 0;
}

void DMXDeviceListWnd::PaintItem(int id, tj::shared::graphics::Graphics& g, Area& row, const ColumnInfo& ci) {
	ref<DMXDevice> device = GetDeviceByIndex(id);
	if(device) {
		strong<Theme> theme = ThemeManager::GetTheme();
		StringFormat sf;
		sf.SetTrimming(StringTrimmingEllipsisPath);

		SolidBrush br(theme->GetColor(Theme::ColorText));
		DrawCellText(g, &sf, &br, theme->GetGUIFontBold(), KColName, row, device->GetDeviceName());
		DrawCellText(g, &sf, &br, theme->GetGUIFont(), KColInfo, row, device->GetDeviceInfo());
		DrawCellText(g, &sf, &br, theme->GetGUIFont(), KColPort, row, device->GetPort());
		DrawCellText(g, &sf, &br, theme->GetGUIFont(), KColSerial, row, device->GetDeviceSerial());
		DrawCellText(g, &sf, &br, theme->GetGUIFont(), KColUniverses, row, Stringify(device->GetSupportedUniversesCount()));
		
		ref<DMXController> dc = _plugin->GetController();
		if(dc && dc->IsDeviceEnabled(device)) {
			DrawCellIcon(g, KColEnabled, row, _checkedIcon);
		}
	}
}

void DMXDeviceListWnd::OnClickItem(int id, int col, Pixels x, Pixels y) {
	SetSelectedRow(id);

	if(col==KColEnabled) {
		ref<DMXDevice> dev = GetDeviceByIndex(id);
		if(dev) {
			ref<DMXController> dc = _plugin->GetController();
			if(dc) {
				dc->ToggleDevice(dev);
			}
		}
		Repaint();
	}
	else {
		ref<DMXDevice> dev = GetDeviceByIndex(id);
		_pg->SetProperties(dev);
	}
}

/* DMXSettingsWnd */
DMXSettingsWnd::DMXSettingsWnd(ref<DMXPlugin> plugin, ref<PropertyGridProxy> pg) {
	SetText(TL(dmx_settings));
	_tab = GC::Hold(new TabWnd(0));
	_tab->SetChildStyle(true);
	_tab->SetDetachAttachAllowed(false);
	Add(_tab);

	_pw = GC::Hold(new DMXPatchWnd(plugin,pg));
	_tab->AddPane(GC::Hold(new Pane(TL(dmx_settings_patches), _pw, false, false, 0, Placement(), L"")),true);

	_devices = GC::Hold(new DMXDeviceListWnd(plugin, pg));
	_tab->AddPane(GC::Hold(new Pane(TL(dmx_settings_devices), _devices, false, false, 0, Placement(), L"")), false);
	_pgp = pg;
}

void DMXSettingsWnd::OnSettingsChanged() {
	_pw->SetSettings(GetSettings()->GetNamespace(L"patch-wnd"));
	_devices->SetSettings(GetSettings()->GetNamespace(L"devices-wnd"));
}

DMXSettingsWnd::~DMXSettingsWnd() {
}

void DMXSettingsWnd::Layout() {
	Area r = GetClientArea();
	_tab->Fill(LayoutFill, r);
}

void DMXSettingsWnd::OnSize(const Area& ns) {
	Layout();
	Repaint();
}

void DMXSettingsWnd::Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme) {
}