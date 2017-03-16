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
#include "../../include/internal/view/tjclientswnd.h"
#include "../../include/internal/view/tjdialogs.h"
#include "../../include/internal/view/tjfilterproperty.h"

using namespace tj::show;
using namespace tj::shared::graphics;
using namespace tj::show::view;

namespace tj {
	namespace show {
		namespace view {
			class ClientListWnd: public EditableListWnd {
				public:
					ClientListWnd(ref<Network> net);
					virtual ~ClientListWnd();
					virtual int GetItemCount();
					virtual void PaintItem(int id, tj::shared::graphics::Graphics& g, Area& row, const ColumnInfo& ci);
					virtual ref<Property> GetPropertyForItem(int id, int col);
					
				protected:
					virtual void OnRightClickItem(int id, int col);
					virtual void OnClickItem(int id, int col, Pixels x, Pixels y);
					virtual void OnEditingDone(int row);

					enum {
						KColIP = 1,
						KColRole,
						KColInstanceID,
						KColAddress,
						KColHostName,
						KColLatency,
						KColFeatures,
						KColDeviceCount,
						KColMAC,
					};

					ref<Client> _editingClient;
					ref<Filter> _editingFilter;
					ref<Network> _network;
			};

			class GroupsListWnd: public EditableListWnd {
				public:
					GroupsListWnd(strong<Groups> net);
					virtual ~GroupsListWnd();
					virtual int GetItemCount();
					virtual void PaintItem(int id, tj::shared::graphics::Graphics& g, Area& row, const ColumnInfo& ci);
					virtual void DoAddGroupDialog();
					virtual void RemoveSelectedGroup();
					
				protected:
					virtual ref<Property> GetPropertyForItem(int id, int col);
					virtual void OnRightClickItem(int id, int col);
					virtual void OnClickItem(int id, int col, Pixels x, Pixels y);

					enum {
						KColID = 1,
						KColColor,
						KColName,
					};

					strong<Groups> _groups;
			};

			class GroupsToolbarWnd: public ToolbarWnd {
				public:
					GroupsToolbarWnd(strong<GroupsListWnd> glw);
					virtual ~GroupsToolbarWnd();
					virtual void OnCommand(ref<ToolbarItem> ti);

					enum {
						KCAdd = 1,
						KCRemove,
					};

					weak<GroupsListWnd> _glw;

			};

			class ClientsToolbarWnd: public ToolbarWnd {
				public:
					enum {
						KCWake = 1,
					};

					ClientsToolbarWnd(ref<Network> net): _net(net) {
						Add(GC::Hold(new ToolbarItem(KCWake, L"icons/wol.png", TL(client_wake), true)));
					}

					virtual void OnCommand(ref<ToolbarItem> ti) {
						OnCommand(ti->GetCommand());
					}

					virtual void OnCommand(int c) {
						if(c==KCWake) {
							ContextMenu cm;
							cm.AddSeparator(TL(client_wake));

							std::vector< ref<Client> > clients;
							_net->GetCachedClients(clients);
							if(clients.size()>0) {
								std::vector< ref<Client> >::iterator it = clients.begin();
								int n = 0;
								while(it!=clients.end()) {
									++n;
									ref<Client> client = *it;
									if(client) {
										cm.AddItem(client->GetHostName()+L" ("+client->GetMACAddress().ToString()+L")", n, false, false);
									}
									++it;
								}
							}
							else {
								cm.AddItem(TL(client_no_cached), -1);
							}

							Area rc = GetClientArea();
							int r = cm.DoContextMenu(this, GetButtonX(KCWake), rc.GetBottom());
							if(r>0) {
								ref<Client> client = clients.at(r-1);
								if(client) {
									Networking::Wake(client->GetMACAddress());
								}
							}
						}
					}

					virtual ~ClientsToolbarWnd() {
					}

				protected:
					ref<Network> _net;
			};
		}
	}
}

/** ClientsWnd **/
ClientsWnd::ClientsWnd(strong<Network> net): ChildWnd(false) {
	_list = GC::Hold(new ClientListWnd(net));
	_tools = GC::Hold(new ClientsToolbarWnd(net));
	Add(_list);
	Add(_tools);
}

ClientsWnd::~ClientsWnd() {
}

void ClientsWnd::Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme) {
}

void ClientsWnd::OnSettingsChanged() {
	ref<Settings> st = GetSettings();
	if(st) {
		_list->SetSettings(st->GetNamespace(L"list"));
		_tools->SetSettings(st->GetNamespace(L"tools"));
	}
}

void ClientsWnd::Layout() {
	if(_tools && _list) {
		Area rc = GetClientArea();
		_tools->Fill(LayoutTop, rc);
		_list->Fill(LayoutFill, rc);
	}
}
void ClientsWnd::OnSize(const Area& ns) {
	Layout();
}

ClientListWnd::ClientListWnd(ref<Network> net) {
	_network = net;

	AddColumn(TL(client_ip), KColIP);
	AddColumn(TL(client_role), KColRole, 0.1f);
	AddColumn(TL(client_instance), KColInstanceID, 0.1f, false);
	AddColumn(TL(client_filter), KColAddress);
	AddColumn(TL(hostname), KColHostName,0.1f,true);
	AddColumn(TL(features), KColFeatures, 0.1f, false);
	AddColumn(TL(latency_ms), KColLatency, 0.1f, false);
	AddColumn(TL(client_device_count), KColDeviceCount, 0.1f, false);
	AddColumn(TL(client_mac_address), KColMAC, 0.1f, false);
	SetEmptyText(TL(client_list_empty));
}

ClientListWnd::~ClientListWnd() {
}

int ClientListWnd::GetItemCount() {
	return int(_network->GetClients()->size());
}

ref<Property> ClientListWnd::GetPropertyForItem(int id, int col) {
	if(col==KColAddress) {
		std::map<InstanceID, ref<Client> >* clients = _network->GetClients();
		std::map<InstanceID, ref<Client> >::const_iterator it = clients->begin();
		while((id--)!=0) {
			++it;
		}

		if(it!=clients->end()) {
			_editingClient = it->second;
			_editingFilter = GC::Hold(new Filter());
			_editingFilter->Parse(it->second->GetAddressing());
			return GC::Hold(new FilterProperty(L"", _editingFilter));
		}
	}
	return null;
}

void ClientListWnd::PaintItem(int id, tj::shared::graphics::Graphics& g, Area& row, const ColumnInfo& ci) {
	strong<Theme> theme = ThemeManager::GetTheme();
	std::map<InstanceID, ref<Client> >* clients = _network->GetClients();
	if(int(clients->size())<id) {
		return;
	}

	// TODO fix this really ugly way of getting the Nth element
	std::map<InstanceID, ref<Client> >::const_iterator it = clients->begin();
	while((id--)!=0) {
		++it;
	}

	// Let's paint
	ref<Client> client = it->second;
	
	StringFormat sf;
	sf.SetLineAlignment(StringAlignmentCenter);
	sf.SetTrimming(StringTrimmingEllipsisCharacter);
	SolidBrush text(theme->GetColor(Theme::ColorText));
	SolidBrush offline(theme->GetColor(Theme::ColorActiveStart));

	tj::shared::graphics::Font* font = theme->GetGUIFont();
	if(client->GetInstanceID()==_network->GetInstanceID()) {
		font = theme->GetGUIFontBold();
	}

	// Draw line in gray if the client did not respond to the last two announces
	int announcePeriod = 2*StringTo<int>(Application::Instance()->GetSettings()->GetValue(L"net.announce-period"),0);
	SolidBrush* brush = client->IsOnline(announcePeriod)?&text:&offline;
	DrawCellText(g, &sf, brush, font, KColIP, row, client->GetIP());
	DrawCellText(g, &sf, brush, font, KColRole, row, Roles::GetRoleName(client->GetRole()));
	DrawCellText(g, &sf, brush, font, KColInstanceID, row, StringifyHex(client->GetInstanceID()));
	DrawCellText(g, &sf, brush, font, KColAddress, row, client->GetAddressing());
	DrawCellText(g, &sf, brush, font, KColHostName, row, client->GetHostName());
	DrawCellText(g, &sf, brush, font, KColFeatures, row, Network::FormatFeatures(client->GetFeatures()));
	DrawCellText(g, &sf, brush, font, KColDeviceCount, row, Stringify(client->GetDeviceCount()));
	DrawCellText(g, &sf, brush, font, KColMAC, row, client->GetMACAddress().ToString());

	long double latency = client->GetLatency();
	if(latency>=0.0) {
		DrawCellText(g, &sf, &text, font, KColLatency, row, Stringify(latency));
	}
}

void ClientListWnd::OnClickItem(int id, int col, Pixels x, Pixels y) {
	SetSelectedRowAndEdit(id);
	Repaint();
}

void ClientListWnd::OnEditingDone(int row) {
	if(_editingClient) {
		std::wstring na = _editingFilter->Dump();
		Application::Instance()->GetNetwork()->SetClientAddress(_editingClient, na);
		_editingClient = null;
		_editingFilter = null;
	}
}

void ClientListWnd::OnRightClickItem(int id, int col) {
	// Context menu for client; currently unused, but code still here, because it might be
	// useful to have a context menu here in the future

	/*
	std::map<InstanceID, ref<Client> >* clients = _network->GetClients();
	if(int(clients->size())<id) {
		return;
	}

	// TODO fix this really ugly way of getting the Nth element
	std::map<InstanceID, ref<Client> >::const_iterator it = clients->begin();
	int oid = id;
	while((oid--)!=0 && it!=clients->end()) {
		++it;
	}
	if(it==clients->end()) return;

	ref<Client> client = it->second;
	if(client) {
		// Make a context menu
		ContextMenu m;

		Area area = GetClientArea();
		Area row = GetRowArea(id);
		Pixels x = Pixels(GetColumnX(col)*area.GetWidth());
		switch(m.DoContextMenu(this, x, row.GetBottom())) {
		}
	}
	*/
	EditableListWnd::OnRightClickItem(id,col);
}

/** GroupsWnd **/
GroupsWnd::GroupsWnd(strong<Groups> groups): ChildWnd(false) {
	_list = GC::Hold(new GroupsListWnd(groups));
	Add(_list);

	_tools = GC::Hold(new GroupsToolbarWnd(_list));
	Add(_tools);
}

GroupsWnd::~GroupsWnd() {
}

void GroupsWnd::Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme) {
}

void GroupsWnd::Layout() {
	Area rc = GetClientArea();
	if(_tools) {
		_tools->Fill(LayoutTop, rc);
	}

	if(_list) {
		_list->Fill(LayoutFill, rc);
	}
}

void GroupsWnd::OnSize(const Area& ns) {
	Layout();
}

void GroupsWnd::OnSettingsChanged() {
	ref<Settings> st = GetSettings();
	if(_list) {
		_list->SetSettings(st->GetNamespace(L"list"));
	}

	if(_tools) {
		_tools->SetSettings(st->GetNamespace(L"tools"));
	}
}

/** GroupsListWnd **/
GroupsListWnd::GroupsListWnd(strong<Groups> gr): _groups(gr) {
	AddColumn(TL(group_id), KColID, 0.1f);
	AddColumn(TL(group_color), KColColor, 0.1f);
	AddColumn(TL(group_name), KColName, 0.8f);
	SetEmptyText(TL(group_list_empty_text));
}

GroupsListWnd::~GroupsListWnd() {
}

int GroupsListWnd::GetItemCount() {
	return int(_groups->GetGroupCount());
}

ref<Property> GroupsListWnd::GetPropertyForItem(int id, int col) {
	ref<Group> group = _groups->GetGroupByIndex(id);
	if(group) {
		if(col==KColName) {
			return GC::Hold(new GenericProperty<std::wstring>(L"", group, &(group->_name), group->_name));
		}
		else if(col==KColColor) {
			return GC::Hold(new ColorProperty(L"", group, &(group->_color), 0));
		}
	}
	return null;
}

void GroupsListWnd::PaintItem(int id, tj::shared::graphics::Graphics& g, Area& row, const ColumnInfo& ci) {
	ref<Group> group = _groups->GetGroupByIndex(id);
	if(group) {
		StringFormat sf;
		sf.SetAlignment(StringAlignmentNear);
		sf.SetTrimming(StringTrimmingEllipsisCharacter);
		strong<Theme> theme = ThemeManager::GetTheme();
		Font* fnt = theme->GetGUIFont();
		SolidBrush tbr(theme->GetColor(Theme::ColorText));

		DrawCellText(g, &sf, &tbr, fnt, KColID, row, Stringify(group->GetID()));
		DrawCellText(g, &sf, &tbr, fnt, KColName, row, group->GetName());

		const RGBColor& color = group->GetColor();
		SolidBrush colorBrush((Color)color);
		Area colorCell(Pixels(GetColumnX(KColColor)*row.GetWidth()), row.GetTop(), Pixels(GetColumnWidth(KColColor)*row.GetWidth()), row.GetHeight());
		colorCell.Narrow(1,1,2,2);
		g.FillRectangle(&colorBrush, colorCell);
	}
}

void GroupsListWnd::OnRightClickItem(int id, int col) {
}

void GroupsListWnd::OnClickItem(int id, int col, Pixels x, Pixels y) {
	SetSelectedRowAndEdit(id);
}

void GroupsListWnd::DoAddGroupDialog() {
	class AddGroupData: public Inspectable {
		public:
			AddGroupData() {
			}

			virtual ref<PropertySet> GetProperties() {
				ref<PropertySet> ps = GC::Hold(new PropertySet());
				ps->Add(GC::Hold(new GenericProperty<GroupID>(TL(group_id), this, &_gid, _gid)));
				ps->Add(GC::Hold(new GenericProperty<std::wstring>(TL(group_name), this, &_name, _name)));
				return ps;
			}

			GroupID _gid;
			std::wstring _name;
	};

	ref<PropertyDialogWnd> pdw = GC::Hold(new PropertyDialogWnd(TL(group_add_dialog), TL(group_add_dialog_question)));
	ref<AddGroupData> agd = GC::Hold(new AddGroupData());
	pdw->SetSize(240, 240);
	pdw->GetPropertyGrid()->Inspect(agd);
	agd->_gid = _groups->GetHighestGroupID() + 1;
	if(pdw->DoModal(this)) {
		ref<Group> group = GC::Hold(new Group(agd->_gid, agd->_name));
		if(!_groups->Add(group)) {
			Throw(L"Cannot add group; probably, another group with the same ID already exists", ExceptionTypeError);
		}
	}

	OnSize(GetClientArea());
	Repaint();
}

void GroupsListWnd::RemoveSelectedGroup() {
	int r = GetSelectedRow();
	if(r>=0 && r<int(_groups->GetGroupCount())) {
		_groups->RemoveGroup(r);
	}
	SetSelectedRow(-1);
	OnSize(GetClientArea());
	Repaint();
}

/** GroupsToolbarWnd **/
GroupsToolbarWnd::GroupsToolbarWnd(strong<GroupsListWnd> glw): _glw(ref<GroupsListWnd>(glw)) {
	Add(GC::Hold(new ToolbarItem(KCAdd, L"icons/add.png", TL(group_add), false)));
	Add(GC::Hold(new ToolbarItem(KCRemove, L"icons/remove.png", TL(group_remove), false)));
}

GroupsToolbarWnd::~GroupsToolbarWnd() {
}

void GroupsToolbarWnd::OnCommand(ref<ToolbarItem> ti) {
	ref<GroupsListWnd> glw = _glw;
	if(glw) {
		if(ti->GetCommand()==KCAdd) {
			glw->DoAddGroupDialog();	
		}
		else if(ti->GetCommand()==KCRemove) {
			glw->RemoveSelectedGroup();
		}
	}
}