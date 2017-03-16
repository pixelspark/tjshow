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
#include "../../include/internal/engine/tjmtengine.h"
#include "../../include/internal/engine/tjtimedthread.h"

using namespace tj::shared;
using namespace tj::show;
using namespace tj::show::engine;

namespace tj {
	namespace show {
		namespace engine {
			class PlayerThread: public TimedThread {
				public:
					PlayerThread(strong<Player> player, strong<TrackWrapper> track);
					virtual ~PlayerThread();
					virtual strong<Player> GetPlayer();

				protected:
					virtual void OnTick(Time current);
					virtual void OnJump(Time t);
					virtual void OnPause(Time current);
					virtual void OnSetSpeed(Time current, float c);
					virtual void OnStop();
					virtual void OnStart(Time t);
					virtual void OnResume(Time t);
					virtual Time GetNextEvent(Time t);

				private:
					void HandleException(const Exception& e);
					strong<Player> _player;
					strong<TrackWrapper> _track;
					volatile unsigned int _tickCount;
			};
		}
	}
}

/** MultiThreadedEngine **/
MultiThreadedEngine::MultiThreadedEngine() {
}

MultiThreadedEngine::~MultiThreadedEngine() {
}

void MultiThreadedEngine::StartTicking(const Time& startTime, const Timestamp& startTicks, ref<Playback> pb, float c) {
	ThreadLock lock(&_lock);
	strong<Event> startEvent = GC::Hold(new Event());

	std::map< ref<Player>, ref<PlayerThread> >::iterator it = _activePlayers.begin();
	while(it!=_activePlayers.end()) {
		ref<PlayerThread> pt = it->second;
		if(pt) {
			pt->Start(startTime, startTicks, c, pb, startEvent);
		}
		++it;
	}
	startEvent->Signal();
}

void MultiThreadedEngine::StartPaused(const Time& startTime, const Timestamp& startTicks, ref<Playback> pb, float c) {
	ThreadLock lock(&_lock);

	std::map< ref<Player>, ref<PlayerThread> >::iterator it = _activePlayers.begin();
	while(it!=_activePlayers.end()) {
		ref<PlayerThread> pt = it->second;
		if(pt) {
			pt->StartPaused(startTime, startTicks, c, pb);
		}
		++it;
	}
}

void MultiThreadedEngine::PauseTicking(bool pause, const Time& pauseTime, const Timestamp& pauseTicks) {
	ThreadLock lock(&_lock);
	std::map< ref<Player>, ref<PlayerThread> >::iterator it = _activePlayers.begin();
	while(it!=_activePlayers.end()) {
		ref<PlayerThread> pt = it->second;
		if(pt) {
			pt->Pause(pauseTime, pauseTicks, pause);
		}
		++it;
	}
}

void MultiThreadedEngine::StopTicking() {
	ThreadLock lock(&_lock);
	std::map< ref<Player>, ref<PlayerThread> >::iterator it = _activePlayers.begin();
	while(it!=_activePlayers.end()) {
		ref<PlayerThread> pt = it->second;
		if(pt) {
			pt->Stop();
		}
		++it;
	}
	_activePlayers.clear();
}

void MultiThreadedEngine::AddPlayer(strong<Player> tr, strong<TrackWrapper> tw) {
	ThreadLock lock(&_lock);
	_activePlayers[tr] = GC::Hold(new PlayerThread(tr, tw));
}

void MultiThreadedEngine::SetTickingSpeed(float c, const Time& startTime, const Timestamp& startTicks) {
	ThreadLock lock(&_lock);
	std::map< ref<Player>, ref<PlayerThread> >::iterator it = _activePlayers.begin();
	while(it!=_activePlayers.end()) {
		ref<PlayerThread> pt = it->second;
		if(pt) {
			pt->SetPlaybackSpeed(startTime, startTicks, c);
		}
		++it;
	}
}

void MultiThreadedEngine::Jump(const Time& time, const Timestamp& startTicks, bool p) {
	ThreadLock lock(&_lock);
	std::map< ref<Player>, ref<PlayerThread> >::iterator it = _activePlayers.begin();
	while(it!=_activePlayers.end()) {
		ref<PlayerThread> pt = it->second;
		if(pt) {
			pt->Jump(time, startTicks);
		}
		++it;
	}
}


/** PlayerThread **/
PlayerThread::PlayerThread(strong<Player> player, strong<TrackWrapper> track): _player(player), _track(track) {
	_tickCount = 0;
}

PlayerThread::~PlayerThread() {
	WaitForCompletion();
}

strong<Player> PlayerThread::GetPlayer() {
	return _player;
}

void PlayerThread::OnPause(Time current) {
	try {
		_player->Pause(current);
	}
	catch(Exception& e) {
		HandleException(e);
	}
	catch(...) {
		Log::Write(L"TJShow/PlayerThread", L"Couldn't jump, player threw exception");
	}
}

void PlayerThread::HandleException(const Exception& e) {
	ref<EventLogger> ev = Application::Instance()->GetEventLogger();
	ev->AddEvent(_track->GetInstanceName()+L" ("+Stringify(_track->GetID())+L"): "+e.ToString(), e.GetType());

}

void PlayerThread::OnStart(Time t) {
	OnTick(t);
}

void PlayerThread::OnResume(Time t) {
	OnJump(t);
}

void PlayerThread::OnTick(Time current) {
	try {
		_tickCount++;
		_track->SetLastTickTime((unsigned int)GetTickCount());
		_player->Tick(current);
	}
	catch(Exception& e) {
		HandleException(e);
	}
	catch(...) {
		Log::Write(L"TJShow/PlayerThread", L"Couldn't tick, player threw an unknown exception");
	}
}

void PlayerThread::OnJump(Time current) {
	try {
		_player->Jump(current, GetPlaybackState()!=PlaybackPlay);
	}
	catch(Exception& e) {
		HandleException(e);
	}
	catch(...) {
		Log::Write(L"TJShow/PlayerThread", L"Couldn't jump, player threw an unknown exception");
	}
}

void PlayerThread::OnStop() {
	try {
		_player->Stop();
		_tickCount = 0;
	}
	catch(Exception& e) {
		HandleException(e);
	}
	catch(...) {
		Log::Write(L"TJShow/PlayerThread", L"Couldn't stop player, player threw an unknown exception");
	}
}

void PlayerThread::OnSetSpeed(Time current, float c) {
	try {
		_player->SetPlaybackSpeed(current,c);
	}
	catch(Exception& e) {
		HandleException(e);
	}
	catch(...) {
		Log::Write(L"TJShow/PlayerThread", L"Couldn't set player speed, player threw an unknown exception");
	}
}

Time PlayerThread::GetNextEvent(Time t) {
	try {
		Time next = _player->GetNextEvent(t);
		return next;
	}
	catch(Exception& e) {
		HandleException(e);
	}
	catch(...) {
		Log::Write(L"TJShow/PlayerThread", L"Couldn't get next event, player threw an unknown exception");
	}
	return 0;
}