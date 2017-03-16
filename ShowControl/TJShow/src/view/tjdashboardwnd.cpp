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
#include "../../include/internal/view/tjdashboardwnd.h"
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
DashboardWnd::DashboardWnd(ref<Dashboard> db): _dashboard(db), _designMode(false), _dragOffsetX(0), _dragOffsetY(0), _isDraggingSize(false), _grabberIcon(Icons::GetIconPath(Icons::IconGrabber)) {
}

DashboardWnd::~DashboardWnd() {
}

void DashboardWnd::SetDashboard(ref<Dashboard> db) {
	_dashboard = db;
	Repaint();
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

	if(!_dashboard) {
		return;
	}

	if(_designMode) {
		Area backRC = rc;
		if(_toolbar) backRC.Narrow(0, _toolbar->GetClientArea().GetHeight(), 0, 0);
		theme->DrawInsetRectangle(g, backRC);

		if(_dashboard->_widgets.size()==0) {
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
	std::deque< ref<Widget> >::iterator it = _dashboard->_widgets.begin();
	while(it!=_dashboard->_widgets.end()) {
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

void DashboardWnd::OnMouse(MouseEvent ev, Pixels x, Pixels y) {
	if(!_dashboard) return;

	if(ev==MouseEventLDown) {
		Focus();
	}

	if(ev==MouseEventRDown) {
		OnContextMenu(x,y);
	}
	else if(IsInDesignMode()) {
		if(ev==MouseEventLDown) {
			_dragging = _dashboard->GetWidgetAt(x,y);
			if(_dragging) {
				_draggingOldArea = _dragging->GetClientArea();
			}

			_focus = _dragging;
			Application::Instance()->GetView()->Inspect(_dragging, null);

			if(_dragging) {
				// TODO: Put it at the top of the item list
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
			ref<Widget> wg = _dashboard->GetWidgetAt(x,y);
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
		ref<Widget> wi = _dashboard->GetWidgetAt(x,y);
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

void DashboardWnd::RepaintIfNecessary() {
	bool needsRepaint = false;
	if(!_dashboard) return;

	std::deque< ref<Widget> >::iterator it = _dashboard->_widgets.begin();
	while(it!=_dashboard->_widgets.end()) {
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
	if(!_dashboard) return;

	if(IsInDesignMode()) {
		ref<Widget> wi = _dashboard->GetWidgetAt(x,y);
		if(wi) {
			ContextMenu cm;
			enum { KCRemoveWidget, };
			cm.AddItem(TL(dashboard_widget_remove), KCRemoveWidget);

			Area wrc = wi->GetClientArea();
			int r = cm.DoContextMenu(this, wrc.GetLeft(), wrc.GetBottom());
			if(r==KCRemoveWidget) {
				_dashboard->RemoveWidget(wi);
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
	if(!_dashboard) return;

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

void DashboardWnd::RemoveAllWidgets() {
	_focus = null;
	_dragging = null;
	if(_dashboard) _dashboard->RemoveAllWidgets();
	Layout();
	Repaint();
}

void DashboardWnd::AddWidget(ref<Widget> w) {
	if(_dashboard) {
		_dashboard->AddWidget(w);

		// Initial layout
		w->Show(true);
		Layout();
		Repaint();
	}
}

void DashboardWnd::RemoveWidget(ref<Widget> w) {
	if(_dashboard) {
		_dashboard->RemoveWidget(w);

		if(_focus==w) {
			_focus = null;
		}

		if(_dragging==w) {
			_dragging = null;
		}
		Layout();
		Repaint();
	}
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