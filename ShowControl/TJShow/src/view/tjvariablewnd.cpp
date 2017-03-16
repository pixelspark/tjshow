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
#include "../../include/internal/view/tjvariablewnd.h"

using namespace tj::show;
using namespace tj::show::view;
using namespace tj::shared::graphics;

namespace tj {
	namespace show {
		namespace view {
			class VariableListWnd: public ListWnd {
				public:
					VariableListWnd(ref<Variables> vars);
					virtual ~VariableListWnd();
					virtual int GetItemCount();
					virtual void PaintItem(int id, tj::shared::graphics::Graphics& g, Area& row, const ColumnInfo& ci);

				protected:
					virtual void OnClickItem(int id, int col, Pixels x, Pixels y);
					virtual void OnRightClickItem(int id, int col);
					virtual void OnContextMenu(Pixels x, Pixels y);
					virtual void OnCopy();
					virtual void OnPaste();
					virtual void OnCut();

					virtual void DoBindDialog(strong<Variable> var);
					
					enum {
						KColName = 1,
						KColValue,
						KColInitial,
						KColType,
						KColDescription,
					};

					ref<Variables> _vars;
			};

			class VariableToolbarWnd: public ToolbarWnd {
				public:
					VariableToolbarWnd(VariableWnd* vw);
					virtual ~VariableToolbarWnd();

				protected:
					virtual void OnCommand(ref<ToolbarItem> ti);
					virtual void OnCommand(int c);

					enum {
						// commands
						KCAdd=1,
						KCRemove,
						KCReset,
						KCChange,
					};

					VariableWnd* _vw;
			};
		}
	}
}

VariableWnd::VariableWnd(ref<Variables> vars): ChildWnd(false), _variables(vars) {
	_toolbar = GC::Hold(new VariableToolbarWnd(this));
	_list = GC::Hold(new VariableListWnd(vars));
	Add(_list);
	Add(_toolbar);
}

VariableWnd::~VariableWnd() {
}

void VariableWnd::OnSettingsChanged() {
	ref<Settings> st = GetSettings();
	if(st) {
		if(_list) _list->SetSettings(st->GetNamespace(L"list"));
	}
}

void VariableWnd::Paint(graphics::Graphics& g, strong<Theme> theme) {
}

void VariableWnd::Layout() {
	Area rc = GetClientArea();
	if(_toolbar && _list) {
		_toolbar->Fill(LayoutTop, rc);
		_list->Fill(LayoutFill,rc);
	}
}

void VariableWnd::Update() {
	if(_list) {
		_list->Update();
		_list->Repaint();
	}
}

void VariableWnd::OnSize(const Area& ns) {
	Layout();
}

/* VariableListWnd */
VariableListWnd::VariableListWnd(ref<Variables> vars): _vars(vars) {
	AddColumn(TL(variable_name), KColName, 0.3f);
	AddColumn(TL(variable_value), KColValue, 0.2f);
	AddColumn(TL(variable_initial_value), KColInitial, 0.2f);
	AddColumn(TL(variable_type), KColType, 0.2f);
	AddColumn(TL(variable_description), KColDescription, 0.3f);
}

VariableListWnd::~VariableListWnd() {
}

int VariableListWnd::GetItemCount() {
	return _vars->GetVariableCount();
}

void VariableListWnd::PaintItem(int id, tj::shared::graphics::Graphics& g, Area& row, const ColumnInfo& ci) {
	ref<Variable> var = _vars->GetVariableByIndex(id);
	if(var) {
		strong<Theme> theme = ThemeManager::GetTheme();
		StringFormat sf;
		SolidBrush tbr(theme->GetColor(Theme::ColorText));

		DrawCellText(g, &sf, &tbr, theme->GetGUIFontBold(), KColName, row, var->GetName());
		DrawCellText(g, &sf, &tbr, theme->GetGUIFont(), KColInitial, row, var->GetInitialValue().ToString());
		DrawCellText(g, &sf, &tbr, theme->GetGUIFontBold(), KColValue, row, var->GetValue().ToString());
		DrawCellText(g, &sf, &tbr, theme->GetGUIFont(), KColDescription, row, var->GetDescription());
		DrawCellText(g, &sf, &tbr, theme->GetGUIFont(), KColType, row, Any::GetTypeName(var->GetType()));

		// Draw in/out icons
		if((var->IsInput()||var->IsOutput()) && IsColumnVisible(KColName)) {
			Pixels ncw = Pixels(GetColumnWidth(KColName) * row.GetWidth());
			if(ncw > 48) {
				if(var->IsOutput()) {
					ncw -= 26;
					Area iconArea(ncw+2, row.GetTop()+2, 26-4, row.GetHeight()-6);
					DrawTag(g, iconArea, TL(variable_tag_output), theme);
				}
				
				if(var->IsInput()) {
					ncw -= 18;
					Area iconArea(ncw+2, row.GetTop()+2, 18-4, row.GetHeight()-6);
					DrawTag(g, iconArea, TL(variable_tag_input), theme);
				}	
			}
		}
	}
}

void VariableListWnd::OnClickItem(int id, int col, Pixels x, Pixels y) {
	if(id>=0 && id<int(_vars->GetVariableCount())) {
		ref<Variable> var = _vars->GetVariableByIndex(id);
		Application::Instance()->GetView()->Inspect(var);
		Focus();
	}
	SetSelectedRow(id);
}

void VariableListWnd::OnRightClickItem(int id, int col) {
	Area row = GetRowArea(id);
	SetSelectedRow(id);
	Pixels x = Pixels(GetColumnX(col)*row.GetWidth());
	OnContextMenu(x,row.GetBottom());
}

void VariableListWnd::DoBindDialog(strong<Variable> var) {
	// Find rules table
	ref<Model> model = Application::Instance()->GetModel();
	if(!model) {
		return;
	}
	ref<input::Rules> rules = model->GetInputRules();
	if(!rules) {
		return;
	}

	// find out my tlid and cid, create our endpoint ID and then call model's input rules dialog
	EndpointID eid = var->GetID();
	ref<input::Rule> rule = rules->DoBindDialog(this, VariableEndpointCategory::KVariableEndpointCategoryID, eid);
	if(rule) {
		rules->AddRule(rule);
	}
}

void VariableListWnd::OnContextMenu(Pixels x, Pixels y) {
	int r = GetSelectedRow();
	ContextMenu cm;
	enum { KCProperties = 1, KCCopy, KCCut, KCPaste, KCBind  };
	ref<Variable> cap = _vars->GetVariableByIndex(r);

	if(cap) {
		cm.AddItem(TL(properties), KCProperties, true, false);

		// Only global variables can be bound to input
		if(_vars == ref<Variables>(Application::Instance()->GetModel()->GetVariables())) {
			cm.AddItem(GC::Hold(new MenuItem(TL(variable_bind_to_input), KCBind, false, MenuItem::NotChecked, L"icons/input.png")));
		}

		cm.AddItem(GC::Hold(new MenuItem(TL(copy), KCCopy, false, MenuItem::NotChecked, Icons::GetIconPath(Icons::IconCopy), L"Ctrl-C")));
		cm.AddItem(GC::Hold(new MenuItem(TL(cut), KCCut, false, MenuItem::NotChecked, Icons::GetIconPath(Icons::IconCut), L"Ctrl-X")));
	}

	bool canPaste = Clipboard::IsObjectAvailable();
	cm.AddItem(GC::Hold(new MenuItem(TL(paste), canPaste ? KCPaste : -1, false, MenuItem::NotChecked, Icons::GetIconPath(Icons::IconPaste), L"Ctrl-V")));

	int res = cm.DoContextMenu(this, x, y);
	if(res==KCProperties) {
		Application::Instance()->GetView()->Inspect(cap);
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
	else if(res==KCBind) {
		DoBindDialog(cap);
	}
}

void VariableListWnd::OnCopy() {
	int r = GetSelectedRow();
	ref<Variable> var = _vars->GetVariableByIndex(r);
	if(var) {
		Clipboard::SetClipboardObject(var);
	}
}

void VariableListWnd::OnPaste() {
	ref<Variable> var = GC::Hold(new Variable());
	if(Clipboard::GetClipboardObject(var)) {
		var->Clone();
		_vars->Add(var);
	}
	Update();
	Repaint();
}

void VariableListWnd::OnCut() {
	int r = GetSelectedRow();
	ref<Variable> var = _vars->GetVariableByIndex(r);
	if(var) {
		Clipboard::SetClipboardObject(var);
		_vars->Remove(var);
	}
	Update();
	Repaint();
}

/* VariableToolbarWnd */
VariableToolbarWnd::VariableToolbarWnd(VariableWnd* vw): ToolbarWnd() {
	_vw = vw;
	Add(GC::Hold(new ToolbarItem(KCAdd, L"icons/add.png", TL(variable_add), false)));
	Add(GC::Hold(new ToolbarItem(KCRemove, L"icons/remove.png", TL(variable_remove), true)));
	Add(GC::Hold(new ToolbarItem(KCReset,  L"icons/reset.png", TL(variable_reset), false)));
	Add(GC::Hold(new ToolbarItem(KCChange, L"icons/write.png", TL(variable_change), false)));
}

VariableToolbarWnd::~VariableToolbarWnd() {
}

void VariableToolbarWnd::OnCommand(ref<ToolbarItem> ti) {
	OnCommand(ti->GetCommand());
}

class VariableAddRemoveChange: public Change {
	public:
		VariableAddRemoveChange(strong<Variables> vs, strong<Variable> var, bool add): Change(add ? TL(change_add_variable) : TL(change_remove_variable)), _add(add), _vs(vs), _var(var) {}
		virtual ~VariableAddRemoveChange() {}
		virtual bool CanUndo() { return true; }
		virtual bool CanRedo() { return true; }
		virtual void Redo() { Do(true);	}
		virtual void Undo() { Do(false); }

		void Do(bool redo) {
			if(!_add) redo = !redo;

			if(redo) {
				_vs->Add(_var);
			}
			else {
				_vs->Remove(_var);
			}
		}

	protected:
		strong<Variables> _vs;
		strong<Variable> _var;
		bool _add;
};

void VariableToolbarWnd::OnCommand(int c) {
	if(c==KCAdd) {
		ref<Variable> v = GC::Hold(new Variable());
		v->SetName(TL(variable_untitled));
		Application::Instance()->GetView()->Inspect(v);
		UndoBlock::AddAndDoChange(GC::Hold(new VariableAddRemoveChange(_vw->_variables, v, true)));
		_vw->_list->Repaint();
	}
	else if(c==KCRemove) {
		ref<Variable> var = _vw->_variables->GetVariableByIndex(_vw->_list->GetSelectedRow());
		if(var) {
			UndoBlock::AddAndDoChange(GC::Hold(new VariableAddRemoveChange(_vw->_variables, var, false)));
		}
	}
	else if(c==KCReset) {
		ref<Variable> var = _vw->_variables->GetVariableByIndex(_vw->_list->GetSelectedRow());
		if(var) {
			_vw->_variables->Reset(var);
		}
	}
	else if(c==KCChange) {
		class WriteDialogData: public Inspectable {
			public:
				WriteDialogData() {}
				virtual ~WriteDialogData() {}
				virtual ref<PropertySet> GetProperties() {
					ref<PropertySet> ps = GC::Hold(new PropertySet());
					ps->Add(GC::Hold(new GenericProperty<std::wstring>(TL(variable_value), this, &_value, _value)));
					return ps;
				}

				std::wstring _value;
		};

		ref<Variable> var = _vw->_variables->GetVariableByIndex(_vw->_list->GetSelectedRow());
		if(var) {
			ref<WriteDialogData> wdd = GC::Hold(new WriteDialogData());
			wdd->_value = var->GetValue().ToString();

			ref<PropertyDialogWnd> dw = GC::Hold(new PropertyDialogWnd(TL(variable_change), TL(variable_change_question)));
			dw->GetPropertyGrid()->Inspect(wdd);
			dw->SetSize(200,130);
			if(dw->DoModal(this)) {
				_vw->_variables->Set(var, Any(var->GetType(), wdd->_value));
			}
		}
	}
	_vw->_list->Repaint();
}