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
#ifndef _TJDMXVIEWERTRACK_H
#define _TJDMXVIEWERTRACK_H

class DMXViewerTrack: public Track {
	friend class DMXViewerWnd; 

	public:
		DMXViewerTrack(ref<DMXViewerPlugin> plugin, ref<Playback> pb);
		virtual ~DMXViewerTrack();
		virtual std::wstring GetTypeName() const;
		virtual Pixels GetHeight();
		virtual Flags<RunMode> GetSupportedRunModes();
		virtual ref<Player> CreatePlayer(ref<Stream> str);
		virtual ref<Item> GetItemAt(Time position, unsigned int h, bool rightClick, int th, float pixelsPerMs);
		virtual void OnDoubleClick(Time position, unsigned int h);
		virtual void Paint(tj::shared::graphics::Graphics* g, Time position, float pixelsPerMs, unsigned int x, unsigned int y, unsigned int h, Time start, Time end);
		virtual ref<PropertySet> GetProperties();
		virtual ref<LiveControl> CreateControl(ref<Stream> str);
		virtual void InsertFromControl(Time t, ref<LiveControl> control);
		virtual void Save(TiXmlElement* el);
		virtual void Load(TiXmlElement* el);

		ref<DMXViewerFixture> GetFixtureAt(Pixels x, Pixels y, float w, float h);
		ref<DMXViewerFixture> CreateFixture();
		ref<DMXViewerFixture> CreateFixture(ref<DMXViewerFixture> other);
		void RemoveFixture(ref<DMXViewerFixture> fix);
		std::set< ref<DMXViewerFixture> > GetFixtures();
		virtual void GetResources(std::vector<ResourceIdentifier>& rids);

	protected:
		std::wstring GetBackgroundImagePath();
		std::vector< ref<DMXViewerFixture> > _fixtures;
		ref<DMXViewerPlugin> _plugin;
		ref<Playback> _pb;
		ResourceIdentifier _background;
		RGBColor _backgroundColor;
		int _defaultRotation;
};


#endif