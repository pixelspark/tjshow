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
#ifndef _TJMEDIAMASTERS_H
#define _TJMEDIAMASTERS_H

namespace tj {
	namespace media {
		namespace master {
			typedef std::wstring MediaMasterID;
			typedef MediaMasterID MediaMasterList;

			class MediaMaster {
				public:
					enum Source {
						MasterSourceManual = 1,
						MasterSourceSequence = 2,
					};

					enum Flavour {
						_FlavourFirst = 1,
						FlavourVolume = 1,
						FlavourAlpha,
						FlavourEffect, // The actual flavor is FlavourEffect + value of Mesh::EffectParameter enumeration
						_FlavourLast,
					};

					virtual ~MediaMaster();
					virtual MediaMasterID GetMasterID() const = 0;
					virtual Flavour GetFlavour() const = 0;					
			};

			class MediaMasters: public virtual Object {
				public:
					MediaMasters();
					virtual ~MediaMasters();
					virtual void AddMaster(ref<master::MediaMaster> mt);
					virtual float GetMasterValue(const MediaMasterList& masterList);
					virtual void GetMasterValues(const MediaMasterList& masterList, std::map<MediaMaster::Flavour, float>& values);
					virtual float GetMasterValue(const MediaMasterID& mid, MediaMaster::Source src);
					virtual void SetMasterValue(const MediaMasterID& mid, float v, MediaMaster::Source m);
					virtual ref<Property> CreateMasterListProperty(const std::wstring& name, ref<Inspectable> holder, MediaMasterList* masterList);
					virtual void Reset();

					struct MasterChangedNotification {
						MasterChangedNotification(const MediaMasterID& mid);
						const MediaMasterID& _mid;
					};

					Listenable<MasterChangedNotification> EventMasterChanged;

				protected:
					struct MasterInfo {
						MasterInfo();
						float _sequence;
						float _manual;
					};

					CriticalSection _mastersLock;
					std::map< std::wstring, MasterInfo > _masterInfo;
					std::vector< weak<master::MediaMaster> > _masters;
			};
		}
	}
}

#endif