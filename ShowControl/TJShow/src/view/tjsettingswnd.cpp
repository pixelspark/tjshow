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
#include "../../include/internal/tjshow.h"
#include "../../include/internal/tjnetwork.h"
#include "../../include/internal/view/tjsettingswnd.h"
#include "../../include/internal/view/tjpluginsettingswnd.h"
#include "../../include/internal/view/tjclientswnd.h"

using namespace tj::show;
using namespace tj::shared::graphics;
using namespace tj::show::view;

namespace tj {
	namespace show {
		namespace view {
			class InputChannelWnd;
			class PatchWnd;
			
			class PatchToolbarWnd: public ToolbarWnd {
				public:
					PatchToolbarWnd(PatchWnd* p);
					virtual ~PatchToolbarWnd();
					virtual void OnCommand(ref<ToolbarItem> ti);
					virtual void OnCommand(int r);

				protected:
					PatchWnd* _pw;

					enum {
						KCAdd = 1,
						KCRemove,
						KCChooseClient,
						KCRediscover,
					};
			};

			class PatchListWnd: public ListWnd {
				public:
					PatchListWnd(ref<Patches> p);
					virtual ~PatchListWnd();
					virtual int GetItemCount();
					virtual void PaintItem(int id, tj::shared::graphics::Graphics& g, Area& row, const ColumnInfo& ci);
					virtual void OnClickItem(int id, int col, Pixels x, Pixels y);
					virtual void Update();
					virtual void SetClient(ref<Client> client);
					virtual ref<Client> GetClient();

				protected:
					enum {
						KColName = 1,
						KColDevice = 2,
						KColIdentifier = 3,
						KColClientDevice = 4,
						KColClientIdentifier = 5,
					};
					ref<Patches> _patches;
					ref<Client> _client;
			};

			class InputSettingsWnd: public ChildWnd {
				public:	
					InputSettingsWnd(ref<input::Rules> rules);
					virtual ~InputSettingsWnd();
					virtual void Layout();
					virtual void OnSize(const Area& ns);
					virtual void Paint(graphics::Graphics& g, strong<Theme> theme);
					virtual void OnSettingsChanged();

				protected:
					ref<input::Rules> _rules;
					ref<SplitterWnd> _splitter;
					ref<ListWnd> _deviceList;
					ref<InputChannelWnd> _channelList;
			};

			class InputChannelListWnd: public EditableListWnd {
				public:
					InputChannelListWnd(ref<input::Rules> rules);
					virtual ~InputChannelListWnd();
					virtual void SetDevice(const PatchIdentifier& pi);
					virtual ref<input::Rule> GetSelectedRule();
					virtual void Rebuild();
					virtual ref<Property> GetPropertyForItem(int id, int col);

				protected:
					enum {
						KColPath,
						KColEndpoint,
					};

					virtual int GetItemCount();
					virtual void OnClickItem(int id, int col, Pixels x, Pixels y);
					virtual void PaintItem(int id, tj::shared::graphics::Graphics& g, Area& row, const ColumnInfo& ci);

					PatchIdentifier _current;
					ref<input::Rules> _rules;
					std::vector< ref<input::Rule> > _deviceRules;
			};

			class InputChannelWnd: public ChildWnd {
				friend class InputChannelToolbarWnd;

				public:
					InputChannelWnd(ref<input::Rules> rules);
					virtual ~InputChannelWnd();
					virtual void SetDevice(const PatchIdentifier& pi);
					virtual void Layout();
					virtual void OnSize(const Area& ns);
					virtual void Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme);

				protected:
					ref<InputChannelListWnd> _list;
					ref<ToolbarWnd> _toolbar;

			};

			class InputDeviceListWnd: public ListWnd {
				public:
					InputDeviceListWnd(ref<InputChannelWnd> clw);
					virtual ~InputDeviceListWnd();

				protected:
					enum {
						KColName,
					};

					virtual ref<Patches> GetPatches();
					virtual int GetItemCount();
					virtual void OnClickItem(int id, int col, Pixels x, Pixels y);
					virtual void PaintItem(int id, tj::shared::graphics::Graphics& g, Area& row, const ColumnInfo& ci);

					weak<InputChannelWnd> _channelList;
					Icon _arrowIcon;
			};

			class InputChannelToolbarWnd: public ToolbarWnd {
				public:
					InputChannelToolbarWnd(InputChannelWnd* cw, ref<input::Rules> rules);
					virtual ~InputChannelToolbarWnd();
					virtual void OnCommand(int c);
					virtual void OnCommand(ref<ToolbarItem> ti);

				protected:
					enum {
						KCRemove = 1,
						KCRemoveAll = 2,
					};
					InputChannelWnd* _cw;
					ref<input::Rules> _rules;
			};
		}
	}
}

PreferencesInspectable::PreferencesInspectable(ref<Settings> st):
	_netPort(st, L"net.port"),
	_netAddress(st, L"net.address"),
	_tickLength(st, L"controller.tick-length"),
	_minTickLength(st, L"controller.min-tick-length"),
	_tooltips(st, L"view.tooltips"),
	_resourceWarnings(st, L"warnings.resources.missing"),
	_noClientWarnings(st, L"warnings.no-addressed-client"),
	_enableAnimations(st, L"view.enable-animations"),
	_debug(st, L"debug"),
	_defaultTrackHeight(st, L"view.min-track-height"),
	_advertiseFiles(st, L"net.web.advertise-resources"),
	_enableEPEndpoint(st, L"net.ep.enabled"),
	_stickyFaders(ThemeManager::GetLayoutSettings(), L"layout.faders.sticky", true),
	_st(st)
	{
}

PreferencesInspectable::~PreferencesInspectable() {
	_st->SetValue(L"locale", _locale);
}

bool PreferencesInspectable::ChangesNeedRestart() {
	return _st->GetValue(L"locale")!=_locale || _debug.IsChanged() || _tickLength.IsChanged() || _minTickLength.IsChanged() || _enableEPEndpoint.IsChanged();
}

ref<PropertySet> PreferencesInspectable::GetProperties() {
	ref<PropertySet> ps = GC::Hold(new PropertySet());
	_locale = (LocaleIdentifier)_st->GetValue(L"locale");

	ps->Add(GC::Hold(new PropertySeparator(TL(properties_preferences))));
	ps->Add(Properties::CreateLanguageProperty(TL(locale), this, &_locale));
	ps->Add(_stickyFaders.CreateProperty(TL(enable_sticky_faders), this));
	ps->Add(_tooltips.CreateProperty(TL(enable_timeline_tooltips), this));
	ps->Add(_resourceWarnings.CreateProperty(TL(enable_resource_warnings), this));
	ps->Add(_noClientWarnings.CreateProperty(TL(enable_noclient_warnings), this));
	ps->Add(_enableAnimations.CreateProperty(TL(enable_animations), this));
	//ps->Add(_defaultTrackHeight.CreateProperty(TL(default_track_height), this, TL(settings_default_track_height_balloon)));

	ps->Add(GC::Hold(new PropertySeparator(TL(properties_network), true)));
	ps->Add(_netPort.CreateProperty(TL(port), this, TL(settings_net_port_balloon)));
	ps->Add(_netAddress.CreateProperty(TL(subnet), this, TL(settings_net_address_balloon)));
	ps->Add(_advertiseFiles.CreateProperty(TL(enable_resource_advertise), this, TL(enable_resource_advertise_balloon)));
	ps->Add(_enableEPEndpoint.CreateProperty(TL(enable_ep_endpoint), this));

	ps->Add(GC::Hold(new PropertySeparator(TL(properties_timing), true)));
	ps->Add(_tickLength.CreateProperty(TL(tick_length), this, TL(settings_tick_length_balloon)));
	ps->Add(_minTickLength.CreateProperty(TL(min_tick_length), this, TL(settings_min_tick_length_balloon)));

	ps->Add(GC::Hold(new PropertySeparator(TL(properties_other), true)));
	ps->Add(_debug.CreateProperty(TL(debug), this));

	return ps;
}

SettingsWnd::SettingsWnd(): ChildWnd(L"") {
	_tabs = GC::Hold(new TabWnd(0L));
	_tabs->SetChildStyle(true);
	_tabs->SetDetachAttachAllowed(false);

	strong<Model> model = Application::Instance()->GetModel();

	// Group settings (output groups)
	_groupsWnd = GC::Hold(new GroupsWnd(model->GetGroups()));
	_tabs->AddPane(GC::Hold(new Pane(TL(settings_groups), _groupsWnd, false, false, null, Placement(), L"icons/tabs/groups.png")));

	// Network client list
	_clientsWnd = GC::Hold(new ClientsWnd(Application::Instance()->GetNetwork()));
	_tabs->AddPane(GC::Hold(new Pane(TL(settings_clients), _clientsWnd, false, false, null, Placement(), L"icons/tabs/network.png")));

	// Patcher
	_patchWnd = GC::Hold(new PatchWnd(Application::Instance()->GetModel()->GetPatches()));
	_tabs->AddPane(GC::Hold(new Pane(TL(settings_patches), _patchWnd, false, false, 0, Placement(), L"icons/tabs/plugins.png")));

	// Input settings
	ref<input::Rules> rules = model->GetInputRules();
	if(rules) {
		_inputSettings = GC::Hold(new InputSettingsWnd(rules));
		_tabs->AddPane(GC::Hold(new Pane(TL(settings_input), _inputSettings, false, false, 0, Placement(), L"icons/tabs/outputs.png")));
	}

	// add plug-in windows
	ref<PluginManager> pm  = PluginManager::Instance();
	if(pm) {
		std::map< PluginHash, ref<PluginWrapper> >* plugins = pm->GetPluginsByHash();
		std::map< PluginHash, ref<PluginWrapper> >::iterator it = plugins->begin();
		while(it!=plugins->end()) {
			std::pair< PluginHash, ref<PluginWrapper> > data = *it;
			if(data.second) {
				ref<Pane> pane = data.second->GetSettingsWindow();
				if(pane) {
					 _pluginSettingsWindows[data.first] = pane;
					_tabs->AddPane(pane);
				}
			}
			++it;
		}
	}

	Add(_tabs);
	Layout();
}

SettingsWnd::~SettingsWnd() {
}

void SettingsWnd::ShowPatcher() {
	_tabs->RevealWindow(_patchWnd);
}

void SettingsWnd::OnSettingsChanged() {
	ref<Settings> st = GetSettings();
	if(_inputSettings) _inputSettings->SetSettings(st->GetNamespace(L"input-wnd"));
	if(_patchWnd) _patchWnd->SetSettings(st->GetNamespace(L"patcher-wnd"));
	if(_groupsWnd) _groupsWnd->SetSettings(st->GetNamespace(L"groups-wnd"));
	if(_clientsWnd) _clientsWnd->SetSettings(st->GetNamespace(L"clients-wnd"));

	std::map< PluginHash, ref<Pane> >::iterator it =  _pluginSettingsWindows.begin();
	while(it!= _pluginSettingsWindows.end()) {
		ref<Wnd> wnd = it->second->GetWindow();
		if(wnd) {
			wnd->SetSettings(st->GetNamespace(Stringify(it->first)));
		}
		++it;
	}
}

void SettingsWnd::Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme) {
	Area r = GetClientArea();
	SolidBrush back(theme->GetColor(Theme::ColorBackground));
	g.FillRectangle(&back,r);
}

void SettingsWnd::Layout() {
	Area area = GetClientArea();
	_tabs->Fill(LayoutFill, area);
}

void SettingsWnd::OnSize(const Area& ns) {
	Layout();
}

void SettingsWnd::Update() {
	Repaint();
}

/* InputSettingsWnd */
InputSettingsWnd::InputSettingsWnd(ref<input::Rules> rules): ChildWnd(L""), _rules(rules) {
	assert(_rules);

	_splitter = GC::Hold(new SplitterWnd(OrientationVertical));
	_splitter->SetRatio(0.2f);
	Add(_splitter, true);

	_channelList = GC::Hold(new InputChannelWnd(rules));
	_splitter->SetSecond(_channelList);

	_deviceList = GC::Hold(new InputDeviceListWnd(_channelList));
	_splitter->SetFirst(_deviceList);
}

InputSettingsWnd::~InputSettingsWnd() {
}

void InputSettingsWnd::Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme) {
	Area rc = GetClientArea();
	SolidBrush bbr(theme->GetColor(Theme::ColorBackground));
	g.FillRectangle(&bbr, rc);
}

void InputSettingsWnd::Layout() {
	Area rc = GetClientArea();
	_splitter->Fill(LayoutFill, rc);
}

void InputSettingsWnd::OnSize(const Area& ns) {
	Layout();
}

void InputSettingsWnd::OnSettingsChanged() {
	_deviceList->SetSettings(GetSettings()->GetNamespace(L"devices"));
	_splitter->SetSettings(GetSettings()->GetNamespace(L"splitter"));
}


/* InputDeviceListWnd */
InputDeviceListWnd::InputDeviceListWnd(ref<InputChannelWnd> clw): _arrowIcon(L"icons/arrow.png") {
	_channelList = clw;
	AddColumn(TL(patch_name), KColName, 1.0f);
}

InputDeviceListWnd::~InputDeviceListWnd() {
}

ref<Patches> InputDeviceListWnd::GetPatches() {
	ref<Model> model = Application::Instance()->GetModel();
	if(model) {
		return model->GetPatches();
	}
	return 0;
}

int InputDeviceListWnd::GetItemCount() {
	ref<Patches> ps = GetPatches();
	if(ps) {
		return ps->GetPatchCount();
	}
	return 0;
}

void InputDeviceListWnd::PaintItem(int id, tj::shared::graphics::Graphics& g, Area& row, const ColumnInfo& ci) {
	strong<Theme> theme = ThemeManager::GetTheme();
	ref<Patches> patches = GetPatches();
	if(patches) {
		PatchIdentifier pi = patches->GetPatchByIndex(id);

		SolidBrush tbr(theme->GetColor(Theme::ColorText));
		StringFormat sf;
		sf.SetAlignment(StringAlignmentNear);
		sf.SetTrimming(StringTrimmingEllipsisCharacter);
		DrawCellText(g, &sf, &tbr, theme->GetGUIFontBold(), KColName, row, pi);

		if(GetSelectedRow()==id) {
			Area iconRC(row.GetRight()-16, row.GetTop(), 16, 16);
			g.DrawImage(_arrowIcon, iconRC);
		}
	}
}

void InputDeviceListWnd::OnClickItem(int id, int col, Pixels x, Pixels y) {
	if(id>=0) {
		SetSelectedRow(id);	
		ref<Patches> patches = GetPatches();
		if(patches) {
			PatchIdentifier pi = patches->GetPatchByIndex(id);

			ref<InputChannelWnd> clw = _channelList;
			if(clw) {
				clw->SetDevice(pi);
			}
		}
	}
}

InputChannelListWnd::InputChannelListWnd(ref<input::Rules> rules): _rules(rules) {
	AddColumn(TL(input_channel_id), KColPath, 0.1f, true);
	AddColumn(TL(input_endpoint), KColEndpoint, 0.8f, true);
	SetEmptyText(TL(input_channel_list_empty_text));
}

InputChannelListWnd::~InputChannelListWnd() {
}

int InputChannelListWnd::GetItemCount() {
	return (int)_deviceRules.size();
}

void InputChannelListWnd::OnClickItem(int id, int col, Pixels x, Pixels y) {
	SetSelectedRowAndEdit(id);
}

void InputChannelListWnd::Rebuild() {
	_deviceRules.clear();
	_rules->FindRulesForPatch(_current, _deviceRules);
	Repaint();
}

void InputChannelListWnd::SetDevice(const PatchIdentifier& dev) {
	_current = dev;
	Rebuild();
	Update();
	Repaint();
}

ref<input::Rule> InputChannelListWnd::GetSelectedRule() {
	int n = GetSelectedRow();
	if(n>=0) {
		return _deviceRules.at(n);
	}
	return 0;
}

void InputChannelListWnd::PaintItem(int id, tj::shared::graphics::Graphics& g, Area& row, const ColumnInfo& ci) {
	strong<Theme> theme = ThemeManager::GetTheme();
	ref<input::Rule> rule = _deviceRules.at(id);

	SolidBrush text(theme->GetColor(Theme::ColorText));
	StringFormat sf;
	sf.SetAlignment(StringAlignmentNear);
	sf.SetTrimming(StringTrimmingEllipsisCharacter);

	DrawCellText(g, &sf, &text, theme->GetGUIFont(), KColPath, row, Stringify(rule->GetPath()));

	ref<Endpoint> ep = rule->GetEndpoint();
	if(ep) {
		DrawCellText(g, &sf, &text, theme->GetGUIFont(), KColEndpoint, row, ep->GetName());
	}
}

ref<Property> InputChannelListWnd::GetPropertyForItem(int id, int col) {
	if(id>=0 && id < int(_deviceRules.size())) {
		ref<input::Rule> rule = _deviceRules.at(id);
		if(col==KColPath) {
			return GC::Hold(new GenericProperty<InputID>(TL(input_channel), rule, &(rule->_path), rule->GetPath()));
		}
	}
	return null;
}

/* InputChannelWnd */
InputChannelWnd::InputChannelWnd(ref<input::Rules> rules): ChildWnd(false) {
	_toolbar = GC::Hold(new InputChannelToolbarWnd(this,rules));
	_list = GC::Hold(new InputChannelListWnd(rules));
	Add(_list, true);
	Add(_toolbar, true);
}

InputChannelWnd::~InputChannelWnd() {
}

void InputChannelWnd::SetDevice(const PatchIdentifier& pi) {
	if(_list) _list->SetDevice(pi);
}

void InputChannelWnd::Layout() {
	Area ns = GetClientArea();
	_toolbar->Fill(LayoutTop, ns);
	_list->Fill(LayoutFill, ns);
}

void InputChannelWnd::OnSize(const Area& ns) {
	Layout();
}

void InputChannelWnd::Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme) {
}

/* InputChannelToolbarWnd */
InputChannelToolbarWnd::InputChannelToolbarWnd(InputChannelWnd* cw, ref<input::Rules> rules): _rules(rules) {
	_cw = cw;
	Add(GC::Hold(new ToolbarItem(KCRemove, L"icons/remove.png", TL(input_detach_selected), false)));
}

InputChannelToolbarWnd::~InputChannelToolbarWnd() {
}

void InputChannelToolbarWnd::OnCommand(ref<ToolbarItem> ti) {
	OnCommand(ti->GetCommand());
}

void InputChannelToolbarWnd::OnCommand(int c) {
	if(c==KCRemove) {
		ref<input::Rule> rule = _cw->_list->GetSelectedRule();
		if(rule) {
			_rules->RemoveRule(rule);
			_cw->_list->Rebuild();
		}
	}

	_cw->_list->Repaint();
}

/* PatchToolbarWnd */
PatchToolbarWnd::PatchToolbarWnd(PatchWnd* p): _pw(p) {
	Add(GC::Hold(new ToolbarItem(KCAdd, L"icons/add.png", TL(patch_add), false)));
	Add(GC::Hold(new ToolbarItem(KCRemove, L"icons/remove.png", TL(patch_remove), true)));
	Add(GC::Hold(new ToolbarItem(KCChooseClient, L"icons/computer.png", TL(patch_choose_client_to_edit), true)));
	Add(GC::Hold(new ToolbarItem(KCRediscover, L"icons/search.png", TL(rediscover_devices), false)));
}

PatchToolbarWnd::~PatchToolbarWnd() {
}

void PatchToolbarWnd::OnCommand(ref<ToolbarItem> ti) {
	OnCommand(ti->GetCommand());
}

void PatchToolbarWnd::OnCommand(int r) {
	if(r==KCAdd) {
		class AddPatchData: public Inspectable {
			public:
				AddPatchData() {
				}

				virtual ~AddPatchData() {
				}

				virtual ref<PropertySet> GetProperties() {
					ref<PropertySet> ps = GC::Hold(new PropertySet());
					ps->Add(GC::Hold(new GenericProperty<std::wstring>(TL(patch_name), this, &_name, _name)));
					return ps;
				}

				std::wstring _name;
		};

		ref<PropertyDialogWnd> dw = GC::Hold(new PropertyDialogWnd(TL(patch_add), TL(patch_add_question)));
		ref<AddPatchData> data = GC::Hold(new AddPatchData());
		dw->GetPropertyGrid()->Inspect(data);
		dw->SetSize(300,200);

		if(dw->DoModal(this)) {
			if(data->_name.length()<1) {
				Throw(TL(error_patch_must_have_name), ExceptionTypeError);
			}
			_pw->GetPatches()->AddPatch(data->_name);
		}
		_pw->Update();
	}
	else if(r==KCRemove) {
		PatchIdentifier pi = _pw->GetSelectedPatch();
		_pw->GetPatches()->RemovePatch(pi);
		_pw->Update();
	}
	else if(r==KCChooseClient) {
		ref<Network> nw = Application::Instance()->GetNetwork();
		if(nw) {
			ref<Client> selected = _pw->GetClient();

			int n = 0;
			std::map<InstanceID, ref<Client> >* clients = nw->GetClients();
			if(clients!=0) {
				ContextMenu cm;
				std::map<InstanceID, ref<Client> >::iterator it = clients->begin();
				while(it!=clients->end()) {
					ref<Client> client = it->second;
					if(client && client->GetInstanceID()!=nw->GetInstanceID()) {
						std::wstring name = client->GetHostName() + L" (" + StringifyHex(client->GetInstanceID()) + L")";
						cm.AddItem(name, client->GetInstanceID(), false, selected==client);
						++n;
					}
					++it;
				}

				if(n<1) {
					cm.AddItem(TL(patch_no_clients_connected), -1, false, false);
				}

				Area rc = GetClientArea();
				int r = cm.DoContextMenu(this, GetButtonX(KCChooseClient), rc.GetBottom());
				if(r>0) {
					ref<Client> client = nw->GetClientByInstanceID((InstanceID)r);
					if(client) {
						_pw->SetClient(client);
					}
				}
			}			
		}
	}
	else if(r==KCRediscover) {
		Application::Instance()->RediscoverDevices();
	}
}

/* PatchWnd */
PatchWnd::PatchWnd(ref<Patches> p): ChildWnd(false), _patches(p) {
	_tools = GC::Hold(new PatchToolbarWnd(this));
	_list = GC::Hold(new PatchListWnd(p));

	Add(_tools);
	Add(_list);
}

PatchWnd::~PatchWnd() {
}

void PatchWnd::SetClient(ref<Client> c) {
	_list->SetClient(c);
}

ref<Client> PatchWnd::GetClient() {
	return _list->GetClient();
}

void PatchWnd::Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme) {
}

void PatchWnd::Update() {
	_list->Update();
	_tools->Update();
}

PatchIdentifier PatchWnd::GetSelectedPatch() {
	int r = _list->GetSelectedRow();
	if(r<0) {
		return L"";
	}

	std::map<PatchIdentifier, DeviceIdentifier>::const_iterator it = _patches->GetPatches().begin();
	std::map<PatchIdentifier, DeviceIdentifier>::const_iterator end = _patches->GetPatches().end();

	for(int a=0;a<r && a<int(_patches->GetPatchCount()) && it!=end;a++) {
		++it;
	}

	if(it==end) return L"";

	return it->first;
}

ref<Patches> PatchWnd::GetPatches() {
	return _patches;
}

void PatchWnd::Layout() {
	Area rc = GetClientArea();
	_tools->Fill(LayoutTop, rc);
	_list->Fill(LayoutFill, rc);
}

void PatchWnd::OnSettingsChanged() {
	ref<Settings> st = GetSettings();
	if(st) {
		_list->SetSettings(st->GetNamespace(L"list"));
		_tools->SetSettings(st->GetNamespace(L"toolbar"));
	}
}

void PatchWnd::OnSize(const tj::shared::Area &ns) {
	Layout();
}

PatchListWnd::PatchListWnd(ref<Patches> p): _patches(p) {
	assert(p);

	AddColumn(TL(patch_name), KColName, 0.2f, true);
	AddColumn(TL(patch_device), KColDevice, 0.3f, true);
	AddColumn(TL(patch_client_device), KColClientDevice, 0.3f, true);
	AddColumn(TL(device_identifier), KColIdentifier, 0.4f, false);
	AddColumn(TL(device_client_identifier), KColClientIdentifier, 0.4f, false);
}

PatchListWnd::~PatchListWnd() {
}

void PatchListWnd::Update() {
	ListWnd::Update();
	Repaint();
}

void PatchListWnd::SetClient(ref<Client> c) {
	_client = c;
	Update();
}

ref<Client> PatchListWnd::GetClient() {
	return _client;
}

void PatchListWnd::PaintItem(int id, tj::shared::graphics::Graphics& g, Area& row, const ColumnInfo& ci) {
	std::map<PatchIdentifier, DeviceIdentifier>::const_iterator it = _patches->GetPatches().begin();
	for(int a=0;a<id;a++) {
		++it;
	}

	strong<Theme> theme = ThemeManager::GetTheme();
	SolidBrush tbr(theme->GetColor(Theme::ColorText));
	SolidBrush dbr(Theme::ChangeAlpha(theme->GetColor(Theme::ColorText),127));
	StringFormat sf;
	sf.SetTrimming(StringTrimmingEllipsisPath);

	ref<PluginManager> pm = PluginManager::Instance();
	ref<Device> dev = pm->GetDeviceByIdentifier(it->second);

	if(IsColumnVisible(KColName)) {
		DrawCellText(g, &sf, &tbr, theme->GetGUIFontBold(), KColName, row, it->first);

		// Draw icon of device next to patch name
		if(dev) {
			ref<Icon> icon = dev->GetIcon();
			if(icon) {
				Area cell(Pixels((GetColumnX(KColName)+GetColumnWidth(KColName))*row.GetWidth()-16), row.GetTop(), 16, 16);
				g.DrawImage(icon->GetBitmap(), cell);
			}
		}
	}
	
	if(dev) {
		DrawCellText(g, &sf, &tbr, theme->GetGUIFont(), KColDevice, row, dev->GetFriendlyName());
		DrawCellText(g, &sf, &tbr, theme->GetGUIFont(), KColIdentifier, row, dev->GetIdentifier());
	}
	else {
		DrawCellText(g, &sf, &dbr, theme->GetGUIFont(), KColDevice, row, TL(patch_no_device));
	}

	DrawCellDownArrow(g, KColDevice, row);

	// Draw client side patch info
	if(_client) {
		DeviceIdentifier di = _client->GetDeviceByPatch(it->first);
		ref<Device> cdev = _client->GetDeviceByIdentifier(di);
		if(cdev) {
			DrawCellText(g, &sf, &tbr, theme->GetGUIFont(), KColClientDevice, row, cdev->GetFriendlyName());
			DrawCellText(g, &sf, &tbr, theme->GetGUIFont(), KColClientIdentifier, row, di);
		}
		else {
			DrawCellText(g, &sf, &dbr, theme->GetGUIFont(), KColClientDevice, row, TL(patch_no_device));
		}
		
		DrawCellDownArrow(g, KColClientDevice, row);
	}
}

int PatchListWnd::GetItemCount() {
	return _patches->GetPatchCount();
}

void PatchListWnd::OnClickItem(int id, int col, Pixels x, Pixels y) {
	SetSelectedRow(id);
	
	if(col==KColDevice) {
		if(id>=0 && id < int(_patches->GetPatchCount())) {
			std::map<PatchIdentifier, DeviceIdentifier>::const_iterator it = _patches->GetPatches().begin();
			for(int a=0;a<id;a++) {
				++it;
			}
			
			// show device list and attach
			strong<PluginManager> pmr = PluginManager::Instance();
			Application::Instance()->RediscoverDevices();
			
			ContextMenu cm;
			std::vector< ref<Device> > options;
			std::map<DeviceIdentifier, ref<Device> >* devs = pmr->GetDevices();
			if(devs!=0) {
				int n = 1;
				options.push_back(0);
				cm.AddItem(TL(patch_none), 1, false, it->second == L"");
				cm.AddSeparator();
				std::map<DeviceIdentifier, ref<Device> >::iterator cit = devs->begin();
				while(cit!=devs->end()) {
					++n;
					ref<Device> dev = cit->second;
					if(dev) {
						options.push_back(dev);
						ref<MenuItem> ci = GC::Hold(new MenuItem(dev->GetFriendlyName(), n, false, (dev->GetIdentifier()==it->second)?MenuItem::RadioChecked:MenuItem::NotChecked, dev->GetIcon()));
						cm.AddItem(ci);
					}
					++cit;
				}
			}

			Area rc = GetClientArea();
			Pixels x = Pixels(GetColumnX(KColDevice)*rc.GetWidth());
			Pixels y = GetRowArea(id).GetBottom();
			int r = cm.DoContextMenu(this,x,y);
			if(r>0) {
				ref<Device> d = options.at(r-1);
				_patches->SetPatch(it->first, d);
			}
			Repaint();
		}
	}
	else if(col==KColClientDevice) {
		if(_client) {
			if(id>=0 && id < int(_patches->GetPatchCount())) {
				std::map<PatchIdentifier, DeviceIdentifier>::const_iterator it = _patches->GetPatches().begin();
				for(int a=0;a<id;a++) {
					++it;
				}

				DeviceIdentifier currentlyPatched = _client->GetDeviceByPatch(it->first);
				
				// show device list and attach				
				ContextMenu cm;
				std::vector< ref<Device> > options;
				std::map<DeviceIdentifier, ref<Device> >* devs = _client->GetDevices();
				if(devs!=0) {
					int n = 1;
					options.push_back(0);
					cm.AddItem(TL(patch_none), 1, false, currentlyPatched == L"");
					cm.AddSeparator();

					std::map<DeviceIdentifier, ref<Device> >::iterator cit = devs->begin();
					while(cit!=devs->end()) {
						++n;
						ref<Device> dev = cit->second;
						if(dev) {
							options.push_back(dev);
							cm.AddItem(dev->GetFriendlyName(), n, false, (dev->GetIdentifier()==currentlyPatched)?MenuItem::RadioChecked:MenuItem::NotChecked);
						}
						++cit;
					}
				}

				Area rc = GetClientArea();
				Pixels x = Pixels(GetColumnX(KColClientDevice)*rc.GetWidth());
				Pixels y = GetRowArea(id).GetBottom();
				int r = cm.DoContextMenu(this,x,y);
				if(r>0) {
					ref<Device> d = options.at(r-1);
					ref<Network> nw = Application::Instance()->GetNetwork();
					if(nw) {
						nw->SetClientPatch(_client, it->first, d?d->GetIdentifier():L"");
					}
				}
				Repaint();
			}
		}
	}
}
