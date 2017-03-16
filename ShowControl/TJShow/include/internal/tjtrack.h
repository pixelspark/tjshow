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
#ifndef _TJWRAPPERS_H
#define _TJWRAPPERS_H

namespace tj {
	namespace show {
		class Variables;
		class Timeline;
		class SubTimeline;
		class Instance;
		class Instancer;

		class VariableOutlet: public Outlet, public virtual Inspectable {
			public:
				VariableOutlet(const std::wstring& name, const std::wstring& id);
				virtual ~VariableOutlet();
				virtual bool IsConnected();
				virtual void Load(TiXmlElement* e);
				virtual void Save(TiXmlElement* s);
				virtual std::wstring GetID() const;
				virtual std::wstring GetName() const;
				virtual void SetName(const std::wstring& name);
				virtual const std::wstring& GetVariableID() const;
				
				std::wstring _varid;

			protected:
				std::wstring _id;
				std::wstring _name;
		};

		class TrackWrapper: public virtual Object, public Inspectable, public Serializable {
			friend class TrackStream;

			public:
				TrackWrapper(strong<Track> track, strong<PluginWrapper> plugin, ref<Network> net, ref<Instance> main);
				virtual ~TrackWrapper();

				std::wstring GetInstanceName();
				void SetInstanceName(const std::wstring& wn);

				TrackID GetID() const;
				void SetID(const TrackID& tid);

				GroupID GetGroup() const;
				void SetGroup(GroupID g);

				void SetLocked(bool l);
				bool IsLocked() const;

				RunMode GetRunMode() const;
				void SetRunMode(RunMode r);
				Flags<RunMode> GetSupportedRunModes();

				ref<PluginWrapper> GetPlugin();
				ref<LiveControl> GetLiveControl();
				std::wstring GetTypeName();
				bool RequiresResource(const ResourceIdentifier& rid);

				strong<Track> GetTrack();
				virtual ref<PropertySet> GetProperties();
				ref<TrackWrapper> Duplicate(strong<Instance> c);
				bool IsSubTimeline() const;
				bool IsInstancer() const;
				ref<SubTimeline> GetSubTimeline();
				ref<Instancer> GetInstancer();

				virtual void Save(TiXmlElement* me);
				virtual void Load(TiXmlElement* you);
				Pixels GetHeight(); // returns the height, or the minimum application track height if it is smaller

				unsigned int GetLastMessageTime() const;
				unsigned int GetLastTickTime() const;
				void SetLastTickTime(unsigned int t);
				void Clone(); // will reset any stored ID information such as cue ID's that might be used by external referencers
				ref<Outlet> GetOutletById(const std::wstring& id);
				ref<Outlet> GetOutletByHash(const OutletHash& hash);
				Channel GetMainChannel(); // This is the channel ID used by the 'main' instance of this track

			protected:
				void RegisterOutlets();
				void UpdateOutlets();

				strong<Track> _track;
				strong<PluginWrapper> _plugin;
				ref<LiveControl> _control;
				weak<Instance> _instance;
				weak<Network> _network;
				std::map< OutletHash, weak<Outlet> > _outlets;
				volatile unsigned int _lastMessageTime;
				volatile unsigned int _lastTickTime;
				
				std::wstring _instanceName;
				TrackID _id;
				GroupID _group;
				RunMode _runMode;
				bool _locked;
				ChannelAllocation _mainChannel;
		};

		class LiveControlEndpointCategory: public EndpointCategory {
			public:
				LiveControlEndpointCategory();
				virtual ~LiveControlEndpointCategory();
				virtual ref<Endpoint> GetEndpointById(const EndpointID& id);
				
				const static EndpointCategoryID KLiveControlEndpointCategoryID;
		};
	}
}

#endif