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
#include "../include/tjtransfer.h"
using namespace tj::transfer;

TransferPlayer::TransferPlayer(ref<TransferTrack> tr, ref<TransferPlugin> plug, ref<Stream> str): _plugin(plug), _track(tr), _stream(str) {
	_outputEnabled = false;
	_last = Time(0);
}

TransferPlayer::~TransferPlayer() {
}

ref<Track> TransferPlayer::GetTrack() {
	return _track;
}

ref<Plugin> TransferPlayer::GetPlugin() {
	return ref<TransferPlugin>(_plugin);
}

void TransferPlayer::Stop() {
	_pb = 0;
}

void TransferPlayer::Start(Time pos, ref<Playback> playback, float speed) {
	_pb = playback;
	Tick(pos);
}

void TransferPlayer::Tick(Time currentPosition) {
	if(_outputEnabled || _stream) {
		std::vector< ref<TransferCue> > cues; 
		_track->GetCuesBetween(_last, currentPosition, cues);
		std::vector< ref<TransferCue> >::iterator it = cues.begin();

		while(it!=cues.end()) {
			ref<TransferCue> cur = *it;
			Fire(cur);
			++it;
		}
	}

	_last = currentPosition;
}

void TransferPlayer::Jump(Time newT, bool paused) {
	if(!paused) Tick(newT);
}

void TransferPlayer::SetOutput(bool enable) {
	_outputEnabled = enable;
}

void TransferPlayer::Fire(ref<TransferCue> cue) {
	std::wstring prid = _pb->ParseVariables(cue->_rid);
	if(_stream) {
		ref<Message> msg = _stream->Create();
		msg->Add(prid);
		_stream->Send(msg);
	}

	if(_outputEnabled) {
		// This plays on the servers. Since servers don't even have a ClientCacheManager in the current
		// versions of TJShow, this doesn't really make sense. GetSupportedRunModes in TransferTrack disabled
		// master playback because of this.
		if(_pb) {
			_pb->QueueDownload(prid);
		}
	}
}

Time TransferPlayer::GetNextEvent(Time t) {
	ref<TransferCue> cue = _track->GetCueAfter(t);
	if(cue) {
		return cue->GetTime()+Time(1);
	}

	return -1;
}

/* TransferStreamPlayer */
TransferStreamPlayer::TransferStreamPlayer(ref<TransferPlugin> tp, ref<Talkback> talk): _plugin(tp), _talk(talk) {
}

TransferStreamPlayer::~TransferStreamPlayer() {
}

ref<Plugin> TransferStreamPlayer::GetPlugin() {
	return ref<TransferPlugin>(_plugin);
}

void TransferStreamPlayer::Message(ref<tj::shared::DataReader> msg, ref<Playback> pb) {
	unsigned int pos = 0;
	std::wstring rid = msg->Get<std::wstring>(pos);
	if(pb) {
		pb->QueueDownload(rid);
	}
}