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
#include "../include/tjtransfer.h"
using namespace tj::transfer;

/* TransferCue */
TransferCue::TransferCue(Time t): _time(t) {
}

TransferCue::~TransferCue() {
}

void TransferCue::Load(TiXmlElement* you) {
	_time = LoadAttributeSmall(you, "time", _time);
	_rid = LoadAttributeSmall(you, "rid", _rid);
}

void TransferCue::Save(TiXmlElement* you) {
	SaveAttributeSmall(you, "time", _time);
	SaveAttributeSmall(you, "rid", _rid);
}

Time TransferCue::GetTime() const {
	return _time;
}

void TransferCue::SetTime(Time t) {
	_time = t;
}

ref<TransferCue> TransferCue::Clone() {
	ref<TransferCue> bc = GC::Hold(new TransferCue(_time));
	bc->_rid = _rid;
	return bc;
}

void TransferCue::Paint(tj::shared::graphics::Graphics* g, ref<Theme> theme, float pixelLeft, unsigned int y, unsigned int h, ref<CueTrack<TransferCue> > track, bool focus) {
	const static Pixels KCueHeight = 4;
	Pen pn(theme->GetColor(Theme::ColorCurrentPosition), focus ? 2.0f : 1.0f);
	tj::shared::graphics::SolidBrush cueBrush = theme->GetColor(Theme::ColorCurrentPosition);

	g->DrawLine(&pn, pixelLeft, (float)y, pixelLeft, float(y+(h/2)));

	graphics::GraphicsContainer gc = g->BeginContainer();
	g->ResetClip();
	g->TranslateTransform(pixelLeft+(CueTrack<TransferCue>::KCueWidth/2), float(y)+KCueHeight*1.5f);
	if(track->IsExpanded()) {
		g->RotateTransform(90.0f);
		g->TranslateTransform(0,-KCueHeight-2);
	}

	graphics::StringFormat sf;
	sf.SetAlignment(track->IsExpanded() ? graphics::StringAlignmentNear: graphics::StringAlignmentCenter);

	Area trc = theme->MeasureText(_rid, theme->GetGUIFontSmall());
	if(!track->IsExpanded()) {
		trc.SetX(-trc.GetWidth()/2);
	}
	trc.Widen(2,1,2,1);
	g->DrawString(_rid.c_str(), (int)_rid.length(), theme->GetGUIFontSmall(), graphics::PointF(0.0f, 0.0f), &sf, &cueBrush);
	if(focus) {
		theme->DrawFocusRectangle(*g, trc);
	}
	g->EndContainer(gc);
}

ref<PropertySet> TransferCue::GetProperties(ref<Playback> pb, strong< CueTrack<TransferCue> > track) {
	ref<PropertySet> ps = GC::Hold(new PropertySet());
	ps->Add(GC::Hold(new GenericProperty<Time>(TL(transfer_cue_time), this, &_time, 0)));
	if(pb) {
		ps->Add(pb->CreateParsedVariableProperty(TL(transfer_cue_rid), this, &_rid));
	}
	else {
		ps->Add(GC::Hold(new GenericProperty<std::wstring>(TL(transfer_cue_rid), this, &_rid, L"")));
	}
	return ps;
}

void TransferCue::Move(Time t, int h) {
	_time = t;
}

/* TransferTrack */
TransferTrack::TransferTrack(ref<TransferPlugin> pl): _plugin(pl), _expanded(false) {
}

TransferTrack::~TransferTrack() {
}

bool TransferTrack::IsExpandable() {
	return true;
}

Pixels TransferTrack::GetHeight() {
	return _expanded ? 61 : CueTrack<TransferCue>::GetHeight();
}

void TransferTrack::SetExpanded(bool t) {
	_expanded = t;
}

bool TransferTrack::IsExpanded() {
	return _expanded;
}

std::wstring TransferTrack::GetTypeName() const {
	return TL(transfer_type_name);
}

Flags<RunMode> TransferTrack::GetSupportedRunModes() {
	Flags<RunMode> rm;
	rm.Set(RunModeClient,true);
	return rm;
}

ref<Player> TransferTrack::CreatePlayer(ref<Stream> st) {
	return GC::Hold(new TransferPlayer(this, _plugin, st));
}

std::wstring TransferTrack::GetEmptyHintText() const {
	return TL(transfer_help_text);
}