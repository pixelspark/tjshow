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
#include "../../include/internal/tjsubtimeline.h"
#include "../../include/internal/view/tjcapacitywnd.h"

using namespace tj::show;
using namespace tj::show::view;
using namespace tj::shared::graphics;

/* CapacityListWnd */
CapacityListWnd::CapacityListWnd(ref<Capacities> caps): ListWnd() {
	AddColumn(TL(capacity_name), KColName, 0.2f);
	AddColumn(TL(capacity_value), KColValue, 0.1f);
	AddColumn(TL(capacity_initial), KColInitial, 0.1f);
	AddColumn(TL(capacity_waiting), KColWaiting, 0.1f);
	AddColumn(TL(capacity_description), KColDescription, 0.5f);
	
	_caps = caps;
}

CapacityListWnd::~CapacityListWnd() {
}

int CapacityListWnd::GetItemCount() {
	return _caps->GetCapacityCount();
}

void CapacityListWnd::PaintItem(int id, tj::shared::graphics::Graphics& g, Area& row, const ColumnInfo& ci) {
	strong<Theme> theme = ThemeManager::GetTheme();
	ref<Capacity> cap = _caps->_caps.at(id);

	SolidBrush br(theme->GetColor(Theme::ColorText));
	SolidBrush gbr(theme->GetColor(Theme::ColorActiveStart));
	SolidBrush rbr(Color(1.0, 0.0, 0.0));
	StringFormat sf;

	sf.SetAlignment(StringAlignmentNear);
	DrawCellText(g, &sf, &br, theme->GetGUIFontBold(), KColName, row, cap->GetName());
	DrawCellText(g, &sf, &br, theme->GetGUIFont(), KColDescription, row, cap->GetDescription());

	int val = cap->GetValue();
	DrawCellText(g, &sf, (val>0)?&gbr:&rbr, theme->GetGUIFont(), KColValue, row, Stringify(val));
	
	DrawCellText(g, &sf, &gbr, theme->GetGUIFont(), KColInitial, row, Stringify(cap->GetInitialValue()));
	DrawCellText(g, &sf, &gbr, theme->GetGUIFont(), KColWaiting, row, cap->GetWaitingList());
}

void CapacityListWnd::OnRightClickItem(int id, int col) {
	Area row = GetRowArea(id);
	SetSelectedRow(id);
	Pixels x = Pixels(GetColumnX(col)*row.GetWidth());
	OnContextMenu(x,row.GetBottom());
}

void CapacityListWnd::OnContextMenu(Pixels x, Pixels y) {
	int r = GetSelectedRow();
	ContextMenu cm;
	enum { KCProperties = 1, KCCopy, KCCut, KCPaste  };
	ref<Capacity> cap = _caps->GetCapacityByIndex(r);

	if(cap) {
		cm.AddItem(TL(properties), KCProperties, true, false);
		cm.AddItem(GC::Hold(new MenuItem(TL(copy), KCCopy, false, MenuItem::NotChecked, Icons::GetIconPath(Icons::IconCopy), L"Ctrl-C")));
		cm.AddItem(GC::Hold(new MenuItem(TL(cut), KCCut, false, MenuItem::NotChecked, Icons::GetIconPath(Icons::IconCut), L"Ctrl-X")));
	}

	bool canPaste = Clipboard::IsObjectAvailable();
	cm.AddItem(GC::Hold(new MenuItem(TL(paste), canPaste ? KCPaste : -1, false, MenuItem::NotChecked, Icons::GetIconPath(Icons::IconPaste), L"Ctrl-V")));

	int res = cm.DoContextMenu(this, x, y);
	if(res==KCProperties) {
		Inspect(cap);
	}
	else if(res==KCCopy) {
		OnCopy();
	}
	else if(res==KCPaste) {
		OnPaste();
	}
	else if(res==KCCut) {
		OnCut();
	}
}

void CapacityListWnd::OnCopy() {
	int id = GetSelectedRow();
	if(id>=0) {
		ref<Capacity> cap = _caps->GetCapacityByIndex(id);
		if(cap) {
			Clipboard::SetClipboardObject(cap);
		}
	}
}

void CapacityListWnd::OnPaste() {
	ref<Capacity> cap = GC::Hold(new Capacity());
	if(Clipboard::GetClipboardObject(cap)) {
		cap->Clone();
		_caps->AddCapacity(cap);
	}
	Update();
	Repaint();
}

void CapacityListWnd::OnCut() {
	int id = GetSelectedRow();
	if(id>=0) {
		ref<Capacity> cap = _caps->GetCapacityByIndex(id);
		if(cap) {
			Clipboard::SetClipboardObject(cap);
			_caps->RemoveCapacity(cap);
		}
	}
	Update();
	Repaint();
}

void CapacityListWnd::OnClickItem(int id, int col, Pixels x, Pixels y) {
	SetSelectedRow(id);

	// get selected capacity
	ref<Capacity> cap = _caps->GetCapacityByIndex(id);
	if(cap) {
		Inspect(cap);
	}
}

void CapacityListWnd::Inspect(ref<Capacity> cap) {
	ref<Path> path = GC::Hold(new Path());
	path->Add(Application::Instance()->GetModel()->CreateModelCrumb());
	path->Add(_caps->CreateCapacityCrumb(cap));
		
	Application::Instance()->GetView()->Inspect(cap,path);
	Focus();
}

namespace tj {
	namespace show {
		namespace view {
			class CapacityToolbarWnd: public ToolbarWnd {
				public:
					enum {
						KCAdd=1,
						KCRemove,
						KCUp,
						KCDown,
						KCRelease,
						KCUse,
						KCReset,
						KCGraph,
					};

					CapacityToolbarWnd(CapacitiesWnd* w) {
						_caps = w;

						Add(GC::Hold(new ToolbarItem(KCAdd, L"icons/add.png", TL(capacity_add), false)));
						Add(GC::Hold(new ToolbarItem(KCRemove, L"icons/remove.png", TL(capacity_remove), false)));
						Add(GC::Hold(new ToolbarItem(KCUp, L"icons/up.png", TL(capacity_up), false)));
						Add(GC::Hold(new ToolbarItem(KCDown, L"icons/down.png", TL(capacity_down), true)));
						//Add(GC::Hold(new ToolbarItem(KCUse, L"icons/decrement.png", TL(capacity_use), false)));
						//Add(GC::Hold(new ToolbarItem(KCRelease, L"icons/increment.png", TL(capacity_release), false)));
						Add(GC::Hold(new ToolbarItem(KCReset, L"icons/reset.png", TL(capacity_reset), false)));
					}

					virtual ~CapacityToolbarWnd() {
					
					}

					virtual void OnCommand(ref<ToolbarItem> ti) {
						OnCommand(ti->GetCommand());
					}

					virtual void OnCommand(int c) {
						if(c==KCAdd) {
							ref<Capacity> nc = GC::Hold(new Capacity());
							nc->SetName(TL(capacity_untitled));
							_caps->_caps->AddCapacity(nc);
							_caps->_list->Repaint();
						}
						else if(c==KCRemove) {
							int r = _caps->_list->GetSelectedRow();


							if(r>=0 && Alert::ShowYesNo(TL(capacity_remove),TL(capacity_remove_confirm), Alert::TypeQuestion)) {
								_caps->_caps->RemoveCapacity(r);
								Application::Instance()->GetView()->Inspect(0);
							}
						}
						else if(c==KCReset) {
							int r = _caps->_list->GetSelectedRow();
							if(r>=0) {
								ref<Capacity> cap = _caps->_caps->GetCapacityByIndex(r);
								if(cap) {
									cap->Reset();
								}
							}
						}
						else if(c==KCUp||c==KCDown) {
							int r = _caps->_list->GetSelectedRow();
							if(r>=0) {
								ref<Capacity> cap = _caps->_caps->GetCapacityByIndex(r);
								if(cap) {
									if(c==KCUp) {
										_caps->_caps->MoveUp(cap);
										_caps->_list->SetSelectedRow(max(r-1,0));
									}
									else if(c==KCDown) {
										_caps->_caps->MoveDown(cap);
										_caps->_list->SetSelectedRow(min(r+1, _caps->_caps->GetCapacityCount()-1));
									}
								}
							}
						}
					}

				protected:
					CapacitiesWnd* _caps;
			};
		}
	}
}

/* CapacitiesWnd */
CapacitiesWnd::CapacitiesWnd(ref<Capacities> caps): ChildWnd(false) {
	_list = GC::Hold(new CapacityListWnd(caps));
	Add(_list);
	
	_tools = GC::Hold(new CapacityToolbarWnd(this));
	Add(_tools);
	
	_caps = caps;
}

CapacitiesWnd::~CapacitiesWnd() {
}

void CapacitiesWnd::Update() {
	_list->Repaint();
}

void CapacitiesWnd::Layout() {
	Area rc = GetClientArea();
	_tools->Fill(LayoutTop, rc);
	_list->Fill(LayoutFill, rc);
}

void CapacitiesWnd::OnSize(const tj::shared::Area &ns) {
	Layout();
}

void CapacitiesWnd::Paint(Graphics& g, strong<Theme> theme) {
}