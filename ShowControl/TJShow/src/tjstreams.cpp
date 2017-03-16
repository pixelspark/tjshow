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
#include "../include/internal/tjshow.h"
#include "../include/internal/tjstreams.h"

TrackStream::TrackStream(strong<TrackWrapper> track, ref<Network> net, ref<Instance> instance, bool isDynamic): _network(net), _track(ref<TrackWrapper>(track)) {
	if(isDynamic) {
		ref<Group> group = Application::Instance()->GetModel()->GetGroups()->GetGroupById(track->GetGroup());
		if(group) {
			_ca.Allocate(group, instance);
			_channel = _ca.GetChannel();
		}
	}
	else {
		_channel = track->GetMainChannel();
		ref<Group> group = Application::Instance()->GetModel()->GetGroups()->GetGroupById(track->GetGroup());
		if(group) {
			group->TakeOwnership(_channel, instance);
		}
	}
}

TrackStream::~TrackStream() {
	// Channel will be deallocated; send a 'channel reset' message
	ref<TrackWrapper> tw = _track;
	ref<Network> network = _network;
	if(tw && network) {
		network->GetSocket()->SendResetChannel(tw->GetGroup(), _channel);
	}
}

strong<Message> TrackStream::Create() {
	ref<TrackWrapper> tw = _track;
	if(tw) {
		return GC::Hold(new Message(false, tw->GetGroup(), _channel, tw->GetPlugin()->GetHash()));
	}
	Throw(L"Could not create stream for track, since track is gone!", ExceptionTypeError);
}

Channel TrackStream::GetChannel() {
	return _channel;
}

void TrackStream::Send(ref<Message> msg, bool reliable) {
	if(!msg) return;
	// acquire socket and track
	ref<Network> network = _network;
	ref<TrackWrapper> track = _track;
	if(!network || !track) {
		Log::Write(L"TrackStream/Send", L"Could not send: network or track does not exist anymore");
		return;
	}

	// check runmode
	RunMode rm = track->GetRunMode();
	if(rm!=RunModeBoth && rm!=RunModeClient) {
		return;
	}

	// really send it now
	network->SendUpdate(msg, reliable);
	track->_lastMessageTime = (int)GetTickCount();
}

LiveStream::LiveStream(ref<TrackWrapper> track, ref<Network> net): _network(net) {
	_track = track;
}

LiveStream::~LiveStream() {
}

strong<Message> LiveStream::Create() {
	ref<TrackWrapper> tw = _track;
	if(tw) {
		/* Live controls always use the channel for the first (main) instance of the track. In principle,
		live control messages are sent to the plug-in instead of a specific track streamplayer on the client. 
		However, some plug-ins work around this by setting SetIsPluginMessage(false), so these messages need
		an accurate channel. */
		return GC::Hold(new Message(true, tw->GetGroup(), tw->GetMainChannel(), tw->GetPlugin()->GetHash()));
	}
	Throw(L"Could not create live control stream for track, since track is gone!", ExceptionTypeError);
}

void LiveStream::Send(ref<Message> msg, bool reliable) {
	if(!msg) return;
	// acquire socket and track
	ref<Network> network = _network;
	ref<TrackWrapper> track = _track;
	
	if(!network || !track) {
		Log::Write(L"TrackStream/Send", L"Could not send livecontrol message: socket or track does not exist anymore");
		return;
	}

	// really send it now
	network->SendUpdate(msg, reliable);
}