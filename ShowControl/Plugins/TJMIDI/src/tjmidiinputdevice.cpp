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
using namespace tj::shared;

void CALLBACK MIDIInputCallback(HMIDIIN hMidiIn, UINT msg, DWORD_PTR dwInstance, DWORD_PTR paramA, DWORD_PTR paramB) {
	if(msg==MM_MIM_DATA) {
		MIDIInputDevice* dev = (MIDIInputDevice*)dwInstance;
		if(dev!=0) {
			dev->Message((unsigned int)paramA);
		}
	}
	else if(msg==MM_MIM_CLOSE) {
		MIDIInputDevice* dev = (MIDIInputDevice*)dwInstance;
		if(dev!=0) {
			dev->Connect(false);
		}
	}
}

MIDIInputDevice::~MIDIInputDevice() {
}

WindowsMIDIInputDevice::WindowsMIDIInputDevice(UINT nativeid, const std::wstring& name, ref<input::Dispatcher> disp): _disp(disp) {
	_icon = GC::Hold(new Icon(L"icons/devices/midi-in.png"));
	_min = (HMIDIIN)INVALID_HANDLE_VALUE;
	_nid = nativeid;
	_name = name;
	Connect(true);
}

WindowsMIDIInputDevice::~WindowsMIDIInputDevice() {
	if(_min!=(HMIDIIN)INVALID_HANDLE_VALUE) {
		midiInStop(_min);
		midiInClose(_min);
		_min = (HMIDIIN)INVALID_HANDLE_VALUE;
	}
}

std::wstring WindowsMIDIInputDevice::GetFriendlyName() const {
	return _name;
}

DeviceIdentifier WindowsMIDIInputDevice::GetIdentifier() const {
	return L"@tjmidi:in:"+Stringify(_nid);
}

ref<tj::shared::Icon> WindowsMIDIInputDevice::GetIcon() {
	return _icon;
}

bool WindowsMIDIInputDevice::IsMuted() const {
	return false;
}

void WindowsMIDIInputDevice::SetMuted(bool t) const {
}

void WindowsMIDIInputDevice::Connect(bool c) {
	if(c) {
		midiInOpen(&_min, _nid, (DWORD_PTR)MIDIInputCallback, (DWORD_PTR)this, CALLBACK_FUNCTION);
		midiInStart(_min);
	}
	else {
		midiInStop(_min);
		midiInClose(_min);
		_min = (HMIDIIN)INVALID_HANDLE_VALUE;
	}
}

HMIDIIN WindowsMIDIInputDevice::GetNativeHandle() {
	return _min;
}

// threaded call, called from InputService thread
void WindowsMIDIInputDevice::Message(unsigned int data) {
	unsigned char message = LOBYTE(LOWORD(data));
	unsigned char subChannel = HIBYTE(LOWORD(data));
	unsigned char valuedata = LOBYTE(HIWORD(data));
	unsigned char d = HIBYTE(HIWORD(data));
	float value = Clamp((1.0f / 127.0f) * float(valuedata), 0.0f, 1.0f);

	MIDI::Channel midiChannel = MIDI::DecodeChannel(message);
	MIDI::Message midiMessage = MIDI::DecodeMessage(message);

	if(midiMessage==MIDI::MessageNoteOn || midiMessage==MIDI::MessageNoteOff) {
		_disp->Dispatch(ref<MIDIInputDevice>(this), L"/note/"+Stringify(midiChannel)+L'/'+Stringify(subChannel), Any((double)value));
	}
	else if(midiMessage==MIDI::MessageControlChange) {
		_disp->Dispatch(ref<MIDIInputDevice>(this), L"/cc/"+Stringify(midiChannel)+L'/'+Stringify(subChannel), Any((double)value));
	}
	else {
		Log::Write(L"TJMIDI/InputDevice", L"Unsupported MIDI message received o="+StringifyHex(message)+L" m="+StringifyHex(midiMessage)+L" c="+StringifyHex(midiChannel));
	}
}
