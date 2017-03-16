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

DMXPositionPlayer::DMXPositionPlayer(ref<DMXPositionTrack> track, ref<Stream> str) {
	assert(track);
	_track = track;
	_output = false;
	_stream = str;
}

DMXPositionPlayer::~DMXPositionPlayer() {
}

ref<Track> DMXPositionPlayer::GetTrack() {
	return ref<Track>(_track);
}

void DMXPositionPlayer::Stop() {
}

void DMXPositionPlayer::Start(Time pos, ref<Playback> pb, float speed) {
	_macro = _track->GetMacro(DMXSequence,pb);
}

void DMXPositionPlayer::Tick(Time t) {
	if(_output) {
		if(_macro._pan) _macro._pan->Set(_track->GetFaderById(DMXPositionTrack::KFaderPan)->GetValueAt(t));
		if(_macro._tilt) _macro._tilt->Set(_track->GetFaderById(DMXPositionTrack::KFaderTilt)->GetValueAt(t));
	}
}

void DMXPositionPlayer::Jump(Time t, bool paused) {
	Tick(t);
}

void DMXPositionPlayer::SetOutput(bool enable) {
	_output = enable;
}

Time DMXPositionPlayer::GetNextEvent(Time t) {
	return _track->GetNextEvent(t);
}