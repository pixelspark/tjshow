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
#include "../include/tjdmx.h"

DMXPlayer::DMXPlayer(ref<DMXTrack> track, ref<Stream> str) {
	assert(track);
	_track = track;
	_sentValue = 123;
	_stream = str;
}

DMXPlayer::~DMXPlayer() {
}

ref<Track> DMXPlayer::GetTrack() {
	return _track;
}

void DMXPlayer::Stop() {
	if(_macro && _track->GetResetOnStop()) {
		_macro->Set(0);
		_macro = 0;
	}
	_pb = null;
}

void DMXPlayer::Start(Time pos, ref<Playback> pb, float speed) {
	ref<DMXPlugin> plug = _track->GetDMXPlugin();
	_pb = pb;
	std::wstring parsedAddress = _track->GetDMXAddress();
	if(_pb) {
		parsedAddress = _pb->ParseVariables(parsedAddress);
	}
	_macro = plug->CreateMacro(parsedAddress, DMXSequence);
}

void DMXPlayer::Tick(Time currentPosition) {
	if(_outputEnabled && _macro) {
		_macro->Set(_track->GetValueAt(currentPosition));
	}
}

Time DMXPlayer::GetNextEvent(Time t) {
	return _track->GetNextEvent(t);
}

void DMXPlayer::Jump(Time t, bool paused) {
	Tick(t);
}

void DMXPlayer::SetOutput(bool enable) {
	_outputEnabled = enable;
}

// streamplayer
DMXStreamPlayer::DMXStreamPlayer(ref<DMXPlugin> plug) {
	_plugin = plug;
}

DMXStreamPlayer::~DMXStreamPlayer() {

}

ref<Plugin> DMXStreamPlayer::GetPlugin() {
	return _plugin;
}

void DMXStreamPlayer::Message(ref<DataReader> msg, HWND videoParent) {
}