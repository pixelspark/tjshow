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
#ifndef _TJMEDIAPLAYER_H
#define _TJMEDIAPLAYER_H

namespace tj {
	namespace media {
		/* Protocol: MediaAction (unsigned int), parameters (where Time: int, volume: int)
		MediaActionStop
		MediaActionStopBlank (same as stop, but then 'really' stops and clears the screen)
		MediaActionStart time (-1: load only) volume card [file]
		MediaActionJump time volume
		MediaActionVolume ?????
		MediaActionMasterValues [float volume] [float opacity]
		MediaActionLiveParameters [float volume] [float opacity]
		MediaActionSpeed [speed]
		MediaActionSetScreen [patch]
		*/
		enum MediaAction {
			MediaActionStop=0,
			MediaActionStart,
			MediaActionVolume,
			MediaActionJump,
			MediaActionLiveParameters,
			MediaActionStopBlank,
			MediaActionMasterValues, 
			MediaActionSpeed,
			MediaActionKeyingParameters,
			MediaActionSetScreen,
		};

		class MediaPlayer: public Player, public Listener<Interactive::MouseNotification>, public Listener<master::MediaMasters::MasterChangedNotification> {
			public:
				MediaPlayer(strong<Track> tr, strong<MediaPlugin> pl, ref<Stream> str);
				virtual ~MediaPlayer();
				ref<Track> GetTrack();
				virtual void Stop();
				virtual void Start(Time pos, ref<Playback> pb, float speed);
				virtual void Tick(Time currentPosition);
				virtual void Jump(Time newT, bool paused);
				virtual bool Paint(tj::shared::graphics::Graphics* g, unsigned int w, unsigned int h);
				virtual void SetOutput(bool enable);
				virtual Time GetNextEvent(Time t);
				virtual void Pause(Time pos);
				virtual void SetPlaybackSpeed(Time t, float c);
				void UpdateMasterValues();
				virtual void Notify(ref<Object> src, const Interactive::MouseNotification& me);
				virtual void Notify(ref<Object> src, const master::MediaMasters::MasterChangedNotification& me);

			protected:
				void Preload(Time pos);
				
				strong<MediaTrack> _track;
				ref<MediaBlock> _playing;
				ref<Deck> _deck;
				strong<MediaPlugin> _plugin;
				strong<master::MediaMasters> _masters;
				ref<Stream> _stream;
				ref<Playback> _pb;

				bool _outputAudioEnabled;
				bool _outputVideoEnabled;
				float _speed;
				float _volume;
				std::wstring _preloadAttempted;
		};

		class MediaPlugin;

		class MediaStreamPlayer: public StreamPlayer {
			public:
				MediaStreamPlayer(ref<MediaPlugin> plug, ref<Playback> pb);
				virtual ~MediaStreamPlayer();
				virtual ref<Plugin> GetPlugin();
				virtual void Message(ref<DataReader> msg, ref<Playback> pb);
			
			protected:
				ref<MediaPlugin> _plugin;
				ref<Playback> _pb;
				ref<Deck> _deck;
		};
	}
}

#endif