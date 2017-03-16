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
#include "../../include/tjmidi.h"
using namespace tj::shared;
using namespace tj::show;

class MMCPlayer: public Player {
	public:
		MMCPlayer(ref<MMCTrack> mt);
		virtual ~MMCPlayer();
		virtual ref<Track> GetTrack();
		virtual void Stop();
		virtual void Start(Time pos, ref<Playback> playback, float speed);
		virtual void Pause(Time pos);
		virtual void Tick(Time currentPosition);
		virtual void Jump(Time t, bool pause);
		virtual Time GetNextEvent(Time t);
		virtual void SetPlaybackSpeed(Time t, float c);
		virtual void SetOutput(bool enable);


	protected:
		weak<MMCTrack> _track;
		bool _output;
		ref<MIDIOutputDevice> _device;
};

/** MMCPlaer */
MMCPlayer::MMCPlayer(ref<MMCTrack> mt): _track(mt), _output(false) {
	assert(mt);
	_device = mt->GetOutputDevice();
}

MMCPlayer::~MMCPlayer() {
}

ref<Track> MMCPlayer::GetTrack() {
	return ref<MMCTrack>(_track);
}

void MMCPlayer::Stop() {
	if(_device && _output) {
		ref<MMCTrack> track = _track;
		if(track) {
			_device->SendMMC(track->GetDeviceID(), MMC::CommandStop);
		}
	}
}

void MMCPlayer::Start(Time pos, ref<Playback> playback, float speed) {
	if(_device && _output) {
		ref<MMCTrack> track = _track;
		if(track) {
			_device->SendMMC(track->GetDeviceID(), MMC::CommandPlay);
		}
	}
}

void MMCPlayer::Pause(Time pos) {
	if(_device && _output) {
		ref<MMCTrack> track = _track;
		if(track) {
			_device->SendMMC(track->GetDeviceID(), MMC::CommandPause);
		}
	}
}

void MMCPlayer::Tick(Time currentPosition) {
}

void MMCPlayer::Jump(Time t, bool pause) {
	if(_device && _output) {
		ref<MMCTrack> track = _track;
		if(track) {
			_device->SendMMCGoto(track->GetDeviceID(), t);
		}
	}
}

Time MMCPlayer::GetNextEvent(Time t) {
	return Time(-1);
}

void MMCPlayer::SetPlaybackSpeed(Time t, float c) {
}

void MMCPlayer::SetOutput(bool enable) {
	_output = enable;
}

MMCTrack::MMCTrack(ref<Playback> pb): _pb(pb), _deviceID(0x00) {
}

MMCTrack::~MMCTrack() {
}

std::wstring MMCTrack::GetTypeName() const {
	return L"MMC";
}

ref<Player> MMCTrack::CreatePlayer(ref<Stream> str) {
	return GC::Hold(new MMCPlayer(this));
}

Flags<RunMode> MMCTrack::GetSupportedRunModes() {
	Flags<RunMode> rm;
	rm.Set(RunModeMaster, true);
	rm.Set(RunModeDont, true);
	return rm;
}

ref<Item> MMCTrack::GetItemAt(Time position, unsigned int h, bool rightClick, int th, float pixelsPerMs) {
	return 0;
}

void MMCTrack::Paint(tj::shared::graphics::Graphics* g, Time position, float pixelsPerMs, unsigned int x, unsigned int y, unsigned int h, Time start, Time end) {
}

ref<PropertySet> MMCTrack::GetProperties() {
	ref<PropertySet> ps = GC::Hold(new PropertySet());
	ps->Add(_pb->CreateSelectPatchProperty(TL(midi_mmc_output_device), this, &_out));
	ps->Add(GC::Hold(new GenericProperty<unsigned int>(TL(midi_mmc_device_id), this, &_deviceID, _deviceID)));
	return ps;
}

ref<LiveControl> MMCTrack::CreateControl(ref<Stream> str) {
	return 0;
}

void MMCTrack::Save(TiXmlElement* parent) {
	SaveAttributeSmall(parent, "output", _out);
	SaveAttributeSmall<unsigned int>(parent, "device-id", _deviceID);
}

void MMCTrack::Load(TiXmlElement* you) {
	_out = LoadAttributeSmall(you, "output", _out);
	_deviceID = (MIDI::DeviceID)LoadAttributeSmall<unsigned int>(you, "device-id", _deviceID);
}

ref<MIDIOutputDevice> MMCTrack::GetOutputDevice() {
	ref<Device> dev = _pb->GetDeviceByPatch(_out);
	if(dev.IsCastableTo<MIDIOutputDevice>()) {
		return dev;
	}
	Throw(L"Invalid device patched, should be a midi output device",ExceptionTypeError);
}

MIDI::DeviceID MMCTrack::GetDeviceID() const {
	return _deviceID;
}