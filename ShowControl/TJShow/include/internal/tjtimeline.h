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
#ifndef _TJTIMELINE_H
#define _TJTIMELINE_H

namespace tj {
	namespace show {
		class CueList;
		class VariableList;
		typedef std::wstring TimelineIdentifier;

		class Timeline: public virtual Object, private Serializable, public Inspectable {
			public:
				friend class SubTimelineTrack; // for _timeLength as property
				enum {DefaultTimelineLength = 60000 };
				
				Timeline(Time length = DefaultTimelineLength);
				virtual ~Timeline();
				virtual void Clear(); // empties the whole model
				virtual Time GetTimeLengthMS() const;
				virtual void SetTimeLengthMS(Time t);
				virtual unsigned int GetTrackCount();
				virtual std::wstring GetName() const;
				virtual void SetName(const std::wstring& x);
				virtual bool IsSingleton() const;
				virtual void SetSingleton(bool t);
				virtual bool IsRemoteControlAllowed() const;
				virtual void SetRemoteControlAllowed(bool r);
				virtual void Interpolate(Time oldTime, Time newTime, Time left, Time right);

				// cues
				virtual void SetCueList(strong<CueList> cl);
				virtual strong<CueList> GetCueList();

				// local variables
				virtual void SetLocalVariables(strong<VariableList> vars);
				virtual strong<VariableList> GetLocalVariables();

				// tracks
				virtual ref< Iterator< ref<TrackWrapper> > > GetTracks();
				virtual ref<TrackWrapper> GetTrackByIndex(int idx);

				virtual void AddTrack(ref<TrackWrapper> t);
				virtual void RemoveTrack(ref<TrackWrapper> t);
				virtual void RemoveItemsBetween(Time a, Time b);
				virtual ref<TrackWrapper> GetTrackByName(const std::wstring& name); 
				virtual ref<TrackWrapper> GetTrackByID(const TrackID& tid, bool deep=false);
				virtual void MoveUp(ref<TrackWrapper> tr);
				virtual void MoveDown(ref<TrackWrapper> tr);
				virtual std::vector< ref<TrackRange> > CopyItemsBetween(Time start, Time end);
				virtual void PasteItems(std::vector< ref<TrackRange> >& selection, Time start);
				virtual ref<PropertySet> GetProperties();
				virtual bool IsParentOf(ref<TrackWrapper> tw, bool recursive = false) const;
				virtual bool IsResourceRequired(const std::wstring& rid);
				virtual ref<Path> CreatePath();
				virtual ref<Crumb> CreateCrumb();
				virtual void Clone();
				const TimelineIdentifier& GetID() const;

				// Speed settings
				void SetDefaultSpeed(float s);
				float GetDefaultSpeed() const;
				
				// Serializable
				virtual void Save(TiXmlElement* parent);
				virtual void Load(TiXmlElement* you, strong<Instance> instance);

				// Resources stuff
				virtual void GetResources(std::vector< ResourceIdentifier>& res);

			protected:
				virtual void Load(TiXmlElement* you);
				void NewID();

				std::vector< ref<TrackWrapper> > _tracks;
				strong<CueList> _cues;
				strong<VariableList> _locals;
				Time _timeLength;
				float _defaultSpeed;
				std::wstring _name;
				std::wstring _id;
				std::wstring _description;
				bool _singleton;
				bool _remoteControlAllowed;
		};
	}
}

#endif