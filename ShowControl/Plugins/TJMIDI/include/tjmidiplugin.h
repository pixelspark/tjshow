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
#ifndef _TJMIDIPLUGIN_H
#define _TJMIDIPLUGIN_H

class MIDIPlugin: public InputPlugin {
	public:
		MIDIPlugin();
		virtual ~MIDIPlugin();
		virtual std::wstring GetName() const;

		/* This returns a localized (same as user language) name for this track. */
		virtual std::wstring GetFriendlyName() const;
		virtual std::wstring GetFriendlyCategory() const;
		virtual std::wstring GetVersion() const;
		virtual std::wstring GetAuthor() const;
		virtual std::wstring GetDescription() const;
		virtual void GetDevices(ref<input::Dispatcher> dp, std::vector< ref<Device> >& devs);
		virtual void Load(TiXmlElement* you, bool showSpecific);
		virtual void Save(TiXmlElement* you, bool showSpecific);
		virtual void GetRequiredFeatures(std::list<std::wstring>& fts) const;

	protected:
		CriticalSection _devicesLock;
		std::map<int, ref<Device> > _outDevices;
		std::map<int, ref<Device> > _inDevices;

		// Serial devices
		std::vector< ref<Device> > _serialDevices;
		std::vector<std::wstring> _serialOutPort;
};

#endif
