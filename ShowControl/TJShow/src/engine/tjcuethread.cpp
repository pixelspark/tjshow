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
#include "../../include/internal/engine/tjcuethread.h"

using namespace tj::show;
using namespace tj::shared;
using namespace tj::show::engine;

CueThread::CueThread(ref<CueList> list, ref<Controller> c, float speed): TimedThread(speed) {
	assert(list && c);
	_list = list;
	_controller = c;
	SetPriority(Thread::PriorityHigh);
}

CueThread::~CueThread() {
}

void CueThread::OnTick(Time c) {
	ref<Controller> controller = _controller;
	if(controller) {
		std::list< ref<Cue> > cues;
		_list->GetCuesBetween(_last, c, cues); // GetCuesBetween is end-inclusive: start < t <= end
		std::list< ref<Cue> >::iterator it = cues.begin();

		bool continueLinearProcessing = true;
		while(continueLinearProcessing && it!=cues.end()) {
			ref<Cue> cue = *it;
			if(cue) {
				try {
					Time diff = c - cue->GetTime();
					if(diff > Time(100)) {
						Log::Write(L"TJShow/CueThread", L"Late cue (before executing it): "+cue->GetName()+L" diff="+Stringify(diff.ToInt()));
					}

					continueLinearProcessing = cue->DoAction(Application::Instance(), controller);
					if(!continueLinearProcessing) {
						_last = cue->GetTime();
					}
				}
				catch(Exception& e) {
					Log::Write(L"TJShow/CueThread", L"Exception in cue execution: "+e.GetMsg());
				}
				catch(...) {
					Log::Write(L"TJShow/CueThread", L"Unknown exception in cue execution");
				}

				/*Time diff = c - cue->GetTime();
				if(diff > Time(100)) {
					Log::Write(L"TJShow/CueThread", L"Late cue (after executing it): "+cue->GetName()+L" diff="+Stringify(diff.ToInt()));
				}*/
			}
			++it;
		}

		if(continueLinearProcessing) {
			// If we're past the end-time, stop the controller
			Time t = controller->GetTimeline()->GetTimeLengthMS();
			if(c>=t) {
				controller->SetPlaybackState(PlaybackStop);
			}
		
			// We've progressed linearly, write the last tick time
			// (when a jump has occurred, _last is already changed in CueThread::OnJump).
			_last = c;
		}
	}
}

void CueThread::OnStart(Time t) {
	_last = t;
}

void CueThread::OnJump(Time t) {
	_last = t;
}

void CueThread::OnResume(Time t) {
	_last = t;
}

void CueThread::OnPause(Time current) {
	_last = current;
}

void CueThread::OnSetSpeed(Time current, float c) {
}

void CueThread::OnStop() {
	_last = 0;
}

Time CueThread::GetNextEvent(Time t) {
	Time next = -1;

	// Ask the controller what the end time is, the cue thread will stop the controller
	// as if a cue were present on the end-time
	ref<Controller> controller = _controller;
	if(controller) {
		next = controller->GetTimeline()->GetTimeLengthMS();
	}

	ref<Cue> cue = _list->GetNextCue(t);
	if(cue) {
		next = cue->GetTime();
	}
	return next;
}