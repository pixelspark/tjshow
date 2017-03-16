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
#include "../../include/internal/engine/tjengine.h"
#include "../../include/internal/engine/tjpoolengine.h"

using namespace tj::shared;
using namespace tj::show;
using namespace tj::show::engine;

const float PoolEngine::KThresholdWithinRange = 0.90f;
const int PoolEngine::KEvaluationIntervalTicks = 100;
const int PoolEngine::KPoolThreadTimeout = 2000;
const int PoolEngine::KMinimumThreadsIdle = 1;
const int PoolEngine::KMinimumThreadsStopped = 0;

/** PoolThread **/
namespace tj {
	namespace show {
		namespace engine {
			class PoolThread: public Thread {
				public:
					PoolThread(ref<PoolEngine> pool);
					virtual ~PoolThread();
					virtual void Run();

				protected:
					void ProcessPoolMessage(PoolMessage& msg);
					void SchedulePoolMessage(PoolMessage& pm, const Timebase& base, const Time& t);
					void ExecutePoolMessage(PoolMessage& msg);
					Time GetTime(const Timebase& base, float speed);
					weak<PoolEngine> _pool;

					// Timer
					HANDLE _timer;
					bool _hasScheduledWork;
					PoolMessage _scheduled;
					int _minTickLength;
			};
		}
	}
}

PoolThread::PoolThread(ref<PoolEngine> pool): _pool(pool), _hasScheduledWork(false) {
	_timer = CreateWaitableTimer(NULL, FALSE, NULL);
	_minTickLength = pool->_minTickLength;
}

PoolThread::~PoolThread() {
	CancelWaitableTimer(_timer);
	CloseHandle(_timer);
	Log::Write(L"TJShow/PoolEngine", L"~PoolThread");
}

Time PoolThread::GetTime(const Timebase& base, float speed) {
	Timestamp now(true);
	return base.time + Time(int(now.Difference(base.ticks).ToMilliSeconds()*speed));
}

void PoolThread::SchedulePoolMessage(PoolMessage& pm, const Timebase& base, const Time& t) {
	if(_hasScheduledWork) {
		Throw(L"Cannot schedule next event when there is still an event scheduled!", ExceptionTypeError);
	}

	Timestamp now(true);
	Time current = GetTime(base,pm.speed);
	Time preferredNextTick = t;

	if(preferredNextTick<Time(0)) {
		return;
	}

	if(preferredNextTick > current) {
		// If the next event is closer than the min tick length, make sure it is not
		Time diff = preferredNextTick - current;
		if(int(diff)<=_minTickLength) {
			preferredNextTick = current + Time(_minTickLength);
		}

		/** Playback speed: when c=0.5, Time(1000) takes 2000 ms in reality
		when c=2.0, Time(1000) takes 500ms in reality **/
		Time delay(int(float(int(preferredNextTick-current))/pm.speed));

		LARGE_INTEGER dueTime;
		memset(&dueTime, 0, sizeof(LARGE_INTEGER));
		dueTime.QuadPart = -LONGLONG(int(delay))*LONGLONG(10000) + 5000;
		if(preferredNextTick<=current) {
			dueTime.QuadPart  = LONGLONG(-1*10000);
		}
		if(SetWaitableTimer(_timer, &dueTime, 0, NULL, NULL, 0)==FALSE) {
			Log::Write(L"TJShow/PoolEngine", L"SetWaitableTimer failed in SchedulePoolMessage");
		}
		else {
			_scheduled = pm;
			_hasScheduledWork = true;
		}
	}
	else {
		ExecutePoolMessage(pm);
	}
}

void PoolThread::ProcessPoolMessage(PoolMessage& msg) {
	ref<Player> player = msg.player;

	if(player) {
		if(msg.action==PoolActionTick) {
			SchedulePoolMessage(msg, msg.timeBase, msg.time);
		}
		else {
			ExecutePoolMessage(msg);
		}
	}
	else {
		Log::Write(L"TJShow/PoolEngine", L"Message has no player!");
	}
}

void PoolThread::ExecutePoolMessage(PoolMessage& msg) {
	//Log::Write(L"TJShow/PoolEngine", L"Tick p="+StringifyHex(msg.player.GetPointer())+L" at t="+Stringify(msg.time.ToInt())+L" a="+Stringify(msg.action));
	if(!msg.player) {
		Log::Write(L"TJShow/PoolEngine", L"Message has no player! Ignoring message in ExecutePoolMessage");
		return;
	}

	// get the current time and adjust it if we're 'early'
	Time t = GetTime(msg.timeBase, msg.speed);
	if(msg.time > t) { // We're early
		t = msg.time;
	}

	ref<PoolEngine> pe = _pool;
	if(!pe) {
		Log::Write(L"TJShow/PoolEngine", L"Cannot execute pool message when there is no pool!");
	}

	// Execute the pool message
	bool shouldCancelScheduled = false;
	bool shouldReschedule = false;

	if(msg.action==PoolActionStop) {
		msg.player->Stop();
	}
	else if(msg.action==PoolActionStart) {
		msg.player->Tick(t);
		shouldReschedule = true;
	}
	else if(msg.action==PoolActionPause) {
		msg.player->Pause(t);
		shouldCancelScheduled = true;
	}
	else if(msg.action==PoolActionUnpause) {
		msg.player->Jump(t, false);
		shouldReschedule = true;
	}
	else if(msg.action==PoolActionSetSpeed) {
		msg.player->SetPlaybackSpeed(t, msg.speed);
		shouldCancelScheduled = true;
		shouldReschedule = true;
	}
	else if(msg.action==PoolActionJumpPlaying) {
		msg.player->Jump(t, false);
		shouldCancelScheduled = true;
		shouldReschedule = true;
	}
	else if(msg.action==PoolActionJumpPaused) {
		msg.player->Jump(t, true);
		shouldCancelScheduled = true;
		shouldReschedule = false;
	}
	else if(msg.action==PoolActionTick) {
		if(pe->_isTicking) {
			msg.player->Tick(t);
		}
		shouldReschedule = true;
	}

	// Re-schedule and/or cancel scheduled messages
	if((!pe->_isTicking || shouldCancelScheduled) && _hasScheduledWork) {
		CancelWaitableTimer(_timer);
		_hasScheduledWork = false;
	}

	if(shouldReschedule && pe->_isTicking) {
		ref<PoolEngine> pe = _pool;
		if(pe) {
			Time tn = msg.player->GetNextEvent(t);
			if(tn>=Time(0)) {
				PoolMessage pm = msg;
				pm.action = PoolActionTick;
				pm.time = tn;
				ThreadLock lock(&(pe->_lock));
				pe->_queue.push_back(pm);
				pe->_queueSemaphore.Release(1);
			}
		}
	}

	// Update statistics
	int lateness = t.ToInt() - msg.time.ToInt();
	pe->_stats.totalTickCount++;
	pe->_stats.totalDeviation += lateness;
	if(lateness > 2*_minTickLength) {
		pe->_stats.lateTickCount++;
	}

	// If it is time to evaluate our timing, tell the PoolEngine to do so (in this thread!)
	if(pe->_stats.totalTickCount >= PoolEngine::KEvaluationIntervalTicks) {
		pe->EvaluatePoolStatistics();
	}
}

void PoolThread::Run() {
	try {
		HANDLE waitObjects[3];

		while(true) {
			ref<PoolEngine> pe = _pool;
			if(pe) {
				 waitObjects[0] = pe->_cancelAllScheduled.GetHandle();
				 waitObjects[1] = _timer;
				 waitObjects[2] = pe->_queueSemaphore.GetHandle();
				

				// See if there is any work to do
				{
					int status = WaitForMultipleObjects(3, waitObjects, FALSE, _hasScheduledWork ? INFINITE  : PoolEngine::KPoolThreadTimeout);
					if(status==WAIT_OBJECT_0+2) {
						bool haveWork = false;
						bool needsSleep = false;
						PoolMessage message;
						
						{
							ThreadLock lock(&(pe->_lock));
							if(pe->_queue.size()>=1) {
								message = (*pe->_queue.begin());

								if(message.NeedsScheduling() && _hasScheduledWork) {
									if(_scheduled.time > message.time) {
										// The new message has to 'happen' before our scheduled message, and needs scheduling; thus,
										// un-schedule and requeue the old message before processing the new one
										CancelWaitableTimer(_timer);
										_hasScheduledWork = false;
										pe->_queue.pop_front();
										pe->_queue.push_back(_scheduled);
										haveWork = true;
									}
									else {
										// Can't take this message; put it on the back of the queue
										haveWork = false;
										pe->_queue.push_back(message);
										pe->_queue.pop_front();
									}

									// In both cases, the queue has a new message it needs to process; signal event
									// To make sure we do not keep 'spinning', sleep here for at least _minTickLength
									// to allow other threads to pick up the new message if we do not have any work to
									// process.
									pe->_queueSemaphore.Release(1);
									if(!haveWork && pe->_queue.size()==1) {
										needsSleep = true;
									}
								}
								else {
									pe->_queue.pop_front();
									haveWork = true;
								}
							}
							else {
								// Another thread was a little faster picking up the work, and there is no work left
							}
						}

						// Execute the work
						if(haveWork) {
							ProcessPoolMessage(message);
						}
						else {
							if(needsSleep) {
								Sleep(_minTickLength);
							}
						}
					}
					else if(status==WAIT_OBJECT_0+1) {
						// Our timer rang, execute scheduled message
						if(_hasScheduledWork) {
							ExecutePoolMessage(_scheduled);
							_hasScheduledWork = false;
						}
						else {
							Log::Write(L"TJShow/PoolEngine", L"Timer rang, but thread has no work scheduled");
						}
					}
					else if(status==WAIT_OBJECT_0 && _hasScheduledWork) {
						CancelWaitableTimer(_timer);
						_hasScheduledWork = false;
					}
					else if(status==WAIT_TIMEOUT) {
						if(!_hasScheduledWork) {
							if(pe->RemoveIdleThread(this)) {
								return;
							}
						}
					}
				}
			}
			else {
				break;
			}
		}
	}
	catch(const Exception& e) {
		Log::Write(L"TJShow/PoolEngine", L"Exception in pool thread: "+std::wstring(e.GetMsg())+L"; thread will end");
		ref<PoolEngine> pe = _pool;
		if(pe) {
			pe->RemoveIdleThread(this);
		}
	}
	catch(...) {
		Log::Write(L"TJShow/PoolEngine", L"Unknown exception in pool thread; thread will end");
		ref<PoolEngine> pe = _pool;
		if(pe) {
			pe->RemoveIdleThread(this);
		}
	}
	Log::Write(L"TJShow/PoolEngine", L"Pool thread ended");
}

/** PoolEngine **/
PoolEngine::PoolEngine(const Time& minTickLength): _speed(1.0f), _isTicking(false), _minTickLength(minTickLength.ToInt()) {
}

PoolEngine::~PoolEngine() {
	Log::Write(L"TJShow/PoolEngine", L"~PoolEngine");
}

void PoolEngine::SetTicking(bool t) {
	ThreadLock lock(&_lock);
	_isTicking = t;
	if(t) {
		_cancelAllScheduled.Reset();
	}
	else {
		_cancelAllScheduled.Signal();
	}
	_stats.Reset();
}

bool PoolEngine::RemoveIdleThread(ref<PoolThread> pt) {
	// Probably called from the poolthread itself
	if(pt) {
		ThreadLock lock(&_lock);

		// Calculate the minimum and maximum number of threads needed in this situation
		int minThreadsNeeded = 0;
		if(_players.size()>0) {
			minThreadsNeeded = KMinimumThreadsIdle;
		}
		else {
			minThreadsNeeded = KMinimumThreadsStopped;
		}

		/* The thread is allowed to stop when there are less threads needed or when
		there are more threads than players (this happens when a player is removed
		during playback when multiple threads have been created) */
		if(int(_workers.size())>minThreadsNeeded || _workers.size()>int(_players.size())) {
			std::set< ref<PoolThread> >::iterator it = _workers.find(pt);
			if(it==_workers.end()) {
				Throw(L"Worker not found in worker set of this pool!", ExceptionTypeError);
			}
			_workers.erase(it);
			return true;
		}
		_stats.Reset();
	}
	return false;
}

void PoolEngine::EvaluatePoolStatistics() {
	bool createNewThread = false;
	{
		ThreadLock lock(&_lock);
		unsigned int currentThreadCount = _workers.size();

		if(_stats.totalTickCount >= int(KEvaluationIntervalTicks*currentThreadCount)) {
			// We want 95% of all ticks to be within the range of [0, 2*_minTickLength] 'lateness' (KThresholdWithinRange).
			// Furthermore, we want the average 'lateness' * 95% to be within the range [0,2*_minTickLength]
			float outsideTwiceRange = float(_stats.lateTickCount) / float(_stats.totalTickCount);
			float averageLateness = float(_stats.totalDeviation)/float(_stats.totalTickCount);

			if(Zones::IsDebug()) {
				Log::Write(L"TJShow/PoolEngine", L"Pool statistics: % really late="+Stringify(outsideTwiceRange)+L" average lateness="+Stringify(averageLateness)+L" n="+Stringify(currentThreadCount)+L" qn="+Stringify(_queue.size()));
			}

			if(outsideTwiceRange > (1.0f-KThresholdWithinRange) || (averageLateness * KThresholdWithinRange) > (2*_minTickLength)) {
				// More than 95% of all ticks is late or the average lateness*0.95 is higher than the min tick length.
				// Thus, we need another thread (but never create more threads than players)
				if(currentThreadCount < _players.size()) {
					createNewThread = true;
				}
			}
		}
	}

	if(createNewThread) {
		ref<PoolThread> pt = GC::Hold(new PoolThread(this));
		{
			ThreadLock lock(&_lock);
			_workers.insert(pt);
			_stats.Reset();
		}
		pt->Start();
	}
}

void PoolEngine::OnWorkQueued(unsigned int numItems) {
	if(numItems>0) {
		{
			ThreadLock lock(&_lock);
			SpawnPoolThreads();
		}
		_queueSemaphore.Release(numItems);
	}
}

void PoolEngine::SpawnPoolThreads() {
	ThreadLock lock(&_lock);
	if(_workers.size()<1) {
		ref<PoolThread> tr = GC::Hold(new PoolThread(this));
		_workers.insert(tr);
		tr->Start();
		_stats.Reset();
	}
}

void PoolEngine::AddPlayer(strong<Player> tr, strong<TrackWrapper> tw) {
	ThreadLock lock(&_lock);
	_players.insert(tr);
}

void PoolEngine::QueueActionForAllPlayers(PoolAction pa, const Timebase& tb, const Time& time, ref<Playback> pb, float c) {
	_cancelAllScheduled.Reset();
	unsigned int messagesQueued = 0;

	PoolMessage pm;
	pm.action = pa;
	pm.playback = pb;
	pm.speed = c;
	pm.time = time;
	pm.timeBase = tb;

	{
		ThreadLock lock(&_lock);
		std::set< ref<Player> >::iterator it = _players.begin();
		while(it!=_players.end()) {
			ref<Player> player = *it;
			if(player) {
				pm.player = player;
				_queue.push_back(pm);
				++messagesQueued;
			}
			++it;
		}

		OnWorkQueued(messagesQueued);
	}
}

void PoolEngine::StartPaused(const Time& startTime, const Timestamp& startTicks, ref<Playback> pb, float c) {
	ThreadLock lock(&_lock);
	SetTicking(false);
	SpawnPoolThreads();
	_speed = c;
}

void PoolEngine::StartTicking(const Time& startTime, const Timestamp& startTicks, ref<Playback> pb, float c) {
	ThreadLock lock(&_lock);
	Timebase tb;
	tb.ticks = startTicks;
	tb.time = startTime;
	_speed = c;
	SetTicking(true);
	QueueActionForAllPlayers(PoolActionTick, tb, startTime, pb, c);
}

void PoolEngine::PauseTicking(bool pause, const Time& pauseTime, const Timestamp& pauseTicks) {
	ThreadLock lock(&_lock);
	SetTicking(!pause);
	_queue.clear();
	Timebase tb;
	tb.ticks = pauseTicks;
	tb.time = pauseTime;
	QueueActionForAllPlayers(pause ? PoolActionPause : PoolActionUnpause, tb, pauseTime, null, _speed);
}

void PoolEngine::StopTicking() {
	ThreadLock lock(&_lock);
	
	SetTicking(false);
	_queue.clear();
	Timebase tb;
	tb.ticks = Timestamp(true);
	tb.time = Time(0);
	QueueActionForAllPlayers(PoolActionStop, tb, tb.time, null, _speed);
	_players.clear();
}

void PoolEngine::Jump(const Time& time, const Timestamp& startTicks, bool paused) {
	ThreadLock lock(&_lock);
	_queue.clear();
	_cancelAllScheduled.Signal();

	Timebase tb;
	tb.ticks = startTicks;
	tb.time = time;
	QueueActionForAllPlayers(paused ? PoolActionJumpPaused : PoolActionJumpPlaying, tb, time, null, _speed);
}

void PoolEngine::SetTickingSpeed(float c, const Time& startTime, const Timestamp& startTicks) {
	ThreadLock lock(&_lock);
	_queue.clear();
	_cancelAllScheduled.Reset();
	Timebase tb;
	tb.ticks = startTicks;
	tb.time = startTime;
	_speed = c;
	QueueActionForAllPlayers(PoolActionSetSpeed, tb, startTime, null, c);
}

/** PoolStatistics **/
PoolStatistics::PoolStatistics() {
	Reset();
}

void PoolStatistics::Reset() {
	totalTickCount = 0;
	lateTickCount = 0;
	totalDeviation = 0;
}