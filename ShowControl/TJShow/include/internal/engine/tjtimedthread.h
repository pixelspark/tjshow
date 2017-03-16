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
#ifndef _TJTIMEDTHREAD_H
#define _TJTIMEDTHREAD_H

namespace tj {
	namespace show {
		namespace engine {
			class Timed {
				public:
					virtual ~Timed();
					virtual HANDLE Start(const Time& time, const Timestamp& startTicks, float speed, ref<Playback> pb, ref<Event> startEvent) = 0;
					virtual void StartPaused(const Time& time, const Timestamp& startTicks, float speed, ref<Playback> pb) = 0;
					virtual void Jump(const Time& t, const Timestamp& startTicks) = 0;
					virtual void Pause(const Time& t, const Timestamp& startTicks, bool paused) = 0;
					virtual void SetPlaybackSpeed(const Time& t, const Timestamp& startTicks, float c) = 0;
					virtual void Stop() = 0;
				
				protected:
					static inline Time GetPlaybackTime(const Time& startTime, const Timestamp& startTicks, float speed) {
						/** When speed=2.0, time passes twice as fast. **/
						Timestamp now(true);
						return startTime + Time(int(now.Difference(startTicks).ToMilliSeconds()*speed));
					}
			};

			/** A timed thread is a thread that is timed by a controller. The
			Controller sends start, stop, jump and speed information to the
			timed thread and the timed thread sends out ticks at the right time
			to the playing object. **/
			class TimedThread: public Thread, public Timed {
				public:
					virtual ~TimedThread();
					virtual HANDLE Start(const Time& time, const Timestamp& startTicks, float speed, ref<Playback> pb, ref<Event> startEvent);
					virtual void StartPaused(const Time& time, const Timestamp& startTicks, float speed, ref<Playback> pb);
					virtual void Jump(const Time& t, const Timestamp& startTicks);
					virtual void Pause(const Time& t, const Timestamp& startTicks, bool paused);
					virtual void SetPlaybackSpeed(const Time& t, const Timestamp& startTicks, float c);
					virtual void Stop();
					
				protected:
					PlaybackState GetPlaybackState() const;
					virtual void Run();
					virtual void SafeRun();
					virtual void OnTick(Time current) = 0;
					virtual void OnJump(Time t) = 0;
					virtual void OnPause(Time current) = 0;
					virtual void OnResume(Time current) = 0;
					virtual void OnSetSpeed(Time current, float c) = 0;
					virtual void OnStop() = 0;
					virtual void OnStart(Time t) = 0;
					virtual Time GetNextEvent(Time t) = 0;
					TimedThread(float speed = 1.0f);

					CriticalSection _lock;

				private:
					void OnCrashed();
					Time _start;
					Timestamp _startTime;
					HANDLE _timer;
					HANDLE _jumpEvent;
					HANDLE _pauseEvent;
					HANDLE _setSpeedEvent;
					HANDLE _stopEvent;
					HANDLE _resumeEvent;
					ref<Event> _startEvent;
					HANDLE _initializedEvent;
					volatile PlaybackState _state;
					ref<Playback> _playback;
					volatile float _speed;			

			};
		}
	}
}

#endif