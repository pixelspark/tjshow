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
#ifndef _TJMIDIINPUTDEVICE_H
#define _TJMIDIINPUTDEVICE_H

class MIDIInputDevice: public virtual tj::shared::Object, public Device {
	public:
		virtual ~MIDIInputDevice();
		virtual void Message(unsigned int data) = 0; // threaded call
		virtual void Connect(bool t) = 0;
};

class MIDIOutputDevice: public Device {
	public:
		virtual ~MIDIOutputDevice();

		virtual void SendMMC(MIDI::DeviceID deviceID, MMC::Command command) = 0;
		virtual void SendMMCGoto(MIDI::DeviceID deviceID, Time t) = 0;

		virtual void SendMSCCommand(MSC::DeviceID deviceID, MSC::CommandFormat format, MSC::Command command, unsigned int dataLength = 0, const char* data = 0);
		
		virtual void SendSysEx(unsigned char* msg, unsigned int length) = 0;
		virtual void SendShort(MIDI::Message message, MIDI::Channel channel, unsigned char a, unsigned char b) = 0;
		virtual void SendProgramChange(MIDI::Channel channel, MIDI::ProgramID program) = 0;
		virtual void SendControlChange(MIDI::Channel channel, MIDI::ControlID control, MIDI::ControlValue value) = 0;
		virtual void SendNoteOn(MIDI::Channel channel, MIDI::NoteID note, MIDI::Velocity velocity) = 0;
		virtual void SendNoteOff(MIDI::Channel channel, MIDI::NoteID note) = 0;
		virtual void SendAllNotesOff(MIDI::Channel channel) = 0;

		virtual MIDI::Velocity GetTrackedNoteStatus(MIDI::Channel channel, MIDI::NoteID note) = 0;
		virtual MIDI::ControlValue GetTrackedControlStatus(MIDI::Channel channel, MIDI::ControlID control) = 0;
		virtual MIDI::ProgramID GetTrackedProgram(MIDI::Channel channel) = 0;
};

class MIDISerialOutputDevice: public MIDIOutputDevice {
	public:
		MIDISerialOutputDevice(const std::wstring& friendlyName, const std::wstring& file);
		virtual ~MIDISerialOutputDevice();
		virtual std::wstring GetFriendlyName() const;
		virtual DeviceIdentifier GetIdentifier() const;
		virtual ref<tj::shared::Icon> GetIcon();
		virtual bool IsMuted() const;
		virtual void SetMuted(bool t) const;

		virtual void SendMMC(MIDI::DeviceID deviceID, MMC::Command command);
		virtual void SendMMCGoto(MIDI::DeviceID deviceID, Time t);
		virtual void SendSysEx(unsigned char* msg, unsigned int length);
		virtual void SendShort(MIDI::Message message, MIDI::Channel channel, unsigned char a, unsigned char b);
		virtual void SendProgramChange(MIDI::Channel channel, MIDI::ProgramID program);
		virtual void SendControlChange(MIDI::Channel channel, MIDI::ControlID control, MIDI::ControlValue value);
		virtual void SendNoteOn(MIDI::Channel channel, MIDI::NoteID note, MIDI::Velocity velocity);
		virtual void SendNoteOff(MIDI::Channel channel, MIDI::NoteID note);
		virtual void SendAllNotesOff(MIDI::Channel channel);

		virtual MIDI::Velocity GetTrackedNoteStatus(MIDI::Channel channel, MIDI::NoteID note);
		virtual MIDI::ControlValue GetTrackedControlStatus(MIDI::Channel channel, MIDI::ControlID control);
		virtual MIDI::ProgramID GetTrackedProgram(MIDI::Channel channel);

	protected:
		CriticalSection _lock;
		HANDLE _out;
		std::wstring _friendly;
		std::wstring _port;
		ref<Icon> _icon;
		MIDI::Velocity _tracked[MIDI::KChannelCount][MIDI::KNoteCount];
		MIDI::ControlValue _trackedControls[MIDI::KChannelCount][MIDI::KMaxControlID+1];
		MIDI::ProgramID _trackedProgram[MIDI::KChannelCount];
};

#ifdef _WIN32
	class WindowsMIDIOutputDevice: public MIDIOutputDevice {
		public:
			WindowsMIDIOutputDevice(unsigned int id, const std::wstring& name);
			virtual ~WindowsMIDIOutputDevice();
			virtual std::wstring GetFriendlyName() const;
			virtual DeviceIdentifier GetIdentifier() const;
			virtual ref<tj::shared::Icon> GetIcon();
			virtual bool IsMuted() const;
			virtual void SetMuted(bool t) const;

			HMIDIOUT GetMIDIOut();
			virtual void SendMMC(MIDI::DeviceID deviceID, MMC::Command command);
			virtual void SendMMCGoto(MIDI::DeviceID deviceID, Time t);
			virtual void SendSysEx(unsigned char* msg, unsigned int length);
			virtual void SendShort(MIDI::Message message, MIDI::Channel channel, unsigned char a, unsigned char b);
			virtual void SendProgramChange(MIDI::Channel channel, MIDI::ProgramID program);
			virtual void SendControlChange(MIDI::Channel channel, MIDI::ControlID control, MIDI::ControlValue value);
			virtual void SendNoteOn(MIDI::Channel channel, MIDI::NoteID note, MIDI::Velocity velocity);
			virtual void SendNoteOff(MIDI::Channel channel, MIDI::NoteID note);
			virtual void SendAllNotesOff(MIDI::Channel channel);

			virtual MIDI::Velocity GetTrackedNoteStatus(MIDI::Channel channel, MIDI::NoteID note);
			virtual MIDI::ControlValue GetTrackedControlStatus(MIDI::Channel channel, MIDI::ControlID control);
			virtual MIDI::ProgramID GetTrackedProgram(MIDI::Channel channel);

		protected:
			void ClearTrack();
			static std::wstring GetError(int r);
			CriticalSection _lock;
			unsigned int _nid;
			std::wstring _friendly;
			HMIDIOUT _out;
			ref<Icon> _icon;
			MIDI::Velocity _tracked[MIDI::KChannelCount][MIDI::KNoteCount];
			MIDI::ControlValue _trackedControls[MIDI::KChannelCount][MIDI::KMaxControlID+1];
			MIDI::ProgramID _trackedProgram[MIDI::KChannelCount];
	};

	// MIDI device; 0-127 is CC, 128-255 is NoteOn
	class WindowsMIDIInputDevice: public MIDIInputDevice {
		public:
			WindowsMIDIInputDevice(UINT midiDeviceID, const std::wstring& name, ref<input::Dispatcher> disp);
			virtual ~WindowsMIDIInputDevice();
			virtual std::wstring GetFriendlyName() const;
			virtual DeviceIdentifier GetIdentifier() const;
			virtual ref<tj::shared::Icon> GetIcon();
			virtual bool IsMuted() const;
			virtual void SetMuted(bool t) const;
			virtual void Connect(bool t);

			// MIDI specific
			HMIDIIN GetNativeHandle();
			virtual void Message(unsigned int data); // threaded call

		protected:
			UINT _nid; // native midi device id
			HMIDIIN _min;
			ref<input::Dispatcher> _disp;
			std::wstring _name;
			ref<Icon> _icon;
	};

#endif

#endif
