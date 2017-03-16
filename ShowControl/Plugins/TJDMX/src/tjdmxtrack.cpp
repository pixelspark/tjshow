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
#include "../include/tjdmx.h"
#include <TJShow/include/extra/tjextra.h>
#include <iomanip>
#include <algorithm>
using namespace tj::shared::graphics;
using namespace tj::script;

class DMXLiveControlEndpoint: public tj::shared::Endpoint {
	public:
		inline DMXLiveControlEndpoint(ref<DMXLiveControl> control) {
			assert(control);
			_control = control;
		}

		virtual ~DMXLiveControlEndpoint() {
		}

		virtual EndpointType GetType() const {
			return EndpointTypeMainThread;
		}

		virtual std::wstring GetName() const {
			return TL(dmx_endpoint);
		}

		virtual void Set(const Any& f);

	protected:
		weak<DMXLiveControl> _control;
};

class DMXLiveWnd: public ChildWnd, public Listener<SliderWnd::NotificationChanged>, public Listener<SliderWnd::NotificationUpdate> {
	friend class DMXLiveControl;

	public:
		DMXLiveWnd(ref<DMXTrack> track): ChildWnd(L""), _track(track) {
			assert(track);

			_slider = GC::Hold(new SliderWnd(L""));
			_slider->SetShowValue(false);
			_cm = -1.0f;
			_cd = -1.0f;
			Add(_slider);
		}

		virtual ~DMXLiveWnd() {
		}

		virtual void OnCreated() {
			_slider->EventChanged.AddListener(ref< Listener<SliderWnd::NotificationChanged> >(this));
			_slider->EventUpdate.AddListener(ref< Listener<SliderWnd::NotificationUpdate> >(this));
			ChildWnd::OnCreated();
		}

		virtual void Layout() {
			Area rc = GetClientArea();
			rc.Narrow(0,0,0,KValueHeight);
			if(_slider) _slider->Fill(LayoutFill, rc);
		}

		virtual void OnSize(const Area& ns) {
			Layout();
		}

		virtual void OnMouse(MouseEvent ev, Pixels x, Pixels y) {
			if(ev==MouseEventRDown) {
				ContextMenu cm;
				ref<DMXTrack> track = _track;
				if(track) {
					cm.AddItem(TL(dmx_display_float), DMXTrack::KDisplayFloat+1, false, track->_displayType==DMXTrack::KDisplayFloat);
					cm.AddItem(TL(dmx_display_byte), DMXTrack::KDisplayByte+1, false, track->_displayType==DMXTrack::KDisplayByte);
					cm.AddItem(TL(dmx_display_word), DMXTrack::KDisplayWord+1, false, track->_displayType==DMXTrack::KDisplayWord);
					cm.AddItem(TL(dmx_display_hex), DMXTrack::KDisplayHex+1, false, track->_displayType==DMXTrack::KDisplayHex);
					
					int r = cm.DoContextMenu(this,x,y)-1;
					if(r>=0) {
						ref<DMXTrack> track = _track;
						if(track) {
							track->_displayType = (DMXTrack::DisplayType)r;
						}
						Repaint();
					}
				}
			}
		}

		virtual void Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme) {
			SolidBrush back(theme->GetColor(Theme::ColorBackground));
			Area value = GetClientArea();
			value.Narrow(0,value.GetHeight()-KValueHeight,0,0);
			g.FillRectangle(&back, value);

			StringFormat sf;
			sf.SetAlignment(StringAlignmentCenter);
			SolidBrush tbr(theme->GetColor(Theme::ColorText));

			// Format display value
			std::wostringstream wos;
			ref<DMXTrack> track = _track;

			if(track) {
				switch(track->_displayType) {
					case DMXTrack::KDisplayByte:
						wos << std::setprecision(0) << int(_cm*255.0f);
						break;
					
					case DMXTrack::KDisplayWord:
						wos << std::setprecision(0) << int(_cm*65535.0f);
						break;

					case DMXTrack::KDisplayHex:
						wos.fill(L'0');
						wos << std::hex << std::setw(2) << std::uppercase << int(_cm*255.0f);
						break;

					default:
					case DMXTrack::KDisplayFloat:
						wos << std::setw(0) << int(_cm*100.0f);
						break;
				}
			}

			std::wstring display = wos.str();
			g.DrawString(display.c_str(), (int)display.length(), theme->GetGUIFontSmall(), value, &sf, &tbr);
		}

		virtual void Notify(ref<Object> source, const SliderWnd::NotificationChanged& evt) {
			_cm = _slider->GetValue();
			ref<DMXTrack> track = _track;
			if(track) {
				track->GetManualMacro()->Set(_cm);
			}
			Repaint();
		}

		virtual void Notify(ref<Object> source, const SliderWnd::NotificationUpdate& evt) {
			ref<DMXTrack> track = _track;
			if(track) {
				_slider->SetColor(track->GetManualMacro()->GetType());
				_slider->SetDisplayValue(track->GetManualMacro()->GetResult(),false);
				_slider->SetValue(track->GetManualMacro()->Get(), false);
			}
			Repaint();
		}

		virtual void Update() {
			ref<DMXTrack> track = _track;
			if(track) {
				float mvalue = float(track->GetManualMacro()->Get());
				float displayValue = float(track->GetManualMacro()->GetResult());

				if(mvalue!=_cm || displayValue!=_cd) {
					_cm = mvalue;
					_cd = displayValue;
					_slider->Update();
					Repaint();
				}
			}
		}

	protected:
		const static Pixels KValueHeight = 14;

		weak<DMXTrack> _track;
		ref<SliderWnd> _slider;
		float _cm, _cd;

};

class DMXLiveControl: public LiveControl {
	friend class DMXLiveControlEndpoint;

	public:
		DMXLiveControl(ref<DMXTrack> track) {
			assert(track);
			_track = track;
		}

		virtual ref<Wnd> GetWindow() {
			if(!_wnd) {
				_wnd = GC::Hold(new DMXLiveWnd(_track));
			}
			return _wnd;
		}

		virtual int GetWidth() {
			return 21;
		}

		virtual ref<tj::shared::Endpoint> GetEndpoint(const std::wstring& name) {
			return GC::Hold(new DMXLiveControlEndpoint(this));
		}

		virtual std::wstring GetGroupName() {
			return L"DMX";
		}

		virtual float GetValue() {
			return _wnd->_slider->GetValue();
		}

		virtual void Update() {
			if(_wnd) _wnd->Update();
		}

	protected:
		weak<DMXTrack> _track;
		ref<DMXLiveWnd> _wnd;
};

void DMXLiveControlEndpoint::Set(const Any& f) {
	ref<DMXLiveControl> control = _control;
	if(control) {
		ref<DMXTrack> track = control->_track;
		if(track) {
			float val = Clamp((float)f, 0.0f, 1.0f);
			track->GetManualMacro()->Set(val);
			control->Update();
		}
	}
}

class DMXTrackRange: public TrackRange {
	public:
		DMXTrackRange(Time a, Time b, ref<DMXTrack> track) {
			_track = track;
			_start = a;
			_end = b;
		}

		virtual ~DMXTrackRange() {
		}

		virtual void RemoveItems() {
			_track->RemoveItemsBetween(_start, _end);
		}

		virtual void Paste(ref<Track>(other), Time start) {
			if(other && other.IsCastableTo<DMXTrack>()) {
				// pasting allowed
				ref< Fader<float> > data = ref<DMXTrack>(other)->_data;
				ref< Fader<float> > my = _track->_data;
				if(data && my) {
					Log::Write(L"TJDMX/TrackRange", L"  all looks valid...");
					my->CopyTo(data, _start, _end, start);
				}
			}
		}

		virtual void InterpolateLeft(Time left, float ratio, Time right) {
			_track->_data->InterpolateLeft(left,ratio,right);
		}

		virtual void InterpolateRight(Time left, float ratio, Time right) {
			_track->_data->InterpolateRight(left,ratio,right);
		}

	protected:
		ref<DMXTrack> _track;
		Time _start, _end;
};

class DMXTrackScriptable: public tj::script::ScriptObject<DMXTrackScriptable> {
	public:
		DMXTrackScriptable(ref<DMXTrack> dt) {
			_track = dt;
		}

		static void Initialize() {
			Bind(L"dmxChannel", &SDMXChannel);
			Bind(L"fader", &SFader);
		}

		virtual bool Set(Field field, ref<Scriptable> value) {
			if(field==L"dmxChannel") {
				int channel = ScriptContext::GetValue<int>(value,0);
				_track->_channel = channel;
				return true;
			}
			return false;
		}

		virtual ref<Scriptable> SDMXChannel(ref<ParameterList> p) {
			return GC::Hold(new ScriptString(_track->_channel));
		}

		virtual ref<Scriptable> SFader(ref<ParameterList> p) {
			return GC::Hold(new FaderScriptable<float>(_track->_data));
		}

		virtual ~DMXTrackScriptable() {
		}

	protected:
		ref<DMXTrack> _track;
};

DMXTrack::DMXTrack(ref<DMXPlugin> plug) {
	_plugin = plug;
	_selectedFadePoint = Time(0);
	_height = 0;
	_channel = L"";
	_data = GC::Hold(new Fader<float>(0.0f,0.0f,1.0f));
	_resetOnStop = true;
	_expanded = KExpandNone;
	_displayType = KDisplayFloat;
}

DMXTrack::~DMXTrack() {
}

bool DMXTrack::IsExpandable() {
	return true;
}

bool DMXTrack::IsExpanded() {
	return _expanded==KExpandFull;
}

int DMXTrack::GetSliderType() {
	return GetManualMacro()->GetType();
}

void DMXTrack::OnCreated() {
	_plugin->AddPatchable(this);
}

void DMXTrack::SetExpanded(bool e) {
	switch(_expanded) {
		case KExpandFull:
			_expanded = KExpandNone;
			break;

		case KExpandHundred:
			_expanded = KExpandFull;
			break;

		case KExpandNone:
		default:
			_expanded = KExpandHundred;
	}
}

Flags<RunMode> DMXTrack::GetSupportedRunModes() {
	return Flags<RunMode>(RunModeMaster);
}

ref<Scriptable> DMXTrack::GetScriptable() {
	return GC::Hold(new DMXTrackScriptable(this));
}

void DMXTrack::OnSetID(const TrackID& tid) {
	_tid = tid;
}

void DMXTrack::OnSetInstanceName(const std::wstring& name) {
	_name = name;
}

std::wstring DMXTrack::GetInstanceName() const {
	return _name;
}

TrackID DMXTrack::GetID() const {
	return _tid;
}

void DMXTrack::SetResetOnStop(bool t) {
	_resetOnStop = t;
}

ref<DMXMacro> DMXTrack::GetManualMacro() {
	/* Notice that this doesn't parse variables on the _channel variable. We can't do this
	here, because we have no Playback-reference.*/
	if(!_manualMacro) {
		_manualMacro = _plugin->CreateMacro(_channel, DMXManual);
	}
	else {
		std::wstring lowerAddress = _channel;
		std::transform(lowerAddress.begin(), lowerAddress.end(), lowerAddress.begin(), tolower);

		if(_manualMacro->GetAddress()!=lowerAddress) {
			_manualMacro = _plugin->CreateMacro(lowerAddress, DMXManual);
		}
	}

	return _manualMacro;
}

ref<Property> DMXTrack::GetPropertyFor(DMXPatchable::PropertyID pid) {
	switch(pid) {
		case DMXPatchable::PropertyAddress:
			return GC::Hold(new GenericProperty<std::wstring>(TL(dmx_address), this, &_channel, _channel));

		case DMXPatchable::PropertyResetOnStop:
			return GC::Hold(new GenericProperty<bool>(TL(dmx_reset_on_stop), this, &_resetOnStop, _resetOnStop));
	}
	return 0;
}

ref<TrackRange> DMXTrack::GetRange(Time a, Time b) {
	return GC::Hold(new DMXTrackRange(a,b, this));
}

float DMXTrack::GetValueAt(Time t) {
	return _data->GetValueAt(t);
}

ref<DMXPlugin> DMXTrack::GetDMXPlugin() {
	return _plugin;
}

std::wstring DMXTrack::GetTypeName() const {
	return L"DMX";
}

ref<Player> DMXTrack::CreatePlayer(ref<Stream> str) {
	return GC::Hold(new DMXPlayer(this, str));
}

void DMXTrack::RemoveItemsBetween(Time a, Time b) {
	_data->RemoveItemsBetween(a,b);
}

Pixels DMXTrack::GetHeight() {
	switch(_expanded) {
		case KExpandNone:
			return _height;

		case KExpandHundred:
			return 100;

		case KExpandFull:
		default:
			return 255;
	}
}

Time DMXTrack::GetNextEvent(Time t) {
	return _data->GetNextEvent(t);
}

std::wstring DMXTrack::GetDMXAddress() const {
	return _channel;
}

ref<Item> DMXTrack::GetItemAt(Time position,unsigned int h, bool rightClick, int trackHeight, float pixelsPerMs) {
	if(rightClick) {
		float valueAtTime = _data->GetValueAt(position);
		_data->AddPoint(position, valueAtTime);
		return 0;
	}
	else {
		// If we are allowed to 'stick', create a new point if we're too far from the nearest point
		_selectedFadePoint = _data->GetNearest(position);
		if(MultifaderTrack::IsStickingAllowed() && ((abs(_selectedFadePoint.ToInt()-position.ToInt())*pixelsPerMs)>MultifaderTrack::KFaderStickLimit)) {
			return GC::Hold(new FaderItem<float>(_data, position,trackHeight ));
		}
		else {
			return GC::Hold(new FaderItem<float>(_data, _selectedFadePoint,trackHeight ));
		}
	}
}

void DMXTrack::OnDoubleClick(Time position, unsigned int h) {

}

void DMXTrack::Paint(tj::shared::graphics::Graphics* g, Time position, float pixelsPerMs, unsigned int x, unsigned int y, unsigned int h, Time start, Time end, ref<Item> focus) {
	strong<Theme> theme = ThemeManager::GetTheme();

	FaderPainter<float> painter(_data);
	Pen pn(theme->GetColor(Theme::ColorFader),1.5f);
	painter.Paint(g, start, end-start, h, int(float(end-start)*pixelsPerMs), x, y, &pn,_selectedFadePoint,true, focus);	
}

ref<PropertySet> DMXTrack::GetProperties() {
	ref<PropertySet> ps = GC::Hold(new PropertySet());

	ps->Add(DMXPlugin::CreateAddressProperty(TL(dmx_address), this, &_channel));
	ps->Add(GC::Hold(new GenericProperty<Pixels>(TL(dmx_track_height), this, &_height, _height)));
	ps->Add(GC::Hold(new GenericProperty<bool>(TL(dmx_reset_on_stop), this, &_resetOnStop, _resetOnStop)));
	ps->Add(GC::Hold(new FaderProperty<float>(_data, TL(fader_data), TL(fader_data_change), L"icons/fader.png")));

	return ps;
}

ref<LiveControl> DMXTrack::CreateControl(ref<Stream> str) {
	return GC::Hold(new DMXLiveControl(this));
}

bool DMXTrack::GetResetOnStop() const {
	return _resetOnStop;
}

void DMXTrack::SetDMXAddress(const std::wstring& a) {
	_channel = a;
}

void DMXTrack::InsertFromControl(Time t, ref<LiveControl> control, bool fade) {
	ref<DMXLiveControl> dmxc(control);
	float val = dmxc->GetValue();
	_data->AddPoint(t, val, fade);
}

void DMXTrack::Save(TiXmlElement* parent) {
	SaveAttributeSmall(parent, "height", _height);
	SaveAttribute(parent, "channel", _channel);
	SaveAttributeSmall(parent, "reset-on-stop", _resetOnStop?1:0);
	SaveAttributeSmall(parent, "display", (int)_displayType);
	
	TiXmlElement fader("value");
	_data->Save(&fader);
	parent->InsertEndChild(fader);
}

void DMXTrack::Load(TiXmlElement* you) {
	// Old faders use values between [0,255]. Convert!
	TiXmlElement* fadera = you->FirstChildElement("fader");
	if(fadera!=0) {
		TiXmlElement* fader = fadera->FirstChildElement("fader");
		if(fader!=0) {
			TiXmlElement* point = fader->FirstChildElement("point");
			while(point!=0) {
				Time time = StringTo<Time>(point->Attribute("time"),-1);
				int value = StringTo<int>(point->Attribute("value"), 0);
				if(time>=Time(0)) {
					_data->AddPoint(time,float(value)/255.0f);
				}
				point = point->NextSiblingElement("point");
			}
		}
	}
	else {
		fadera = you->FirstChildElement("value");
		if(fadera!=0) {
			TiXmlElement* fader = fadera->FirstChildElement("fader");
			if(fader!=0) {
				_data->Load(fader);
			}
		}
	}

	_height = LoadAttributeSmall(you, "height", _height);
	_channel = LoadAttribute(you, "channel",std::wstring(L""));
	_resetOnStop = LoadAttributeSmall<int>(you, "reset-on-stop", 1) == 1;
	_displayType = (DisplayType)LoadAttributeSmall<int>(you, "display", (int)KDisplayFloat);
}
	