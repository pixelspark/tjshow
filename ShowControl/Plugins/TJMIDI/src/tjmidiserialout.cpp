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
#include <windows.h>

using namespace tj::shared;

MIDISerialOutputDevice::MIDISerialOutputDevice(const std::wstring& friendlyName, const std::wstring& file): _friendly(friendlyName), _port(file) {
	_icon = GC::Hold(new Icon(L"icons/devices/midi.png"));

	#ifdef _WIN32
		_out = CreateFile(file.c_str(), GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		if(_out==INVALID_HANDLE_VALUE) {
			Log::Write(L"TJMIDI/MIDISerialOutputDevice", L"Could not open serial port '"+file+L"'");
		}

		COMMTIMEOUTS cto;
		cto.ReadIntervalTimeout = 1;
		cto.ReadTotalTimeoutConstant = 100;
		cto.ReadTotalTimeoutMultiplier = 1000;
		cto.WriteTotalTimeoutConstant = 100;
		cto.WriteTotalTimeoutMultiplier = 1000;
		if(!SetCommTimeouts(_out,&cto)) {
			Log::Write(L"TJMIDI/MIDISerialOutputDevice", L"Could not set communication time-outs for serial port '"+file+L"'");
		}

		DCB dcb;
		memset(&dcb,0,sizeof(dcb));
		dcb.DCBlength = sizeof(dcb);
		dcb.BaudRate = 19200;
		dcb.fBinary = 1;
		dcb.fDtrControl = DTR_CONTROL_DISABLE;
		dcb.fRtsControl = RTS_CONTROL_DISABLE;
		dcb.Parity = NOPARITY;
		dcb.StopBits = ONESTOPBIT;
		dcb.ByteSize = 8;

		if(!SetCommState(_out,&dcb)) {
			Log::Write(L"TJMIDI/MIDISerialOutputDevice", L"Could not set communication state for serial port '"+file+L"'");
		}
	#endif

	// Clear tracking buffers
	for(int a=0;a<MIDI::KChannelCount;a++) {
		for(int b=0;b<MIDI::KNoteCount;b++) {
			_tracked[a][b] = 0;
		}
		for(int c=0;c<=MIDI::KMaxControlID;c++) {
			_trackedControls[a][c] = 0;
		}
		_trackedProgram[a] = 0;
	}
}

MIDISerialOutputDevice::~MIDISerialOutputDevice() {
	#ifdef _WIN32
		CloseHandle(_out);
	#endif
}

std::wstring MIDISerialOutputDevice::GetFriendlyName() const {
	return _friendly+_port;
}

DeviceIdentifier MIDISerialOutputDevice::GetIdentifier() const {
	return L"@tjmidi:out:serial:"+_port;
}

ref<tj::shared::Icon> MIDISerialOutputDevice::GetIcon() {
	return _icon;
}

bool MIDISerialOutputDevice::IsMuted() const {
	return false;
}

void MIDISerialOutputDevice::SetMuted(bool t) const {
}

void MIDISerialOutputDevice::SendMMC(MIDI::DeviceID deviceID, MMC::Command command) {
	unsigned char sysex[] = {0xF0, 0x7F, (unsigned char)deviceID, (unsigned char)MIDI::SubTypeMMCCommand, (char)command, 0xF7};
	SendSysEx(sysex, sizeof(sysex)/sizeof(char));
}

void MIDISerialOutputDevice::SendMMCGoto(MIDI::DeviceID deviceID, Time t) {
	// We use 25 fps SMTPE for now
	const static int FrameMS = 40; // 1000/25

	int time = abs(t.ToInt());
	float ft = float(time);
	float seconds = ft/1000.0f;
	unsigned int frame = (time%FrameMS);
	
	unsigned char mseconds = (unsigned char)floor(fmod(seconds,60.0f));
	unsigned char minutes = (unsigned char)floor(fmod(seconds/60.0f, 60.0f));
	unsigned hours = (unsigned char)floor(seconds/3600.0f);

	// TODO make this more precise (e.g. replace the 0x00's with some subframe stuff)
	unsigned char sysex[] = {0xF0, 0x7F, (unsigned char)deviceID, (unsigned char)MIDI::SubTypeMMCCommand, 0x44, 0x06, 0x01, hours, minutes, mseconds, frame, 0x00, 0xF7};

	SendSysEx(sysex, sizeof(sysex)/sizeof(char));
}

void MIDISerialOutputDevice::SendSysEx(unsigned char* msg, unsigned int length) {
	ThreadLock lock(&_lock);

	#ifdef _WIN32
		if(WriteFile(_out, msg, length, NULL, NULL)==0) {
			Log::Write(L"TJMIDI/MIDISerialOutputDevice", L"Could not write sysex message to file");
		}
	#endif
}

void MIDISerialOutputDevice::SendShort(MIDI::Message message, MIDI::Channel channel, unsigned char a, unsigned char b) {
	ThreadLock lock(&_lock);
	// Limit a and b parameters to range [0,127]
	a &= 0x7F;
	b &= 0x7F;

	BYTE data[4];
	data[0] = MIDI::MessageOnChannel(message,channel);
	data[1] = a;
	data[2] = b;
	data[3] = 0;
	
	DWORD written = 0;
	#ifdef _WIN32
		if(WriteFile(_out, (void*)&data[0], 3, &written, NULL)==0) {
			Log::Write(L"TJMIDI/MIDISerialOutputDevice", L"Could not write short message to file");
		}
	#endif
}

void MIDISerialOutputDevice::SendProgramChange(MIDI::Channel channel, MIDI::ProgramID program) {
	if(channel > MIDI::KMaxChannel) {
		Throw(L"Channel value too large", ExceptionTypeError);
	}

	if(program > MIDI::KMaxProgramID) {
		Throw(L"Control id too large", ExceptionTypeError);
	}

	SendShort(MIDI::MessageProgramChange, channel, program, 0);
	_trackedProgram[channel] = program;
}

void MIDISerialOutputDevice::SendControlChange(MIDI::Channel channel, MIDI::ControlID control, MIDI::ControlValue value) {
	if(channel > MIDI::KMaxChannel) {
		Throw(L"Channel value too large", ExceptionTypeError);
	}

	if(control > MIDI::KMaxControlID) {
		Throw(L"Control id too large", ExceptionTypeError);
	}

	if(value > MIDI::KMaxControlValue) {
		Throw(L"Value too large", ExceptionTypeError);
	}

	SendShort(MIDI::MessageControlChange, channel, control, value);
	_trackedControls[channel][control] = value;
}

void MIDISerialOutputDevice::SendNoteOn(MIDI::Channel channel, MIDI::NoteID note, MIDI::Velocity velocity) {
if(channel > MIDI::KMaxChannel) {
		Throw(L"Channel value too large", ExceptionTypeError);
	}

	if(note > MIDI::KMaxNoteID) {
		Throw(L"Note value too large", ExceptionTypeError);
	}

	if(velocity > MIDI::KMaxVelocity) {
		Throw(L"Velocity value too large", ExceptionTypeError);
	}

	SendShort(MIDI::MessageNoteOn, channel, note, velocity);
	_tracked[channel][note] = velocity;
}

void MIDISerialOutputDevice::SendNoteOff(MIDI::Channel channel, MIDI::NoteID note) {
	if(channel > MIDI::KMaxChannel) {
		Throw(L"Channel value too large", ExceptionTypeError);
	}

	if(note > MIDI::KMaxNoteID) {
		Throw(L"Note value too large", ExceptionTypeError);
	}

	SendShort(MIDI::MessageNoteOff, channel, note, 0);
	_tracked[channel][note] = 0;
}

void MIDISerialOutputDevice::SendAllNotesOff(MIDI::Channel channel) {
	if(channel > MIDI::KMaxChannel) {
		Throw(L"Channel value too large", ExceptionTypeError);
	}

	for(MIDI::NoteID n = 0; n <= 127; n++) {
		SendNoteOff(channel, n);
	}

	for(int a=0;a<MIDI::KNoteCount;a++) {
		_tracked[channel][a] = 0;
	}
}

MIDI::Velocity MIDISerialOutputDevice::GetTrackedNoteStatus(MIDI::Channel channel, MIDI::NoteID note) {
	if(channel > MIDI::KMaxChannel) {
		Throw(L"Channel value too large", ExceptionTypeError);
	}

	if(note > MIDI::KMaxNoteID) {
		Throw(L"Note value too large", ExceptionTypeError);
	}

	return _tracked[channel][note];
}

MIDI::ControlValue MIDISerialOutputDevice::GetTrackedControlStatus(MIDI::Channel channel, MIDI::ControlID control) {
	if(channel > MIDI::KMaxChannel) {
		Throw(L"Channel value too large", ExceptionTypeError);
	}

	if(control > MIDI::KMaxControlID) {
		Throw(L"Control id too large", ExceptionTypeError);
	}

	return _trackedControls[channel][control];
}

MIDI::ProgramID MIDISerialOutputDevice::GetTrackedProgram(MIDI::Channel channel) {
	if(channel > MIDI::KMaxChannel) {
		Throw(L"Channel value too large", ExceptionTypeError);
	}
	return _trackedProgram[channel];
}
