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
#include "../include/internal/tjdashboard.h"
using namespace tj::show;


/** Widget **/
Widget::Widget(): _wantsRepaint(true), _layoutSizing(LayoutResizeRetainAspectRatio), _layoutAnchor(LayoutAnchorNone), _designMode(false), _labelPosition(LabelPositionDefault) {
}

Widget::~Widget() {
}

ref<PropertySet> Widget::GetProperties() {
	ref<PropertySet> ps = GC::Hold(new PropertySet());
	ps->Add(GC::Hold(new PropertySeparator(TL(dashboard_widget_label), _label.length()==0)));
	ps->Add(GC::Hold(new GenericProperty<std::wstring>(TL(dashboard_widget_label_text), this, &_label, L"")));
	
	ref<GenericListProperty<LabelPosition> > lpp = GC::Hold(new GenericListProperty<LabelPosition>(TL(dashboard_widget_label_position), this, &_labelPosition, LabelPositionDefault));
	lpp->AddOption(TL(dashboard_widget_label_position_bottom), LabelPositionBottom);
	lpp->AddOption(TL(dashboard_widget_label_position_right), LabelPositionRight);
	ps->Add(lpp);

	return ps;
}

void Widget::Move(Pixels x, Pixels y, Pixels w, Pixels h) {
	Element::Move(x,y,w,h);
	Repaint();
}

void Widget::OnPropertyChanged(void* member) {
	Repaint();
}

LabelPosition Widget::GetLabelPosition() const {
	return _labelPosition;
}

void Widget::SetLabelPosition(LabelPosition lps) {
	_labelPosition = lps;
	// TODO queue repaint
}

bool Widget::IsInDesignMode() const {
	return _designMode;
}

void Widget::SetDesignMode(bool dm) {
	_designMode = true;
}

Area Widget::GetPreferredSize() const {
	return Area(0,0,_pw,_ph);
}

Flags<LayoutSizing>& Widget::GetLayoutSizing() {
	return _layoutSizing;
}

Flags<LayoutAnchor>& Widget::GetLayoutAnchor() {
	return _layoutAnchor;
}

void Widget::SetPreferredSize(const Area& ps) {
	_pw = ps.GetWidth();
	_ph = ps.GetHeight();
}

void Widget::SetLayoutSizing(const LayoutSizing& ls) {
	_layoutSizing = ls;
}

void Widget::SetLayoutAnchor(const LayoutAnchor& la) {
	_layoutAnchor = la;
}

void Widget::SaveWidget(TiXmlElement* el) {
	TiXmlElement widget("contents");
	Save(&widget);
	el->InsertEndChild(widget);

	Area wrc = GetClientArea();
	SaveAttributeSmall(el, "x", wrc.GetLeft());
	SaveAttributeSmall(el, "y", wrc.GetTop());
	SaveAttributeSmall(el, "width", wrc.GetWidth());
	SaveAttributeSmall(el, "height", wrc.GetHeight());
}

void Widget::Repaint() {
	_wantsRepaint = true;
}

void Widget::LoadWidget(TiXmlElement* el) {
	TiXmlElement* contents = el->FirstChildElement("contents");
	if(contents!=0) {
		Load(contents);
	}

	Area wrc = GetClientArea();
	Move(LoadAttributeSmall(el, "x", wrc.GetLeft()), LoadAttributeSmall(el, "y", wrc.GetTop()), LoadAttributeSmall(el, "width", wrc.GetWidth()), LoadAttributeSmall(el, "height", wrc.GetHeight()));
}

bool Widget::CheckAndClearRepaintFlag() {
	if(_wantsRepaint) {
		_wantsRepaint = false;
		return true;
	}
	return false;
}

const std::wstring& Widget::GetLabel() const {
	return _label;
}

/** WidgetFactory **/
void WidgetFactory::SaveWidget(ref<Widget> wt, TiXmlElement* ti) {
	if(wt) {
		WidgetType widgetType = GetTypeOfObject(wt);
		SaveAttributeSmall(ti, "type", widgetType);
		wt->SaveWidget(ti);
	}
}

ref<Widget> WidgetFactory::LoadWidget(TiXmlElement* el) {
	WidgetType widgetType = LoadAttributeSmall<WidgetType>(el, "type", L"");
	ref<Widget> widget = CreateObjectOfType(widgetType);
	if(widget) {
		widget->LoadWidget(el);
		widget->Show(true);
	}
	else {
		Log::Write(L"TJShow/Dashboard", L"Could not create widget of type '"+widgetType+L"'; not loading widget");
	}
	return widget;
}

/** Dashboard **/
Dashboard::Dashboard() {
	Clone();
}

Dashboard::~Dashboard() {
}

void Dashboard::Clear() {
	RemoveAllWidgets();
	Clone();
}

void Dashboard::Clone() {
	_id = Util::RandomIdentifier(L'D');
	// TODO clone all widgets (if any)? implement as soon as you can copy/paste whole dashboards...
}

void Dashboard::Load(TiXmlElement* el) {
	_id = LoadAttributeSmall<std::wstring>(el, "id", _id);
	TiXmlElement* widget = el->FirstChildElement("widget");
	while(widget!=0) {
		ref<Widget> wd = WidgetFactory::Instance()->LoadWidget(widget);
		if(wd) {
			AddWidget(wd);
		}
		widget = widget->NextSiblingElement("widget");
	}
}

void Dashboard::Save(TiXmlElement* el) {
	SaveAttributeSmall(el, "id", _id);
	std::deque< ref<Widget> >::reverse_iterator it = _widgets.rbegin();
	while(it!=_widgets.rend()) {
		ref<Widget> widget = *it;
		if(widget) {
			TiXmlElement widgetElement("widget");
			WidgetFactory::Instance()->SaveWidget(widget, &widgetElement);
			el->InsertEndChild(widgetElement);
		}
		++it;
	}
}

void Dashboard::RemoveAllWidgets() {
	_widgets.clear();
}

void Dashboard::AddWidget(ref<Widget> wi) {
	_widgets.push_back(wi);
}

ref<Widget> Dashboard::GetWidgetAt(Pixels x, Pixels y) {
	std::deque< ref<Widget> >::iterator it = _widgets.begin();
	while(it!=_widgets.end()) {
		ref<Widget> widget = *it;
		if(widget) {
			if(widget->IsShown() && widget->GetClientArea().IsInside(x,y)) {
				return widget;
			}
		}
		++it;
	}
	return null;
}

void Dashboard::RemoveWidget(ref<Widget> wi) {
	std::deque< ref<Widget> >::iterator it = _widgets.begin();
	while(it!=_widgets.end()) {
		ref<Widget> w = *it;
		if(w==wi) {
			it = _widgets.erase(it);
		}
		else {
			++it;
		}
	}
}