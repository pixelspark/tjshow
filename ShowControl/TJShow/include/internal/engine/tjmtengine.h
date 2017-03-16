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
#ifndef _TJMTENGINE_H
#define _TJMTENGINE_H

#include "tjengine.h"

namespace tj {
	namespace show {
		namespace engine {
			/** The multithreaded playback engine is the engine that is used in all old versions of TJShow (except in the
			really old single threaded one, of course). It simply starts a thread for each player that has to play back. 
			Because threads use a lot of memory, cause a lot of time to set up and then often sleep most of the time,
			this is not the most efficient engine (but will probably work great if there are zillions of processors - 
			maybe in the future). This engine however is easy to understand and pretty much 'proven' by usage in the older
			versions.
			**/
			class PlayerThread;

			class MultiThreadedEngine: public Engine {
				public:
					MultiThreadedEngine();
					virtual ~MultiThreadedEngine();
					virtual void StartPaused(const Time& startTime, const Timestamp& startTicks, ref<Playback> pb, float c);
					virtual void StartTicking(const Time& startTime, const Timestamp& startTicks, ref<Playback> pb, float c);
					virtual void PauseTicking(bool pause, const Time& pauseTime, const Timestamp& pauseTicks);
					virtual void StopTicking();
					virtual void Jump(const Time& time, const Timestamp& startTicks, bool paused);
					virtual void AddPlayer(strong<Player> tr, strong<TrackWrapper> tw);
					virtual void SetTickingSpeed(float c, const Time& startTime, const Timestamp& startTicks);

				protected:
					CriticalSection _lock;
					std::map< ref<Player>, ref<PlayerThread> > _activePlayers;
			};
		}
	}
}

#endif