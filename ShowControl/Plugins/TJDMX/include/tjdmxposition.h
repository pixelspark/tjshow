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
#ifndef _TJTRACKSPOT_H
#define _TJTRACKSPOT_H

#include <TJShow/include/extra/tjextra.h>

class DMXPositionPlugin: public OutputPlugin {
	public:
		DMXPositionPlugin(ref<DMXPlugin> plugin);
		virtual ~DMXPositionPlugin();
		virtual std::wstring GetName() const;
		virtual ref<Track> CreateTrack(ref<Playback> pb);
		virtual ref<StreamPlayer> CreateStreamPlayer(ref<Playback> pb, ref<Talkback> talk);
		virtual std::wstring GetVersion() const;
		virtual std::wstring GetAuthor() const;
		virtual std::wstring GetFriendlyName() const;
		virtual std::wstring GetFriendlyCategory() const;
		virtual std::wstring GetDescription() const;
		virtual void GetRequiredFeatures(std::list<std::wstring>& fts) const;

	protected:
		ref<DMXPlugin> _plugin;
};

struct DMXPositionMacro {
	ref<DMXMacro> _pan;
	ref<DMXMacro> _tilt;
};

class DMXPositionTrack: public MultifaderTrack {
	friend class DMXPositionWnd; 
	friend class DMXPositionPlayer;
	friend class DMXPositionEditorWnd;
	friend class DMXPositionPatchable;

	public:
		DMXPositionTrack(ref<DMXPlugin> dp);
		virtual ~DMXPositionTrack();
		virtual std::wstring GetTypeName() const;
		virtual ref<Player> CreatePlayer(ref<Stream> str);
		virtual Flags<RunMode> GetSupportedRunModes();

		virtual ref<PropertySet> GetProperties();
		virtual ref<LiveControl> CreateControl(ref<Stream> str);
		virtual void InsertFromControl(Time t, ref<LiveControl> control, bool fade);

		virtual DMXPositionMacro GetMacro(DMXSource ds, ref<Playback> pb);

		virtual void Load(TiXmlElement* you);
		virtual void Save(TiXmlElement* parent);

		virtual void OnSetInstanceName(const std::wstring& name);
		virtual void OnSetID(const TrackID& tid);

		enum {
			KFaderPan = 1,
			KFaderTilt,
		};

	protected:
		virtual void OnCreated();

		ref<DMXPlugin> _plugin;
		ref<DMXPatchable> _xPatchable, _yPatchable;
		std::wstring _panAddress, _tiltAddress;
		std::wstring _instanceName;
		TrackID _tid;

		const static Time DefaultPathRange;
};

class DMXPositionPatchable: public DMXPatchable {
	public:
		DMXPositionPatchable(ref<DMXPositionTrack> dp, bool x);
		virtual ~DMXPositionPatchable();

		virtual void SetDMXAddress(const std::wstring& address);
		virtual std::wstring GetDMXAddress() const;
		virtual std::wstring GetTypeName() const;
		virtual std::wstring GetInstanceName() const;
		virtual TrackID GetID() const;
		virtual bool GetResetOnStop() const;
		virtual void SetResetOnStop(bool t);
		virtual int GetSliderType();
		virtual int GetSubChannelID() const;
		virtual ref<Property> GetPropertyFor(DMXPatchable::PropertyID pid);

	protected:
		weak<DMXPositionTrack> _dp;
		bool _x;
};

class DMXPositionPlayer: public Player {
	public:
		DMXPositionPlayer(ref<DMXPositionTrack> track, ref<Stream> str);
		virtual ~DMXPositionPlayer();
		virtual ref<Track> GetTrack();
		virtual void Stop();
		virtual void Start(Time pos, ref<Playback> pb, float speed); 
		virtual void Tick(Time currentPosition);
		virtual void Jump(Time t, bool paused);
		virtual void SetOutput(bool e);
		virtual Time GetNextEvent(Time t);
		

	protected:
		bool _output;
		ref<DMXPositionTrack> _track;
		DMXPositionMacro _macro;
		ref<Stream> _stream;
};

#endif