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
#ifndef _TJDMXTRACK_H
#define _TJDMXTRACK_H

class DMXTrack: public Track, public DMXPatchable {
	friend class DMXTrackRange; 
	friend class DMXTrackScriptable;
	friend class DMXLiveWnd;

	public:
		DMXTrack(ref<DMXPlugin> plug);
		virtual ~DMXTrack();
		virtual std::wstring GetTypeName() const;
		virtual ref<Player> CreatePlayer(ref<Stream> str);
		virtual Flags<RunMode> GetSupportedRunModes();

		virtual ref<Item> GetItemAt(Time position, unsigned int h, bool rightClick, int th, float pixelsPerMs);
		virtual void OnDoubleClick(Time position, unsigned int h);
		virtual void Paint(tj::shared::graphics::Graphics* g, Time position, float pixelsPerMs, unsigned int x, unsigned int y, unsigned int h, Time start, Time end, ref<Item> focus);
		virtual ref<PropertySet> GetProperties();
		virtual ref<LiveControl> CreateControl(ref<Stream> str);
		virtual ref<tj::script::Scriptable> GetScriptable();
		virtual void InsertFromControl(Time t, ref<LiveControl> control, bool fade);
		virtual Pixels GetHeight();

		// save 'n load
		virtual void Save(TiXmlElement* parent);
		virtual void Load(TiXmlElement* you);
		ref<DMXPlugin> GetDMXPlugin();
		virtual std::wstring GetDMXAddress() const;
		virtual void SetDMXAddress(const std::wstring& a);
		float GetValueAt(Time t);
		Time GetNextEvent(Time t);
		virtual int GetSliderType();
		ref<DMXMacro> GetManualMacro();
		ref<TrackRange> GetRange(Time a, Time b);
		
		virtual bool IsExpandable();
		virtual void SetExpanded(bool e);
		virtual bool IsExpanded();

		// DMXPatchable interface implementation
		virtual void OnSetID(const TrackID& tid);
		virtual void OnSetInstanceName(const std::wstring& name);
		virtual std::wstring GetInstanceName() const;
		virtual bool GetResetOnStop() const;
		virtual void SetResetOnStop(bool r);
		virtual TrackID GetID() const;
		virtual ref<Property> GetPropertyFor(DMXPatchable::PropertyID pid);
		
	protected:
		virtual void OnCreated();
		void RemoveItemsBetween(Time start, Time end);

		enum ExpandMode {
			KExpandNone=0,
			KExpandHundred,
			KExpandFull,
		};

		enum DisplayType {
			KDisplayFloat=0,
			KDisplayByte=1,
			KDisplayWord=2,
			KDisplayHex=3,
		};

		ref<DMXMacro> _manualMacro;
		ref< Fader<float> > _data;
		std::wstring _channel;
		ref<DMXPlugin> _plugin;
		Time _selectedFadePoint;
		Pixels _height; 
		bool _resetOnStop;
		ExpandMode _expanded;
		std::wstring _name;
		DisplayType _displayType;
		TrackID _tid;
};

#endif