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
#include "../../include/internal/engine/tjtimedthread.h"

using namespace tj::show;
using namespace tj::shared;
using namespace tj::show::engine;

TimedThread::TimedThread(float speed) {
	_timer = CreateWaitableTimer(NULL,FALSE,NULL);
	_state = PlaybackStop;
	_jumpEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	_pauseEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	_setSpeedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	_stopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	_resumeEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	_initializedEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	_speed = speed;
}

TimedThread::~TimedThread() {
	CloseHandle(_stopEvent);
	CloseHandle(_jumpEvent);
	CloseHandle(_pauseEvent);
	CloseHandle(_setSpeedEvent);
	CloseHandle(_timer);	
	CloseHandle(_resumeEvent);
	CloseHandle(_initializedEvent);
}

PlaybackState TimedThread::GetPlaybackState() const {
	return _state;
}

void TimedThread::Stop() {
	_state = PlaybackStop;
	_startEvent = null;
	SetEvent(_stopEvent);
}

HANDLE TimedThread::Start(const Time& t, const Timestamp& startTime, float speed, ref<Playback> pb, ref<Event> startEvent) {
	{
		ThreadLock lock(&_lock);
		_speed = speed;
		_start = t;
		_startTime = startTime;
		_state = PlaybackPlay;
		_playback = pb;
		_startEvent = startEvent;
	}
	Thread::Start();
	return _initializedEvent;
}

void TimedThread::StartPaused(const Time& t, const Timestamp& startTime, float speed, ref<Playback> pb) {
	ThreadLock lock(&_lock);
	_speed = speed;
	_start = t;
	_startTime = startTime;
	_state = PlaybackPause;
	_playback = pb;
	Thread::Start();
}

void TimedThread::Pause(const Time& t, const Timestamp& startTicks, bool p) {
	ThreadLock lock(&_lock);

	if(p) {
		_state = PlaybackPause;
		CancelWaitableTimer(_timer);
		SetEvent(_pauseEvent);
	}
	else {
		_state = PlaybackPlay;
		CancelWaitableTimer(_timer);
		_start = t;
		_startTime = startTicks;
		SetEvent(_resumeEvent);
	}
}

void TimedThread::Jump(const Time& t, const Timestamp& startTicks) {
	ThreadLock lock(&_lock);
	CancelWaitableTimer(_timer);
	_start = t;
	_startTime = startTicks;

	CancelWaitableTimer(_timer); // next tick is not valid anymore

	// let the thread know we've jumped
	SetEvent(_jumpEvent);
}

void TimedThread::Run() {
	#ifdef TJ_OS_WIN
		// This prevents the system from going to standby when timed threads are active
		SetThreadExecutionState(ES_CONTINUOUS|ES_SYSTEM_REQUIRED|ES_DISPLAY_REQUIRED);

		__try {
			CoInitializeEx(NULL, COINIT_MULTITHREADED);
			SafeRun();
			CoUninitialize();
		}
		__except(EXCEPTION_EXECUTE_HANDLER) {
			OnCrashed();
		}
		SetThreadExecutionState(ES_CONTINUOUS);
	#else
		SafeRun();
	#endif
}

void TimedThread::OnCrashed() {
	ref<EventLogger> ew = Application::Instance()->GetEventLogger();
	ew->AddEvent(TL(error_timed_thread_crashed), ExceptionTypeError, false);
}

void TimedThread::SetPlaybackSpeed(const Time& t, const Timestamp& ts, float c) {
	ThreadLock lock(&_lock);
	_start = t;
	_startTime = ts;
	_speed = c;
	SetEvent(_setSpeedEvent);
}

void TimedThread::SafeRun() {
	try {
		Time preferredNextTick = 0;

		// Starting tick; locks the thread so nothing can happen in the meantime
		if(_state==PlaybackPlay) {
			ThreadLock lock(&_lock);
			OnStart(_start);
		}
		else if(_state==PlaybackPause) {
			ThreadLock lock(&_lock);
			OnJump(_start);
		}

		/* These are the handles for the events. WaitForMultipleObjects waits for any of these. If multiple events are signalled
		at the same time, it uses the first one. So the order here is pretty important! We handle stop events always first, since
		they're the most important. Then pause/resume, jump, set speed and finally timer. */
		// TODO: replace with TJShared Wait API (and maybe even PeriodicTimer)
		HANDLE handles[6];
		handles[0] = _stopEvent;
		handles[1] = _pauseEvent;
		handles[2] = _resumeEvent;
		handles[3] = _jumpEvent;
		handles[4] = _setSpeedEvent;
		handles[5] = _timer;

		int minTickLength = StringTo<int>(Application::Instance()->GetSettings()->GetValue(L"controller.min-tick-length"),0);
		SetEvent(_initializedEvent);
		bool rescheduleTimer = false;

		// Wait for start event if we are starting in play state
		if(_state==PlaybackPlay) {
			if(_startEvent) {
				_startEvent->Wait();
				_startEvent = null;
			}
			rescheduleTimer = true;
		}

		while(true) {
			if(!rescheduleTimer) {
				// Don't wait for the _startEvent if we have received it once
				int watskeburt = WaitForMultipleObjects(6, handles, FALSE, INFINITE);
				Timestamp now(true);

				// If we have to stop, just do so directly
				if(watskeburt==WAIT_OBJECT_0 || watskeburt==WAIT_FAILED) {
					if(watskeburt==WAIT_FAILED) {
						Log::Write(L"TJShow/TimedThread", L"Wait failed!");
					}
					OnStop();
					return;
				}

				float currentSpeed = 1.0f;
				Time current;
				Time start;
				PlaybackState state = PlaybackStop;

				/* We first lock the TimedThread and get some information. Then, the action is ran and if we want to set the timer,
				we lock it again. With this method, the lock is never taken by the thread when it is executing OnTick. Since OnTick may
				call some code that calls a method like Start() or Jump() on this thread, this could deadlock if we still had the lock. 
				CueThreads for example are doing this all the time...
				*/
				{
					ThreadLock lock(&_lock);

					/** Make sure we get all the variables we need right now. When _speed = 2.0, this means that time passes twice as fast. **/
					currentSpeed = _speed;
					start = _start;
					state = _state;
					current = _start + Time(int(now.Difference(_startTime).ToMilliSeconds()*_speed));

					/** Check which event caused this thread to wake up. If this was _resumeEvent, _jumpEvent, _setSpeedEvent or _pauseEvent, reset
					that event, so a new event can be queued.  Also set rescheduleTimer if this is an event that (somehow) changes the current time.
					The timer is cancelled by the controlling (main) thread, so we do not have to worry about it here. **/
					switch(watskeburt) {
						case WAIT_OBJECT_0+3:
							ResetEvent(_jumpEvent);
							rescheduleTimer = true;
							break;

						case WAIT_OBJECT_0+4:
							ResetEvent(_setSpeedEvent);
							rescheduleTimer = true;
							break;

						case WAIT_OBJECT_0+1:
							ResetEvent(_pauseEvent);
							break;

						case WAIT_OBJECT_0+2:
							ResetEvent(_resumeEvent);
							rescheduleTimer = true;
							break;
					}
				}

				// Now really handle the event (this all happens without having _lock!)
				if(watskeburt == WAIT_OBJECT_0+3) {
					// a jump has occurred		
					OnJump(start);
				}
				else if(watskeburt == WAIT_OBJECT_0+2) {
					// Resume from pause
					OnResume(start);
				}
				else if(watskeburt == WAIT_OBJECT_0+4) {
					 // set speed
					OnSetSpeed(current, currentSpeed);
				}
				else if(watskeburt == WAIT_OBJECT_0+1) {
					// pause event
					OnPause(current);
				}
				else if(watskeburt == WAIT_OBJECT_0+6) {
					// Start event
				}
				else {
					if(current < preferredNextTick) {
						// the last tick, the player said it wanted a tick, but our timer is too early... just cheat
						current = preferredNextTick;
					}
					
					// if we're not paused, tick and plan the next event
					if(state != PlaybackPause) {
						OnTick(current);
						rescheduleTimer = true;
					}
				}
			}

			// Lock again to set the timer for the next event
			if(rescheduleTimer) {
				Timestamp now(true);
				ThreadLock lock(&_lock);

				/* Our state could be changed, because a pause cue might have called Controller::SetPlaybackState,
				which in turn may have called CueThread::Pause */
				if(_state==PlaybackPlay) {
					Time current = _start + Time(int(now.Difference(_startTime).ToMilliSeconds()*_speed));
					preferredNextTick = GetNextEvent(current);
					
					if(int(preferredNextTick) >= 0 && preferredNextTick >= current) {
						// If the next event is closer than the min tick length, make sure it is not
						Time diff = preferredNextTick - current;
						if(int(diff)<=minTickLength) {
							preferredNextTick = current + Time(minTickLength);
						}

						/** Playback speed: when c=0.5, Time(1000) takes 2000 ms in reality
						when c=2.0, Time(1000) takes 500ms in reality **/
						Time delay(int(float(int(preferredNextTick-current))/_speed));
						
						LARGE_INTEGER dueTime;
						memset(&dueTime, 0, sizeof(LARGE_INTEGER));
						dueTime.QuadPart = -LONGLONG(int(delay))*LONGLONG(10000);
						if(preferredNextTick<=current) {
							dueTime.QuadPart  = LONGLONG(-1*10000);
						}
						SetWaitableTimer(_timer, &dueTime, 0, NULL, NULL, 0);
					}
				}

				rescheduleTimer = false;
			}
		}
	}
	catch(const Exception& e) {
		Log::Write(L"TJShow/TimedThread", L"Exception occurred in timed thread: "+e.ToString());
		OnCrashed();
	}
	catch(const std::exception& se) {
		Log::Write(L"TJShow/TimedThread", L"STL Exception occurred in timed thread"+Wcs(se.what()));
		OnCrashed();
	}
	catch(...) {
		Log::Write(L"TJShow/TimedThread", L"Unknown Exception occurred in timed thread");
		OnCrashed();
	}
}

Timed::~Timed() {
}