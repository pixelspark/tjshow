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
using namespace tj::show;

Resources::Resources() {
}

Resources::~Resources() {
}

void Resources::Clear() {
	_resources.clear();
}

void Resources::Load(TiXmlElement* you) {
	TiXmlElement* res = you->FirstChildElement("resource");
	while(res!=0) {
		ResourceIdentifier rid = LoadAttributeSmall<ResourceIdentifier>(you, "rid", L"");
		Add(rid);
		res = res->NextSiblingElement("resource");
	}
}

void Resources::Remove(const ResourceIdentifier& rs) {
	std::set<ResourceIdentifier>::iterator it = _resources.begin();
	while(it!=_resources.end()) {
		if(*it==rs) {
			_resources.erase(it);
			return;
		}
		++it;
	}
}

void Resources::Save(TiXmlElement* you) {
	std::set<ResourceIdentifier>::iterator it = _resources.begin();
	while(it!=_resources.end()) {
		TiXmlElement resource("resource");
		SaveAttributeSmall(&resource, "rid", *it);
		you->InsertEndChild(resource);
		++it;
	}
}

int Resources::GetResourceCount() const {
	return (int)_resources.size();
}

std::set<ResourceIdentifier>* Resources::GetResourceList() {
	return &_resources;
}

void Resources::Add(const ResourceIdentifier& rs) {
	_resources.insert(rs);
	Sort();
}

void Resources::Sort() {
	// std::set is automagically sorted
}

ResourceIdentifier Resources::GetResourceAt(unsigned int i) const {
	std::set<ResourceIdentifier>::const_iterator it = _resources.begin();
	for(unsigned int a=0;a<i;a++) {
		++it;
		if(it==_resources.end()) {
			return L"";
		}
	}

	return *it;
}

bool Resources::ContainsResource(const ResourceIdentifier& rid) {
	return _resources.find(rid)!=_resources.end();
}