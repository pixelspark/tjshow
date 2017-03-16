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
#ifndef _TJINGLECOLLECTION_H
#define _TJINGLECOLLECTION_H

namespace tj {
	namespace jingle {
		class Jingle;

		class JingleCollection: public tj::shared::Inspectable, public tj::shared::Serializable {
			friend class JinglePane;
			friend class JingleApplication;
			friend class JingleView;

			public:
				JingleCollection();
				virtual ~JingleCollection();
				virtual std::wstring GetName() const;
				virtual tj::shared::ref<tj::shared::PropertySet> GetProperties();
				virtual tj::shared::ref<Jingle> GetJingle(wchar_t letter);
				virtual void Cache();
				virtual void Clear();
				virtual void Save(TiXmlElement* parent);
				virtual void Load(TiXmlElement* you);
				virtual void StopAll();
				virtual void FadeOutAll();
				virtual void PlayRandom();
				virtual void FindJinglesByName(const std::wstring& q, std::deque< tj::shared::ref<Jingle> >& results);

			protected:
				std::wstring _name;
				std::map<wchar_t, tj::shared::ref<Jingle> > _jingles;
		};
	}
}

#endif