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
#ifndef _TJDMXPLUGIN_H
#define _TJDMXPLUGIN_H

#include "tjdmxpatchable.h"

class DMXPlugin: public OutputPlugin {
	friend class DMXTrack;
	friend class DMXLiveControl;
	friend class DMXPlayer;
	friend class DMXPatchListWnd;
	friend class DMXSettingsToolbarWnd;

	public:
		DMXPlugin();
		virtual ~DMXPlugin();
		virtual std::wstring GetName() const;
		virtual ref<Track> CreateTrack(ref<Playback> pb);
		virtual ref<StreamPlayer> CreateStreamPlayer(ref<Playback> pb, ref<Talkback> talk);
		virtual std::wstring GetVersion() const;
		virtual std::wstring GetAuthor() const;
		virtual void Message(ref<DataReader> code);
		virtual std::wstring GetFriendlyName() const;
		virtual std::wstring GetFriendlyCategory() const;
		virtual std::wstring GetDescription() const;
		virtual void GetRequiredFeatures(std::list<std::wstring>& fs) const;

		virtual ref<DMXMacro> CreateMacro(std::wstring address, DMXSource source);
		virtual int GetChannelResult(int channel);
		strong<DMXController> GetController();
		virtual ref<Pane> GetSettingsWindow(ref<PropertyGridProxy> pg);
		virtual void Reset();

		virtual void Load(TiXmlElement* you, bool showSpecific);
		virtual void Save(TiXmlElement* you, bool showSpecific);
		static ref<Property> CreateAddressProperty(const std::wstring& name, ref<Inspectable> holder, std::wstring* address);

		// Patch-set saving/loading
		void SavePatchSet(const std::wstring& fn);
		void LoadPatchSet(const std::wstring& fn);

		// Patchables
		void AddPatchable(ref<DMXPatchable> pt);

	protected:
		CriticalSection _tlLock;
		void SortTrackList();
		std::vector< weak<DMXPatchable> > _tracks;
};

strong<DMXController> GetController();

#endif