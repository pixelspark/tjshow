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
#include "../include/tjmedia.h"
#include "../include/analyzer/tjaudioanalyzer.h"
using namespace tj::media;

const Time MediaBlock::KMinimumDuration(250);

MediaBlock::MediaBlock(Time start, const std::wstring& file, bool isVideo): _isVideo(isVideo), _keyEnabled(false), _keyColor(0.0, 1.0, 0.0), _keyTolerance(0.1f) {
	_start = start;
	_file = file;
	_userDuration = 0;
}

MediaBlock::~MediaBlock() {
}

ref<PropertySet> MediaBlock::GetProperties(bool withKeying, ref<Playback> pb) {
	ref<PropertySet> ps = GC::Hold(new PropertySet());

	ps->Add(GC::Hold(new FileProperty(TL(media_block_filename), this, &_file, pb->GetResources())));
	if(_isVideo) ps->Add(pb->CreateSelectPatchProperty(TL(media_block_live_source), this, &_liveSource));
	
	ps->Add(GC::Hold(new GenericProperty<Time>(TL(media_block_begin_time), this, &_start, _start)));
	ps->Add(GC::Hold(new GenericProperty<Time>(TL(media_block_length), this, &_userDuration, -1)));

	if(_isVideo && withKeying) {
		ps->Add(GC::Hold(new PropertySeparator(TL(media_keying), !_keyEnabled)));
		ps->Add(GC::Hold(new GenericProperty<bool>(TL(media_keying_enabled), this, &_keyEnabled, _keyEnabled)));
		ps->Add(GC::Hold(new GenericProperty<float>(TL(media_keying_tolerance), this, &_keyTolerance, _keyTolerance)));
		ps->Add(GC::Hold(new ColorProperty(TL(media_keying_color), this, &_keyColor)));
	}
	return ps;
}

bool MediaBlock::IsKeyingEnabled() const {
	return _keyEnabled && _isVideo;
}

float MediaBlock::GetKeyingTolerance() const {
	return _keyTolerance;
}

const RGBColor& MediaBlock::GetKeyingColor() const {
	return _keyColor;
}

void MediaBlock::Move(Time t, int h) {
	_start = t;
}

std::wstring MediaBlock::GetFile() const {
	return _file;
}

bool MediaBlock::IsLiveSource() const {
	return _liveSource != L"";
}

const PatchIdentifier& MediaBlock::GetLiveSource() const {
	return _liveSource;
}

bool MediaBlock::IsValid(strong<Playback> pb) {
	if(IsLiveSource()) {
		return true;
	}

	std::wstring path;
	if(!pb->GetResources()->GetPathToLocalResource(_file,path)) {
		return false;
	}

	if(_analysisFor==_file && _analysis && _analysis->GetDuration()<=Time(0)) {
		return false;
	}

	return true;
}

void MediaBlock::CalculateDuration(strong<Playback> pb) {
	if(!IsLiveSource() && _analysisFor!=_file) {
		_analysis = null;
		try {
			std::wstring path;
			if(pb->GetResources()->GetPathToLocalResource(_file,path)) {
				_analysis = analyzer::AudioAnalyzer::Analyze(path);
			}
		}
		catch(const Exception& e) {
			Log::Write(L"TJMedia/MediaBlock", L"Could not analyze file: "+e.GetMsg());
		}
		_analysisFor = _file;
	}
}

Time MediaBlock::GetDuration(strong<Playback> pb) {
	CalculateDuration(pb);
	Time durationToReturn = 0;

	// For live sources, the duration is the user duration
	if(IsLiveSource()) {
		durationToReturn = _userDuration;
	}
	else {
		Time fileDuration = -1;
		if(_analysis) {
			fileDuration = _analysis->GetDuration();
		}

		// Calculate the duration we want
		if(_userDuration<=Time(0)) {
			if(fileDuration<=Time(0)) {
				durationToReturn = -1;
			}
			else {
				durationToReturn = fileDuration;
			}
		}
		else {
			if(fileDuration<=Time(0)) {
				durationToReturn = _userDuration;
			}
			else {
				durationToReturn =  min(_userDuration, fileDuration);
			}
		}
	}

	// A block has a minimal duration
	if(durationToReturn < KMinimumDuration) {
		return KMinimumDuration;
	}
	return durationToReturn;
}

Time MediaBlock::GetTime() const {
	return _start;
}

void MediaBlock::SetDuration(Time t) {
	_userDuration = t;
}

void MediaBlock::SetTime(Time start) {
	_start = start;
}

void MediaBlock::Paint(graphics::Graphics& g, Area rc, strong<Theme> theme, const Time& start, const float pixelsPerMs) { 
	if(_analysis) {
		ThreadLock lock(&(_analysis->_lock));
		if(_analysis->HasPeaks()) {
			Pen pn(Color(172, 0,255,0), 1.0f);

			// Draw a line each pixel
			Pixels minIncrement = Pixels(analyzer::AudioAnalysis::KResolutionMS * pixelsPerMs);
			for(Pixels left = rc.GetLeft()+Pixels((start.ToInt()-_start.ToInt())*pixelsPerMs); left < rc.GetRight(); left+=max(1,minIncrement)) {
				int timeAtLeft = (int)(float(left-rc.GetLeft())/pixelsPerMs);
				if(timeAtLeft>0) {
					unsigned int pos = (unsigned int)(timeAtLeft / analyzer::AudioAnalysis::KResolutionMS);
					if(pos < _analysis->_peaksSize) {
						float vol = (float(_analysis->_peaks[pos]) / 255.0f);
						float height = (rc.GetHeight()-4) * max(0,log(vol*10.0f));
						g.DrawLine(&pn, PointF(float(left), float(rc.GetBottom() - 2)), PointF(float(left), float(rc.GetBottom())-height - 2));
					}
				}
			}
		}
	}
}

void MediaBlock::Save(TiXmlElement* parent) {
	TiXmlElement me("block");
	SaveAttributeSmall(&me, "file", _file);
	SaveAttributeSmall(&me, "start", _start);
	SaveAttributeSmall(&me, "length", _userDuration);
	if(_liveSource!=L"") {
		SaveAttributeSmall<PatchIdentifier>(&me, "live-source", _liveSource);
	}

	if(_isVideo) {
		SaveAttributeSmall<bool>(&me, "keying", _keyEnabled);
		if(_keyEnabled) {
			SaveAttributeSmall(&me, "keying-tolerance", _keyTolerance);
			TiXmlElement keyColor("keying-color");
			_keyColor.Save(&keyColor);
			me.InsertEndChild(keyColor);
		}
	}
	parent->InsertEndChild(me);
}

void MediaBlock::Load(TiXmlElement* you) {
	_file = LoadAttributeSmall(you, "file", std::wstring(L""));
	_start = LoadAttributeSmall(you, "start", Time(0));
	_userDuration = LoadAttributeSmall(you, "length", Time(0));
	
	if(_isVideo) {
		_liveSource = LoadAttributeSmall<PatchIdentifier>(you, "live-source", _liveSource);
		_keyEnabled = LoadAttributeSmall<bool>(you, "keying", false);
		if(_keyEnabled) {
			_keyTolerance = LoadAttributeSmall<float>(you, "keying-tolerance", _keyTolerance);
			TiXmlElement* keyColor = you->FirstChildElement("keying-color");
			if(keyColor!=0) {
				_keyColor.Load(keyColor);
			}
		}
	}
}