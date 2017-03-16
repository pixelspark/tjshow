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

MIDIOutputDevice::~MIDIOutputDevice() {
}

void MIDIOutputDevice::SendMSCCommand(MSC::DeviceID deviceID, MSC::CommandFormat format, MSC::Command command, unsigned int dataLength, const char* data) {
	if(command==MSC::None) {
		return; // 0x00 command is meant for extensions, TJShow uses it as 'no command' identifier; so just never send it
	}

	unsigned char sysex[MSC::KMaxCommandLength];
	unsigned int len = 0;

	sysex[0] = 0xF0;
	sysex[1] = 0x7F;
	sysex[2] = MIDI::EnsureDataByte(deviceID);
	sysex[3] = 0x02;
	sysex[4] = MIDI::EnsureDataByte(format);
	sysex[5] = MIDI::EnsureDataByte(command);
	len = 6;

	if(dataLength > 0 && data != 0 && (dataLength + len + 1)<MSC::KMaxCommandLength) {
		for(unsigned int a=0;a<dataLength;a++) {
			sysex[len+a] = data[a];
		}
		len += dataLength;
	}

	sysex[len] = 0xF7;
	++len;

	/*std::wostringstream friendly;
	for(unsigned int a=0;a<len;a++) {
		friendly << std::hex << sysex[a] << L' ';
	}
	Log::Write(L"TJMIDI/MIDIOutputDevice", L"Sent sysex for MSC: "+friendly.str());*/

	SendSysEx(sysex, len);
}

#ifdef _WIN32
	WindowsMIDIOutputDevice::WindowsMIDIOutputDevice(unsigned int id, const std::wstring& name): _nid(id), _friendly(name), _out(0) {
		_icon = GC::Hold(new Icon(L"icons/devices/midi.png"));
		ClearTrack();

		for(int a=0;a<MIDI::KChannelCount;a++) {
			for(int b=0;b<=MIDI::KMaxControlID;b++) {
				_trackedControls[a][b] = 0;
			}
			_trackedProgram[a] = 0;
		}
	}

	WindowsMIDIOutputDevice::~WindowsMIDIOutputDevice() {
		if(_out!=0) {
			midiOutClose(_out);
			_out = 0;
		}
	}

	void WindowsMIDIOutputDevice::ClearTrack() {
		for(int a=0;a<MIDI::KChannelCount;a++) {
			for(int b=0;b<MIDI::KNoteCount;b++) {
				_tracked[a][b] = 0;
			}
		}
	}

	std::wstring WindowsMIDIOutputDevice::GetFriendlyName() const {
		return _friendly;
	}

	DeviceIdentifier WindowsMIDIOutputDevice::GetIdentifier() const {
		return L"@tjmidi:out:"+Stringify(_nid);
	}

	ref<tj::shared::Icon> WindowsMIDIOutputDevice::GetIcon() {
		return _icon;
	}

	bool WindowsMIDIOutputDevice::IsMuted() const {
		return false;
	}

	void WindowsMIDIOutputDevice::SetMuted(bool t) const {
	}

	HMIDIOUT WindowsMIDIOutputDevice::GetMIDIOut() {
		ThreadLock lock(&_lock);

		if(_out==0) {
			int r = midiOutOpen(&_out, _nid, 0, 0, CALLBACK_NULL);
			if(r!=MMSYSERR_NOERROR) {
				Log::Write(L"TJMIDI/OutputDevice", L"Could not open output MIDI device '"+_friendly+L"': error "+StringifyHex(r));
			}
		}

		return _out;
	}

	void WindowsMIDIOutputDevice::SendMMCGoto(MIDI::Channel deviceID, Time t) {
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

	void WindowsMIDIOutputDevice::SendProgramChange(MIDI::Channel channel, MIDI::ProgramID program) {
		if(channel > MIDI::KMaxChannel) {
			Throw(L"Channel value too large", ExceptionTypeError);
		}

		if(program > MIDI::KMaxProgramID) {
			Throw(L"Control id too large", ExceptionTypeError);
		}

		SendShort(MIDI::MessageProgramChange, channel, program, 0);
		_trackedProgram[channel] = program;
	}

	MIDI::ProgramID WindowsMIDIOutputDevice::GetTrackedProgram(MIDI::Channel channel) {
		if(channel > MIDI::KMaxChannel) {
			Throw(L"Channel value too large", ExceptionTypeError);
		}
		return _trackedProgram[channel];
	}

	std::wstring WindowsMIDIOutputDevice::GetError(int r) {
		wchar_t* buffer = new wchar_t[256];
		midiOutGetErrorText(r, buffer, 255);
		std::wstring data(buffer);
		delete[] buffer;
		return data;
	}

	void WindowsMIDIOutputDevice::SendShort(MIDI::Message message, MIDI::Channel channel, unsigned char a, unsigned char b) {
		ThreadLock lock(&_lock);
		// Limit a and b parameters to range [0,127]
		a &= 0x7F;
		b &= 0x7F;

		union { 
			DWORD dword; 
			BYTE data[4]; 
		} u;
	 
		u.data[0] = MIDI::MessageOnChannel(message,channel);
		u.data[1] = a;
		u.data[2] = b;
		u.data[3] = 0;
		int r = midiOutShortMsg(GetMIDIOut(), u.dword);
		if(r!=MMSYSERR_NOERROR) {
			Log::Write(L"TJMIDI/OutputDevice", L"midiOutShortMsg failed: "+GetError(r));
		}
	}

	void WindowsMIDIOutputDevice::SendSysEx(unsigned char* sysex, unsigned int len) {
		unsigned int size = len*sizeof(char);
		ThreadLock lock(&_lock);

		MIDIHDR header;
		HMIDIOUT handle = GetMIDIOut();

		HGLOBAL buffer = GlobalAlloc(GHND, size);
		if (buffer) {
			/* Lock that buffer and store pointer in MIDIHDR */
			header.lpData = (CHAR*)GlobalLock(buffer);
			if(header.lpData!=0) {
				header.dwBufferLength = size;
				header.dwFlags = 0;

				/* Prepare the buffer and MIDIHDR */
				if (midiOutPrepareHeader(handle,  &header, sizeof(MIDIHDR))==MMSYSERR_NOERROR) {
					/* Copy the SysEx message to the buffer */
					memcpy(header.lpData, &sysex[0], size);

					/* Output the SysEx message */
					if(midiOutLongMsg(handle, &header, sizeof(MIDIHDR))!=MMSYSERR_NOERROR) {
						// TODO print some error
						Log::Write(L"TJMIDI/OutputDevice", L"midiOutLongMsg failed");
					}

					/* Unprepare the buffer and MIDIHDR */
					while(MIDIERR_STILLPLAYING == midiOutUnprepareHeader(handle, &header, sizeof(MIDIHDR))) {
						Sleep(0); // yield to other thread to wait a little
					}
				}
				else {
					Log::Write(L"TJMIDI/OutputDevice", L"midiOutPrepareHeader failed");
				}

				/* Unlock the buffer */
				GlobalUnlock(buffer);
			}

			/* Free the buffer */
			GlobalFree(buffer);
		}
	}

	void WindowsMIDIOutputDevice::SendMMC(MIDI::Channel deviceID, MMC::Command command) {
		unsigned char sysex[] = {0xF0, 0x7F, (unsigned char)deviceID, (unsigned char)MIDI::SubTypeMMCCommand, (char)command, 0xF7};
		SendSysEx(sysex, sizeof(sysex)/sizeof(char));
	}

	void WindowsMIDIOutputDevice::SendControlChange(MIDI::Channel channel, MIDI::ControlID control, MIDI::ControlValue value) {
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

	void WindowsMIDIOutputDevice::SendNoteOn(MIDI::Channel channel, MIDI::NoteID note, MIDI::Velocity velocity) {
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

	void WindowsMIDIOutputDevice::SendNoteOff(MIDI::Channel channel, MIDI::NoteID note) {
		if(channel > MIDI::KMaxChannel) {
			Throw(L"Channel value too large", ExceptionTypeError);
		}

		if(note > MIDI::KMaxNoteID) {
			Throw(L"Note value too large", ExceptionTypeError);
		}

		SendShort(MIDI::MessageNoteOff, channel, note, 0);
		_tracked[channel][note] = 0;
	}

	void WindowsMIDIOutputDevice::SendAllNotesOff(MIDI::Channel channel) {
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

	MIDI::Velocity WindowsMIDIOutputDevice::GetTrackedNoteStatus(MIDI::Channel channel, MIDI::NoteID note) {
		if(channel > MIDI::KMaxChannel) {
			Throw(L"Channel value too large", ExceptionTypeError);
		}

		if(note > MIDI::KMaxNoteID) {
			Throw(L"Note value too large", ExceptionTypeError);
		}

		return _tracked[channel][note];
	}

	MIDI::ControlValue WindowsMIDIOutputDevice::GetTrackedControlStatus(MIDI::Channel channel, MIDI::ControlID control) {
		if(channel > MIDI::KMaxChannel) {
			Throw(L"Channel value too large", ExceptionTypeError);
		}

		if(control > MIDI::KMaxControlID) {
			Throw(L"Control id too large", ExceptionTypeError);
		}

		return _trackedControls[channel][control];
	}
#endif