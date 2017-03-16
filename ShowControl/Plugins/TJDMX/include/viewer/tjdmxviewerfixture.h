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
#ifndef _TJDMXFIXTURE_H
#define _TJDMXFIXTURE_H

class DMXViewerFixture: public Inspectable {
	friend class DMXViewerTrack;
	friend class DMXViewerWnd;

	public:
		enum FixtureType {
			FixtureTypeDefault = 0,
			FixtureTypeSquare,
			FixtureTypePC,
			FixtureTypePar,
			FixtureTypePar64,
			FixtureTypePCTop,
			FixtureTypeText,
			FixtureTypeCIEFresnel,
			FixtureTypeCIEHorizon,
			FixtureTypeCIEParShort,
			FixtureTypeCIEPar64,
			FixtureTypeCIEPC,
			FixtureTypeCIEProfile,
		};

		DMXViewerFixture();
		DMXViewerFixture(ref<DMXViewerFixture> other);
		virtual ~DMXViewerFixture();
		virtual void Save(TiXmlElement* you);
		virtual void Load(TiXmlElement* you);
		virtual ref<PropertySet> GetProperties();

		AreaF GetArea(tj::shared::graphics::Graphics& g) const;

	protected:
		float _x, _y;
		int _channel;
		float _size;
		float _rotate;
		RGBColor _color;
		FixtureType _type;
		std::wstring _text;
};


#endif