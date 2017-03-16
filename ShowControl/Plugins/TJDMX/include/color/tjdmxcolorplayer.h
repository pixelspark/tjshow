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
#ifndef _TJDMXCOLORPLAYER_H
#define _TJDMXCOLORPLAYER_H

namespace tj {
	namespace dmx {
		namespace color {
			class DMXColorPlayer: public Player {
				public:
					DMXColorPlayer(ref<DMXColorTrack> track);
					virtual ~DMXColorPlayer();
					virtual ref<Track> GetTrack();
					virtual void Stop();
					virtual void Start(Time pos, ref<Playback> playback, float speed);
					virtual void Pause(Time pos);
					virtual void Tick(Time currentPosition);
					virtual void Jump(Time t, bool pause);
					virtual Time GetNextEvent(Time t);
					virtual void SetPlaybackSpeed(Time t, float c);
					virtual void SetOutput(bool enable);

				protected:
					ref<DMXColorTrack> _track;
					bool _output;
					ref<DMXMacro> _macros[_ColorChannelLast];
			};
		}
	}
}

#endif