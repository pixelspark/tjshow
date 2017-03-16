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
#include "../include/tjdmx.h"
using namespace tj::shared::graphics;
#include <windowsx.h>

const int DMXPositionEditorWnd::ToolbarHeight = 24;

DMXPositionEditorWnd::DMXPositionEditorWnd(ref<Wnd> parent, ref<DMXPositionTrack> track): Wnd(parent) {
	_editBackground = 0;
	_track = track;

	SetStyle(WS_OVERLAPPEDWINDOW);
	SetStyleEx(WS_EX_WINDOWEDGE|WS_EX_TOOLWINDOW|WS_EX_CONTROLPARENT);
	SetText(TL(dmx_trackspot_editor_title));

	_from = GC::Hold(new EditWnd());
	_to = GC::Hold(new EditWnd());
	Add(_from);
	Add(_to);
	
	// buttons
	_delete = GC::Hold(new ButtonWnd(L"icons/remove.png", TL(dmx_trackspot_editor_delete_all)));
	_rightStep = GC::Hold(new ButtonWnd(L"icons/step_forward.png", TL(dmx_trackspot_editor_step_right) ));
	_leftStep = GC::Hold(new ButtonWnd(L"icons/step_backward.png", TL(dmx_trackspot_editor_step_left) ));
	_rightBlock = GC::Hold(new ButtonWnd(L"icons/go_forward.png", TL(dmx_trackspot_editor_block_left) ));
	_leftBlock = GC::Hold(new ButtonWnd(L"icons/go_backward.png", TL(dmx_trackspot_editor_block_right) ));
	Add(_delete); Add(_rightStep); Add(_leftStep); Add(_rightBlock); Add(_leftBlock);
	
	_drawing = false;
	SetSize(400,400);
	Layout();
	_timeMode = ShowTimesMS;
}

DMXPositionEditorWnd::~DMXPositionEditorWnd() {
	if(_editBackground!=0) DeleteObject(_editBackground);
}

void DMXPositionEditorWnd::OnCreated() {
	// Because the this -> ref<T> conversion is slow, cache it
	ref<Listener<ButtonWnd::NotificationClicked> > tl = this;

	_delete->EventClicked.AddListener(tl);
	_rightStep->EventClicked.AddListener(tl);
	_leftStep->EventClicked.AddListener(tl);
	_rightBlock->EventClicked.AddListener(tl);
	_leftBlock->EventClicked.AddListener(tl);
}

void DMXPositionEditorWnd::Notify(ref<Object> source, const ButtonWnd::NotificationClicked& nf) {
	if(source==ref<Object>(_rightStep)) {
		Step(Time(50));
	}
	else if(source==ref<Object>(_leftStep)) {
		Step(Time(-50));
	}
	else if(source==ref<Object>(_leftBlock)) {
		Range<Time> t = GetRange();
		Step(Time(-t.Length().ToInt())+Time(1));
	}
	else if(source==ref<Object>(_rightBlock)) {
		Range<Time> t = GetRange();
		Step(t.Length()+Time(1));
	}
	else if(source==ref<Object>(_delete)) {
		Range<Time> t = GetRange();
		_track->GetFaderById(DMXPositionTrack::KFaderPan)->RemoveItemsBetween(t.Start(), t.End());
		_track->GetFaderById(DMXPositionTrack::KFaderTilt)->RemoveItemsBetween(t.Start(), t.End());
		Repaint();
	}
}

void DMXPositionEditorWnd::Step(Time s) {
	Range<Time> t = GetRange();
	Time diff = t.Length();
	t.SetStart(Time(max(0, t.Start().ToInt()+s.ToInt())));
	t.SetEnd(t.Start() + diff);
	SetRange(t);
}

void DMXPositionEditorWnd::SetRange(Range<Time> t) {
	_from->SetText(Stringify(int(t.Start())));
	_to->SetText(Stringify(int(t.End())));
	Repaint();
}

void DMXPositionEditorWnd::Layout() {
	int x = 2;
	_leftBlock->Move(x+2, 4, 16, 16); x += 20;
	_leftStep->Move(x+2, 4, 16, 16); x += 20;
	_from->Move(x, 4, 75, 16); x += 77;
	_to->Move(x, 4, 75, 16); x += 77;
	_rightStep->Move(x+2, 4, 16, 16); x += 20;
	_rightBlock->Move(x+2, 4, 16, 16); x += 20;

	_delete->Move(x+2, 4, 135, 16); x += 137;
	Update();
}

Range<Time> DMXPositionEditorWnd::GetRange() {
	Time from(_from->GetText());
	Time to(_to->GetText());

	return Range<Time>(from,to);
}

void DMXPositionEditorWnd::Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme) {
	Area rc = GetClientArea();

	SolidBrush back(theme->GetColor(Theme::ColorBackground));
	g.FillRectangle(&back,(RectF)rc);

	SolidBrush toolbar(theme->GetColor(Theme::ColorTimeBackground));
	RectF toolbarRect = (RectF)rc;
	toolbarRect.SetHeight((float)ToolbarHeight);
	g.FillRectangle(&toolbar,toolbarRect);	

	Area ts = rc;
	ts.Narrow(0, ToolbarHeight, 0, 0);
	g.SetClip((RectF)ts);
	Range<Time> range = GetRange();
	if(!range.IsValid()) return; // start > end
	if(range.Start()<Time(0) || range.End()<Time(0)) return;
	if(!_track) {
		return;
	}

	ref< Fader<float> > pan = _track->GetFaderById(DMXPositionTrack::KFaderPan);
	ref< Fader<float> > tilt = _track->GetFaderById(DMXPositionTrack::KFaderTilt);
	if(!(pan&&tilt)) {
		return;
	}

	float pr = float(ts.GetWidth());
	float tr = float(ts.GetHeight());

	// iterators from begin
	std::map<Time, float>::iterator panit = pan->GetPoints()->lower_bound(range.Start());
	std::map<Time, float>::iterator tiltit = tilt->GetPoints()->lower_bound(range.Start());

	// end
	std::map<Time, float>::iterator panend = pan->GetPoints()->upper_bound(range.End());
	std::map<Time, float>::iterator tiltend = tilt->GetPoints()->upper_bound(range.End());

	if(!(panit==panend || tiltend==tiltit)) {
		Time current = range.Start();
		if(range.Start()>Time(0)) range.SetStart(range.Start()-Time(1));
		float pv = pan->GetValueAt(current);
		float tv = tilt->GetValueAt(current);
		Pen line(theme->GetColor(Theme::ColorCommandMarker), 2.0f);
		SolidBrush tbr(theme->GetColor(Theme::ColorText));
		SolidBrush pointBrush(theme->GetColor(Theme::ColorCommandMarker));
		Time lastStringDrawn = Time(-1);

		int n = 0;

		while(panit!=panend || tiltit!=tiltend) {
			n++;
			// draw for t=current
			float npv = pan->GetValueAt(current);
			float ntv = tilt->GetValueAt(current);
			g.DrawLine(&line, pv*pr+ts.GetLeft(), tv*tr+ts.GetY(), npv*pr+ts.GetX(), ntv*tr+ts.GetY());
			g.FillEllipse(&pointBrush, RectF(pv*pr+ts.GetX()-3, tv*tr+ts.GetY()-3,6.0f, 6.0f));
			
			if(_timeMode!=ShowTimesNone) {
				std::wstring time;
				if(_timeMode==ShowTimesFormatted) {
					time = current.Format();
				}
				else if(_timeMode==ShowTimesMS) {
					time = Stringify(current.ToInt());
				}
				else if(_timeMode==ShowTimesSequential) {
					time = Stringify(n);
				}

				g.DrawString(time.c_str(), (INT)time.length(), theme->GetGUIFontSmall(), PointF(pv*pr+ts.GetLeft()+5, tv*tr+ts.GetTop()-3), &tbr);
			}

			pv = npv;
			tv = ntv;
			if(current>=range.End()) break;

			// find next event
			Time next(current);
			bool pan = true;
			if(panit!=panend) {
				if(panit!=panend && panit->first > current) {
					next = panit->first;
				}
			}

			if(tiltit!=tiltend) {
				if(tiltit!=tiltend && tiltit->first < next && tiltit->first > current) {
					next = tiltit->first;
					pan = false;
				}
			}

			if(pan && panit!=panend) {
				++panit;
			}
			else if(tiltit!=tiltend) {
				++tiltit;
			}

			current = next;
		}
	}

	// if we're editing, overlay something
	if(_drawing && _points.size() >0) {
		SolidBrush overlay(theme->GetColor(Theme::ColorDisabledOverlay));
		Pen pen(theme->GetColor(Theme::ColorLine),2.0f);
		g.FillRectangle(&overlay, (RectF)ts);

		std::list< std::pair<int,int> >::iterator it = _points.begin();
		int x = -1;
		int y = -1;
		while(it!=_points.end()) {
			std::pair<int,int> coord = *it;

			if(x!=-1 && y !=-1) {
				g.DrawLine(&pen, PointF(float(x), float(y)), PointF(float(coord.first), float(coord.second)));
			}

			x = coord.first;
			y = coord.second;
			
			++it;
		}
	}
}

LRESULT DMXPositionEditorWnd::Message(UINT msg, WPARAM wp, LPARAM lp) {
	if(msg==WM_SIZE) {
		Layout();
	}
	else if(msg==WM_CTLCOLOREDIT) {
		strong<Theme> theme = ThemeManager::GetTheme();
		//if(_editBackground==0 || _editBackgroundColor.ToCOLORREF()==theme->GetColor(Theme::ColorBackground).ToCOLORREF()) {
			_editBackgroundColor = theme->GetColor(Theme::ColorBackground);
			if(_editBackground!=0) DeleteObject(_editBackground);
			_editBackground = CreateSolidBrush(RGB(_editBackgroundColor.GetR(),_editBackgroundColor.GetG(), _editBackgroundColor.GetB() ));
		//}

		SetBkMode((HDC)wp, TRANSPARENT);
		Color text = theme->GetColor(Theme::ColorText);
		SetTextColor((HDC)wp, RGB(text.GetRed(),text.GetGreen(),text.GetBlue()));
		return (LRESULT)(HBRUSH)_editBackground;
	}
	else if(msg==WM_COMMAND) {
		// edit fields probably changed
		Repaint();
	}
	else if(msg==WM_KEYDOWN) {
		if(wp==VK_RIGHT) {
			Step(Time(50));
		}
		else if(wp==VK_LEFT) {
			Step(Time(-50));
		}
		else if(wp==VK_DOWN) {
			Range<Time> t = GetRange();
			Step(Time(-t.Length().ToInt())+Time(1));
		}
		else if(wp==VK_UP) {
			Range<Time> t = GetRange();
			Step(t.Length()+Time(1));
		}
	}
	else if(msg==WM_LBUTTONDOWN) {
		if(_track) {
			_drawing = true;
			_points.clear();
			_capture.StartCapturing(Mouse::Instance(), ref<Wnd>(this));
			Repaint();
		}
	}
	else if(msg==WM_LBUTTONUP) {
		if(_track) {
			_drawing = false;

			if(_points.size()>0) {
				Area ts = GetClientArea();
				ts.Narrow(0, ToolbarHeight, 0, 0);
				Range<Time> range = GetRange();
				_track->GetFaderById(DMXPositionTrack::KFaderPan)->RemoveItemsBetween(range.Start(), range.End());
				_track->GetFaderById(DMXPositionTrack::KFaderTilt)->RemoveItemsBetween(range.Start(), range.End());

				Time timePerPoint = Time(int(range.Length().ToInt()/_points.size()));
				Time current = range.Start();
				if(timePerPoint>Time(0)) {
					std::list< std::pair<int,int> >::iterator it = _points.begin();
					while(it!=_points.end()) {
						std::pair<int, int> coord = *it;
						int panValue = int((float(coord.first) / float(ts.GetWidth())) * 255.0f);
						int tiltValue = int((float(coord.second-ToolbarHeight) / float(ts.GetHeight()))*255.0f);
						if(panValue<0) panValue = 0;
						if(panValue>255) panValue = 255;
						if(tiltValue<0) tiltValue = 0;
						if(tiltValue>255) tiltValue = 255;
						_track->GetFaderById(DMXPositionTrack::KFaderPan)->AddPoint(current, float(panValue)/255.0f, true);
						_track->GetFaderById(DMXPositionTrack::KFaderTilt)->AddPoint(current, float(tiltValue)/255.0f, true);
						current = current+timePerPoint;
						++it;
					}
				}
			}
			
			_points.clear();
			_capture.StopCapturing();
			Repaint();
		}
	}
	else if(msg==WM_MOUSEMOVE) {
		if(_drawing && IsKeyDown(KeyMouseLeft)) {
			int x = GET_X_LPARAM(lp);
			int y = GET_Y_LPARAM(lp);
			_points.push_back(std::pair<int,int>(x,y));
			Repaint();
		}
	}
	else if(msg==WM_CONTEXTMENU) {
		ContextMenu m;
		enum {timeNone=1, timeMS, timeFormat, timeSeq};
		m.AddItem(TL(dmx_trackspot_editor_time_none), timeNone, false, _timeMode==ShowTimesNone);
		m.AddItem(TL(dmx_trackspot_editor_time_ms), timeMS, false, _timeMode==ShowTimesMS);
		m.AddItem(TL(dmx_trackspot_editor_time_formatted), timeFormat, false, _timeMode==ShowTimesFormatted);
		m.AddItem(TL(dmx_trackspot_editor_time_sequential), timeSeq, false, _timeMode==ShowTimesSequential);

		int result = m.DoContextMenu(this);
		switch(result) {
			case timeNone:
				_timeMode = ShowTimesNone;
				break;
			case timeMS:
				_timeMode = ShowTimesMS;
				break;
			case timeFormat:
				_timeMode = ShowTimesFormatted;
				break;
			case timeSeq:
				_timeMode = ShowTimesSequential;
				break;
		}

		Repaint();
		return 0;
	}
	return Wnd::Message(msg,wp,lp);
}