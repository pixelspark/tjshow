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
#include "../include/tjdataplugin.h"

DataPlayer::DataPlayer(ref<DataTrack> tr, ref<Stream> str): _track(tr), _stream(str), _n(0), _hasNextRow(false) {
}

DataPlayer::~DataPlayer() {
}

ref<Track> DataPlayer::GetTrack() {
	return ref<Track>(_track);
}

void DataPlayer::Stop() {
	_pb = 0;
	_last = Time(-1);
	_stream = 0;
	_query = 0;
	_n = 0;
	_hasNextRow = false;
	_db = null;
}

void DataPlayer::Start(Time pos, ref<Playback> playback, float speed) {
	_pb = playback;
	_last = pos;
	_db = _track->GetDatabase();
}

void DataPlayer::Tick(Time currentPosition) {
	std::vector< ref<DataCue> > cues;
	_track->GetCuesBetween(_last, currentPosition, cues);
	std::vector< ref<DataCue> >::iterator it = cues.begin();
	ref<Outlet> tupleOut = _track->GetTupleOutlet();
	ref<Outlet> currentRowOut = _track->GetCurrentRowNumberOutlet();
	strong<DataQueryHistory> hist = _track->GetHistory();

	if(_db) {
		while(it!=cues.end()) {
			ref<DataCue> cur = *it;
			if(cur) {
				_query = cur->Fire(_db, _pb, tupleOut, _stream, _query, _n, _hasNextRow, hist);

			}
			++it;
		}
	}

	_pb->SetOutletValue(currentRowOut, Any(_n));
	_pb->SetOutletValue(_track->GetHasNextRowOutlet(), Any(_hasNextRow));
	_last = currentPosition;
}

void DataPlayer::Jump(Time newT, bool paused) {
	_last = newT;
}

void DataPlayer::SetOutput(bool enable) {
	_output = enable;
}

Time DataPlayer::GetNextEvent(Time t) {
	ref<DataCue> cue = _track->GetCueAfter(t);
	if(cue) {
		return cue->GetTime()+Time(1);
	}

	return -1;
}