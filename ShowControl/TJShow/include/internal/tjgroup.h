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
#ifndef _TJGROUPS_H
#define _TJGROUPS_H

namespace tj {
	namespace show {
		class Group: public Serializable, public Inspectable {
			public:
				Group(GroupID gid = 0, const std::wstring& name = L"");
				virtual ~Group();
				virtual void Save(TiXmlElement* me);
				virtual void Load(TiXmlElement* me);
				virtual const std::wstring& GetName() const;
				virtual GroupID GetID() const;
				virtual const RGBColor& GetColor() const;

				virtual Channel AllocateChannel(ref<Instance> allocatedTo);
				virtual void DeallocateChannel(const Channel& ch);
				virtual ref<Instance> GetInstanceByChannel(const Channel& ch);
				virtual void TakeOwnership(const Channel& ch, ref<Instance> owner);

				std::wstring _name;
				GroupID _gid;
				RGBColor _color;

			protected:
				CriticalSection _channelLock;
				std::map<Channel, weak<Instance> > _allocatedChannels;
		};

		class ChannelAllocation {
			public:
				ChannelAllocation();
				ChannelAllocation(strong<Group> group, ref<Instance> inst);
				~ChannelAllocation();
				void Deallocate();
				void Allocate(strong<Group> group, ref<Instance> inst);
				Channel GetChannel() const;
				ref<Group> GetGroup();

			protected:
				weak<Group> _group;
				Channel _channel;
		};

		class Groups: public virtual Object, public Serializable {
			public:
				Groups();
				virtual ~Groups();
				virtual void Save(TiXmlElement* me);
				virtual void Load(TiXmlElement* me);
				virtual void Clear();
				virtual unsigned int GetGroupCount() const;
				virtual strong<Group> GetGroupByIndex(unsigned int i);
				virtual ref<Group> GetGroupById(GroupID gid);
				virtual void RemoveGroup(unsigned int i);
				virtual GroupID GetHighestGroupID() const;
				virtual bool Add(strong<Group> group);
				virtual ref<Property> CreateGroupProperty(const std::wstring& title, ref<Inspectable> holder, GroupID* gid);

			protected:
				mutable CriticalSection _lock;
				strong<Group> _defaultGroup;
				std::vector< strong<Group> > _groups;
		};

		class Filter: public Serializable {
			public:
				Filter();
				~Filter();
				void Parse(const std::wstring& fs);
				std::wstring Dump() const;
				std::wstring DumpFriendly(strong<Groups> groups) const;
				void Clear();
				bool IsEmpty() const;
				bool IsMemberOf(GroupID g) const;
				virtual void AddGroupMembership(GroupID gid);
				virtual void RemoveGroupMembership(GroupID gid);

				virtual void Save(TiXmlElement* me);
				virtual void Load(TiXmlElement* me);

			protected:
				std::set<GroupID> _member;
				const static GroupID KMaxGroupID = 65535;
		};
	}
}

#endif