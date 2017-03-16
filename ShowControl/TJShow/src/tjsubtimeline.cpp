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
#include "../include/internal/tjcontroller.h"
#include "../include/internal/view/tjtimelinewnd.h"
#include "../include/internal/view/tjsplittimelinewnd.h"
#include "../include/internal/tjsubtimeline.h"
#include "../include/internal/tjscriptapi.h"
using namespace tj::shared::graphics;
using namespace tj::show::view;

SubTimelineTrack::SubTimelineTrack() {
	_time = GC::Hold(new Timeline());
	_controller = GC::Hold(new Controller(_time, Application::Instance()->GetNetwork(), _time->GetLocalVariables(), ref<Variables>(Application::Instance()->GetModel()->GetVariables()), false));
	_initiallyVisible = true;
}

SubTimelineTrack::~SubTimelineTrack() {
	/* Removing a sub-timeline should cause all contained tracks to be removed. Normally, this would be
	automatic (by means of the reference counting) but since tracks need to be explicitly removed from the view
	using View::OnRemoveTrack, that's what we do right here... */
	if(_time) {
		ref<View> view = Application::Instance()->GetView();
		if(view) {
			ref< Iterator< ref<TrackWrapper> > > it = _time->GetTracks();
			if(it) {
				while(it->IsValid()) {
					ref<TrackWrapper> tw = it->Get();
					if(tw) {
						view->OnRemoveTrack(tw);	
					}
					it->Next();
				}
			}
		}
		_time->Clear();
	}
}

Pixels SubTimelineTrack::GetHeight() {
	return 19;
}

void SubTimelineTrack::Clone() {
	_time->Clone();
}

void SubTimelineTrack::Save(TiXmlElement* parent) {
	_time->Save(parent);
	_time->GetCueList()->Save(parent);

	TiXmlElement locals("locals");
	_time->GetLocalVariables()->Save(&locals);
	parent->InsertEndChild(locals);

	ref<SplitTimelineWnd> sw = _controller->GetSplitTimelineWindow();
	if(sw) {
		SaveAttribute<std::wstring>(parent, "visible", Application::Instance()->GetView()->GetRootWindow()->IsOrphanPane(sw)?L"no":L"yes");
	}
}

void SubTimelineTrack::GetResources(std::vector<ResourceIdentifier>& rids) {
	 _time->GetResources(rids);
}

void SubTimelineTrack::Load(TiXmlElement* you) {
	TiXmlElement* tracks = you->FirstChildElement("tracks");
	if(tracks!=0) {
		_time->Load(tracks, ref<Instance>(_controller));
	}

	TiXmlElement* cues = you->FirstChildElement("cues");
	if(cues!=0) {
		_time->GetCueList()->Load(cues);
	}

	TiXmlElement* locals = you->FirstChildElement("locals");
	if(locals!=0) {
		_time->GetLocalVariables()->Load(locals);
	}

	_initiallyVisible = LoadAttribute<std::wstring>(you, "visible", _initiallyVisible?L"yes":L"no")==L"yes";
}

ref<Timeline> SubTimelineTrack::GetTimeline() {
	return _time;
}

void SubTimelineTrack::OnSetInstanceName(const std::wstring& name) {
	_time->SetName(name);
}

std::wstring SubTimelinePlugin::GetFriendlyName() const {
	return TL(sub_timeline);
}

std::wstring SubTimelinePlugin::GetFriendlyCategory() const {
	return L"";
}

std::wstring SubTimelinePlugin::GetDescription() const {
	return TL(sub_timeline_description);
}

Flags<RunMode> SubTimelineTrack::GetSupportedRunModes() {
	Flags<RunMode> fl;
	return fl;
}

std::wstring SubTimelineTrack::GetTypeName() const { 
	return TL(sub_timeline);
}

std::wstring SubTimelineTrack::GetInstanceName() const {
	return _time->GetName();
}

ref<Player> SubTimelineTrack::CreatePlayer(ref<Stream> str) {
	return 0;
}

ref<Scriptable> SubTimelineTrack::Execute(Command c, ref<ParameterList> p) {
	if(c==L"tracks") {
		return GC::Hold(new tj::show::script::TracksIterator(_controller->GetTimeline()));
	}
	else {
		return GC::Hold(new tj::show::script::InstanceScriptable(ref<Instance>(_controller)))->Execute(c,p);
	}
}

ref<Scriptable> SubTimelineTrack::GetScriptable() {
	return this;
}

ref<PropertySet> SubTimelineTrack::GetProperties() {
	return _controller->GetTimeline()->GetProperties();
}

void SubTimelineTrack::Paint(tj::shared::graphics::Graphics* g, Time position, float pixelsPerMs, unsigned int x, unsigned int y, unsigned int h, Time start, Time end) {
}

ref<Item> SubTimelineTrack::GetItemAt(Time position,unsigned int height, bool rightClick, int trackHeight, float pixelsPerMs) {
	return 0;
}

ref<LiveControl> SubTimelineTrack::CreateControl(ref<Stream> str) {
	return GC::Hold(new SubTimelineControl(this, _initiallyVisible));
}

ref<Controller> SubTimelineTrack::GetController() {
	return _controller;
}

ref<Instance> SubTimelineTrack::GetInstance() {
	return ref<Instance>(_controller);
}

// SubTimelineControl
SubTimelineControl::SubTimelineControl(ref<SubTimelineTrack> track, bool iv) {
	_track = track;
	_initiallyVisible = iv;
}

SubTimelineControl::~SubTimelineControl() {
}

bool SubTimelineControl::IsInitiallyVisible() {
	return _initiallyVisible;
}

ref<Wnd> SubTimelineControl::GetWindow() {
	ref<SplitTimelineWnd> wnd = _wnd;
	if(!wnd) {
		// sub timeline controls are handled differently by the view component
		ref<SubTimelineTrack> track = _track;
		if(track) {
			wnd = GC::Hold(new SplitTimelineWnd(ref<Instance>(track->_controller)));
		}
		else {
			Throw(L"Could not create timeline view in SubTimelineControl", ExceptionTypeError);
		}
		_wnd = wnd;
	}
	
	return wnd;
}

bool SubTimelineControl::IsSeparateTab() {
	return true;
}

std::wstring SubTimelineControl::GetGroupName() {
	return L""; // not applicable for separate tab controls
}

void SubTimelineControl::Update() {
	GetWindow()->Update();
}

// SubTimelinePlugin
SubTimelinePlugin::SubTimelinePlugin() {
}

SubTimelinePlugin::~SubTimelinePlugin() {
}

std::wstring SubTimelinePlugin::GetName() const {
	return L"SubTimeline";
}

ref<Track> SubTimelinePlugin::CreateTrack(ref<Playback> pb) {
	return GC::Hold(new SubTimelineTrack());
}

ref<StreamPlayer> SubTimelinePlugin::CreateStreamPlayer(ref<Playback> pb, ref<Talkback> talk) {
	return 0;
}

std::wstring SubTimelinePlugin::GetVersion() const {
	return L"";
}

std::wstring SubTimelinePlugin::GetAuthor() const {
	return L"";
}