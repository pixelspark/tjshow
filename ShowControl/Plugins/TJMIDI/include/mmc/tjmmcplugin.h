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
#ifndef _TJMMCPLUGIN_H
#define _TJMMCPLUGIN_H

class MMCPlugin: public OutputPlugin {
	public:
		MMCPlugin();
		virtual ~MMCPlugin();
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

class MMCTrack: public Track {
	public:
		MMCTrack(ref<Playback> pb);
		virtual ~MMCTrack();
		virtual std::wstring GetTypeName() const;
		virtual ref<Player> CreatePlayer(ref<Stream> str);
		virtual Flags<RunMode> GetSupportedRunModes();
		virtual ref<Item> GetItemAt(Time position, unsigned int h, bool rightClick, int th, float pixelsPerMs);
		virtual void Paint(tj::shared::graphics::Graphics* g, Time position, float pixelsPerMs, unsigned int x, unsigned int y, unsigned int h, Time start, Time end);
		virtual ref<PropertySet> GetProperties();
		virtual ref<LiveControl> CreateControl(ref<Stream> str);
		virtual void Save(TiXmlElement* parent);
		virtual void Load(TiXmlElement* you);
		ref<MIDIOutputDevice> GetOutputDevice();
		MIDI::DeviceID GetDeviceID() const;

	protected:
		PatchIdentifier _out;
		ref<Playback> _pb;
		MIDI::DeviceID _deviceID;
};

#endif