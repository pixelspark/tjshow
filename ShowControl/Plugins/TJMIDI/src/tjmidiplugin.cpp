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
#include "../include/tjmidi.h"

MIDIPlugin::MIDIPlugin() {
}

MIDIPlugin::~MIDIPlugin() {
}

void MIDIPlugin::GetRequiredFeatures(std::list<std::wstring>& fts) const {
	fts.push_back(L"MIDI");
}

std::wstring MIDIPlugin::GetVersion() const {
	std::wostringstream os;
	os << __DATE__ << " @ " << __TIME__;
	#ifdef UNICODE
	os << L" Unicode";
	#endif

	#ifdef NDEBUG
	os << " Release";
	#endif

	return os.str();
}

void MIDIPlugin::Load(TiXmlElement* you, bool showSpecific) {
	if(showSpecific) {
		TiXmlElement* serial = you->FirstChildElement("serial");
		while(serial!=0) {
			std::wstring outport = LoadAttributeSmall<std::wstring>(serial, "port", L"");
			if(outport.length()>0) {
				_serialOutPort.push_back(outport);
			}
			serial = serial->NextSiblingElement("serial");
		}
	}
}
void MIDIPlugin::Save(TiXmlElement* you, bool showSpecific) {
	if(showSpecific) {
		std::vector< std::wstring>::const_iterator it = _serialOutPort.begin();
		while(it!=_serialOutPort.end()) {
			TiXmlElement serial("serial");
			SaveAttributeSmall(&serial, "port", *it);
			you->InsertEndChild(serial);
			++it;
		}
	}
}

void MIDIPlugin::GetDevices(ref<input::Dispatcher> dp, std::vector< ref<Device> >& devs) {
	ThreadLock lock(&_devicesLock);
	
	#ifdef _WIN32
		// Add output devices
		UINT numDevices = midiOutGetNumDevs();
		for(unsigned int a=0;a<numDevices && numDevices>0;a++) {
			MIDIOUTCAPS caps;
			midiOutGetDevCaps(a, &caps, sizeof(caps));
			
			if(_outDevices.find(a)!=_outDevices.end() && _outDevices[a]->GetFriendlyName()==caps.szPname) {
				// Device is probably already discovered
				devs.push_back(_outDevices[a]);
			}
			else {
				ref<WindowsMIDIOutputDevice> midv = GC::Hold(new WindowsMIDIOutputDevice(a, Stringify(caps.szPname)));
				devs.push_back(midv);
				_outDevices[a] = midv;
			}
		}

		// Add input devices
		numDevices = midiInGetNumDevs();
		for(unsigned int a=0;a<numDevices && numDevices>0;a++) {
			MIDIINCAPS caps;
			midiInGetDevCaps(a,&caps, sizeof(caps));

			if(_inDevices.find(a)!=_inDevices.end() && _inDevices[a]->GetFriendlyName()==caps.szPname) {
				// Device is probably already discovered
				devs.push_back(_inDevices[a]);
			}
			else {
				ref<WindowsMIDIInputDevice> midv = GC::Hold(new WindowsMIDIInputDevice(a, Stringify(caps.szPname), dp));
				devs.push_back(midv);
				_inDevices[a] = midv;
			}			
		}
	#endif	
	
	// Build serial devices list if we haven't done that already
	if(_serialDevices.size()==0) {
		std::vector<std::wstring>::const_iterator it = _serialOutPort.begin();
		while(it!=_serialOutPort.end()) {
			ref<MIDISerialOutputDevice> msod = GC::Hold(new MIDISerialOutputDevice(TL(midi_serial_out_device), *it));
			_serialDevices.push_back(msod);
			++it;
		}
	}

	// Add created serial devices to the list
	std::vector< ref<Device> >::iterator it = _serialDevices.begin();
	while(it!=_serialDevices.end()) {
		devs.push_back(*it);
		++it;
	}
}

std::wstring MIDIPlugin::GetAuthor() const {
	return std::wstring(L"Tommy van der Vorst");
}

std::wstring MIDIPlugin::GetName() const {
	return std::wstring(L"MIDI Input");
}
	
std::wstring MIDIPlugin::GetFriendlyName() const {
	return TL(midi_input_friendly_name);
}

std::wstring MIDIPlugin::GetFriendlyCategory() const {
	return TL(midi_category);
}


std::wstring MIDIPlugin::GetDescription() const {
	return TL(midi_input_description);
}
