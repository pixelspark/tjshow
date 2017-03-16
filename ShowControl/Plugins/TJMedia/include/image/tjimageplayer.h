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
#ifndef _TJIMAGEPLAYER_H
#define _TJIMAGEPLAYER_H

#include "tjmediamasters.h"

class ImagePlayer: public Player, public Listener<Interactive::MouseNotification>, public Listener<Interactive::FocusNotification>, public Listener<Interactive::KeyNotification>, public Listener<tj::media::master::MediaMasters::MasterChangedNotification> {
	friend class ImageStreamPlayer; // for message constants

	public:
		ImagePlayer(ref<ImageTrack> track, ref<Stream> str);
		virtual ~ImagePlayer();

		// Player methods
		virtual ref<Track> GetTrack();
		virtual void Stop();
		virtual void Start(Time pos, ref<Playback> playback, float speed);
		virtual void Tick(Time currentPosition);
		virtual Time GetNextEvent(Time t);
		virtual void SetOutput(bool enable);
		virtual void Jump(Time t, bool pause);
		virtual void Notify(ref<Object> source, const Interactive::MouseNotification& mo);
		virtual void Notify(ref<Object> source, const Interactive::KeyNotification& kn);
		virtual void Notify(ref<Object> source, const Interactive::FocusNotification& fn);
		virtual void Notify(ref<Object> src, const tj::media::master::MediaMasters::MasterChangedNotification& me);
		virtual void UpdateMasterValues();

	protected:
		void UpdateImage(ref<ImageBlock> block);

		ref<Surface> _surface;
		ref<ImageBlock> _playing;
		ref<ImageTrack> _track;
		ref<Playback> _pb;
		ref<Stream> _stream;
		bool _output;
		bool _down;
};

class ImageStreamPlayer: public StreamPlayer, public Listener<Interactive::MouseNotification> {
	public:
		ImageStreamPlayer(ref<ImagePlugin> p, ref<Talkback> t);
		virtual ~ImageStreamPlayer();
		virtual ref<Plugin> GetPlugin();
		virtual void Message(ref<DataReader> msg, ref<Playback> pb);
		virtual void Notify(ref<Object> src, const Interactive::MouseNotification& mn);

		// Stream messages
		const static int KMessageLoad = 1;
		const static int KMessageHide = 2;
		const static int KMessageUpdate = 3;
		const static int KMessageLoadText = 4;
		const static int KMessageMasterValues = 5;

	protected:	
		ref<ImagePlugin> _plugin;
		ref<Surface> _surface;
		ref<Talkback> _talk;
};

#endif