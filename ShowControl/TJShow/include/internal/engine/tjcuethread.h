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
#ifndef _TJCUETHREAD_H
#define _TJCUETHREAD_H

#include "tjtimedthread.h"

namespace tj {
	namespace show {
		namespace engine {
			/** The CueThread handles cues; the controller starts or stops the CueThread when the controller is
			started/stopped and hands it a cue-list and start-time **/
			class CueThread: public TimedThread {
				public:
					CueThread(ref<CueList> list, ref<Controller> c, float speed);
					virtual ~CueThread();

				protected:
					virtual void OnTick(Time current);
					virtual void OnJump(Time t);
					virtual void OnPause(Time current);
					virtual void OnSetSpeed(Time current, float c);
					virtual void OnStop();
					virtual void OnStart(Time t);
					virtual void OnResume(Time t);
					virtual Time GetNextEvent(Time t);

					ref<CueList> _list;
					weak<Controller> _controller;
					Time _last;
			};
		}
	}
}

#endif