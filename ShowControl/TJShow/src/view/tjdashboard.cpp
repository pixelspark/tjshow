#include "../../include/internal/tjshow.h"
#include "../../include/internal/view/tjdashboard.h"
using namespace tj::show;
using namespace tj::show::view;
using namespace tj::shared::graphics;

class WidgetAreaChange: public Change {
	public:
		WidgetAreaChange(ref<Widget> wg, const Area& old, const Area& ns): _widget(wg), _old(old), _ns(ns) {
		}

		virtual ~WidgetAreaChange() {
		}

		virtual bool CanUndo() {
			return ref<Widget>(_widget);
		}

		virtual bool CanRedo() {
			return CanUndo();
		}

		virtual void Redo() {
			ref<Widget> wg = _widget;
			if(wg) {
				wg->Move(_ns.GetLeft(), _ns.GetTop(), _ns.GetWidth(), _ns.GetHeight());
			}
		}

		virtual void Undo() {
			ref<Widget> wg = _widget;
			if(wg) {
				wg->Move(_old.GetLeft(), _old.GetTop(), _old.GetWidth(), _old.GetHeight());
			}
		}

	private:
		Area _old, _ns;
		weak<Widget> _widget;
};

/** DashboardWnd **/
DashboardWnd::DashboardWnd(): _designMode(false), _dragOffsetX(0), _dragOffsetY(0), _isDraggingSize(false), _grabberIcon(Icons::GetIconPath(Icons::IconGrabber)) {
}

DashboardWnd::~DashboardWnd() {
}

void DashboardWnd::OnCreated() {
	_toolbar = GC::Hold(new DashboardToolbarWnd(this));
	Add(_toolbar);
	Layout();
	StartTimer(Time(990), 1);
}

void DashboardWnd::OnTimer(unsigned int id) {
	if(IsShown()) {
		Repaint();
	}
}

void DashboardWnd::Paint(graphics::Graphics& g, strong<Theme> theme) {
	SolidBrush backBrush(theme->GetColor(Theme::ColorBackground));
	Area rc = GetClientArea();
	g.FillRectangle(&backBrush, rc);

	if(_designMode) {
		Area backRC = rc;
		if(_toolbar) backRC.Narrow(0, _toolbar->GetClientArea().GetHeight(), 0, 0);
		theme->DrawInsetRectangle(g, backRC);

		if(_widgets.size()==0) {
			Area rect = rc;
			StringFormat sf;
			sf.SetAlignment(StringAlignmentCenter);
			Pixels headHeight = theme->GetMeasureInPixels(Theme::MeasureListHeaderHeight);
			rect.Narrow(0, Pixels(headHeight*1.5f), 0, 0);
			std::wstring text = TL(dashboard_empty);
			SolidBrush descBrush(theme->GetColor(Theme::ColorDescriptionText));
			g.DrawString(text.c_str(), (INT)text.length(), theme->GetGUIFont(), rect, &sf, &descBrush);
		}
	}

	// Draw widgets
	std::deque< ref<Widget> >::iterator it = _widgets.begin();
	while(it!=_widgets.end()) {
		ref<Widget> widget = *it;
		if(widget && widget->IsShown()) {
			Area wrc = widget->GetClientArea();
			widget->Paint(g, theme);

			if(widget==_focus) {
				theme->DrawFocusRectangle(g, wrc);

				// Draw grabber if we're in design mode
				if(IsInDesignMode()) {
					Area grc(wrc.GetRight()-KResizeAreaSize, wrc.GetBottom()-KResizeAreaSize, KResizeAreaSize, KResizeAreaSize);
					_grabberIcon.Paint(g, grc);
				}
			}

			// Draw widget label
			const std::wstring& widgetLabel = widget->GetLabel();
			if(widgetLabel.length()>0) {
				SolidBrush labelBrush(theme->GetColor(Theme::ColorActiveStart));
				StringFormat sf;
				LabelPosition labelPosition = widget->GetLabelPosition();
				
				if(labelPosition==LabelPositionBottom) {
					sf.SetAlignment(StringAlignmentCenter);
					Area labelRC(wrc.GetLeft(), wrc.GetBottom(), wrc.GetWidth(), 20);
					g.DrawString(widgetLabel.c_str(), (int)widgetLabel.length(), theme->GetGUIFontSmall(), labelRC, &sf, &labelBrush);
				}
				else if(labelPosition==LabelPositionRight) {
					sf.SetAlignment(StringAlignmentNear);
					Area labelRC(wrc.GetRight(), wrc.GetBottom()-wrc.GetHeight()/2, rc.GetWidth(), rc.GetHeight()/2);
					g.DrawString(widgetLabel.c_str(), (int)widgetLabel.length(), theme->GetGUIFontSmall(), labelRC, &sf, &labelBrush);
				}
			}
		}
		++it;
	}
}

void DashboardWnd::Layout() {
	Area rc = GetClientArea();
	if(_toolbar) {
		_toolbar->Fill(LayoutTop, rc, true);
	}
}

void DashboardWnd::OnSize(const Area& ns) {
	Layout();
	Repaint();
}

void DashboardWnd::Load(TiXmlElement* el) {
	TiXmlElement* widget = el->FirstChildElement("widget");
	while(widget!=0) {
		ref<Widget> wd = WidgetFactory::Instance()->LoadWidget(widget);
		if(wd) {
			AddWidget(wd);
		}
		widget = widget->NextSiblingElement("widget");
	}
}

void DashboardWnd::Save(TiXmlElement* el) {
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

void DashboardWnd::RemoveAllWidgets() {
	_focus = null;
	_dragging = null;
	_widgets.clear();
	Layout();
	Repaint();
}

void DashboardWnd::AddWidget(ref<Widget> wi) {
	_widgets.push_back(wi);

	// Initial layout
	wi->Show(true);
	Repaint();
}

ref<Widget> DashboardWnd::GetWidgetAt(Pixels x, Pixels y) {
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

void DashboardWnd::OnMouse(MouseEvent ev, Pixels x, Pixels y) {
	if(ev==MouseEventLDown) {
		Focus();
	}

	if(ev==MouseEventRDown) {
		OnContextMenu(x,y);
	}
	else if(IsInDesignMode()) {
		if(ev==MouseEventLDown) {
			_dragging = GetWidgetAt(x,y);
			if(_dragging) {
				_draggingOldArea = _dragging->GetClientArea();
			}

			_focus = _dragging;
			Application::Instance()->GetView()->Inspect(_dragging, null);

			if(_dragging) {
				// TODO: Put it at the top of the item list
				//std::remove(_widgets.begin(), _widgets.end(), _dragging);
				//_widgets.push_front(_dragging);

				Area wrc = _dragging->GetClientArea();
				_dragOffsetX = x - wrc.GetLeft();
				_dragOffsetY = y - wrc.GetTop();
				if(_dragOffsetX > (wrc.GetWidth()-KResizeAreaSize) && _dragOffsetY > (wrc.GetHeight()-KResizeAreaSize)) {
					_isDraggingSize = true;
					Mouse::Instance()->SetCursorType(CursorSizeNorthSouth);
				}
				else {
					_isDraggingSize = false;
					Mouse::Instance()->SetCursorType(CursorHandGrab);
				}
			}
		}
		else if(ev==MouseEventMove && _dragging) {
			Area wrc = _dragging->GetClientArea();
			if(_isDraggingSize) {
				Area preferredSize = _dragging->GetPreferredSize();

				// Check if the widget can be resized to any size or just the size with the preferred aspect ratio
				if(_dragging->GetLayoutSizing().IsSet(LayoutResizeRetainAspectRatio)) {
					float newH = float(y - wrc.GetTop());
					float newW  = newH * (float(preferredSize.GetWidth())/float(preferredSize.GetHeight()));
					_dragging->Move(wrc.GetLeft(), wrc.GetTop(), max(preferredSize.GetWidth(),(Pixels)newW), max(preferredSize.GetHeight(),(Pixels)newH));
				}
				else {
					_dragging->Move(wrc.GetLeft(), wrc.GetTop(), max(preferredSize.GetWidth(), x - wrc.GetLeft()), max(preferredSize.GetHeight(),y - wrc.GetTop()));
				}
			}
			else {
				_dragging->Move(x-_dragOffsetX, y-_dragOffsetY, wrc.GetWidth(), wrc.GetHeight());
			}
			Mouse::Instance()->SetCursorType(_isDraggingSize ? CursorSizeNorthSouth : CursorHandGrab);
			Layout();
			Repaint();
		}
		else if(ev==MouseEventMove && !_dragging) {
			// Show an open hand whenever you can drag something
			ref<Widget> wg = GetWidgetAt(x,y);
			if(wg) {
				Area wrc = wg->GetClientArea();
				if(x > (wrc.GetRight()-KResizeAreaSize) && y > (wrc.GetBottom()-KResizeAreaSize)) {
					Mouse::Instance()->SetCursorType(CursorSizeNorthSouth);
				}
				else {
					Mouse::Instance()->SetCursorType(CursorHand);
				}
			}
			else {
				Mouse::Instance()->SetCursorType(CursorDefault);
			}
		}
		else if(ev==MouseEventLUp) {
			if(_dragging) {
				Area newArea = _dragging->GetClientArea();
				if(newArea!=_draggingOldArea) {
					UndoBlock::AddChange(GC::Hold(new WidgetAreaChange(_dragging, _draggingOldArea, newArea)));
				}
				_dragging = null;
			}
			Mouse::Instance()->SetCursorType(CursorDefault);
		}
		else {
			ChildWnd::OnMouse(ev, x, y);
		}
	}
	else {
		ref<Widget> wi = GetWidgetAt(x,y);
		if(ev==MouseEventLDown) {
			_focus = wi;
			Repaint();
		}
			
		if(wi) {
			Area wrc = wi->GetClientArea();
			wi->OnMouse(ev, x - wrc.GetLeft(), y - wrc.GetTop());
			RepaintIfNecessary();
		}
		else {
			ChildWnd::OnMouse(ev, x,y);
		}
	}
}

void DashboardWnd::RemoveWidget(ref<Widget> wi) {
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

	if(_focus==wi) {
		_focus = null;
	}

	if(_dragging==wi) {
		_dragging = null;
	}
	Layout();
	Repaint();
}

void DashboardWnd::RepaintIfNecessary() {
	bool needsRepaint = false;

	std::deque< ref<Widget> >::iterator it = _widgets.begin();
	while(it!=_widgets.end()) {
		ref<Widget> w = *it;
		if(w && w->CheckAndClearRepaintFlag()) {
			needsRepaint = true;
		}
		++it;
	}

	if(needsRepaint) {
		Repaint();
	}
}

void DashboardWnd::OnContextMenu(Pixels x, Pixels y) {
	if(IsInDesignMode()) {
		ref<Widget> wi = GetWidgetAt(x,y);
		if(wi) {
			ContextMenu cm;
			enum { KCRemoveWidget, };
			cm.AddItem(TL(dashboard_widget_remove), KCRemoveWidget);

			Area wrc = wi->GetClientArea();
			int r = cm.DoContextMenu(this, wrc.GetLeft(), wrc.GetBottom());
			if(r==KCRemoveWidget) {
				RemoveWidget(wi);
			}
		}
		else {
			ref<Widget> wx = WidgetFactory::Instance()->DoCreateWidgetMenu(this, x, y);
			if(wx) {
				AddWidget(wx);
				Area preferredSize = wx->GetPreferredSize();
				wx->Move(x, y, preferredSize.GetWidth(), preferredSize.GetHeight());
			}
		}
	}
	else {
		// Widgets are windowless, so they cannot really present any context menu in their OnContextMenu method... so
		// just don't call it for now.
	}

	RepaintIfNecessary();
}

void DashboardWnd::GetAccelerators(std::vector<Accelerator>& accs) {
	accs.push_back(Accelerator(KeyDelete, TL(key_delete), TL(dashboard_widget_remove), false));
}

void DashboardWnd::OnKey(Key k, wchar_t ch, bool down, bool isAccelerator) {
	if(IsInDesignMode()) {
		if(k==KeyDelete && _focus) {
			RemoveWidget(_focus);
		}
	}
}

void DashboardWnd::SetDesignMode(bool d) {
	_designMode = d;
	_dragging = null;
	if(!d) {
		Application::Instance()->GetView()->Inspect(null,null);
	}
	Repaint();
}

bool DashboardWnd::IsInDesignMode() const {
	return _designMode;
}

/** DashboardToolbarWnd **/
DashboardToolbarWnd::DashboardToolbarWnd(ref<DashboardWnd> dw): _dw(dw) {
	_designItem = GC::Hold(new ToolbarItem(0, L"icons/layout.png", TL(dashboard_design), false));
	Add(_designItem);
}

DashboardToolbarWnd::~DashboardToolbarWnd() {
}

void DashboardToolbarWnd::OnCommand(ref<ToolbarItem> ti) {
	if(ti==_designItem) {
		ref<DashboardWnd> dw = _dw;
		if(dw) {
			dw->SetDesignMode(!dw->IsInDesignMode());
		}
	}
}

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
	}
	else {
		Log::Write(L"TJShow/Dashboard", L"Could not create widget of type '"+widgetType+L"'; not loading widget");
	}
	return widget;
}