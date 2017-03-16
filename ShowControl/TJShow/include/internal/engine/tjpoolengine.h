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
#ifndef _TJPOOLENGINE_H
#define _TJPOOLENGINE_H

#include "tjengine.h"

namespace tj {
	namespace show {
		namespace engine {
			/** The pooled playback engine is an engine that uses worker threads that are 'pooled' between players (as opposed to
			the multithreaded playback engine, in which each thread is assigned to exactly one player). The pooled engine works with a
			queue, in which 'pool messages' are placed. Pool threads pick these messages up and execute them. If a message requires scheduling
			(that is, tick messages), the thread sets a timer and waits for both the timer and for new queue messages. Whenever a new message is
			received that does not require scheduling, the message is executed by the thread (the assumption here is that executing a message
			takes little time and the scheduled event is far in the future anyway). If the newly received message does require scheduling,
			the 'future time' of the message is compared to the 'fuure time' of the scheduled message. When the new message has to be executed
			before the scheduled message, the scheduled message is 'unscheduled' and placed back in the queue. The new message is then processed by
			the thread. When the new message has to happen 'later' than the already scheduled message, the thread doesn't pop it from the queue,
			and releases the queue sempahore once more (after which it will Sleep for minTickLength, to prevent 'spinning'). 
			
			The pooled thread is actually single-threaded when only one pool thread is created.
			**/
			enum PoolAction {
				PoolActionNothing = 0,
				PoolActionTick = 1,
				PoolActionPause,
				PoolActionUnpause,
				PoolActionSetSpeed,
				PoolActionJumpPlaying,
				PoolActionJumpPaused,
				PoolActionStart,
				PoolActionStop,
			};

			struct Timebase {
				inline Timebase(): time(0), ticks(true) {
				}

				Time time;
				Timestamp ticks;
			};

			struct PoolMessage {
				PoolAction action;
				Time time;
				Timebase timeBase;
				ref<Player> player;
				ref<Playback> playback;
				float speed;

				inline PoolMessage(): action(PoolActionNothing), time(-1), speed(0.0f) {
				}

				inline bool NeedsScheduling() const {
					// PoolActionTick messages need to be scheduled
					return (action == PoolActionTick);
				}
			};

			struct PoolStatistics {
				PoolStatistics();
				void Reset();

				int totalTickCount;
				int totalDeviation;
				int lateTickCount;
			};

			class PoolThread;

			class PoolEngine: public Engine {
				friend class PoolThread;

				public:
					PoolEngine(const Time& minimumTickLength = Time(5));
					virtual ~PoolEngine();
					virtual void StartPaused(const Time& startTime, const Timestamp& startTicks, ref<Playback> pb, float c);
					virtual void StartTicking(const Time& startTime, const Timestamp& startTicks, ref<Playback> pb, float c);
					virtual void PauseTicking(bool pause, const Time& pauseTime, const Timestamp& pauseTicks);
					virtual void StopTicking();
					virtual void Jump(const Time& time, const Timestamp& startTicks, bool paused);
					virtual void SetTickingSpeed(float c, const Time& startTime, const Timestamp& startTicks);

					virtual void AddPlayer(strong<Player> tr, strong<TrackWrapper> tw);

				protected:
					// Work queue management
					virtual void QueueActionForAllPlayers(PoolAction pa, const Timebase& tb, const Time& time, ref<Playback> pb, float c);
					virtual void SetTicking(bool t);
					virtual void OnWorkQueued(unsigned int numItems);

					// Worker thread management
					virtual void SpawnPoolThreads();
					virtual void EvaluatePoolStatistics();
					virtual bool RemoveIdleThread(ref<PoolThread> pt);
					
					CriticalSection _lock;
					int _minTickLength;
					float _speed;
					ref<Playback> _pb;
					std::set< ref<Player> > _players;
					std::deque<PoolMessage> _queue;
					std::set< ref<PoolThread> > _workers;
					Semaphore _queueSemaphore;
					Event _cancelAllScheduled;
					bool _isTicking;
					PoolStatistics _stats;

					// Constants for thread scheduling
					const static float KThresholdWithinRange;	// The percentage of ticks we want to be 'on time' (within the [0,minTickLength] interval
					const static int KEvaluationIntervalTicks;	// Every KEvaluationIntervalTicks, we evaluate statistics
					const static int KPoolThreadTimeout;		// Number of ms after which a pool thread is declared 'idle'
					const static int KMinimumThreadsIdle;		// Number of threads to keep alive, even when there is no work
					const static int KMinimumThreadsStopped;	// Number of threads to keep alive, even when there is no work and no players
			};
		}
	}
}

#endif