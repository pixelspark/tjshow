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

void JingleCollection::Save(TiXmlElement* parent) {
	std::map<wchar_t, ref<Jingle> >::iterator it = _jingles.begin();
	while(it!=_jingles.end()) {
		std::pair<wchar_t, ref<Jingle> > data = *it;
		ref<Jingle> jingle = data.second;
		if(jingle && !jingle->IsEmpty()) {
			TiXmlElement jtag("jingle");
			SaveAttributeSmall(&jtag, "index", Stringify(data.first));
			jingle->Save(&jtag);
			parent->InsertEndChild(jtag);
		}
		++it;
	}
}

void JingleCollection::FindJinglesByName(const std::wstring& q, std::deque< ref<Jingle> >& results) {
	std::map<wchar_t, ref<Jingle> >::iterator it = _jingles.begin();
	while(it!=_jingles.end()) {
		ref<Jingle> ji = it->second;
		if(ji) {
			std::wstring lowName = Util::StringToLower(ji->GetName());
			if(lowName.find(q)!=std::wstring::npos) {
				results.push_back(ji);
			}
		}
		++it;
	}
}

void JingleCollection::StopAll() {
	std::map<wchar_t, ref<Jingle> >::iterator it = _jingles.begin();
	while(it!=_jingles.end()) {
		std::pair<wchar_t, ref<Jingle> > data = *it;
		ref<Jingle> jingle = data.second;
		if(jingle) {
			jingle->Stop();
		}
		++it;
	}
}

void JingleCollection::PlayRandom() {
	std::vector< ref<Jingle> > possible;
	std::map<wchar_t, ref<Jingle> >::iterator it = _jingles.begin();
	while(it!=_jingles.end()) {
		if(it->second && !it->second->IsEmpty()) {
			possible.push_back(it->second);
		}
		++it;
	}

	if(possible.size()>0) {
		int n = rand()%int(possible.size());
		possible.at(n)->Play();
	}
}

void JingleCollection::FadeOutAll() {
	std::map<wchar_t, ref<Jingle> >::iterator it = _jingles.begin();
	while(it!=_jingles.end()) {
		std::pair<wchar_t, ref<Jingle> > data = *it;
		ref<Jingle> jingle = data.second;
		if(jingle) {
			jingle->FadeOut();
		}
		++it;
	}
}


void JingleCollection::Load(TiXmlElement* you) {
	TiXmlElement* ele = you->FirstChildElement("jingle");
	while(ele!=0) {
		std::wstring idx = LoadAttributeSmall<std::wstring>(ele, "index", L"0");
		if(idx.length()>0) {
			wchar_t index = idx.at(0);
			ref<Jingle> jingle = _jingles[index];
			if(jingle) {
				jingle->Load(ele);
			}
		}
		ele = ele->NextSiblingElement("jingle");
	}
}

ref<PropertySet> Jingle::GetProperties() {
	ref<PropertySet> ps = GC::Hold(new PropertySet());
	ps->Add(GC::Hold(new GenericProperty<std::wstring>(TL(jingle_file), this, &_file, _file)));
	return ps;

}

/* Collection */
JingleCollection::JingleCollection() {
	for(wchar_t a='A';a<='Z';a++) {
		_jingles[a] = GC::Hold(new Jingle());
	}
	for(wchar_t a='0';a<='9';a++) {
		_jingles[a] = GC::Hold(new Jingle());
	}
}

JingleCollection::~JingleCollection() {
}

std::wstring JingleCollection::GetName() const {
	return _name;
}

void JingleCollection::Cache() {
	std::map<wchar_t, ref<Jingle> >::iterator it = _jingles.begin();
	while(it!=_jingles.end()) {
		std::pair<wchar_t, ref<Jingle> > data = *it;
		ref<Jingle> jingle = data.second;
		if(jingle) {
			jingle->Load(false);
		}
		++it;
	}
}

void JingleCollection::Clear() {
	std::map<wchar_t, ref<Jingle> >::iterator it = _jingles.begin();
	while(it!=_jingles.end()) {
		std::pair<wchar_t, ref<Jingle> > data = *it;
		ref<Jingle> jingle = data.second;
		if(jingle) {
			jingle->Clear();
		}
		++it;
	}
}

ref<PropertySet> JingleCollection::GetProperties() {
	ref<PropertySet> ps = GC::Hold(new PropertySet());
	ps->Add(GC::Hold(new GenericProperty<std::wstring>(TL(jingle_collection_name), this, &_name, _name)));
	return ps;
}

ref<Jingle> JingleCollection::GetJingle(wchar_t letter) {
	return _jingles[letter];
}