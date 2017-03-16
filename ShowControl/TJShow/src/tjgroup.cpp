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
#include <limits>
#undef max
using namespace tj::show;
using namespace tj::np;

/** ChannelAllocation **/
ChannelAllocation::ChannelAllocation(): _channel(0) {
}

ChannelAllocation::ChannelAllocation(strong<Group> group, ref<Instance> inst) {
	Allocate(group, inst);
}

ChannelAllocation::~ChannelAllocation() {
	Deallocate();
}

Channel ChannelAllocation::GetChannel() const {
	return _channel;
}

ref<Group> ChannelAllocation::GetGroup() {
	return _group;
}

void ChannelAllocation::Deallocate() {
	if(_channel!=0) {
		ref<Group> group = _group;
		if(group) {
			group->DeallocateChannel(_channel);
		}
	}
}

void ChannelAllocation::Allocate(strong<Group> group, ref<Instance> inst) {
	Deallocate();
	_group = ref<Group>(group);
	_channel = group->AllocateChannel(inst);
	Log::Write(L"TJShow/ChannelAllocation", L"Allocation in group "+group->GetName()+L" ch="+StringifyHex(_channel));
}

/** Group **/
Group::Group(GroupID gid, const std::wstring &name): _gid(gid), _name(name), _color(0.0,1.0,0.0) {
}

Group::~Group() {
}

void Group::Load(TiXmlElement* me) {
	_name = LoadAttributeSmall(me, "name", _name);
	_gid = LoadAttributeSmall(me, "id", _gid);
	
	TiXmlElement* col = me->FirstChildElement("color");
	if(col!=0) {
		_color.Load(col);
	}
}

void Group::Save(TiXmlElement* me) {
	SaveAttributeSmall(me, "name", _name);
	SaveAttributeSmall(me, "id", _gid);

	TiXmlElement col("color");
	_color.Save(&col);
	me->InsertEndChild(col);
}

void Group::TakeOwnership(const Channel& ch, ref<Instance> inst) {
	ThreadLock lock(&_channelLock);
	_allocatedChannels[ch] = inst;
}

const RGBColor& Group::GetColor() const {
	return _color;
}

const std::wstring& Group::GetName() const {
	return _name;
}

GroupID Group::GetID() const {
	return _gid;
}

ref<Instance> Group::GetInstanceByChannel(const Channel& ch) {
	ThreadLock lock(&_channelLock);
	std::map<Channel, weak<Instance> >::iterator it = _allocatedChannels.find(ch);
	if(it!=_allocatedChannels.end()) {
		return it->second;
	}
	return null;
}

Channel Group::AllocateChannel(ref<Instance> allocatedTo) {
	const static Channel KMaxChannel = std::numeric_limits<Channel>::max()-1;
	ThreadLock lock(&_channelLock);
	size_t used = _allocatedChannels.size();
	if(used >= KMaxChannel) {
		Throw(L"Out of channel numbers for this group; this means there are too many tracks being played in this group. Try to move tracks from this group to a separate group.", ExceptionTypeError);
	}

	for(Channel c = (Channel)used+1; c < KMaxChannel; c++) {
		if(_allocatedChannels.find(c)==_allocatedChannels.end()) {
			_allocatedChannels[c] = allocatedTo;
			return c;
		}
	}

	for(Channel a = 1; (a < (Channel)used && a < KMaxChannel); a++) {
		if(_allocatedChannels.find(a)==_allocatedChannels.end()) {
			_allocatedChannels[a] = allocatedTo;
			return a;
		}
	}
	
	Throw(L"Could not allocate channel number from this group; this usually means that there are too many tracks playing back in this group. Try to move tracks from this group to a separate group.", ExceptionTypeError);
}

void Group::DeallocateChannel(const Channel& ch) {
	if(ch!=0) {
		ThreadLock lock(&_channelLock);
		std::map<Channel, weak<Instance> >::iterator it = _allocatedChannels.find(ch);
		if(it!=_allocatedChannels.end()) {
			_allocatedChannels.erase(it);
		}
	}
}

/** Groups **/
Groups::Groups(): _defaultGroup(GC::Hold(new Group())) {
}

Groups::~Groups() {
}

void Groups::Load(TiXmlElement* you) {
	TiXmlElement* group = you->FirstChildElement("group");
	while(group!=0) {
		strong<Group> groupObject = GC::Hold(new Group());
		groupObject->Load(group);
		_groups.push_back(groupObject);
		group = group->NextSiblingElement("group");
	}
}

void Groups::Save(TiXmlElement* me) {
	ThreadLock lock(&_lock);
	std::vector< strong<Group> >::iterator it = _groups.begin();
	while(it!=_groups.end()) {
		strong<Group> group = *it;
		TiXmlElement groupElement("group");
		group->Save(&groupElement);
		me->InsertEndChild(groupElement);
		++it;
	}
}

void Groups::Clear() {
	ThreadLock lock(&_lock);
	_groups.clear();
}

GroupID Groups::GetHighestGroupID() const {
	ThreadLock lock(&_lock);
	GroupID highest = 0;
	std::vector< strong<Group> >::const_iterator it = _groups.begin();
	while(it!=_groups.end()) {
		strong<Group> group = *it;
		GroupID g = group->GetID();
		if(g > highest) {
			highest = g;
		}
		++it;
	}

	return highest;
}

bool Groups::Add(strong<Group> group) {
	ThreadLock lock(&_lock);

	// Check if there is a group with the same ID
	std::vector< strong<Group> >::iterator it = _groups.begin();
	while(it!=_groups.end()) {
		strong<Group> g = *it;
		if(g->GetID()==group->GetID()) {
			return false;
		}
		++it;
	}

	_groups.push_back(group);
	return true;
}

strong<Group> Groups::GetGroupByIndex(unsigned int i) {
	ThreadLock lock(&_lock);
	return _groups.at(i);
}

unsigned int Groups::GetGroupCount() const {
	ThreadLock lock(&_lock);
	return (unsigned int)_groups.size();
}

void Groups::RemoveGroup(unsigned int i) {
	ThreadLock lock(&_lock);
	_groups.erase(_groups.begin() + i);
}

ref<Group> Groups::GetGroupById(GroupID gid) {
	if(gid==0) {
		return _defaultGroup;
	}

	ThreadLock lock(&_lock);
	std::vector< strong<Group> >::iterator it = _groups.begin();
	while(it!=_groups.end()) {
		strong<Group> group = *it;
		if(group->GetID()==gid) {
			return group;
		}
		++it;
	}
	return null;
}

ref<Property> Groups::CreateGroupProperty(const std::wstring& title, ref<Inspectable> holder, GroupID* gid) {
	ref<GenericListProperty<GroupID> > gl = GC::Hold(new GenericListProperty<GroupID>(title, holder, gid, *gid, L"icons/group.png"));
	gl->AddOption(TL(group_all), 0);

	if(_groups.size()>0) {
		gl->AddSeparator();
		std::vector< strong<Group> >::iterator it = _groups.begin();
		while(it!=_groups.end()) {
			strong<Group> group = *it;
			gl->AddOption(group->GetName(), group->GetID());
			++it;
		}
	}

	return gl;
}

#include <set>

/* Filter */
Filter::Filter() {
	Clear();
}

Filter::~Filter() {
}

void Filter::Parse(const std::wstring& filterString) {
	std::vector<std::wstring> addresses = Explode(filterString, std::wstring(L" "));
	std::vector<std::wstring>::iterator it = addresses.begin();

	while(it!=addresses.end()) {
		std::wstring address = *it;

		if(address.find(L"-")!=std::wstring::npos) {
			 // A range was specified
			int first = -1;
			int last = -1;
			wchar_t dummy;
			std::wistringstream is(address);
			is >> first;
			is >> dummy;
			is >> last;
			if(first<0 || last<0 || last<first) {++it; continue;}
			if(first>KMaxGroupID || last>KMaxGroupID) {++it; continue;}

			for(int a=first;a<=last;a++) {
				_member.insert((GroupID)a);
			}
		}
		else {
			int channel = -1;
			std::wistringstream is(address);
			is >> channel;
			if(channel<0) {++it; continue;}
			if(channel>KMaxGroupID) { ++it; continue; }
			_member.insert(channel);
		}
		++it;
	}
}

std::wstring Filter::Dump() const {
	std::wostringstream wos;

	std::set<GroupID>::const_iterator it = _member.begin();
	while(it!=_member.end()) {
		const GroupID& gid = *it;
		wos << gid << L" ";
		++it;
	}
	return wos.str();
}

std::wstring Filter::DumpFriendly(strong<Groups> groups) const {
	std::wostringstream wos;

	std::set<GroupID>::const_iterator it = _member.begin();
	while(it!=_member.end()) {
		const GroupID& gid = *it;
		ref<Group> group = groups->GetGroupById(gid);
		if(group) {
			wos << group->GetName();
		}
		else {
			wos << gid;
		}

		++it;
		if(it!=_member.end()) {
			wos << L", ";
		}
	}
	return wos.str();
}

bool Filter::IsEmpty() const {
	return _member.size()==0;
}

void Filter::Clear() {
	_member.insert((GroupID)0);
	_member.clear();
}

bool Filter::IsMemberOf(GroupID g) const {
	return _member.find(g) != _member.end();
}

void Filter::Save(TiXmlElement* me) {
	std::set<GroupID>::const_iterator it = _member.begin();
	while(it!=_member.end()) {
		GroupID gid = *it;
		TiXmlElement group("group");
		SaveAttributeSmall(&group, "id", gid);
		me->InsertEndChild(group);
		++it;
	}
}

void Filter::Load(TiXmlElement* me) {
	TiXmlElement* group = me->FirstChildElement("group");
	while(group!=0) {
		GroupID gid = LoadAttributeSmall(group, "group", (GroupID)0);
		_member.insert(gid);
		group = group->NextSiblingElement("group");
	}
}

void Filter::AddGroupMembership(GroupID gid) {
	_member.insert(gid);
}

void Filter::RemoveGroupMembership(GroupID gid) {
	_member.erase(gid);
}