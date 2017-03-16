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
#include "../../include/color/tjdmxcolor.h"
using namespace tj::dmx::color;
using namespace tj::shared::graphics;

namespace tj {
	namespace dmx {
		namespace color {
			class DMXColorLiveWnd: public ChildWnd, public Listener<ColorPopupWnd::NotificationChanged> {
				public:
					DMXColorLiveWnd(ref<DMXColorTrack> track);
					virtual ~DMXColorLiveWnd();
					virtual void Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme);
					virtual void Notify(ref<Object> source, const ColorPopupWnd::NotificationChanged& nv);

				protected:
					virtual void OnMouse(MouseEvent ev, Pixels x, Pixels y);
					virtual void OnManualColorChanged();
					virtual void GetMacros(bool force);

					weak<DMXColorTrack> _track;
					ref<ColorPopupWnd> _colorPopup;
					ref<DMXMacro> _macros[_ColorChannelLast];
					std::wstring _dmx[_ColorChannelLast];
					HSVColor _manual;
			};

			class DMXColorLiveControl: public LiveControl {
				public:
					DMXColorLiveControl(ref<DMXColorTrack> track);
					virtual ~DMXColorLiveControl();
					virtual ref<tj::shared::Wnd> GetWindow();
					virtual void Update();
					virtual std::wstring GetGroupName();

				protected:
					weak<DMXColorTrack> _track;
					ref<DMXColorLiveWnd> _control;
					
			};
		}
	}
}

DMXColorTrack::DMXColorTrack(ref<DMXPlugin> dp): _plugin(dp) {
	AddFader(KFaderHue, TL(dmx_color_hue), 0.0f);
	AddFader(KFaderSaturation, TL(dmx_color_saturation), 0.0f);
	AddFader(KFaderValue, TL(dmx_color_value), 0.0f);
}

DMXColorTrack::~DMXColorTrack() {
}

std::wstring DMXColorTrack::GetTypeName() const {
	return TL(dmx_color_track_type);
}

tj::shared::Flags<RunMode> DMXColorTrack::GetSupportedRunModes() {
	Flags<RunMode> rm;
	rm.Set(RunModeMaster,true);
	return rm;
}

ref<Player> DMXColorTrack::CreatePlayer(ref<Stream> stream) {
	return GC::Hold(new DMXColorPlayer(this));
}

ref<LiveControl> DMXColorTrack::CreateControl(ref<Stream> stream) {
	return GC::Hold(new DMXColorLiveControl(this));
}

ref<PropertySet> DMXColorTrack::GetProperties() {
	ref<PropertySet> prs = GC::Hold(new PropertySet());	
	prs->Add(GC::Hold(new GenericProperty<std::wstring>(TL(dmx_color_red_address),		this, &_dmx[ColorChannelRed],	_dmx[ColorChannelRed])));
	prs->Add(GC::Hold(new GenericProperty<std::wstring>(TL(dmx_color_green_address),	this, &_dmx[ColorChannelGreen],	_dmx[ColorChannelGreen])));
	prs->Add(GC::Hold(new GenericProperty<std::wstring>(TL(dmx_color_blue_address),		this, &_dmx[ColorChannelBlue],	_dmx[ColorChannelBlue])));
	prs->Add(GC::Hold(new GenericProperty<std::wstring>(TL(dmx_color_cyan_address),		this, &_dmx[ColorChannelCyan],	_dmx[ColorChannelCyan])));
	prs->Add(GC::Hold(new GenericProperty<std::wstring>(TL(dmx_color_magenta_address),	this, &_dmx[ColorChannelMagenta],_dmx[ColorChannelMagenta])));
	prs->Add(GC::Hold(new GenericProperty<std::wstring>(TL(dmx_color_yellow_address),	this, &_dmx[ColorChannelYellow],_dmx[ColorChannelYellow])));
	prs->Add(GC::Hold(new GenericProperty<std::wstring>(TL(dmx_color_hue_address),		this, &_dmx[ColorChannelHue],	_dmx[ColorChannelHue])));
	prs->Add(GC::Hold(new GenericProperty<std::wstring>(TL(dmx_color_saturation_address),this, &_dmx[ColorChannelSaturation],_dmx[ColorChannelSaturation])));
	prs->Add(GC::Hold(new GenericProperty<std::wstring>(TL(dmx_color_value_address),	this, &_dmx[ColorChannelValue],	_dmx[ColorChannelValue])));

	return prs;
}

void DMXColorTrack::Save(TiXmlElement* parent) {
	SaveAttributeSmall(parent, "red", _dmx[ColorChannelRed]);
	SaveAttributeSmall(parent, "green", _dmx[ColorChannelGreen]);
	SaveAttributeSmall(parent, "blue", _dmx[ColorChannelBlue]);

	SaveAttributeSmall(parent, "hue", _dmx[ColorChannelHue]);
	SaveAttributeSmall(parent, "sat", _dmx[ColorChannelSaturation]);
	SaveAttributeSmall(parent, "value", _dmx[ColorChannelValue]);

	SaveAttributeSmall(parent, "cyan", _dmx[ColorChannelCyan]);
	SaveAttributeSmall(parent, "magenta", _dmx[ColorChannelMagenta]);
	SaveAttributeSmall(parent, "yellow", _dmx[ColorChannelYellow]);

	TiXmlElement faders("faders");
	MultifaderTrack::Save(&faders);
	parent->InsertEndChild(faders);
}
void DMXColorTrack::Load(TiXmlElement* you) {
	_dmx[ColorChannelRed] = LoadAttributeSmall(you, "red", _dmx[ColorChannelRed]);
	_dmx[ColorChannelGreen] = LoadAttributeSmall(you, "green", _dmx[ColorChannelGreen]);
	_dmx[ColorChannelBlue] = LoadAttributeSmall(you, "blue", _dmx[ColorChannelBlue]);

	_dmx[ColorChannelHue] = LoadAttributeSmall(you, "hue", _dmx[ColorChannelHue]);
	_dmx[ColorChannelSaturation] = LoadAttributeSmall(you, "sat", _dmx[ColorChannelSaturation]);
	_dmx[ColorChannelValue] = LoadAttributeSmall(you, "value", _dmx[ColorChannelValue]);

	_dmx[ColorChannelCyan] = LoadAttributeSmall(you, "cyan", _dmx[ColorChannelCyan]);
	_dmx[ColorChannelMagenta] = LoadAttributeSmall(you, "magenta", _dmx[ColorChannelMagenta]);
	_dmx[ColorChannelYellow] = LoadAttributeSmall(you, "yellow", _dmx[ColorChannelYellow]);

	TiXmlElement* faders = you->FirstChildElement("faders");
	if(faders!=0) {
		MultifaderTrack::Load(faders);
	}
}

void DMXColorTrack::Notify(ref<Object> source, const ColorPopupWnd::NotificationChanged& nf) {
	// Get HSV color from popup and save it in the faders
	HSVColor color = _colorPopup->GetHSVColor();
	GetFaderById(KFaderHue)->SetPoint(_colorPopupCurrent, (float)color._h);
	GetFaderById(KFaderSaturation)->SetPoint(_colorPopupCurrent, (float)color._s);
	GetFaderById(KFaderValue)->SetPoint(_colorPopupCurrent, (float)color._v);
}

ref<Item> DMXColorTrack::GetItemAt(Time position,unsigned int h, bool rightClick, int trackHeight, float pixelsPerMs) {
	if(h>GetHeight()-((unsigned int)GetFaderHeight())) {
		 // Clearly, this is meant for us! When it is a right click, show the color chooser popup
		if(rightClick) {
			if(!_colorPopup) {
				_colorPopup = GC::Hold(new ColorPopupWnd());
				_colorPopup->EventChanged.AddListener(ref< Listener<ColorPopupWnd::NotificationChanged> >(this));
			}

			HSVColor color(GetFaderById(KFaderHue)->GetValueAt(position), GetFaderById(KFaderSaturation)->GetValueAt(position), GetFaderById(KFaderValue)->GetValueAt(position));
			RGBColor rgbColor = ColorSpaces::HSVToRGB(color._h, color._s, color._v);
			_colorPopup->SetColor(rgbColor);
			_colorPopupCurrent = position;
			_colorPopup->PopupAtMouse();
		}
		return 0;
	}
	else {
		return MultifaderTrack::GetItemAt(position,h,rightClick,trackHeight, pixelsPerMs);
	}
}

void DMXColorTrack::Paint(Graphics* g, Time position, float pixelsPerMs, unsigned int x, unsigned int y, unsigned int h, Time start, Time end, ref<Item> focus) {
	ref<Fader<float> > hue = GetFaderById(KFaderHue);
	ref<Fader<float> > sat = GetFaderById(KFaderSaturation);
	ref<Fader<float> > val = GetFaderById(KFaderValue);

	Time t = start;
	HSVColor color(hue->GetValueAt(start), sat->GetValueAt(start), val->GetValueAt(start));

	do {
		Time hnext = hue->GetNextPoint(t);
		Time snext = sat->GetNextPoint(t);
		Time vnext = val->GetNextPoint(t);
		if(hnext<=t) hnext = end;
		if(snext<=t) snext = end;
		if(vnext<=t) vnext = end;
		Time next = min(hnext, min(snext, vnext));

		// Calculate colors
		HSVColor nextColor(hue->GetValueAt(next), sat->GetValueAt(next), val->GetValueAt(next));	

		Color col = ColorSpaces::HSVToRGB(color._h, color._s, color._v);
		Color colNext = ColorSpaces::HSVToRGB(nextColor._h, nextColor._s, nextColor._v);
		float left = x+pixelsPerMs*float(t-start);
		float right = x+pixelsPerMs*float(next-start);
		LinearGradientBrush lbr(PointF(left-1.0f,0.0f), PointF(right,0.0f), col, colNext);
		g->FillRectangle(&lbr, RectF(left-1.0f, float(y), right-left+1, float(GetFaderHeight()-3)));

		t = next;
		color = nextColor;
	} 
	while(t < end && t >= Time(0));
	
	MultifaderTrack::Paint(g,position,pixelsPerMs,x,y,h,start,end,focus);
}

/** DMXColorLiveControl **/
DMXColorLiveControl::DMXColorLiveControl(ref<DMXColorTrack> track): _track(track) {
	_control = GC::Hold(new DMXColorLiveWnd(track));
}

DMXColorLiveControl::~DMXColorLiveControl() {
}

ref<tj::shared::Wnd> DMXColorLiveControl::GetWindow() {
	return _control;
}

void DMXColorLiveControl::Update() {
	_control->Repaint(); // TODO: check if the color has changed and only repaint when it has changed
}

DMXColorLiveWnd::DMXColorLiveWnd(ref<DMXColorTrack> track): ChildWnd(L""), _track(track) {
	GetMacros(true);
}

DMXColorLiveWnd::~DMXColorLiveWnd() {
}

void DMXColorLiveWnd::Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme) {
	Area rc = GetClientArea();
	SolidBrush back(theme->GetColor(Theme::ColorBackground));
	g.FillRectangle(&back, rc);

	Area colors(rc.GetTop(), rc.GetLeft(), rc.GetWidth(), rc.GetWidth());
	ref<DMXColorTrack> track = _track;
	if(track) { 
		HSVColor result(track->_lastColor); // This is actually the last sent color from sequence
		Color manualColor = ColorSpaces::HSVToRGB(_manual._h, _manual._s, _manual._v);
		Color resultColor = ColorSpaces::HSVToRGB(result._h, result._s, result._v);
		SolidBrush manualBr(manualColor);
		SolidBrush resultBr(resultColor);

		Area colorLeft = colors;
		Area colorRight = colors;
		colorLeft.Narrow(0,0,colors.GetWidth()/2,0);
		colorRight.Narrow(colors.GetWidth()/2,0,0,0);

		// Manual value on the left
		g.FillRectangle(&manualBr, colorLeft);
		g.FillRectangle(&resultBr, colorRight);
	}
}

std::wstring DMXColorLiveControl::GetGroupName() {
	return L"DMX";
}

void DMXColorLiveWnd::OnMouse(MouseEvent ev, Pixels x, Pixels y) {
	if(ev==MouseEventRDown) {
		Area rc = GetClientArea();
		if(x<rc.GetWidth()) { // color choosers
			if(!_colorPopup) {
				_colorPopup = GC::Hold(new ColorPopupWnd());
				_colorPopup->EventChanged.AddListener(ref< Listener<ColorPopupWnd::NotificationChanged> >(this));
			}
			_colorPopup->SetColor(_manual);
			_colorPopup->PopupAt(rc.GetLeft(), rc.GetWidth()+1, this);
		}
	}
}

void DMXColorLiveWnd::Notify(ref<Object> source, const ColorPopupWnd::NotificationChanged& data) {
	_manual = _colorPopup->GetHSVColor();
	Repaint();
	OnManualColorChanged();
}

void DMXColorLiveWnd::GetMacros(bool force) {
	ref<DMXColorTrack> track = _track;
	if(track) {
		ref<DMXController> dc = track->_plugin->GetController();

		for(int a=ColorChannelRed; a<int(_ColorChannelLast); a++) {
			if(force || _dmx[a] != track->_dmx[a]) {
				_dmx[a] = track->_dmx[a];
				_macros[a] = dc->CreateMacro(_dmx[a], DMXManual);
			}
		}
	}
}

void DMXColorLiveWnd::OnManualColorChanged() {
	GetMacros(false);
	RGBColor rgbColor = ColorSpaces::HSVToRGB(_manual._h, _manual._s, _manual._v);
	CMYKColor cmykColor = ColorSpaces::RGBToCMYK(rgbColor._r, rgbColor._g, rgbColor._b);

	if(_macros[ColorChannelHue]) _macros[ColorChannelHue]->Set((float)_manual._h);
	if(_macros[ColorChannelSaturation]) _macros[ColorChannelSaturation]->Set((float)_manual._s);
	if(_macros[ColorChannelValue]) _macros[ColorChannelValue]->Set((float)_manual._v);

	if(_macros[ColorChannelRed]) _macros[ColorChannelRed]->Set((float)rgbColor._r);
	if(_macros[ColorChannelGreen]) _macros[ColorChannelGreen]->Set((float)rgbColor._g);
	if(_macros[ColorChannelBlue]) _macros[ColorChannelBlue]->Set((float)rgbColor._b);

	if(_macros[ColorChannelCyan]) _macros[ColorChannelCyan]->Set((float)cmykColor._c);
	if(_macros[ColorChannelMagenta]) _macros[ColorChannelMagenta]->Set((float)cmykColor._m);
	if(_macros[ColorChannelYellow]) _macros[ColorChannelYellow]->Set((float)cmykColor._y);
}