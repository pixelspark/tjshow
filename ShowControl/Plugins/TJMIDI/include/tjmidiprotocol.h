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
#ifndef _TJMIDIPROTOCOL_H
#define _TJMIDIPROTOCOL_H

class MIDI {
	public:	
		enum SubType {
			SubTypeLongMTC = 0x01,
			SubTypeMSC = 0x02,
			SubTypeNotation = 0x03,
			SubTypeDeviceControl = 0x04,
			SubTypeMTCCueing = 0x05,
			SubTypeMMCCommand = 0x06,
			SubTypeMMCResponse = 0x07,
			SubTypeNoteRetune = 0x08,
		};

		// Last nibble (the 0) can be filled with the MIDI channel [0x0,0xF]
		enum Message {
			MessageNoteOff = 0x80,
			MessageNoteOn = 0x90,
			MessageAfterTouch = 0xA0,
			MessageControlChange = 0xB0,
			MessageProgramChange = 0xC0,
			MessageChannelPressure = 0xD0,
			MessagePitchWheel = 0xE0,
			MessageSystem = 0xF0,
		};

		typedef unsigned int DeviceID; // cannot be larger than unsigned char, but using unsigned int here since GenericProperty doesn't work with unsigned char
		typedef unsigned int Channel; // range [0,15] or [0x0, 0xF]
		typedef unsigned int ProgramID; // program change identifier; can be [0x00, 0xFF]
		typedef unsigned int ControlID; // range [0,127]
		typedef unsigned int ControlValue; // range [0,127]
		typedef unsigned int Velocity; // range [0,127]
		typedef unsigned int NoteID; // range [0,127]

		static const unsigned int KNoteCount = 127;
		static const unsigned int KChannelCount = 16;
		static const Channel KMaxChannel = 15;
		static const NoteID KMaxNoteID = 127;
		static const ControlID KMaxControlID = 127;
		static const ControlValue KMaxControlValue = 127;
		static const Velocity KMaxVelocity = 127;
		static const ProgramID KMaxProgramID = 255;

		const static Channel BroadcastChannel = 0xF; // all devices respond to channel 0xF (channel 16 when starting from channel 0, or 17 in the user interface)

		static inline unsigned char MessageOnChannel(Message m, Channel c) {
			return (m | (c & 0x0000000F));
		}

		static inline Message DecodeMessage(unsigned char c) {
			return (Message)(c & 0x000000F0);
		}
		
		static inline Channel DecodeChannel(unsigned char c) {
			return (Channel)(c & 0x0000000F);
		}

		// Ensures a character is in the range [0x00, 0x7F], because anything >0x80 is a status byte
		static inline unsigned char EnsureDataByte(unsigned char c) {
			return (c & 0x7F);
		}

		static const wchar_t* Programs[128];
		static const wchar_t* Notes[128];
		static const wchar_t* Controls[128];
};

class MMC {
	public:
		enum Command {
			CommandStop = 0x01,
			CommandPlay = 0x02,
			CommandDeferredPlay = 0x03,
			CommandFastForward = 0x04,
			CommandRewind = 0x05,
			CommandRecordStrobe = 0x06, /* punch in */
			CommandRecordExit = 0x07, /* punch out */
			CommandRecordReady = 0x08,
			CommandPause = 0x09,
			CommandEject = 0x0A,
			CommandReset = 0x0F,
			CommandWrite = 0x40,
			CommandLocate = 0x44,
		};
};

class MSC {
	public:
		typedef unsigned int DeviceID; // Can be [0x00, 0x7F]
		static const Bytes KMaxCommandLength = 128; // maximum command message length is 128 bytes

		struct CueID {
			public:
				CueID(const std::wstring& number = L"", const std::wstring& list = L"", const std::wstring& path = L"");
				std::wstring _cueNumber;
				std::wstring _cueList;
				std::wstring _cuePath;

				void Serialize(strong<DataWriter> code);
				void Save(TiXmlElement* you);
				void Load(TiXmlElement* you);
				void AddProperties(ref<PropertySet> existing, ref<Inspectable> holder);

			private:
				static void SerializeNumber(strong<DataWriter> code, const std::wstring& num);
		};

		enum Command {
			// General basic commands [0x01, 0x0B]
			None = 0x00,
			Go = 0x01,
			Stop = 0x02,
			Resume = 0x03,
			TimedGo = 0x04,
			Load = 0x05,
			Set = 0x06,
			Fire = 0x07,
			AllOff = 0x08,
			Restore = 0x09,
			Reset = 0x0A,
			GoOff = 0x0B,

			// Sound commands [0x10, 0x1E]
			GoJamClock = 0x10,
			StandbyIncrease = 0x11,
			StandbyDecrease = 0x12,
			SequenceIncrease = 0x13,
			SequenceDecrease = 0x14,
			StartClock = 0x15,
			StopClock = 0x16,
			ZeroClock = 0x17,
			SetClock = 0x18,
			MTCChaseOn = 0x19,
			MTCChaseOff = 0x1A,
			OpenCueList = 0x1B,
			CloseCueList = 0x1C,
			OpenCuePath = 0x1D,
			CloseCuePath = 0x1E,
			
			_LastCommand = 0x7F,
		};

		enum CommandFormat {
			_Reserved = 0x00,
			Lighting = 0x01,
			MovingLights = 0x02,
			ColourChangers = 0x03,
			Strobes = 0x04,
			Lasers = 0x05,
			Chasers = 0x06,

			Sound = 0x10,
			Music = 0x11,
			CDPlayers = 0x12,
			EPROMPlayback = 0x13,
			AudioTapeMachines = 0x14,
			Intercoms = 0x15,
			Amplifiers = 0x16,
			AudioEffectDevices = 0x17,
			Equalizers = 0x18,
			
			Machinery = 0x20,
			Rigging = 0x21,
			Flys = 0x22,
			Lifts = 0x23,
			Turntables = 0x24,
			Trusses = 0x25,
			Robots = 0x26,
			Animation = 0x27,
			Floats = 0x28,
			Breakaways = 0x29,
			Barges = 0x2A,

			Video = 0x30,
			VideoTapeMachines = 0x31,
			VideoCassetteMachines = 0x32,
			VideoDiscPlayers = 0x33,
			VideoSwitches = 0x34,
			VideoEffects = 0x35,
			VideoCharacterGenerators = 0x36,
			VideoStillStores = 0x37,
			VideoMonitors = 0x38,

			Projection = 0x40,
			FilmProjectors = 0x41,
			SlideProjectors = 0x42,
			VideoProjectors = 0x43,
			Dissolvers = 0x44,
			ShutterControls = 0x45,

			ProcessControl = 0x50,
			HydraulicOil = 0x51,
			H2O = 0x52,
			CO2 = 0x53,
			CompressedAir = 0x54,
			NaturalGas = 0x55,
			Fog = 0x56,
			Smoke = 0x57,
			CrackedHaze = 0x58,

			Pyro = 0x60,
			Fireworks = 0x61,
			Explosions = 0x62,
			Flame = 0x63,
			SmokePots = 0x64,

			AllTypes = 0x7F,
			_LastCommandFormat = 0x7F,
		};

		static const unsigned int KMaxCommand = _LastCommand;
		static const unsigned int KMaxCommandFormat = _LastCommandFormat;
		static const wchar_t* Commands[_LastCommand+1];
		static const wchar_t* CommandFormats[_LastCommandFormat+1];

		static bool IsIndividualDeviceID(const DeviceID& id) {
			return (id >= 0x00) && (id <= 0x6F);
		}

		static bool IsGroupDeviceID(const DeviceID& id) {
			return (id >= 0x70) && (id <= 0x7E);
		}

		static bool IsAllCallDeviceID(const DeviceID& id) {
			return id == 0x7F;
		}

		static std::wstring GetCommandName(const Command& c) {
			if(c > _LastCommand) {
				return L"";
			}
			return Commands[c];
		}

		static std::wstring GetCommandFormatName(const CommandFormat& c) {
			if(c > _LastCommandFormat) {
				return L"";
			}
			return CommandFormats[c];
		}
};

#endif