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
#include "../include/tjingle.h"
#include <shlwapi.h>
using namespace tj::shared;
using namespace tj::jingle;

/* Jingle */
Jingle::Jingle() {
	_sample = 0;
	_channel = 0;
	_stream = 0;
}

Jingle::~Jingle() {
}

bool Jingle::IsPlaying() {
	return BASS_ChannelIsActive(_channel)==BASS_ACTIVE_PLAYING;
}

bool Jingle::IsLoaded() {
	return _loadedFile==_file;
}

bool Jingle::IsLoadedAsSample() {
	return _sample!=0;
}

bool Jingle::IsLoadedAsStream() {
	return _stream!=0;
}

void Jingle::Unload() {
	if(IsLoadedAsSample()) {
		BASS_SampleFree(_sample);
		_sample = 0;
	}
	if(IsLoadedAsStream()) {
		BASS_StreamFree(_stream);
		_stream = 0;
	}
}

std::wstring Jingle::GetJingleID() const {
	return File::GetFileName(_file);
}

void Jingle::FadeIn() {
	JingleApplication::Instance()->OnJingleEvent(GetJingleID(), JingleFadeIn);

	if(IsLoadedAsStream()) {
		BASS_ChannelSetPosition(_channel, 0);
	}
	
	if(!IsLoaded()) {
		// load as stream
		Load(true);
	}
	
	HCHANNEL channel;
	if(IsLoadedAsSample()) {
		channel = BASS_SampleGetChannel(_sample, FALSE);
	}
	else if(IsLoadedAsStream()) {
		channel = _stream;
	}

	if(channel!=0) {
		_channel = channel;
		BASS_ChannelSetAttributes(_channel, -1, 0, -101);
		BASS_ChannelSlideAttributes(_channel, -1, 100, -101, 1000);
		BASS_ChannelPlay(_channel, FALSE);
	}
}

void Jingle::FadeOut() {
	JingleApplication::Instance()->OnJingleEvent(GetJingleID(), JingleFadeOut);
	if(_channel!=0) {
		BASS_ChannelSlideAttributes(_channel, -1, 0, -101, 1000);
	}
}

void Jingle::Load(bool asStream) {
	// if loaded as stream, reload as sample for instant start
	bool reload = false;
	if(!asStream && IsLoadedAsStream()) {
		Unload();
		reload = true;
	}

	if(!IsLoaded() || reload) {
		Unload();
		std::string fn = Mbs(_file);

		if(!asStream) {
			_sample = BASS_SampleLoad(FALSE, fn.c_str(), 0, 0, 10, 0);
			
		}
		else {
			_stream = BASS_StreamCreateFile(FALSE, fn.c_str(), 0, 0, 0);
		}
		_loadedFile = _file;
	}
}

void Jingle::Play() {
	// Send event
	JingleApplication::Instance()->OnJingleEvent(GetJingleID(), JinglePlay);

	if(IsLoadedAsStream()) {
		BASS_ChannelSetPosition(_channel, 0);
	}
	
	if(!IsLoaded()) {
		// load as stream
		Load(true);
	}
	
	HCHANNEL channel;
	if(IsLoadedAsSample()) {
		channel = BASS_SampleGetChannel(_sample, FALSE);
	}
	else if(IsLoadedAsStream()) {
		channel = _stream;
	}

	if(channel!=0) {
		_channel = channel;
		BASS_ChannelSetAttributes(_channel, -1, 100, -101);
		BASS_ChannelPlay(_channel, FALSE);
	}
}

std::wstring Jingle::GetFile() const {
	return _file;
}

void Jingle::Clear() {
	_file = L"";
	Unload();
}

std::wstring Jingle::GetName() {
	wchar_t* fn = _wcsdup(_file.c_str());
	wchar_t* filename = PathFindFileName(fn);
	PathRemoveExtension(filename);
	std::wstring ret(filename);
	delete[] fn;
	return ret;
}

void Jingle::Stop() {
	JingleApplication::Instance()->OnJingleEvent(GetJingleID(), JingleStop);

	if(IsLoadedAsSample()) {
		BASS_SampleStop(_sample);
	}
	if(IsLoadedAsStream()) {
		BASS_ChannelStop(_channel);
	}
}

bool Jingle::IsEmpty() {
	return _file.length()<1;
}

void Jingle::SetFile(std::wstring fn) {
	_file = fn;
}

int Jingle::GetRemainingSeconds() {
	QWORD pos = BASS_ChannelGetPosition(_channel);
	QWORD length = BASS_ChannelGetLength(_channel);
	return int(BASS_ChannelBytes2Seconds(_channel, length-pos));
}

tj::shared::Time Jingle::GetRemainingTime() {
	QWORD pos = BASS_ChannelGetPosition(_channel);
	QWORD length = BASS_ChannelGetLength(_channel);
	return Time(int(1000.0f*BASS_ChannelBytes2Seconds(_channel, length-pos)));
}

void Jingle::Save(TiXmlElement* parent) {
	SaveAttribute(parent, "file", _file);
}

void Jingle::Load(TiXmlElement* you) {
	_file = LoadAttribute<std::wstring>(you, "file", L"");
}

float Jingle::GetPosition() {
	if(!IsPlaying()) return 0.0f;

	QWORD pos = BASS_ChannelGetPosition(_channel);
	QWORD length = BASS_ChannelGetLength(_channel);
	if(length==0) return 0.0f;

	return float(pos)/float(length);
}