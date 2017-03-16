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
#include "../../include/internal/view/tjoutputwnd.h"
#include "../../include/internal/tjcontroller.h"
#include <windowsx.h>
using namespace tj::shared::graphics;
using namespace tj::show::view;

namespace tj {
	namespace show {
		namespace view {
			class OutputToolbarWnd: public ToolbarWnd {
				public:
					OutputToolbarWnd(OutputWnd* wnd) {
						Add(GC::Hold(new ToolbarItem(KStopAll, L"icons/stop-all.png", TL(timeline_stop_all), false)));
						Add(GC::Hold(new ToolbarItem(KPauseAll, L"icons/pause.png", TL(timeline_pause_all), false)));
						Add(GC::Hold(new ToolbarItem(KResumeAll, L"icons/resume.png", TL(timeline_resume_all), true)));
						Add(GC::Hold(new ToolbarItem(KNextPlay, L"icons/trigger-cue.png", TL(timeline_next_play_cue), false)));
						_ow = wnd;
					}

					virtual ~OutputToolbarWnd() {
					}

					virtual void OnCommand(ref<ToolbarItem> ti) {
						OnCommand(ti->GetCommand());
					}

					virtual void OnCommand(int c) {
						if(c==KStopAll) {
							_ow->_instance->SetPlaybackStateRecursive(PlaybackStop);
						}
						else if(c==KPauseAll) {
							_ow->_instance->SetPlaybackStateRecursive(PlaybackPause, PlaybackPlay);
						}
						else if(c==KResumeAll) {
							_ow->_instance->SetPlaybackStateRecursive(PlaybackPlay, PlaybackPause);
						}
						else if(c==KNextPlay) {
							ref<Cue> cue = _ow->_instance->GetCueList()->GetNextCue(_ow->_instance->GetTime(), Cue::ActionStart);
							if(cue) {
								_ow->_instance->Trigger(cue, false);
							}
						}
					}

				protected:
					enum {
						KStopAll = 1,
						KPauseAll,
						KResumeAll,
						KNextPlay,
					};
					OutputWnd* _ow;
			};
		}
	}
}

OutputWnd::OutputWnd(strong<Instance> inst): _instance(inst) {
	SetText(TL(outputs));
	_slider = GC::Hold(new SliderWnd(L""));
	_slider->SetValue(0.5f, false);
	_slider->SetShowValue(false);
	_slider->SetSnapToHalf(true);
	Add(_slider);

	_toolbar = GC::Hold(new OutputToolbarWnd(this));
	Add(_toolbar);
	
	Layout();
}

OutputWnd::~OutputWnd() {
}

void OutputWnd::OnCreated() {
	_slider->EventChanged.AddListener(ref<Listener<SliderWnd::NotificationChanged> >(this));
}

void OutputWnd::Notify(ref<Object> source, const SliderWnd::NotificationChanged& evt) {
	float sliderValue = (_slider->GetValue()-0.5f)*2.0f; // -1, 1
	float speed = 1.0f;
	if(sliderValue==0.0f) {
		// speed = 1.0f
	}
	else if(sliderValue<0.0f) {
		speed = 1.0f+sliderValue; // -0.25 => 0.75, -1.0 => 0.0
	}
	else if(sliderValue>0.0f) {
		speed = sliderValue * 10.0f +1.0f;
	}

	_instance->SetPlaybackSpeed(speed);
	Update();
}

void OutputWnd::Update() {
	float speed = _instance->GetPlaybackSpeed();
	float slider = 0.5f;
	if(speed<1.0f) {
		slider = (speed/2.0f);
	}
	else if(speed>1.0f) {
		// 5.0 => 0.5(5.0/10.0f) = 0.5*0.5f + 0.5f = 0.75
		slider = 0.5f*((speed-1.0f) / 10.0f) + 0.5f;
	}
	_slider->SetValue(slider, false);
	Repaint();
}

void OutputWnd::Layout() {
	Area rc = GetClientArea();
	_toolbar->Fill(LayoutTop, rc);
	rc.Narrow(0,0,0,32); // for speed display 32 pixels
	_slider->Fill(LayoutRight, rc);
}

void OutputWnd::Paint(graphics::Graphics& g, strong<Theme> theme) {
	Area rc = GetClientArea();
	g.Clear(theme->GetColor(Theme::ColorBackground));

	// bottom bar
	Area bottomRC = rc;
	bottomRC.SetHeight(KSliderWidth);
	bottomRC.SetY(rc.GetBottom()-KSliderWidth);
	theme->DrawInsetRectangle(g, bottomRC);
	StringFormat sf;
	sf.SetAlignment(StringAlignmentNear);
	sf.SetFormatFlags(StringFormatFlagsLineLimit);
	SolidBrush tb(theme->GetColor(Theme::ColorText));

	SolidBrush disabled(theme->GetColor(Theme::ColorDisabledOverlay));
	g.FillRectangle(&disabled, bottomRC);

	// Start/end dates
	if(ref<Instance>(_instance).IsCastableTo<Controller>()) {
		ref<Controller> controller = ref<Instance>(_instance);
		std::wstring start = TL(controller_started_time) + controller->GetLastStartTime();
		std::wstring stop = TL(controller_stopped_time) + controller->GetLastStopTime();
		g.DrawString(start.c_str(), (int)start.length(), theme->GetGUIFont(), RectF(5.0f, float(rc.GetBottom()-KSliderWidth), float(rc.GetWidth()-KSliderWidth), KSliderWidth/2.0f), &sf, &tb);
		g.DrawString(stop.c_str(), (int)stop.length(), theme->GetGUIFont(), RectF(5.0f, float(rc.GetBottom()-KSliderWidth+KSliderWidth/2), float(rc.GetWidth()-KSliderWidth), KSliderWidth/2.0f), &sf, &tb);
	}

	// speed meter
	std::wstring speed = L"==";
	float speedF = _instance->GetPlaybackSpeed();
	if(speedF>1.0f) {
		speed = L"x"+Stringify(speedF);
	}
	else if(speedF<1.0f) {
		speed = L"/"+Stringify((1.0f/speedF));
	}

	g.DrawString(speed.c_str(), int(speed.length()), theme->GetGUIFontBold(), RectF(float(rc.GetRight()-KSliderWidth), float(rc.GetBottom()-15), 32.0f, 15.0f), &sf, &tb);
}

void OutputWnd::OnSize(const Area& newSize) {
	Layout();
	Repaint();
}