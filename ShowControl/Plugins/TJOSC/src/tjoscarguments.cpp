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
#include <limits>
#include "../include/tjoscarguments.h"
#include <OSCPack/OscOutboundPacketStream.h>

using namespace tj::show;
using namespace tj::shared;
using namespace tj::osc;
namespace oscp = osc;

namespace tj {
	namespace osc {
		namespace intern {
			class OSCArgumentListWnd: public EditableListWnd, public virtual Inspectable {
				public:
					OSCArgumentListWnd(strong<OSCArgumentList> al, ref<Playback> pb);
					virtual ~OSCArgumentListWnd();
					virtual int GetItemCount();
					virtual void PaintItem(int id, graphics::Graphics& g, Area& row, const ColumnInfo& ci);
					virtual void OnClickItem(int id, int col, Pixels x, Pixels y);
					virtual void Update();

				protected:
					virtual ref<Property> GetPropertyForItem(int id, int col);

					enum {
						KColType = 1,
						KColValue,
						KColDescription,
					};

					strong<OSCArgumentList> _args;
					ref<Playback> _pb;
			};

			class OSCArgumentToolbarWnd: public ToolbarWnd {
				public:
					OSCArgumentToolbarWnd(ref<OSCArgumentListWnd> aw, strong<OSCArgumentList> al);
					virtual ~OSCArgumentToolbarWnd();
					virtual void OnCommand(ref<ToolbarItem> ti);

				private:
					enum {
						KCAdd = 1,
						KCRemove,
						KCUp,
						KCDown,
					};
					strong<OSCArgumentList> _args;
					weak<OSCArgumentListWnd> _aw;
			};

			class OSCArgumentPopupWnd: public PopupWnd {
				public:
					OSCArgumentPopupWnd(strong<OSCArgumentList> al, ref<Playback> pb);
					virtual ~OSCArgumentPopupWnd();
					virtual void Layout();
					virtual void OnSettingsChanged();
					virtual void Paint(graphics::Graphics& g, strong<Theme> theme);
					virtual void OnSize(const Area& ns);

				protected:
					ref<OSCArgumentListWnd> _list;
					ref<OSCArgumentToolbarWnd> _tools;
			};

			class OSCArgumentWnd: public ChildWnd {
				public:
					OSCArgumentWnd(strong<OSCArgumentList> al, ref<Playback> pb);
					virtual ~OSCArgumentWnd();
					
					virtual void OnSize(const Area& ns);
					virtual void OnSettingsChanged();
					virtual void Paint(graphics::Graphics& g, strong<Theme> theme);
					virtual void OnMouse(MouseEvent ev, Pixels x, Pixels y);

				protected:
					void DoPopup();

					Icon _icon;
					strong<OSCArgumentPopupWnd> _popup;
					strong<OSCArgumentList> _args;
			};
		}
	}
}

OSCArgument::OSCArgument(): _type(oscp::NIL_TYPE_TAG) {
}

std::wstring OSCArgument::GetTypeName(const oscp::TypeTagValues& tv) {
	switch(tv) {
		case oscp::TRUE_TYPE_TAG:
			return TL(osc_argument_type_true);

		case oscp::FALSE_TYPE_TAG:
			return TL(osc_argument_type_false);

		case oscp::NIL_TYPE_TAG:
			return TL(osc_argument_type_nil);

		case oscp::INFINITUM_TYPE_TAG:
			return TL(osc_argument_type_infinitum);

		case oscp::INT32_TYPE_TAG:
			return TL(osc_argument_type_int32);

		case oscp::FLOAT_TYPE_TAG:
			return TL(osc_argument_type_float);

		case oscp::CHAR_TYPE_TAG:
				return TL(osc_argument_type_char);

		case oscp::RGBA_COLOR_TYPE_TAG:
			return TL(osc_argument_type_color);

		case oscp::MIDI_MESSAGE_TYPE_TAG:
			return TL(osc_argument_type_midi);

		case oscp::INT64_TYPE_TAG:
			return TL(osc_argument_type_int64);

		case oscp::TIME_TAG_TYPE_TAG:
			return TL(osc_argument_type_time);

		case oscp::DOUBLE_TYPE_TAG:
			return TL(osc_argument_type_double);

		case oscp::STRING_TYPE_TAG:
			return TL(osc_argument_type_string);

		case oscp::SYMBOL_TYPE_TAG:
			return TL(osc_argument_type_symbol);

		case oscp::BLOB_TYPE_TAG:
			return TL(osc_argument_type_blob);

		default:
			return TL(osc_argument_type_unknown);
	}
}

using namespace tj::osc::intern;
using namespace tj::shared::graphics;

/** OSCArgumentToolbarWnd **/
OSCArgumentToolbarWnd::OSCArgumentToolbarWnd(ref<OSCArgumentListWnd> aw, strong<OSCArgumentList> al): _args(al), _aw(aw) {
	Add(GC::Hold(new ToolbarItem(KCAdd, L"icons/add.png", TL(osc_argument_add), false)));
	Add(GC::Hold(new ToolbarItem(KCRemove, L"icons/remove.png", TL(osc_argument_remove), false)));
	Add(GC::Hold(new ToolbarItem(KCUp, L"icons/up.png", TL(osc_argument_up), false)));
	Add(GC::Hold(new ToolbarItem(KCDown, L"icons/down.png", TL(osc_argument_down), false)));
}

OSCArgumentToolbarWnd::~OSCArgumentToolbarWnd() {
}

void OSCArgumentToolbarWnd::OnCommand(ref<ToolbarItem> ti) {
	if(ti) {
		ref<OSCArgumentListWnd> aw = _aw;

		if(aw) {
			aw->EndEditing();
			int cmd = ti->GetCommand();
			if(cmd==KCAdd) {
				OSCArgument oa;
				_args->_arguments.push_back(oa);	
			}
			else if(cmd==KCRemove) {
				int cur = aw->GetSelectedRow();
				std::vector<OSCArgument>::iterator it = _args->_arguments.begin() + cur;
				if(it!=_args->_arguments.end()) {
					_args->_arguments.erase(it);
				}
			}
			else if(cmd==KCUp) {
				int cur = aw->GetSelectedRow();
				std::vector<OSCArgument>::iterator it = _args->_arguments.begin() + cur;
				if(it!=_args->_arguments.end() && it!=_args->_arguments.begin()) {
					std::vector<OSCArgument>::iterator prev = it-1;
					if(prev!=_args->_arguments.end()) {
						OSCArgument temp = *prev;
						*prev = *it;
						*it = temp;
					}
				}
			}
			else if(cmd==KCDown) {
				int cur = aw->GetSelectedRow();
				std::vector<OSCArgument>::iterator it = _args->_arguments.begin() + cur;
				if(it!=_args->_arguments.end() && it!=_args->_arguments.begin()) {
					std::vector<OSCArgument>::iterator prev = it+1;
					if(prev!=_args->_arguments.end()) {
						OSCArgument temp = *prev;
						*prev = *it;
						*it = temp;
					}
				}
			}
			aw->Update();
		}
	}
}

/** OSCArgumentListWnd **/
OSCArgumentListWnd::OSCArgumentListWnd(strong<OSCArgumentList> al, ref<Playback> pb): _args(al), _pb(pb) {
	AddColumn(TL(osc_argument_type), KColType, 0.2f);
	AddColumn(TL(osc_argument_value), KColValue, 0.4f);
	AddColumn(TL(osc_argument_description), KColDescription, 0.6f);
}

OSCArgumentListWnd::~OSCArgumentListWnd() {
}

int OSCArgumentListWnd::GetItemCount() {
	return (int)_args->_arguments.size();
}

void OSCArgumentListWnd::PaintItem(int id, graphics::Graphics& g, Area& row, const ColumnInfo& ci) {
	const OSCArgument& oa = _args->_arguments.at(id);
	strong<Theme> theme = ThemeManager::GetTheme();
	StringFormat sf;
	sf.SetTrimming(StringTrimmingEllipsisPath);
	sf.SetFormatFlags(StringFormatFlagsLineLimit);
	SolidBrush br(theme->GetColor(Theme::ColorText));

	DrawCellText(g, &sf, &br, theme->GetGUIFont(), KColType, row, OSCArgument::GetTypeName(oa._type));
	DrawCellText(g, &sf, &br, theme->GetGUIFont(), KColValue, row, oa._value);
	DrawCellText(g, &sf, &br, theme->GetGUIFont(), KColDescription, row, oa._description);
}

void OSCArgumentListWnd::Update() {
	EditableListWnd::OnSize(GetClientArea());
	EditableListWnd::Update();
	Layout();
	Repaint();
}

void OSCArgumentListWnd::OnClickItem(int id, int col, Pixels x, Pixels y) {
	SetSelectedRowAndEdit(id);
}

ref<Property> OSCArgumentListWnd::GetPropertyForItem(int id, int col) {
	if(id<int(_args->_arguments.size())) {
		OSCArgument& oa = _args->_arguments.at(id);
		if(col==KColType) {
			ref<GenericListProperty<oscp::TypeTagValues> > ls = GC::Hold(new GenericListProperty<oscp::TypeTagValues>(L"", this, &(oa._type), oa._type));
			ls->AddOption(TL(osc_argument_type_string), oscp::STRING_TYPE_TAG);
			ls->AddOption(TL(osc_argument_type_int32), oscp::INT32_TYPE_TAG);
			ls->AddOption(TL(osc_argument_type_int64), oscp::INT64_TYPE_TAG);
			ls->AddOption(TL(osc_argument_type_float), oscp::FLOAT_TYPE_TAG);
			ls->AddOption(TL(osc_argument_type_nil), oscp::NIL_TYPE_TAG);
			ls->AddOption(TL(osc_argument_type_true), oscp::TRUE_TYPE_TAG);
			ls->AddOption(TL(osc_argument_type_false), oscp::FALSE_TYPE_TAG);
			ls->AddOption(TL(osc_argument_type_color), oscp::RGBA_COLOR_TYPE_TAG);
			return ls;
		}
		else if(col==KColValue) {
			if(_pb) {
				return _pb->CreateParsedVariableProperty(L"", this, &(oa._value));
			}
			else {
				return GC::Hold(new GenericProperty<std::wstring>(L"", this, &(oa._value), oa._value));
			}
		}
		else if(col==KColDescription) {
			return GC::Hold(new GenericProperty<std::wstring>(L"", this, &(oa._description), oa._description));
		}
	}
	return null;
}

/** OSCArgumentPopupWnd **/
OSCArgumentPopupWnd::OSCArgumentPopupWnd(strong<OSCArgumentList> al, ref<Playback> pb) {
	_list = GC::Hold(new OSCArgumentListWnd(al, pb));
	_tools = GC::Hold(new OSCArgumentToolbarWnd(_list, al));
	Add(_list);
	Add(_tools);
}

OSCArgumentPopupWnd::~OSCArgumentPopupWnd() {
}

void OSCArgumentPopupWnd::OnSettingsChanged() {
	ref<Settings> st = GetSettings();
	if(st) {
		_list->SetSettings(st->GetNamespace(L"list"));
		_tools->SetSettings(st->GetNamespace(L"tools"));
	}
}

void OSCArgumentPopupWnd::Layout() {
	Area rc = GetClientArea();
	rc.Narrow(1,1,1,1);
	_tools->Fill(LayoutTop, rc);
	_list->Fill(LayoutFill, rc);
}

void OSCArgumentPopupWnd::OnSize(const Area& ns) {
	Layout();
}

void OSCArgumentPopupWnd::Paint(graphics::Graphics& g, strong<Theme> theme) {
	SolidBrush back(theme->GetColor(Theme::ColorBackground));
	Area rc = GetClientArea();
	g.FillRectangle(&back,rc);

	rc.Narrow(0,0,1,1);
	SolidBrush border(theme->GetColor(Theme::ColorActiveStart));
	Pen pn(&border, 1.0f);
	g.DrawRectangle(&pn, rc);
}

/** OSCArgumentWnd **/
OSCArgumentWnd::OSCArgumentWnd(strong<OSCArgumentList> al, ref<Playback> pb): ChildWnd(true), _args(al), _popup(GC::Hold(new OSCArgumentPopupWnd(al, pb))), _icon(L"icons/osc/arguments.png") {
	SetWantMouseLeave(true);
}

OSCArgumentWnd::~OSCArgumentWnd() {
}

void OSCArgumentWnd::DoPopup() {
	Area rc = GetClientArea();
	_popup->SetSize(300,200);
	_popup->PopupAt(rc.GetLeft(), rc.GetBottom(), this);
	ModalLoop ml;
	ml.Enter(_popup->GetWindow(), false);
	_popup->Show(false);
}

void OSCArgumentWnd::OnSettingsChanged() {
	ref<Settings> st = GetSettings();
	if(st) {
		_popup->SetSettings(st->GetNamespace(L"popup"));
	}
}

void OSCArgumentWnd::OnMouse(MouseEvent ev, Pixels x, Pixels y) {
	if(ev==MouseEventLUp) {
		DoPopup();
	}
	else if(ev==MouseEventMove||ev==MouseEventLeave) {
		Repaint();
	}
}

void OSCArgumentWnd::Paint(graphics::Graphics& g, strong<Theme> theme) {
	Area rc = GetClientArea();
	SolidBrush back(theme->GetColor(Theme::ColorBackground));
	g.FillRectangle(&back, rc);

	if(IsMouseOver()) {
		theme->DrawToolbarBackground(g, 0.0f, 0.0f, float(rc.GetWidth()), float(rc.GetHeight()));
	}

	_icon.Paint(g, Area(0,0,16,16));

	std::wostringstream infos;
	if(_args->_arguments.size()>0) {
		std::vector<OSCArgument>::const_iterator it = _args->_arguments.begin();
		while(it!=_args->_arguments.end()) {
			const OSCArgument& oa = *it;
			infos << oa._value;
			++it;
			if(it!=_args->_arguments.end()) {
				infos << L", ";
			}
		}
	}
	else {
		infos << TL(osc_no_arguments);
	}

	std::wstring info = infos.str();
	SolidBrush tbr(theme->GetColor(Theme::ColorActiveStart));
	g.DrawString(info.c_str(), (int)info.length(), theme->GetGUIFont(), PointF(20.0f, 2.0f), &tbr);
}

void OSCArgumentWnd::OnSize(const Area& ns) {
	Repaint();
}

/** OSCArgumentProperty **/
OSCArgumentProperty::OSCArgumentProperty(const std::wstring& name, strong<OSCArgumentList> al, ref<Playback> pb): Property(name), _args(al), _pb(pb) {
}

OSCArgumentProperty::~OSCArgumentProperty() {
}

ref<Wnd> OSCArgumentProperty::GetWindow() {
	if(!_wnd) {
		_wnd = GC::Hold(new OSCArgumentWnd(_args, _pb));
	}
	return _wnd;
}

void OSCArgumentProperty::Update() {
	if(_wnd) {
		_wnd->Update();
	}
}

/** OSCArgumentList **/
OSCArgumentList::OSCArgumentList() {
}

OSCArgumentList::~OSCArgumentList() {
}