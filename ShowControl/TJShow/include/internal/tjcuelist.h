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
#ifndef _TJCUELIST_H
#define _TJCUELIST_H

namespace tj {
	namespace show {
		class Controller; 

		class CueList: public virtual Object, public Serializable {
			friend class Controller;
			friend class Cue;

			public:
				CueList(ref<Instance> controller = null);
				virtual ~CueList();
				virtual void Clear();

				// Serializable
				virtual void Save(TiXmlElement* parent);
				virtual void Load(TiXmlElement* you);

				virtual void RemoveCuesBetween(Time a, Time b);
				virtual std::vector< ref<Cue> >::iterator GetCuesBegin();
				virtual std::vector< ref<Cue> >::iterator GetCuesEnd();
				virtual void AddCue(ref<Cue> c);
				virtual void RemoveCue(ref<Cue> c);
				virtual void RemoveAllCues();
				virtual ref<Cue> GetCueAt(Time t); 
				virtual unsigned int GetCueCount() const;
				virtual ref<Cue> GetNextCue(const Time& t, Cue::Action filterAction = Cue::ActionNone);
				virtual ref<Cue> GetPreviousCue(Time t);
				virtual ref<Cue> GetCueByName(const std::wstring& cs);
				virtual ref<Cue> GetCueByID(const CueIdentifier& id);
				virtual void GetCuesBetween(Time start, Time end, std::list< ref<Cue> >& cues);
				virtual void Clone();

				// TODO: can we remove this? As far as I know, only Cue::DoAction is using this, and can do it in another way
				virtual void SetStaticInstance(ref<Instance> ctrl);

			protected:
				std::vector< ref<Cue> > _cues;
				CriticalSection _lock;
				weak<Instance> _controller;
		};
	}
}

#endif