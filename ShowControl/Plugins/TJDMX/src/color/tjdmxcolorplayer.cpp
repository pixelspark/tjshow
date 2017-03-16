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

DMXColorPlayer::DMXColorPlayer(ref<DMXColorTrack> track): _track(track), _output(false) {
}

DMXColorPlayer::~DMXColorPlayer() {
}

ref<Track> DMXColorPlayer::GetTrack() {
	return _track;
}

void DMXColorPlayer::Stop() {
}

void DMXColorPlayer::Start(Time pos, ref<Playback> playback, float speed) {
	ref<DMXPlugin> dp = _track->_plugin;
	ref<DMXController> dc = dp->GetController();

	for(int a = int(ColorChannelRed); a < int(_ColorChannelLast); a++) {
		const std::wstring& address = _track->_dmx[a];
		_macros[a] = dc->CreateMacro(address, DMXSequence);
	}
}

void DMXColorPlayer::Pause(Time pos) {
}

void DMXColorPlayer::Tick(Time t) {
	if(_output) {
		HSVColor color(_track->GetFaderById(DMXColorTrack::KFaderHue)->GetValueAt(t), _track->GetFaderById(DMXColorTrack::KFaderSaturation)->GetValueAt(t), _track->GetFaderById(DMXColorTrack::KFaderValue)->GetValueAt(t));
		_track->_lastColor = color;
		RGBColor rgbColor = ColorSpaces::HSVToRGB(color._h, color._s, color._v);
		CMYKColor cmykColor = ColorSpaces::RGBToCMYK(rgbColor._r, rgbColor._g, rgbColor._b);

		// RGB
		if(_macros[ColorChannelRed]) _macros[ColorChannelRed]->Set(float(rgbColor._r));
		if(_macros[ColorChannelGreen]) _macros[ColorChannelGreen]->Set(float(rgbColor._g));
		if(_macros[ColorChannelBlue]) _macros[ColorChannelBlue]->Set(float(rgbColor._b));

		// CMY
		if(_macros[ColorChannelCyan]) _macros[ColorChannelCyan]->Set(float(cmykColor._c));
		if(_macros[ColorChannelMagenta]) _macros[ColorChannelMagenta]->Set(float(cmykColor._m));
		if(_macros[ColorChannelYellow]) _macros[ColorChannelYellow]->Set(float(cmykColor._y));

		// HSV
		if(_macros[ColorChannelHue]) _macros[ColorChannelHue]->Set(float(color._h));
		if(_macros[ColorChannelSaturation]) _macros[ColorChannelSaturation]->Set(float(color._s));
		if(_macros[ColorChannelValue]) _macros[ColorChannelValue]->Set(float(color._v));
	}
}

void DMXColorPlayer::Jump(Time t, bool pause) {
	Tick(t);
}

Time DMXColorPlayer::GetNextEvent(Time t) {
	return _track->GetNextEvent(t);
}

void DMXColorPlayer::SetPlaybackSpeed(Time t, float c) {
}

void DMXColorPlayer::SetOutput(bool enable) {
	_output = enable;
}