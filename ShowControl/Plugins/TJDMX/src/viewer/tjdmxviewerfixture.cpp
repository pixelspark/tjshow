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
using namespace tj::shared::graphics;

DMXViewerFixture::DMXViewerFixture(): _color(1.0, 1.0, 1.0) {
	_x = _y = 0.0f;
	_size = 16.0f;
	_channel = -1;
	_type = FixtureTypeDefault;
	_rotate = 0.0f;
}

DMXViewerFixture::DMXViewerFixture(ref<DMXViewerFixture> other) {
	assert(other);
	_x = other->_x;
	_y = other->_y;
	_size = other->_size;
	_channel = other->_channel;
	_type = other->_type;
	_rotate = other->_rotate;
	_color = other->_color;
}

DMXViewerFixture::~DMXViewerFixture() {
}

void DMXViewerFixture::Save(TiXmlElement* you) {
	SaveAttributeSmall(you, "x", _x);
	SaveAttributeSmall(you, "y", _y);
	SaveAttributeSmall(you, "r", (int)(_color._r*255.0));
	SaveAttributeSmall(you, "g", (int)(_color._g*255.0));
	SaveAttributeSmall(you, "b", (int)(_color._b*255.0));
	SaveAttributeSmall(you, "channel", _channel);
	SaveAttributeSmall(you, "size", _size);
	SaveAttributeSmall(you, "type", (int)_type);
	SaveAttributeSmall(you, "rotate", _rotate);
	SaveAttributeSmall(you, "text", _text);
}

void DMXViewerFixture::Load(TiXmlElement* you) {
	_x = LoadAttributeSmall(you, "x", _x);
	_y = LoadAttributeSmall(you, "y", _y);

	_channel = LoadAttributeSmall(you, "channel", _channel);
	int r = LoadAttributeSmall(you, "r", (int)(_color._r*255.0));
	int g = LoadAttributeSmall(you, "g", (int)(_color._g*255.0));
	int b = LoadAttributeSmall(you, "b", (int)(_color._b*255.0));
	_color = RGBColor(double(r)/255.0,double(g)/255.0,double(b)/255.0);
	_size = LoadAttributeSmall(you, "size", _size);
	_type = (FixtureType) LoadAttributeSmall(you, "type", (int)FixtureTypeDefault);
	_rotate = LoadAttributeSmall(you, "rotate", _rotate);
	_text = LoadAttributeSmall(you, "text", std::wstring(L""));
}

ref<PropertySet> DMXViewerFixture::GetProperties() {
	ref<PropertySet> prs = GC::Hold(new PropertySet());
	
	ref< GenericListProperty<FixtureType> > type = GC::Hold(new GenericListProperty<FixtureType>(TL(fixture_type), this, &_type, _type));
	type->AddOption(TL(fixture_type_default),FixtureTypeDefault);
	type->AddOption(TL(fixture_type_square),FixtureTypeSquare);
	type->AddOption(TL(fixture_type_pc),FixtureTypePC);
	type->AddOption(TL(fixture_type_pc_top),FixtureTypePCTop);
	type->AddOption(TL(fixture_type_par),FixtureTypePar);
	type->AddOption(TL(fixture_type_par_64),FixtureTypePar64);
	type->AddOption(TL(fixture_type_text),FixtureTypeText);
	type->AddOption(TL(fixture_type_fresnel), FixtureTypeCIEFresnel);
	type->AddOption(TL(fixture_type_horizon), FixtureTypeCIEHorizon);
	type->AddOption(TL(fixture_type_cie_par_short), FixtureTypeCIEParShort);
	type->AddOption(TL(fixture_type_cie_par_64), FixtureTypeCIEPar64);
	type->AddOption(TL(fixture_type_cie_pc), FixtureTypeCIEPC);
	type->AddOption(TL(fixture_type_cie_profile), FixtureTypeCIEProfile);
	prs->Add(type);

	prs->Add(GC::Hold(new GenericProperty<int>(TL(dmx_address), this, &_channel, _channel)));
	prs->Add(GC::Hold(new GenericProperty<float>(TL(fixture_x), this, &_x, _x)));
	prs->Add(GC::Hold(new GenericProperty<float>(TL(fixture_y), this, &_y, _y)));
	prs->Add(GC::Hold(new GenericProperty<float>(TL(fixture_size), this, &_size, _size)));
	prs->Add(GC::Hold(new GenericProperty<float>(TL(fixture_rotation), this, &_rotate, _rotate)));
	prs->Add(GC::Hold(new GenericProperty<std::wstring>(TL(fixture_text), this, &_text, _text)));
	prs->Add(GC::Hold(new ColorProperty(TL(fixture_color), this, &_color)));

	return prs;
}

AreaF DMXViewerFixture::GetArea(tj::shared::graphics::Graphics& g) const {
	switch(_type) {
		case FixtureTypeText: {
			strong<Theme> theme = ThemeManager::GetTheme();
			std::wstring uiFontName = theme->GetGUIFontName();
			Font myfont(uiFontName.c_str(), _size, FontStyleRegular);
			RectF r;
			g.MeasureString(_text.c_str(), (int)_text.length(), &myfont, PointF(_x, _y), &r);
			return AreaF(_x, _y, r.GetWidth(), r.GetHeight());
		}

		case FixtureTypeCIEHorizon:
			return AreaF(_x,_y, _size*1.0f, _size*2.0f);

		case FixtureTypeCIEFresnel:
			return AreaF(_x, _y, _size*1.5f, _size);

		case FixtureTypePar:
		case FixtureTypeCIEPC:
			return AreaF(_x, _y, _size*2.0f, _size);

		case FixtureTypeCIEPar64:
		case FixtureTypePar64:
			return AreaF(_x, _y, _size*2.0f, _size);

		case FixtureTypeDefault:
		case FixtureTypeSquare:
		case FixtureTypePC:
		case FixtureTypePCTop:
		case FixtureTypeCIEProfile:
		case FixtureTypeCIEParShort:
		default:
			return AreaF(_x, _y, _size, _size);
	}
}