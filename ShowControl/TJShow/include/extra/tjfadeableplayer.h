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
#ifndef _TJFADEABLEPLAYER_H
#define _TJFADEABLEPLAYER_H

namespace tj {
	namespace show {
		template<typename T> class FaderPlayer {
			public:
				FaderPlayer(ref< Fader<T> > f): _fader(f), _time(0) {
					assert(_fader);
					_lastSentValue = _fader->GetMinimum()-1;
				}

				virtual ~FaderPlayer() {
				}

				void Jump(Time t, ref<tj::np::Message> msg) {
					_time = 0;
					if(msg) {
						T value = _fader->GetValueAt(_time);
						msg->Add(value);
						_lastSentValue = value;
					}
				}

				bool UpdateNecessary(Time pos) {
					return _fader->GetValueAt(pos) != _lastSentValue;
				}

				void Tick(Time pos, ref<tj::np::Message> msg) {
					T value = _fader->GetValueAt(pos);
					if(value!=_lastSentValue) {
						if(msg) {
							msg->Add(value);
						}
						_lastSentValue = value;
					}
					_time = pos;
				}

			protected:
				ref< Fader<T> > _fader;
				Time _time;
				T _lastSentValue;
		};
	}
}

#endif