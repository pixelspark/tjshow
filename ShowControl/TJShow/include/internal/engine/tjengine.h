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
#ifndef _TJ_ENGINE_H
#define _TJ_ENGINE_H

namespace tj {
	namespace show {
		namespace engine {
			class Engine: public virtual tj::shared::Object {
				public:
					virtual ~Engine();
					virtual void StartPaused(const Time& startTime, const Timestamp& startTicks, ref<Playback> pb, float c) = 0;
					virtual void StartTicking(const Time& startTime, const Timestamp& startTicks, ref<Playback> pb, float c) = 0;
					virtual void PauseTicking(bool pause, const Time& pauseTime, const Timestamp& pauseTicks) = 0;
					virtual void StopTicking() = 0;
					virtual void AddPlayer(strong<Player> tr, strong<TrackWrapper> tw) = 0;
					virtual void Jump(const Time& time, const Timestamp& startTicks, bool paused) = 0;
					virtual void SetTickingSpeed(float c, const Time& startTime, const Timestamp& startTicks) = 0;

				protected:
					Engine();
			};

			class Engines {
				public:
					enum EngineType {
						TypeThreadPerPlayer = 1,
						TypeThreadPooled = 2,
					};

					static ref<Engine> CreateEngineOfType(EngineType t);
			};
		}
	}
}

#endif