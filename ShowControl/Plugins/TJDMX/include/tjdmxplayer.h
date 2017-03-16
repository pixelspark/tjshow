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
#ifndef _TJDMXPLAYER_H
#define _TJDMXPLAYER_H

class DMXTrack;

class DMXPlayer: public Player {
	public:
		DMXPlayer(ref<DMXTrack> track, ref<Stream> str);
		virtual ~DMXPlayer();
		virtual ref<Track> GetTrack();
		virtual void Stop();
		virtual void Start(Time pos, ref<Playback> pb, float c); 
		virtual void Tick(Time currentPosition);
		virtual void Jump(Time t, bool paused);
		virtual void SetOutput(bool enable);
		virtual Time GetNextEvent(Time t);

	protected:
		ref<DMXTrack> _track;
		ref<DMXMacro> _macro;
		ref<Stream> _stream;
		ref<Playback> _pb;
		bool _outputEnabled;
		unsigned char _sentValue;
};

class DMXStreamPlayer: public StreamPlayer {
	public:
		DMXStreamPlayer(ref<DMXPlugin> plug);
		virtual ~DMXStreamPlayer();

		virtual ref<Plugin> GetPlugin();
		virtual void Message(ref<DataReader> msg, HWND videoParent);
		
	protected:
		ref<DMXPlugin> _plugin;
};

#endif