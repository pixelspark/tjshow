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
#include "../../../TJShow/include/extra/tjextra.h"
#include <windowsx.h>
using namespace tj::shared::graphics;

const Time DMXPositionTrack::DefaultPathRange(10000);

class DMXPositionWnd: public ChildWnd, public Listener<ButtonWnd::NotificationClicked> {
	public:
		DMXPositionWnd(ref<DMXPositionTrack> tr): ChildWnd(L"") {
			assert(tr);
			_track = tr;
			_base = -100;
			_startX = 0;
			_startV = 0;

			_path = GC::Hold(new ButtonWnd(L"icons/pen.png", TL(dmx_trackspot_edit_path)));
			Add(_path);

			Layout();
		}

		virtual ~DMXPositionWnd() {
		}

		virtual void OnCreated() {
			_path->EventClicked.AddListener(ref< Listener<ButtonWnd::NotificationClicked> >(this));
		}

		virtual void Notify(ref<Object> source, const ButtonWnd::NotificationClicked& evt) {
			if(source==ref<Object>(_path)) {
				ReplyMessage(0);
				if(!_editor) {
					_editor = GC::Hold(new DMXPositionEditorWnd(0L, _track));
				}
				
				Range<Time> rt = Range<Time>::Widest(_track->GetFaderById(DMXPositionTrack::KFaderPan)->GetRange(), _track->GetFaderById(DMXPositionTrack::KFaderTilt)->GetRange());
				_editor->SetRange(Range<Time>(min(Time(0),rt.Start()-Time(1)), rt.End()+Time(1)));
				_editor->Show(true);
				SetForegroundWindow(_editor->GetWindow());	
			}
		}

		virtual void Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme) {
			Area rc = GetClientArea();
			SolidBrush back(theme->GetColor(Theme::ColorBackground));
			g.FillRectangle(&back, rc);

			// pan/tilt rectangle
			DMXPositionMacro dm = _track->GetMacro(DMXManual,null);

			int w = rc.GetWidth()-4;
			LinearGradientBrush border(PointF(0.0f, float(rc.GetTop()+2)), PointF(0.0f, float(w+rc.GetTop()+3)), theme->GetColor(Theme::ColorActiveStart), theme->GetColor(Theme::ColorActiveEnd));
			Pen pn(&border, 1.0f);
			g.DrawRectangle(&pn,RectF(float(rc.GetLeft()+2), float(rc.GetTop()+2), float(w), float(w)));
			g.DrawLine(&pn, rc.GetLeft()+2+(w/2), rc.GetTop()+2, rc.GetLeft()+2+(w/2), rc.GetTop()+w+2);
			g.DrawLine(&pn, rc.GetLeft()+2, rc.GetTop()+2+(w/2), rc.GetLeft()+2+w, rc.GetTop()+2+(w/2));

			Color backColor = theme->GetColor(Theme::ColorBackground);
			SolidBrush inactive(Color((unsigned char)150, backColor.GetR(), backColor.GetG(), backColor.GetB()));
			g.FillRectangle(&inactive, RectF(float(rc.GetLeft()+3), float(rc.GetTop()+3), float(w-2), float(w-2)));

			SolidBrush pointer(theme->GetColor(Theme::ColorText));

			if(dm._pan && dm._tilt) {
				float panValue = dm._pan->GetResult();
				float tiltValue = dm._tilt->GetResult();
				g.FillRectangle(&pointer, RectF(panValue*float(w), tiltValue*float(w), 4.0f,4.0f));
			}
		}

		virtual void Update() {
			Repaint();
		}

		virtual void Layout() {
			Area rc = GetClientArea();
			Pixels h = rc.GetWidth() + 20 + 22 + 12;
			Pixels sh = rc.GetBottom()-h-20;
			_path->Move(2, h+sh, rc.GetWidth()-4, 20);
		}

		virtual LRESULT Message(UINT msg, WPARAM wp, LPARAM lp) {
			if(msg==WM_MOUSEMOVE) {
				if(IsKeyDown(KeyMouseLeft)) {
					int x = GET_X_LPARAM(lp);
					int y = GET_Y_LPARAM(lp);
					Area rc = GetClientArea();
					if(y<(rc.GetWidth()+2)) {
						y -= 2;
						x -= 2;
						DMXPositionMacro dm = _track->GetMacro(DMXManual,null);
						if(dm._pan) {
							dm._pan->Set(float(x)/float(rc.GetWidth()-4));
						}
						if(dm._tilt) {
							dm._tilt->Set(float(y)/float(rc.GetWidth()-4));
						}		
					}
					Repaint();
				}
			}
			else if(msg==WM_SIZE) {
				Layout();
			}
			return ChildWnd::Message(msg, wp, lp);
		}

	protected:
		ref<DMXPositionTrack> _track;
		ref<DMXPositionEditorWnd> _editor;
		ref<ButtonWnd> _path;
		int _base;
		int _startX, _startV;
};

class DMXPositionControl: public LiveControl {
	public:
		DMXPositionControl(ref<DMXPositionTrack> track) {
			assert(track);
			_track = track;
			_wnd = GC::Hold(new DMXPositionWnd(track));
		}

		virtual ref<Wnd> GetWindow() {
			return _wnd;
		}

		virtual std::wstring GetGroupName() {
			return L"DMX";
		}

		virtual void Tick() {
			_wnd->Update();
		}

		virtual int GetWidth() {
			return 90;
		}

		virtual void Update() {
			_wnd->Update();
		}

	protected:
		ref<DMXPositionTrack> _track;
		ref<DMXPositionWnd> _wnd;
};

DMXPositionTrack::DMXPositionTrack(ref<DMXPlugin> dp) {
	_plugin = dp;
	_panAddress = L"";
	_tiltAddress = L"";

	MultifaderTrack::AddFader(KFaderPan, TL(pan), 0.0f);
	MultifaderTrack::AddFader(KFaderTilt, TL(tilt), 0.0f);
}

DMXPositionTrack::~DMXPositionTrack() {
}

void DMXPositionTrack::OnCreated() {
	_xPatchable = GC::Hold(new DMXPositionPatchable(this,true));
	_yPatchable = GC::Hold(new DMXPositionPatchable(this,false));

	_plugin->AddPatchable(_xPatchable);
	_plugin->AddPatchable(_yPatchable);
}

DMXPositionMacro DMXPositionTrack::GetMacro(DMXSource ds, ref<Playback> pb) {
	DMXPositionMacro dm;
	std::wstring panAddress = _panAddress;
	std::wstring tiltAddress = _tiltAddress;
	
	if(pb) {
		panAddress = pb->ParseVariables(panAddress);
		tiltAddress = pb->ParseVariables(tiltAddress);
	}

	dm._pan = _plugin->CreateMacro(panAddress, ds);
	dm._tilt = _plugin->CreateMacro(tiltAddress, ds);
	return dm;
}

void DMXPositionTrack::Save(TiXmlElement* parent) {
	SaveAttributeSmall(parent, "pan", _panAddress);
	SaveAttributeSmall(parent, "tilt", _tiltAddress);

	MultifaderTrack::Save(parent);
}

void DMXPositionTrack::Load(TiXmlElement* me) {
	_panAddress = LoadAttributeSmall(me, "pan", _panAddress);
	_tiltAddress = LoadAttributeSmall(me, "tilt", _tiltAddress);
	MultifaderTrack::Load(me);
}

std::wstring DMXPositionTrack::GetTypeName() const {
	return L"DMX Position";
}

ref<Player> DMXPositionTrack::CreatePlayer(ref<Stream> str) {
	return GC::Hold(new DMXPositionPlayer(this, str));
}

Flags<RunMode> DMXPositionTrack::GetSupportedRunModes() {
	return Flags<RunMode>(RunModeMaster);
}

ref<PropertySet> DMXPositionTrack::GetProperties() {
	ref<PropertySet> prs = GC::Hold(new PropertySet());
	prs->Add(DMXPlugin::CreateAddressProperty(TL(dmx_position_pan_address), this, &_panAddress));
	prs->Add(DMXPlugin::CreateAddressProperty(TL(dmx_position_tilt_address), this, &_tiltAddress));
	
	return prs;
}

ref<LiveControl> DMXPositionTrack::CreateControl(ref<Stream> str) {
	return GC::Hold(new DMXPositionControl(this));
}

void DMXPositionTrack::InsertFromControl(Time t, ref<LiveControl> control, bool fade) {
}

void DMXPositionTrack::OnSetInstanceName(const std::wstring& name) {
	_instanceName = name;
}

void DMXPositionTrack::OnSetID(const TrackID& c) {
	_tid = c;
}

/* DMXPositionPatchable */
DMXPositionPatchable::DMXPositionPatchable(ref<DMXPositionTrack> dp, bool x): _dp(dp), _x(x) {
}

DMXPositionPatchable::~DMXPositionPatchable() {
}

ref<Property> DMXPositionPatchable::GetPropertyFor(DMXPatchable::PropertyID pid) {
	ref<DMXPositionTrack> dp = _dp;

	switch(pid) {
		case DMXPatchable::PropertyAddress:
			std::wstring* a = (_x ? &dp->_panAddress : &dp->_tiltAddress);
			return GC::Hold(new GenericProperty<std::wstring>(TL(dmx_address), this, a, *a));
	}

	return 0;
}

void DMXPositionPatchable::SetDMXAddress(const std::wstring& address) {
	ref<DMXPositionTrack> dp = _dp;
	if(dp) {
		if(_x) {
			dp->_panAddress = address;
		}
		else {
			dp->_tiltAddress = address;
		}
	}
}

std::wstring DMXPositionPatchable::GetDMXAddress() const {
	ref<DMXPositionTrack> dp = _dp;
	if(dp) {
		if(_x) {
			return dp->_panAddress;
		}
		else {
			return dp->_tiltAddress;
		}
	}
	return L"";
}

std::wstring DMXPositionPatchable::GetTypeName() const {
	ref<DMXPositionTrack> dp = _dp;
	if(dp) {
		return dp->GetTypeName();
	}
	return L"";
}

std::wstring DMXPositionPatchable::GetInstanceName() const {
	ref<DMXPositionTrack> dp = _dp;
	if(dp) {
		return dp->_instanceName + (_x?L": Pan":L": Tilt");
	}
	return L"";
}

TrackID DMXPositionPatchable::GetID() const {
	ref<DMXPositionTrack> dp = _dp;
	if(dp) {
		return dp->_tid;
	}
	return L"";
}

int DMXPositionPatchable::GetSubChannelID() const {
	return _x?0:1;
}

bool DMXPositionPatchable::GetResetOnStop() const {
	return false; // not supported yet
}

void DMXPositionPatchable::SetResetOnStop(bool t) {
	// not supported yet
}

int DMXPositionPatchable::GetSliderType() {
	return Theme::SliderNormal;
}