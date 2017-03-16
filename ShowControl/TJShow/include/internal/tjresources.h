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
#ifndef _TJRESOURCES_H
#define _TJRESOURCES_H

namespace tj {
	namespace show {
		class Resources: public virtual Object, public Serializable {
			public:
				Resources();
				virtual ~Resources();
				virtual void Load(TiXmlElement* you);
				virtual void Save(TiXmlElement* you);
				void Clear();
				int GetResourceCount() const;
				std::set<ResourceIdentifier>* GetResourceList();
				void Remove(const ResourceIdentifier& rs);
				void Add(const ResourceIdentifier& rs);
				void Sort();
				bool ContainsResource(const ResourceIdentifier& rid);
				ResourceIdentifier GetResourceAt(unsigned int i) const;

			protected:
				std::set<ResourceIdentifier> _resources;
		};
	}
}

#endif