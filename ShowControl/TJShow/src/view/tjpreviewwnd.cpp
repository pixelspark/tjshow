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
#include "../../include/internal/view/tjpreviewwnd.h"

using namespace tj::shared;
using namespace tj::shared::graphics;
using namespace tj::show;
using namespace tj::show::view;

namespace tj {
	namespace show {
		namespace view {
			class PreviewToolbarWnd: public ToolbarWnd {
				public:
					PreviewToolbarWnd(ref<PreviewWnd> pw): _pw(pw) {
						Add(GC::Hold(new ToolbarItem(KCProperties, L"icons/toolbar/properties.png", TL(preview_properties), false)));
					}

					virtual void OnCommand(ref<ToolbarItem> item) {
						if(item->GetCommand()==KCProperties) {
							ref<PreviewWnd> pw = _pw;
							if(pw) {
								ref<PropertyDialogWnd> pdw = GC::Hold(new PropertyDialogWnd(TL(preview_properties), TL(preview_properties_question)));
								pdw->SetSize(400,300);
								pdw->GetPropertyGrid()->Inspect(pw->_pw->GetDefinition());

								if(pdw->DoModal(this)) {
									// Nothing
								}
							}
						}
					}

					enum {
						KCProperties = 1,
					};
					weak<PreviewWnd> _pw;
			};
		}
	}
}

PreviewWnd::PreviewWnd(strong<PlayerWnd> pw): ChildWnd(false) {
	_pw = pw;
	Add(_pw);
}

PreviewWnd::PreviewWnd(ref<OutputManager> mgr, strong<ScreenDefinition> def): ChildWnd(false) {
	_pw = GC::Hold(new PlayerWnd(mgr,def));
	Add(_pw);
}

void PreviewWnd::OnCreated() {
	ChildWnd::OnCreated();
	_tools = GC::Hold(new PreviewToolbarWnd(this));
	Add(_tools);
	Layout();
}

PreviewWnd::~PreviewWnd() {
}

void PreviewWnd::OnSettingsChanged() {
	ref<Settings> st = GetSettings();
	if(st) {
		_pw->SetSettings(st);
	}
}

void PreviewWnd::SetDefinition(strong<ScreenDefinition> sd) {
	if(_pw) {
		_pw->SetDefinition(sd);
	}
}

void PreviewWnd::Layout() {
	Area rc = GetClientArea();
	if(_tools) {
		_tools->Fill(LayoutTop, rc);
	}
	if(_pw) {
		_pw->Fill(LayoutFill, rc);
	}
}

void PreviewWnd::OnSize(const Area& ns) {
	Layout();
}

void PreviewWnd::Paint(graphics::Graphics& g, strong<Theme> theme) {
}