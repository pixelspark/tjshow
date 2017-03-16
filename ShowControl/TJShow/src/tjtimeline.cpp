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
#include "../include/internal/tjsubtimeline.h"
#include <algorithm>

Timeline::Timeline(Time length): 
	_cues(GC::Hold(new CueList())),
	_locals(GC::Hold(new VariableList())),
	_timeLength(length),
	_defaultSpeed(1.0f),
	_singleton(false),
	_remoteControlAllowed(false) {

	NewID();
}

Timeline::~Timeline() {
}

Time Timeline::GetTimeLengthMS() const {
	return max(Time(0), _timeLength);
}
 
void Timeline::SetDefaultSpeed(float s) {
	_defaultSpeed = s;
}

float Timeline::GetDefaultSpeed() const {
	return _defaultSpeed;
}
 
std::wstring Timeline::GetName() const {
	return _name;
}

bool Timeline::IsSingleton() const {
	return _singleton;
}

void Timeline::SetSingleton(bool t) {
	_singleton = t;
}

bool Timeline::IsRemoteControlAllowed() const {
	return _remoteControlAllowed;
}

void Timeline::SetRemoteControlAllowed(bool r) {
	_remoteControlAllowed = r;
}

void Timeline::NewID() {
	_id = Util::RandomIdentifier(L'Q');
}

void Timeline::Interpolate(Time old, Time newTime, Time left, Time right) {
	if(right<=left) return; // cannot interpolate
	if(newTime <= left || newTime >= right) return; 

	std::vector< ref<TrackWrapper> >::iterator it = _tracks.begin();
	float leftRatio = float(newTime-left)/float(old-left);
	float rightRatio = float(newTime-right)/float(old-right);

	while(it!=_tracks.end()) {
		ref<TrackWrapper> track = *it;
		if(!track) {
			++it;
			continue; 
		}

		// Move all items before the old time
		ref<TrackRange> tr = track->GetTrack()->GetRange(left, old);
		if(tr) {
			tr->InterpolateLeft(left, leftRatio, old);
		}

		// Move all items after the old time
		tr = track->GetTrack()->GetRange(old, right);
		if(tr) {
			tr->InterpolateRight(old, rightRatio, right);
		}

		++it;
	}
}

bool Timeline::IsParentOf(ref<TrackWrapper> tw, bool recursive) const {
	std::vector< ref<TrackWrapper> >::const_iterator it = _tracks.begin();
	while(it!=_tracks.end()) {
		if(*it==tw) {
			return true;
		}

		if(recursive && *it) {
			ref<TrackWrapper> t = *it;
			if(t->IsSubTimeline()) {
				ref<SubTimelineTrack> stt = ref<Track>(t->GetTrack());
				if(stt) {
					ref<Timeline> childTimeline = stt->GetTimeline();
					if(childTimeline && childTimeline.GetPointer()!=this && childTimeline->IsParentOf(tw, true)) {
						return true;
					}
				}
			}
		}
		++it;
	}
	return false;
}

void Timeline::Clone() {
	NewID();

	std::vector< ref<TrackWrapper> >::iterator it = _tracks.begin();
	while(it!=_tracks.end()) {
		ref<TrackWrapper> tw = *it;
		tw->Clone();
		++it;
	}

	_cues->Clone();
}

void Timeline::SetName(const std::wstring& n) {
	_name = n;
}

const std::wstring& Timeline::GetID() const {
	return _id;
}

bool Timeline::IsResourceRequired(const std::wstring& rid) {
	std::vector< ref<TrackWrapper> >::iterator it = _tracks.begin();
	while(it!=_tracks.end()) {
		ref<TrackWrapper> tw = *it;

		if(tw->RequiresResource(rid)) return true; // RequiresResource should also work for sub timelines?
		++it;
	}
	return false;
}

void Timeline::RemoveItemsBetween(Time a, Time b) { 
	std::vector< ref<TrackWrapper> >::iterator it = _tracks.begin();
	while(it!=_tracks.end()) {
		ref<TrackWrapper> tw = *it;
		ref<Track> track = tw->GetTrack();
		if(track) {
			ref<TrackRange> range = track->GetRange(a,b);

			if(range) {
				range->RemoveItems();
			}
		}
		++it;
	}
}

std::vector< ref<TrackRange> > Timeline::CopyItemsBetween(Time a, Time b) {
	std::vector< ref<TrackRange> > ranges;

	std::vector< ref<TrackWrapper> >::iterator it = _tracks.begin();
	while(it!=_tracks.end()) {
		ref<TrackWrapper> tw = *it;
		ref<Track> track = 0;
		
		if(tw && (track = tw->GetTrack())) {
			ref<TrackRange> rng = track->GetRange(a,b);

			if(rng) {
				ranges.push_back(rng);
			}
			else {
				ranges.push_back(0); // no range
			}
		}
		++it;
	}

	return ranges;
}


void Timeline::PasteItems(std::vector< ref<TrackRange> >& sel, Time start) {
	if(sel.size()!=_tracks.size()) {
		return;
	}

	std::vector< ref<TrackWrapper> >::iterator it = _tracks.begin();
	std::vector< ref<TrackRange> >::iterator itt = sel.begin();

	while(it!=_tracks.end()) {
		ref<TrackWrapper> tw = *it;
		ref<TrackRange> tr = *itt;
		ref<Track> track = 0;

		if(tw && tr && (track = tw->GetTrack())) {
			tr->Paste(track, start);
		}
		++it;
		itt++;
	}
}

void Timeline::SetTimeLengthMS(Time t) {
	if(t<_timeLength) {
		// Remove items that 'fall off' the timeline
		if(_tracks.size()>0) {
			RemoveItemsBetween(t+Time(1), _timeLength+Time(1));
		}
		_cues->RemoveCuesBetween(t+Time(1), _timeLength+Time(1));
	}
	_timeLength = t;
}

void Timeline::AddTrack(ref<TrackWrapper> tr) {
	_tracks.push_back(tr);
}

void Timeline::Clear() {
	_tracks.clear();
	_cues->Clear();
	_locals->Clear();
	_name = L"";
	_timeLength = DefaultTimelineLength;
	_defaultSpeed = 1.0f;
	_singleton = false;
	_remoteControlAllowed = false;
}

strong<CueList> Timeline::GetCueList() {
	return _cues;
}

void Timeline::SetCueList(strong<CueList> cl) {
	_cues = cl;
}

strong<VariableList> Timeline::GetLocalVariables() {
	return _locals;
}

void Timeline::SetLocalVariables(strong<VariableList> vars) {
	_locals = vars;
}

void Timeline::MoveUp(ref<TrackWrapper> tr) {
	int items = (int)_tracks.size();
	for(int a=0;a<items;a++) {
		ref<TrackWrapper> track = _tracks.at(a);
		if(track==tr) {
			if(a-1>=0) {
				ref<TrackWrapper> next = _tracks.at(a-1);
				_tracks[a] = next;
				_tracks[a-1] = track;
				break;
			}
		}
	}

	Application::Instance()->GetView()->OnMoveTrackUp(tr);
}

void Timeline::MoveDown(ref<TrackWrapper> tr) {
	size_t items = _tracks.size();
	for(size_t a=0;a<items;a++) {
		ref<TrackWrapper> track = _tracks.at(a);
		if(track==tr) {
			if(a+1<items) {
				ref<TrackWrapper> next = _tracks.at(a+1);
				_tracks[a] = next;
				_tracks[a+1] = track;
				break;
			}
		}
	}

	Application::Instance()->GetView()->OnMoveTrackDown(tr);
}

void Timeline::Save(TiXmlElement* parent) {
	TiXmlElement tracks("tracks");
	SaveAttributeSmall(&tracks, "length",_timeLength.ToInt());
	SaveAttributeSmall<float>(&tracks, "default-speed", _defaultSpeed);
	SaveAttributeSmall<std::wstring>(&tracks, "name", _name);
	SaveAttributeSmall<std::wstring>(&tracks, "id", _id);
	SaveAttributeSmall(&tracks, "singleton", _singleton);
	SaveAttributeSmall(&tracks, "remote-control", _remoteControlAllowed);

	if(_description.length()>0) {
		SaveAttribute<std::wstring>(&tracks, "description", _description);
	}

	std::vector< ref<TrackWrapper> >::iterator it = _tracks.begin();
	while(it!=_tracks.end()) {
		ref<TrackWrapper> track = *it;
		TiXmlElement trackElement("track");
		track->Save(&trackElement);
		tracks.InsertEndChild(trackElement);

		++it;
	}
	parent->InsertEndChild(tracks);
}

ref<TrackWrapper> Timeline::GetTrackByName(const std::wstring& nx) {
	std::wstring n = nx;
	std::transform(n.begin(),n.end(), n.begin(), tolower);

	std::vector< ref<TrackWrapper> >::iterator it = _tracks.begin();
	while(it!=_tracks.end()) {
		ref<TrackWrapper> tw = *it;
		std::wstring name = tw->GetInstanceName();
		std::transform(name.begin(), name.end(), name.begin(), tolower);
		if(name==n) {
			return tw;
		}
		++it;
	}

	return 0;
}

ref<TrackWrapper> Timeline::GetTrackByID(const TrackID& tid, bool deep) {
	std::vector< ref<TrackWrapper> >::iterator it = _tracks.begin();
	while(it!=_tracks.end()) {
		ref<TrackWrapper> tw = *it;
	
		if(tw->GetID()==tid) {
			return tw;
		}
		else if(deep && tw->IsSubTimeline()) {
			ref<SubTimelineTrack> stt = ref<Track>(tw->GetTrack());
			if(stt) {
				ref<TrackWrapper> track = stt->GetTimeline()->GetTrackByID(tid, deep);
				if(track) {
					return track;
				}
			}
		}
		++it;
	}

	return null;
}

void Timeline::Load(TiXmlElement* you) {
	Throw(L"Not implemented", ExceptionTypeSevere);
}

void Timeline::Load(TiXmlElement* tracks, strong<Instance> instance) {
	/* newer versions use this to store their time, but the model loader will call SetTimeLength
	 when it encounters a <time>-tag with timeline length in seconds, which older versions used. */
	_timeLength = Time(LoadAttributeSmall<int>(tracks, "length", _timeLength.ToInt()));
	_name = LoadAttributeSmall<std::wstring>(tracks, "name", _name);
	_defaultSpeed = LoadAttributeSmall<float>(tracks, "default-speed", _defaultSpeed);
	_id = LoadAttributeSmall<std::wstring>(tracks, "id", _id);
	_description = LoadAttribute<std::wstring>(tracks, "description", _description);
	_singleton = LoadAttributeSmall<bool>(tracks, "singleton", _singleton);
	_remoteControlAllowed = LoadAttributeSmall<bool>(tracks, "remote-control", _remoteControlAllowed);

	TiXmlElement* track = tracks->FirstChildElement("track");
	ref<EventLogger> logger = Application::Instance()->GetEventLogger();

	bool errors = false;
	while(track!=0) {		
		PluginHash hash = StringTo<PluginHash>(track->Attribute("plugin"), 0);
		
		try {
			/* If a hash is given (newer versions), use it to locate the plugin. If a hash is not present
			 or the plugin cannot be found, use the type. Later versions store both type and hash in the file */
			ref<PluginWrapper> plug;
			if(hash!=0) {
				plug = PluginManager::Instance()->GetPluginByHash(hash);
			}

			if(!plug) {
				// TODO: Create a stub track that just saves the xml information
				std::wstring name = LoadAttributeSmall<std::wstring>(track, "name", L"");
				logger->AddEvent(TL(error_plugin_not_found_for_track)+name, ExceptionTypeError, false);
			}
			else {
				ref<Application> app = Application::InstanceReference();
				ref<view::View> view = app->GetView();
				ref<TrackWrapper> nt = plug->CreateTrack(instance->GetPlayback(), app->GetNetwork(), instance);
				if(nt) {
					nt->Load(track);
					AddTrack(nt);
					view->OnAddTrack(nt);
				}
				else {
					errors = true;
				}
			}
		}
		catch(Exception&) {
			errors = true;
		}

		// Show an error message once if a file failed to load
		if(errors) {
			Alert::Show(TL(error), TL(could_not_load_track_from_file), Alert::TypeError);
		}

		track = track->NextSiblingElement("track");
	}
}

void Timeline::RemoveTrack(ref<TrackWrapper> tr) {
	std::vector< ref<TrackWrapper> >::iterator it = _tracks.begin();
	while(it!=_tracks.end()) {
		ref<TrackWrapper> track = *it;
		if(track==tr) {
			it = _tracks.erase(it);
		}
		else {
			++it;
		}
	}
	Application::Instance()->GetView()->OnRemoveTrack(tr);
}

unsigned int Timeline::GetTrackCount() {
	return (unsigned int)_tracks.size();
}

void Timeline::GetResources(std::vector< ResourceIdentifier>& rc) { 
	std::vector< ref<TrackWrapper> >::iterator it = _tracks.begin();
	while(it!=_tracks.end()) {
		ref<TrackWrapper> track = *it;
		std::vector<ResourceIdentifier> rs;
		track->GetTrack()->GetResources(rs);

		if(rs.size()>0) {
			std::vector<ResourceIdentifier>::iterator ix = rs.begin();
			while(ix!=rs.end()) {
				const ResourceIdentifier& resource = *ix;
				rc.push_back(resource);
				ix++;
			}
		}
		++it;
	}
}

ref< Iterator< ref<TrackWrapper> > > Timeline::GetTracks() {
	return GC::Hold(new VectorIterator< ref<TrackWrapper> >(_tracks));
}

ref<PropertySet> Timeline::GetProperties() {
	ref<PropertySet> prs = GC::Hold(new PropertySet());

	prs->Add(GC::Hold(new GenericProperty<std::wstring>(TL(timeline_name), this, &_name, _name)));
	prs->Add(GC::Hold(new TextProperty(TL(timeline_description), this, &_description, 150)));

	class TimelineLengthLinkedProperty: public virtual Object, public LinkProperty, public Inspectable {
		public:
			TimelineLengthLinkedProperty(strong<Timeline> st): LinkProperty(TL(timeline_length), st->_timeLength.Format(), L"icons/timelines.png"), _timeline(st) {
			}

			virtual ~TimelineLengthLinkedProperty() {
			}

			virtual void OnClicked() {
				ref<PropertyDialogWnd> pdw = GC::Hold(new PropertyDialogWnd(TL(set_timeline_length), TL(set_timeline_length_question)));
				pdw->GetPropertyGrid()->Inspect(this);

				if(pdw->DoModal(LinkProperty::_wnd)) {
					if(_newLength >= _timeline->GetTimeLengthMS() || Alert::ShowYesNo(TL(set_timeline_length), TL(timeline_shrink_warning), Alert::TypeWarning, true)) {
						_timeline->SetTimeLengthMS(_newLength);
					}
				}

				if(LinkProperty::_wnd) {
					std::wstring nlt = _timeline->GetTimeLengthMS().Format();
					LinkProperty::_wnd->SetText(nlt.c_str());
				}
			}

			virtual ref<PropertySet> GetProperties() {
				_newLength = _timeline->GetTimeLengthMS();
				ref<PropertySet> ps = GC::Hold(new PropertySet());
				ps->Add(GC::Hold(new GenericProperty<Time>(TL(timeline_length), this, &_newLength, _newLength)));
				return ps;
			}

		protected:
			Time _newLength;
			strong<Timeline> _timeline;
	};

	prs->Add(GC::Hold(new TimelineLengthLinkedProperty(strong<Timeline>(this))));
	prs->Add(GC::Hold(new GenericProperty<bool>(TL(enable_ep_endpoint), this, &_remoteControlAllowed, _remoteControlAllowed)));
	return prs;
}

ref<TrackWrapper> Timeline::GetTrackByIndex(int idx) {
	return _tracks.at(idx);
}

namespace tj {
	namespace show {
		class TimelineCrumb: public Crumb {
			public:
				TimelineCrumb(ref<Timeline> tl): Crumb(TL(timeline), L"icons/timeline.png") {
					_timeline = tl;
				}

				virtual ~TimelineCrumb() {
				}

				virtual ref<Inspectable> GetSubject() {
					return ref<Timeline>(_timeline);
				}

				virtual void GetChildren(std::vector< ref<Crumb> >& cs) {
					ref<Timeline> tl = _timeline;
					if(tl) {
						ref< Iterator< ref<TrackWrapper> > > it = tl->GetTracks();
						while(it->IsValid()) {
							ref<TrackWrapper> track = it->Get();
							if(track) {
								cs.push_back(GC::Hold(new BasicCrumb(track->GetInstanceName(), L"icons/track.png", track)));
							}
							it->Next();
						}
					}
				}

			protected:
				weak<Timeline> _timeline;

		};
	}
}

ref<Path> Timeline::CreatePath() {
	ref<Path> path = GC::Hold(new Path());
	path->Add(Application::Instance()->GetModel()->CreateModelCrumb());
	path->Add(GC::Hold(new TimelineCrumb(this)));
	return path;
}

ref<Crumb> Timeline::CreateCrumb() {
	return GC::Hold(new TimelineCrumb(this));
}