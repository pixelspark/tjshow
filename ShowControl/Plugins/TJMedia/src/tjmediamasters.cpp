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
#include "../include/tjmediamasters.h"

using namespace tj::shared;
using namespace tj::show;
using namespace tj::media;
using namespace tj::media::master;

/** MediaMaster */
MediaMaster::~MediaMaster() {
}

/** MediaMasters **/
MediaMasters::MasterInfo::MasterInfo() {
	_manual = 1.0f;
	_sequence = 1.0f;
}

MediaMasters::MediaMasters() {
}

MediaMasters::~MediaMasters() {
}

void MediaMasters::AddMaster(ref<master::MediaMaster> mt) {
	if(mt) {
		ThreadLock lock(&_mastersLock);
		_masters.push_back(mt);
	}
}

void MediaMasters::GetMasterValues(const MediaMasterList& masterList, std::map<MediaMaster::Flavour, float>& values) {
	// Enumerate all masters and get their flavour
	std::map<MediaMasterID, MediaMaster::Flavour> flavours;
	std::vector< weak<MediaMaster> >::iterator it = _masters.begin();
	while(it!=_masters.end()) {
		ref<MediaMaster> mm = *it;
		if(mm) {
			MediaMaster::Flavour flavour = mm->GetFlavour();
			flavours[mm->GetMasterID()] = flavour;
			if(values.find(flavour)==values.end()) {
				values[flavour] = 1.0f;
			}
		}
		++it;
	}

	MediaMasterList::size_type p = masterList.find_first_of(L',');
	if(p==MediaMasterList::npos) {
		// just one master
		std::map<MediaMasterID, MasterInfo>::const_iterator it = _masterInfo.find(masterList);
		if(it!=_masterInfo.end()) {
			const MasterInfo& mi = it->second;
			std::map<MediaMasterID, MediaMaster::Flavour>::const_iterator flit = flavours.find(masterList);
			if(flit!=flavours.end()) {
				values[flit->second] *= mi._manual * mi._sequence;
			}
			else {
				// Unknown master? shouldn't happen!
			}
		}
	}
	else {
		MediaMasterList::size_type start = 0;
		MediaMasterList::size_type end = 0;

		while((end = masterList.find (L',', start)) != std::wstring::npos) {
			const MediaMasterID& mid = masterList.substr(start, end-start);
			std::map<MediaMasterID, MasterInfo>::const_iterator it = _masterInfo.find(mid);
			if(it!=_masterInfo.end()) {
				const MasterInfo& mi = it->second;
				std::map<MediaMasterID, MediaMaster::Flavour>::const_iterator flit = flavours.find(mid);
				if(flit!=flavours.end()) {
					values[flit->second] *= mi._manual * mi._sequence;
				}
				else {
					// Unknown master? shouldn't happen!
				}
			}
			start = end + 1;
		};

		// last part
		const MediaMasterID& mid = masterList.substr(start);
		std::map<MediaMasterID, MasterInfo>::const_iterator it = _masterInfo.find(mid);
		if(it!=_masterInfo.end()) {
			const MasterInfo& mi = it->second;
			std::map<MediaMasterID, MediaMaster::Flavour>::const_iterator flit = flavours.find(mid);
			if(flit!=flavours.end()) {
				values[flit->second] *= mi._manual * mi._sequence;
			}
			else {
				// Unknown master? shouldn't happen!
			}
		}
	}
}

float MediaMasters::GetMasterValue(const MediaMasterList& masterList) {
	float v = 1.0f;

	MediaMasterList::size_type p = masterList.find_first_of(L',');
	if(p==MediaMasterList::npos) {
		// just one master
		std::map<MediaMasterID, MasterInfo>::const_iterator it = _masterInfo.find(masterList);
		if(it!=_masterInfo.end()) {
			const MasterInfo& mi = it->second;
			v *= mi._manual * mi._sequence;
		}
	}
	else {
		MediaMasterList::size_type start = 0;
		MediaMasterList::size_type end = 0;

		while((end = masterList.find (L',', start)) != std::wstring::npos) {
			const MediaMasterID& mid = masterList.substr(start, end-start);
			std::map<MediaMasterID, MasterInfo>::const_iterator it = _masterInfo.find(mid);
			if(it!=_masterInfo.end()) {
				const MasterInfo& mi = it->second;
				v *= mi._manual * mi._sequence;
			}
			start = end + 1;
		};

		// last part
		const MediaMasterID& mid = masterList.substr(start);
		std::map<MediaMasterID, MasterInfo>::const_iterator it = _masterInfo.find(mid);
		if(it!=_masterInfo.end()) {
			const MasterInfo& mi = it->second;
			v *= mi._manual * mi._sequence;
		}
	}
	return v;
}

float MediaMasters::GetMasterValue(const MediaMasterID& mid, MediaMaster::Source source) {
	std::map<MediaMasterID, MasterInfo>::const_iterator it = _masterInfo.find(mid);
	if(it!=_masterInfo.end()) {
		const MasterInfo& mi = it->second;
		switch(source) {
			case MediaMaster::MasterSourceSequence:
				return mi._sequence;

			case MediaMaster::MasterSourceManual:
				return mi._manual;
		}
	}

	return 1.0f;
}

MediaMasters::MasterChangedNotification::MasterChangedNotification(const MediaMasterID& mid): _mid(mid) {
}

void MediaMasters::Reset() {
	ThreadLock lock(&_mastersLock);
	_masters.clear();
}

void MediaMasters::SetMasterValue(const MediaMasterID& mid, float v, MediaMaster::Source source) {
	ThreadLock lock(&_mastersLock);

	switch(source) {
		case MediaMaster::MasterSourceSequence:
			_masterInfo[mid]._sequence = v;
			break;

		case MediaMaster::MasterSourceManual:
			_masterInfo[mid]._manual = v;
			break;
	}

	EventMasterChanged.Fire(this, MasterChangedNotification(mid));
}


ref<Property> MediaMasters::CreateMasterListProperty(const std::wstring& name, ref<Inspectable> holder, MediaMasterList* masterList) {
	ref<SuggestionProperty> sp = GC::Hold(new SuggestionProperty(name, holder, masterList));
	sp->SetSuggestionMode(SuggestionEditWnd::SuggestionModeInsert);
	strong<Menu> menu = sp->GetSuggestionMenu();
	sp->SetHint(TL(media_masters_hint));

	{
		ThreadLock lock(&_mastersLock);
		std::vector< weak<MediaMaster> >::const_iterator it = _masters.begin();
		while(it!=_masters.end()) {
			ref<MediaMaster> mmt = *it;
			if(mmt) {
				const std::wstring& mid = mmt->GetMasterID();
				menu->AddItem(ref<MenuItem>(GC::Hold(new SuggestionMenuItem(mid + L",",mid))));
			}
			++it;
		}
	}

	return sp;
}