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
#include "../../include/internal/view/tjfilterproperty.h"

using namespace tj::show;
using namespace tj::shared::graphics;
using namespace tj::show::view;

namespace tj {
	namespace show {
		namespace view {
			class FilterPropertyWnd: public ChildWnd {
				public:
					FilterPropertyWnd(strong<Filter> filt);
					virtual ~FilterPropertyWnd();
					virtual void Paint(graphics::Graphics& g, strong<Theme> theme);
					virtual void OnSize(const Area& ns);
					virtual void Layout();
					virtual void OnMouse(MouseEvent ev, Pixels x, Pixels y);

				private:
					strong<Filter> _filter;
					Icon _downArrow;
			};
		}
	}
}

/** FilterProperty **/
FilterProperty::FilterProperty(const std::wstring& name, strong<Filter> filter): Property(name, false), _filter(filter) {
}

FilterProperty::~FilterProperty() {
}

ref<Wnd> FilterProperty::GetWindow() {
	if(!_fpw) {
		_fpw = GC::Hold(new FilterPropertyWnd(_filter));
	}
	return _fpw;
}

void FilterProperty::Update() {
}

/** FilterPropertyWnd **/
FilterPropertyWnd::FilterPropertyWnd(strong<Filter> filt): ChildWnd(true), _filter(filt), _downArrow(Icons::GetIconPath(Icons::IconDownArrow)) {
	SetWantMouseLeave(true);
}

FilterPropertyWnd::~FilterPropertyWnd() {
}

void FilterPropertyWnd::OnMouse(MouseEvent ev, Pixels x, Pixels y) {
	if(ev==MouseEventLDown) {
		ContextMenu cm;
		strong<Groups> groups = Application::Instance()->GetModel()->GetGroups();

		if(groups->GetGroupCount()<1) {
			cm.AddItem(TL(groups_none_defined), -1);
			Area rc = GetClientArea();
			cm.DoContextMenu(this, rc.GetLeft(), rc.GetBottom());
		}
		else {
			for(unsigned int a = 0; a < groups->GetGroupCount(); a++) {
				strong<Group> group = groups->GetGroupByIndex(a);
				cm.AddItem(GC::Hold(new MenuItem(group->GetName(), a, false, _filter->IsMemberOf(group->GetID()) ? MenuItem::Checked : MenuItem::NotChecked)));
			}

			Area rc = GetClientArea();
			int r = cm.DoContextMenu(this, rc.GetLeft(), rc.GetBottom());

			if(r>=0 && r < int(groups->GetGroupCount())) {
				strong<Group> group = groups->GetGroupByIndex(r);
				if(_filter->IsMemberOf(group->GetID())) {
					_filter->RemoveGroupMembership(group->GetID());
				}
				else {
					_filter->AddGroupMembership(group->GetID());
				}
			}
			Repaint();
		}
	}
}

void FilterPropertyWnd::Paint(graphics::Graphics& g, strong<Theme> theme) {
	Area rc = GetClientArea();

	SolidBrush back(theme->GetColor(Theme::ColorBackground));
	g.FillRectangle(&back, rc);

	//g.DrawImage(_linkIcon, PointF(0.0f, 0.0f));
	std::wstring info = _filter->DumpFriendly(Application::Instance()->GetModel()->GetGroups());

	SolidBrush tbr(theme->GetColor(Theme::ColorActiveStart));
	Area text = rc;
	text.Narrow(2,2,20,2);
	StringFormat sf;
	sf.SetTrimming(StringTrimmingEllipsisCharacter);
	sf.SetAlignment(StringAlignmentNear);
	g.DrawString(info.c_str(), (int)info.length(), theme->GetGUIFont(), text, &sf, &tbr);

	Area downIconArea(rc.GetRight()-20, rc.GetTop(), 16, 16);
	_downArrow.Paint(g, downIconArea);

	SolidBrush border(theme->GetColor(Theme::ColorActiveStart));
	Area borderRC = rc;
	borderRC.Narrow(0,0,1,1);
	Pen borderPen(&border, 1.0f);
	g.DrawRectangle(&borderPen, borderRC);
}

void FilterPropertyWnd::OnSize(const Area& ns) {
	Layout();
	Repaint();
}

void FilterPropertyWnd::Layout() {
	Repaint();
}