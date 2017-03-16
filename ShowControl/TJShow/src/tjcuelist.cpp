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
#include <algorithm>

CueList::CueList(ref<Instance> controller) {
	_controller = controller;
} 

CueList::~CueList() {
}

namespace tj {
	namespace show {
		struct CueSorter {
			 bool operator()(ref<Cue>& start,ref<Cue>& end) {
				  return start->GetTime() < end->GetTime();
			 }
		};
	}
}

void CueList::RemoveCuesBetween(Time a, Time b) {
	ThreadLock lock(&_lock);
	std::vector< ref<Cue> >::iterator it = _cues.begin();
	while(it!=_cues.end()) {
		ref<Cue> cue = *it;
		Time ct = cue->GetTime();
		if(ct <= b && ct >= a) {
			_cues.erase(it);
			_cues.begin();
		}
		else {
			++it;
		}
	}
}

void CueList::Clone() {
	ThreadLock lock(&_lock);
	std::vector< ref<Cue> >::iterator it = _cues.begin();
	while(it!=_cues.end()) {
		ref<Cue> cue = *it;
		cue->Clone();
		++it;
	}
}

void CueList::Save(TiXmlElement* parent) {
	TiXmlElement cues("cues");
	std::vector< ref<Cue> >::iterator itc = _cues.begin();
	while(itc!=_cues.end()) {
		ref<Cue> cue = *itc;
		TiXmlElement cueElement("cue");
		cue->Save(&cueElement);
		cues.InsertEndChild(cueElement);
		itc++;
	}
	parent->InsertEndChild(cues);
}

void CueList::Load(TiXmlElement* cues) {
	ThreadLock lock(&_lock);

	TiXmlElement* cue = cues->FirstChildElement("cue");
	while(cue!=0) {
		ref<Cue> cueC = GC::Hold(new Cue(this));
		cueC->Load(cue);
		AddCue(cueC);
		cue = cue->NextSiblingElement("cue");
	}
}

void CueList::SetStaticInstance(ref<Instance> ctrl) {
	_controller = ctrl;
}

std::vector< ref<Cue> >::iterator CueList::GetCuesBegin() {
	return _cues.begin();
}

std::vector< ref<Cue> >::iterator CueList::GetCuesEnd() {
	return _cues.end();
}

void CueList::AddCue(ref<Cue> c) {
	ThreadLock lock(&_lock);
	_cues.push_back(c);
	std::sort(_cues.begin(), _cues.end(), CueSorter());
}

void CueList::RemoveCue(ref<Cue> c) {
	ThreadLock lock(&_lock);
	std::vector< ref<Cue> >::iterator it = _cues.begin();
	while(it!=_cues.end()) {
		if(*it==c) {
			it = _cues.erase(it);
			break;
		}
		++it;
	}
}

void CueList::RemoveAllCues() {
	ThreadLock lock(&_lock);
	_cues.clear();
}

ref<Cue> CueList::GetCueAt(Time t) {
	std::vector<ref<Cue> >::iterator it = _cues.begin();
	while(it!=_cues.end()) {
		ref<Cue> c = *it;
		if(c->GetTime()==t) {
			return c;
		}
		++it;
	}
	return 0;
}

ref<Cue> CueList::GetNextCue(const Time& t, Cue::Action filter) {
	ref<Cue> match = 0;
	std::vector<ref<Cue> >::iterator it = _cues.begin();
	while(it!=_cues.end()) {
		ref<Cue> cue = *it;
		if(cue->GetTime()>t && (filter==Cue::ActionNone || cue->GetAction()==filter) ) {
			if(!match) {
				match = cue;
			}
			else {
				Time mdiff = match->GetTime() - t;
				Time cdiff = cue->GetTime() - t;
				if(cdiff < mdiff) {
					match = cue;
				}
			}
		}
		
		++it;
	}

	return match;
}

void CueList::GetCuesBetween(Time start, Time end, std::list< ref<Cue> >& matches) {	
	std::vector< ref<Cue> >::iterator it = _cues.begin();
	while(it!=_cues.end()) {
		ref<Cue> cue = *it;
		Time ctime = cue->GetTime();
		if(ctime>start && ctime <= end) {
			matches.push_back(cue);
		}
		
		++it;
	}
}

unsigned int CueList::GetCueCount() const {
	return (unsigned int)_cues.size();
}

ref<Cue> CueList::GetPreviousCue(Time t) {
	ref<Cue> match = 0;
	std::vector<ref<Cue> >::iterator it = _cues.begin();
	while(it!=_cues.end()) {
		ref<Cue> cue = *it;
		if(cue->GetTime()<t) {
			if(!match) {
				match = cue;
			}
			else {
				Time mdiff = match->GetTime() - t;
				Time cdiff = cue->GetTime() - t;
				if(cdiff > mdiff) {
					match = cue;
				}
			}
		}
		++it;
	}

	return match;
}

ref<Cue> CueList::GetCueByName(const std::wstring& pname) {
	std::wstring name;
	std::transform(pname.begin(), pname.end(), name.begin(), tolower);

	std::vector< ref<Cue> >::iterator it = _cues.begin();
	while(it!=_cues.end()) {
		ref<Cue> cue = *it;
		std::wstring cname = cue->GetName();
		std::transform(cname.begin(), cname.end(), cname.begin(), tolower);
		if(cname==name) {
			return cue;
		}
		++it;
	}

	return 0;
}

ref<Cue> CueList::GetCueByID(const CueIdentifier& id) {
	std::vector< ref<Cue> >::iterator it = _cues.begin();
	while(it!=_cues.end()) {
		ref<Cue> cue = *it;
		if(cue->GetID()==id) return cue;
		++it;
	}

	return 0;
}

void CueList::Clear() {
	ThreadLock lock(&_lock);
	_cues.clear();
}