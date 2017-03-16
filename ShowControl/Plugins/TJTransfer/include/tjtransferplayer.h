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
#ifndef _TJTRANSFERPLAYER_H
#define _TJTRANSFERPLAYER_H

namespace tj {
	namespace transfer {
		class TransferPlayer: public Player {
			public:
				TransferPlayer(ref<TransferTrack> tr, ref<TransferPlugin> plug, ref<Stream> str);
				virtual ~TransferPlayer();

				ref<Track> GetTrack();
				virtual ref<Plugin> GetPlugin();
				virtual void Stop();
				virtual void Start(Time pos, ref<Playback> playback, float speed);
				virtual void Tick(Time currentPosition);
				virtual void Jump(Time newT, bool paused);
				virtual void SetOutput(bool enable);
				virtual Time GetNextEvent(Time t);

			protected:
				virtual void Fire(ref<TransferCue> tc);

				ref<TransferTrack> _track;
				ref<Playback> _pb;
				weak<TransferPlugin> _plugin;
				ref<Stream> _stream;
				bool _outputEnabled;
				Time _last;
		};

		class TransferStreamPlayer: public StreamPlayer {
			public:
				TransferStreamPlayer(ref<TransferPlugin> tp, ref<Talkback> talk);
				virtual ~TransferStreamPlayer();
				virtual ref<Plugin> GetPlugin();
				virtual void Message(ref<tj::shared::DataReader> msg, ref<Playback> pb);

			protected:
				weak<TransferPlugin> _plugin;
				ref<Talkback> _talk;
		};
	}
}

#endif