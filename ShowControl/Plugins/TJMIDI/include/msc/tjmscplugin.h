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
#ifndef _TJMSCPLUGIN_H
#define _TJMSCPLUGIN_H

class MSCPlugin: public OutputPlugin {
	public:
		MSCPlugin();
		virtual ~MSCPlugin();
		virtual std::wstring GetName() const;
		virtual std::wstring GetFriendlyName() const;
		virtual std::wstring GetFriendlyCategory() const;
		virtual void GetRequiredFeatures(std::list<std::wstring>& fts) const;
		virtual std::wstring GetVersion() const;
		virtual std::wstring GetAuthor() const;
		virtual std::wstring GetDescription() const;
		virtual ref<Track> CreateTrack(ref<Playback> playback);
		virtual ref<StreamPlayer> CreateStreamPlayer(ref<Playback> playback, ref<Talkback> talk);
};

class MSCCue: public Serializable, public virtual Inspectable {
	public:
		MSCCue(Time pos = 0);
		virtual ~MSCCue();

		virtual void Load(TiXmlElement* you);
		virtual void Save(TiXmlElement* me);
		virtual void Move(Time t, int h);
		virtual ref<MSCCue> Clone();
		virtual ref<PropertySet> GetProperties(ref<Playback> pb, strong< CueTrack<MSCCue> > track);
		Time GetTime() const;
		void SetTime(Time t);
		virtual void Paint(tj::shared::graphics::Graphics* g, ref<Theme> theme, float pixelLeft, unsigned int y, unsigned int h, ref< CueTrack<MSCCue> > track, bool focus);
		virtual void Fire(ref<MIDIOutputDevice> mo, const MSC::DeviceID& did, const MSC::CommandFormat& cf, const std::wstring& defList, const std::wstring& defPath);

	protected:
		bool NeedToSendCue() const;
		Time _time;
		MSC::Command _command;
		MSC::CueID _cue;
		std::wstring _description;
};


class MSCTrack: public CueTrack<MSCCue> {
	public:
		MSCTrack(ref<Playback> pb);
		virtual ~MSCTrack();
		virtual std::wstring GetTypeName() const;
		virtual ref<Player> CreatePlayer(ref<Stream> str);
		virtual Flags<RunMode> GetSupportedRunModes();
		virtual ref<PropertySet> GetProperties();
		virtual ref<LiveControl> CreateControl(ref<Stream> str);
		virtual void Save(TiXmlElement* parent);
		virtual void Load(TiXmlElement* you);
		ref<MIDIOutputDevice> GetOutputDevice();
		MSC::DeviceID GetDeviceID() const;
		virtual std::wstring GetEmptyHintText() const;
		MSC::CommandFormat GetCommandFormat() const;
		const std::wstring& GetDefaultCuePath() const;
		const std::wstring& GetDefaultCueList() const;
		virtual bool IsExpandable();
		virtual Pixels GetHeight();
		virtual void SetExpanded(bool t);
		virtual bool IsExpanded();

	protected:
		bool _expanded;
		PatchIdentifier _out;
		ref<Playback> _pb;
		MSC::DeviceID _deviceID;
		MSC::CommandFormat _commandFormat;

		std::wstring _defaultCueList;
		std::wstring _defaultCuePath;
};

class MSCPlayer: public Player {
	public:
		MSCPlayer(ref<MSCTrack> tr, ref<Stream> str);
		virtual ~MSCPlayer();

		ref<Track> GetTrack();
		virtual void Stop();
		virtual void Start(Time pos, ref<Playback> playback, float speed);
		virtual void Tick(Time currentPosition);
		virtual void Jump(Time newT, bool paused);
		virtual void SetOutput(bool enable);
		virtual Time GetNextEvent(Time t);

	protected:
		ref<MSCTrack> _track;
		ref<MIDIOutputDevice> _out;
		ref<Stream> _stream;
		bool _output;
		Time _last;
};

#endif