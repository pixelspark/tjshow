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
#include "../../include/internal/tjcontroller.h"
#include "../../include/internal/view/tjsplittimelinewnd.h"
#include "../../include/internal/view/tjlivewnd.h"
#include <windowsx.h>
#include <algorithm>
using namespace tj::show::view;
using namespace tj::shared::graphics;

class LiveGroupToolbarWnd: public SearchToolbarWnd {
	public:
		LiveGroupToolbarWnd(LiveGroupWnd* lg): _lg(lg) {
			assert(lg!=0);
		}

		virtual ~LiveGroupToolbarWnd() {
		}

		virtual void OnSearchChange(const std::wstring& q) {
			_lg->SetFilter(q);
		}

		virtual void OnCommand(ref<ToolbarItem> ti) {
		}


	protected:
		LiveGroupWnd* _lg;
};

/* Live Group Wnd */
LiveGroupWnd::LiveGroupWnd() {
	SetText(TL(live));
	SetStyle(WS_CLIPCHILDREN);
	SetStyleEx(WS_EX_CONTROLPARENT);
	SetHorizontallyScrollable(true);
	_tools = GC::Hold(new LiveGroupToolbarWnd(this));
	Add(_tools,true);
	Layout();
}

void LiveGroupWnd::Clear() {
	_controls.clear();
}

void LiveGroupWnd::SetFilter(const std::wstring& f) {
	_filter = f;
	std::transform(_filter.begin(), _filter.end(), _filter.begin(), tolower);
	Layout();
	Repaint();
}

bool LiveGroupWnd::MatchesFilter(ref<TrackWrapper> tr, ref<LiveControl> lc) const {
	if(_filter.length()==0) return true;
	if(!tr || !lc) return false;

	std::wstring trackName = tr->GetInstanceName() + L" " + tr->GetTrack()->GetTypeName();
	std::transform(trackName.begin(), trackName.end(), trackName.begin(), tolower);
	return trackName.find(_filter, 0)!=std::string::npos;
}

LiveGroupWnd::~LiveGroupWnd() {
}

void LiveGroupWnd::Update() {
	int x = 0;
	int left = GetHorizontalPos();
	int right = left + GetClientArea().GetWidth();

	std::vector< std::pair< ref<TrackWrapper>, ref<LiveControl> > >::iterator it = _controls.begin();
	while(it!=_controls.end()) {
		if(MatchesFilter(it->first, it->second)) {
			x += it->second->GetWidth();
			if(x>=left && x<=right) {
				it->second->Update();
			}
		}
		++it;
	}
}

void LiveGroupWnd::SetFocus(int c) {
	unsigned int hpos = GetHorizontalPos();
	int offset = hpos / (KDefaultControlWidth + KDefaultMarginH);
	std::pair< ref<TrackWrapper>, ref<LiveControl> > p = _controls.at(c+offset);
	::SetFocus(p.second->GetWindow()->GetWindow());
}

void LiveGroupWnd::MoveUp(ref<TrackWrapper> tr) {
	int items = (int)_controls.size();
	for(int a=0;a<items;a++) {
		std::pair< ref<TrackWrapper>, ref<LiveControl> > data = _controls.at(a);
		if(data.first==tr) {
			if(a-1>=0) {
				std::pair< ref<TrackWrapper>, ref<LiveControl> > next = _controls.at(a-1);
				_controls[a] = next;
				_controls[a-1] = data;
				break;
			}
		}
	}
	Update();
}

void LiveGroupWnd::MoveDown(ref<TrackWrapper> tr) {
	size_t items = _controls.size();
	for(size_t a=0;a<items;a++) {
		std::pair< ref<TrackWrapper>, ref<LiveControl> > data = _controls.at(a);
		if(data.first==tr) {
			if(a+1<items) {
				std::pair< ref<TrackWrapper>, ref<LiveControl> > next = _controls.at(a+1);
				_controls[a] = next;
				_controls[a+1] = data;
				break;
			}
		}
	}

	Update();
}

void LiveGroupWnd::Layout(bool alsoSetScrollInfo) {
	Area rc = GetClientArea();
	_tools->Fill(LayoutTop, rc);
	rc.Narrow(1,1,0,0);

	int hpos = (int)GetHorizontalPos();
	Pixels height = min(KMaximalControlHeight,rc.GetHeight()-KDefaultHeaderHeight);

	Pixels cx = rc.GetLeft();
	Pixels totalWidth = 0;

	ref<Model> model = Application::Instance()->GetModel();
	std::vector< std::pair< ref<TrackWrapper>, ref<LiveControl> > >::iterator it = _controls.begin();
	
	while(it!=_controls.end()) {
		std::pair< ref<TrackWrapper>, ref<LiveControl> > data = *it;
		if(data.second && data.first && MatchesFilter(data.first, data.second)) {
			Pixels width = data.second->GetWidth();
			totalWidth += width + KDefaultMarginH;
			ref<Wnd> control = data.second->GetWindow();

			if(control) {
				control->Show(!(cx-hpos > rc.GetWidth() || cx < 0));
				control->Move(cx-hpos, int(rc.GetTop()+KDefaultHeaderHeight), width, height);
			}
			cx += width + KDefaultMarginH;
		}
		else {
			ref<Wnd> control = data.second->GetWindow();

			if(control) {
				control->Show(false);
			}
		}
		++it;
	}

	if(alsoSetScrollInfo) {
		SetHorizontalScrollInfo(Range<int>(0, (int)totalWidth),rc.GetWidth());
		if(totalWidth<rc.GetWidth()) {
			SetHorizontalPos(0);
		}
	}
}

void LiveGroupWnd::Layout() {
	Layout(false);
}

std::pair<ref< TrackWrapper>, ref<LiveControl> > LiveGroupWnd::GetControlAt(Pixels x, Pixels y) {
	Area rc = GetClientArea();
	rc.Narrow(1,1,1,1);

	Pixels cx = rc.GetLeft();
	ref<Model> model = Application::Instance()->GetModel();
	std::vector< std::pair< ref<TrackWrapper>, ref<LiveControl> > >::iterator it = _controls.begin();
	while(it!=_controls.end()) {
		if(MatchesFilter(it->first, it->second)) {
			if(x>cx) {
				cx += it->second->GetWidth() + KDefaultMarginH;
				if(x < cx ) {
					return *it;
				}
			}
		}
		++it;
	}	

	return std::pair< ref<TrackWrapper>, ref<LiveControl> >(0,0);
}

void LiveGroupWnd::OnScroll(ScrollDirection d) {
	Layout();
	UpdateWindow(GetWindow());
}

void LiveGroupWnd::AddControl(ref<TrackWrapper> track, ref<LiveControl> control) {
	assert(track && control);
	ref<Wnd> window = control->GetWindow();
	if(!window) return;
	SetParent(window->GetWindow(), GetWindow());
	window->Show(true);
	_controls.push_back(std::pair< ref<TrackWrapper>, ref<LiveControl> >(track,control));
	Layout();
}

void LiveGroupWnd::RemoveControl(ref<TrackWrapper> track) {
	std::vector< std::pair<ref<TrackWrapper>, ref<LiveControl> > >::iterator it = _controls.begin();
	while(it!=_controls.end()) {
		std::pair<ref<TrackWrapper>, ref<LiveControl> > data = *it;
		if(data.first==track) {
			_controls.erase(it);
			Layout();
			return;
		}
		++it;
	}
}

int LiveGroupWnd::GetCount() {
	return int(_controls.size());
}

void LiveGroupWnd::Paint(Graphics& g, strong<Theme> theme) {
	Area rect = GetClientArea();
	rect.Narrow(0,_tools->GetClientArea().GetHeight(), 0, 0);

	// background
	g.Clear(theme->GetColor(Theme::ColorBackground));
	rect.Narrow(1,1,1,1);

	// Shadowed stuff
	Area shadowed(rect.GetLeft(), rect.GetTop()+KMaximalControlHeight+KDefaultHeaderHeight, rect.GetWidth(), rect.GetHeight());
	theme->DrawInsetRectangle(g, shadowed);
	SolidBrush disabled(theme->GetColor(Theme::ColorDisabledOverlay));
	g.FillRectangle(&disabled, shadowed);

	int hpos = GetHorizontalPos();
	// track names and other stuff
	int cx = 0;
	tj::shared::graphics::Font* font = theme->GetGUIFont();
	tj::shared::graphics::Font* boldFont = theme->GetGUIFontSmall();
	SolidBrush textBrush(theme->GetColor(Theme::ColorText));
	ref<Model> model = Application::Instance()->GetModel();
	std::vector< std::pair< ref<TrackWrapper>, ref<LiveControl> > >::iterator it = _controls.begin();
	int fkey = 1;
	while(it!=_controls.end()) {
		std::pair< ref<TrackWrapper>, ref<LiveControl> > data = *it;
		if(MatchesFilter(data.first, data.second)) {
			int width = data.second->GetWidth();

			std::wstring name = data.first->GetInstanceName();
			RectF lr(float(cx-hpos), float(rect.GetTop()+3), float(width), 12.0f);

			// Draw group color
			ref<Group> group = Application::Instance()->GetModel()->GetGroups()->GetGroupById(data.first->GetGroup());
			if(group) {
				Area colorBar(cx-hpos, rect.GetTop()+KDefaultHeaderHeight-4, width, 3);
				colorBar.Narrow(1,0,1,0);
				SolidBrush colorBrush(group->GetColor());
				g.FillRectangle(&colorBrush, colorBar);
			}
			
			// Draw name
			StringFormat sf;
			sf.SetAlignment(StringAlignmentNear);
			bool current = false;
			GraphicsContainer cont = g.BeginContainer();
			g.TranslateTransform(float(rect.GetLeft()+cx-hpos+20.0f), float(rect.GetTop()+KMaximalControlHeight+KDefaultHeaderHeight+4.0f));
			g.RotateTransform(90.0f);
			g.DrawString(name.c_str(), (unsigned int)name.length(), font, PointF(0.0f, 0.0f), &sf, &textBrush);
			g.EndContainer(cont);

			sf.SetAlignment(StringAlignmentCenter);
			std::wstring cid = Stringify(data.first->GetInstanceName());
			g.DrawString(cid.c_str(), (int)cid.length(), boldFont, RectF(float(rect.GetLeft()+cx-hpos), rect.GetTop()+1.0f, (float)width, KDefaultHeaderHeight-5.0f), &sf, &textBrush);

			cx += width + KDefaultMarginH;
		}
		++it;
	}
}

void LiveGroupWnd::OnSize(const Area& ns) {
	Layout(true);
	Update();
}

void LiveGroupWnd::OnMouse(MouseEvent ev, Pixels x, Pixels y) {
	if(ev==MouseEventRDown) {
		// Find control at that position
		std::pair<ref< TrackWrapper>, ref<LiveControl> > control = GetControlAt(x,y);
		if(control.second) {
			ContextMenu cm;
			enum {KCRevealTrack = 1, KCInsertToTrack, KCBindInput, KCBindSpecific };
			cm.AddItem(TL(live_wnd_reveal_track), KCRevealTrack, false, false);
			cm.AddItem(TL(toolbar_insert_one), KCInsertToTrack, false, false);

			std::vector<LiveControl::EndpointDescription> endpoints;
			control.second->GetEndpoints(endpoints);
			if(endpoints.size()>0) {
				cm.AddSeparator(TL(live_control_bind_to_input));
				int n = 0;
				std::vector< LiveControl::EndpointDescription >::const_iterator it = endpoints.begin();
				while(it!=endpoints.end()) {
					const LiveControl::EndpointDescription& desc = *it;
					cm.AddItem(desc._friendlyName, KCBindSpecific+n, false, false);
					++it;
					++n;
				}
			}
			else {
				cm.AddItem(GC::Hold(new MenuItem(TL(live_control_bind_to_input), KCBindInput, false, MenuItem::NotChecked, L"icons/input.png")));
			}

			int r = cm.DoContextMenu(this);
			if(r==KCRevealTrack) {
				ref<Instance> parent = Application::Instance()->GetInstances()->GetRootInstance()->GetParentOf(control.first);
				
				if(parent && parent.IsCastableTo<Controller>()) {
					ref<Controller> parentController = parent;
					if(parentController) {
						Application::Instance()->GetView()->RevealTimeline(parentController->GetSplitTimelineWindow());
					}
				}
			}
			else if(r==KCInsertToTrack) {
				ref<Instance> parent = Application::Instance()->GetInstances()->GetRootInstance()->GetParentOf(control.first);
				
				if(parent) {
					control.first->GetTrack()->InsertFromControl(parent->GetTime(), control.second, true);
				}
			}
			else if(r>=KCBindInput) {
				ref<Model> model = Application::Instance()->GetModel();
				if(model && control.first) {
					ref<input::Rules> rules = model->GetInputRules();
					if(rules) {
						// Create endpoint ID
						std::wstring eid;
						if(r>=KCBindSpecific) {
							int n = r-KCBindSpecific;
							const LiveControl::EndpointDescription& desc = endpoints.at(n);
							eid = Stringify(control.first->GetID()) + L":" + desc._id;
						}
						else {
							 eid = Stringify(control.first->GetID());
						}

						// Ask the user to bind
						ref<input::Rule> rule = rules->DoBindDialog(this, LiveControlEndpointCategory::KLiveControlEndpointCategoryID, eid);
						if(rule) {
							rules->AddRule(rule);
						}
					}
				}
			}
		}
	}
}