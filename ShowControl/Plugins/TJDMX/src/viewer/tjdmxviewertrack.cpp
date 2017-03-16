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
#include "../../include/tjdmx.h"

DMXViewerTrack::DMXViewerTrack(ref<DMXViewerPlugin> plugin, ref<Playback> pb): _pb(pb), _backgroundColor(1.0, 1.0, 1.0) {
	_plugin = plugin;
	_defaultRotation = 0;
}

DMXViewerTrack::~DMXViewerTrack() {
}

std::wstring DMXViewerTrack::GetTypeName() const {
	return L"DMX Viewer";
}

Pixels DMXViewerTrack::GetHeight() { 
	return 19; 
}

Flags<RunMode> DMXViewerTrack::GetSupportedRunModes() {
	return Flags<RunMode>(RunModeDont);
}

ref<Player> DMXViewerTrack::CreatePlayer(ref<Stream> str) {
	return 0;
}

ref<Item> DMXViewerTrack::GetItemAt(Time position, unsigned int h, bool rightClick, int th, float pixelsPerMs) {
	return 0;
}

void DMXViewerTrack::OnDoubleClick(Time position, unsigned int h) {
}

void DMXViewerTrack::Paint(tj::shared::graphics::Graphics* g, Time position, float pixelsPerMs, unsigned int x, unsigned int y, unsigned int h, Time start, Time end) {
}

ref<PropertySet> DMXViewerTrack::GetProperties() {
	ref<PropertySet> prs = GC::Hold(new PropertySet());

	prs->Add(GC::Hold(new FileProperty(TL(background_image), this, &_background, _pb->GetResources())));
	prs->Add(GC::Hold(new GenericProperty<int>(TL(dmx_viewer_default_rotation), this, &_defaultRotation, _defaultRotation)));
	prs->Add(GC::Hold(new ColorProperty(TL(dmx_viewer_background_color), this, &_backgroundColor)));

	return prs;
};

void DMXViewerTrack::GetResources(std::vector<ResourceIdentifier>& rids) {
	rids.push_back(_background);
}

void DMXViewerTrack::InsertFromControl(Time t, ref<LiveControl> control) {
}

std::wstring DMXViewerTrack::GetBackgroundImagePath() {
	std::wstring path;
	_pb->GetResources()->GetPathToLocalResource(_background, path);
	return path;
}

void DMXViewerTrack::Save(TiXmlElement* el) {
	SaveAttribute(el, "background", _background);
	SaveAttribute(el, "default-rotation", _defaultRotation);

	TiXmlElement color("background-color");
	_backgroundColor.Save(&color);
	el->InsertEndChild(color);

	std::vector< ref<DMXViewerFixture>  >::iterator it = _fixtures.begin();
	while(it!=_fixtures.end()) {
		ref<DMXViewerFixture> fix = *it;
		TiXmlElement fe("fixture");
		fix->Save(&fe);
		el->InsertEndChild(fe);
		++it;
	}
}

void DMXViewerTrack::Load(TiXmlElement* el) {
	_background = LoadAttribute(el, "background", _background);
	
	TiXmlElement* fixture = el->FirstChildElement("fixture");
	while(fixture!=0) {
		ref<DMXViewerFixture> fix = CreateFixture();
		fix->Load(fixture);
		fixture = fixture->NextSiblingElement("fixture");
	}

	TiXmlElement* backColor = el->FirstChildElement("background-color");
	if(backColor) {
		TiXmlElement* color = backColor->FirstChildElement("color");
		if(color) {
			_backgroundColor.Load(color);
		}
	}
}

ref<DMXViewerFixture> DMXViewerTrack::GetFixtureAt(Pixels x, Pixels y,float w, float h) {
	tj::shared::graphics::Graphics g((HWND)0); // for text measurements
	std::vector< ref<DMXViewerFixture>  >::iterator it = _fixtures.begin();
	while(it!=_fixtures.end()) {
		ref<DMXViewerFixture> fix = *it;
		AreaF bounds = fix->GetArea(g);
		bounds.SetX(bounds.GetX()*w);
		bounds.SetY(bounds.GetY()*h);
		bounds.Widen(5,5,5,5);
		if(bounds.IsInside(float(x),float(y))) {
			return fix;
		}
		++it;
	}

	return 0;
}

ref<DMXViewerFixture> DMXViewerTrack::CreateFixture() {
	ref<DMXViewerFixture> fix = GC::Hold(new DMXViewerFixture());
	fix->_rotate = float(_defaultRotation);
	_fixtures.push_back(fix);
	return fix;
}

ref<DMXViewerFixture> DMXViewerTrack::CreateFixture(ref<DMXViewerFixture> other) {
	ref<DMXViewerFixture> fix = GC::Hold(new DMXViewerFixture(other));
	_fixtures.push_back(fix);
	return fix;
}

void DMXViewerTrack::RemoveFixture(ref<DMXViewerFixture> fix) {
	std::vector< ref<DMXViewerFixture>  >::iterator it = _fixtures.begin();
	while(it!=_fixtures.end()) {
		if(*it == fix) {
			_fixtures.erase(it);
			return;
		}
		++it;
	}
}

std::set< ref<DMXViewerFixture> > DMXViewerTrack::GetFixtures() {
	std::set< ref<DMXViewerFixture> > ns;
	std::vector< ref<DMXViewerFixture> >::iterator it = _fixtures.begin();
	while(it!=_fixtures.end()) {
		ns.insert(*it);
		++it;
	}
	return ns;
}