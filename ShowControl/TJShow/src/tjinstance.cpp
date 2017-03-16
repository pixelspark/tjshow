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
using namespace tj::show;
using namespace tj::shared;

/** NamedInstance **/
NamedInstance::NamedInstance(const std::wstring& n, ref<Instance> c): _name(n), _controller(c) {
}

NamedInstance::~NamedInstance() {
}

/** Instances **/
Instances::Instances(strong<Model> model, strong<Network> net): _model(model) {
	strong<Timeline> rootTimeline = model->GetTimeline();
	_root = GC::Hold(new Controller(rootTimeline, net, rootTimeline->GetLocalVariables(), _model->GetVariables(), false));
}

Instances::~Instances() {
}

void Instances::Clear() {
	_root->Clear();
}

strong<Instance> Instances::GetRootInstance() {
	return _root;
}

void Instances::RecursiveGetStaticInstances(strong<Instance> parent, std::vector< ref<NamedInstance> >& list, int level) {
	strong<Timeline> tl = parent->GetTimeline();
	ref<Iterator<ref<TrackWrapper> > > it = tl->GetTracks();
	while(it->IsValid()) {
		ref<TrackWrapper> tw = it->Get();

		ref<SubTimeline> sub = tw->GetSubTimeline();
		if(sub) {
			std::wostringstream name;
			for(int a=0;a<level;a++) {
				name << L" ";
			}
			name << tw->GetInstanceName();

			ref<Instance> subInstance = sub->GetInstance();
			if(subInstance) {
				list.push_back(GC::Hold(new NamedInstance(name.str(), subInstance)));
				RecursiveGetStaticInstances(subInstance, list, level+1);
			}
		}

		it->Next();
	}
}

void Instances::GetStaticInstances(std::vector< ref<NamedInstance> >& lst) {
	lst.push_back(GC::Hold(new NamedInstance(TL(timeline), GetRootInstance())));
	RecursiveGetStaticInstances(_root, lst, 1);
}

/** Finds a controller based on the timeline id (Timeline::_id). **/
ref<Instance> Instances::GetInstanceByTimelineID(const std::wstring& tlid, ref<Instance> parent) {
	if(!parent) {
		parent = _root;
	}

	ref<Timeline> tl = parent->GetTimeline();

	if(tl->GetID()==tlid) {
		return parent;
	}
	else {
		ref<Iterator<ref<TrackWrapper> > > it = tl->GetTracks();
		while(it->IsValid()) {
			ref<TrackWrapper> tw = it->Get();
			ref<SubTimeline> sub = tw->GetSubTimeline();
			if(sub) {
				ref<Instance> cr = GetInstanceByTimelineID(tlid, sub->GetInstance());
				if(cr) {
					return cr;
				}
			}

			it->Next();
		}
	}

	return 0;
}

/** Instance **/
Instance::~Instance() {
}

/** InstanceHolder **/
InstanceHolder::~InstanceHolder() {
}