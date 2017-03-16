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
#include "../../include/tjmidi.h"
using namespace tj::shared;
using namespace tj::show;

// MSCCue
MSCCue::MSCCue(Time pos): _time(pos), _command((MSC::Command)0x00) {
}

MSCCue::~MSCCue() {
}

void MSCCue::Load(TiXmlElement* you) {
	_time = LoadAttributeSmall(you, "time", _time);
	_description = LoadAttribute(you, "description", _description);
	_command = (MSC::Command)LoadAttributeSmall<unsigned int>(you, "command", _command);

	TiXmlElement* cue = you->FirstChildElement("msc-cue");
	if(cue!=0) {
		_cue.Load(cue);
	}
	else {
		_cue = MSC::CueID();
	}
}

void MSCCue::Save(TiXmlElement* you) {
	TiXmlElement cue("cue");
	SaveAttributeSmall(&cue, "time", _time);
	SaveAttributeSmall(&cue, "command", (unsigned int)_command);
	SaveAttribute(&cue, "description", _description);

	TiXmlElement mscCue("msc-cue");
	_cue.Save(&mscCue);
	cue.InsertEndChild(mscCue);
	you->InsertEndChild(cue);
}

void MSCCue::Move(Time t, int h) {
	_time = t;
}

ref<MSCCue> MSCCue::Clone() {
	ref<MSCCue> nc = GC::Hold(new MSCCue(_time));
	nc->_command = _command;
	nc->_description = _description;
	nc->_cue = _cue;
	return nc;
}

bool MSCCue::NeedToSendCue() const {
	return _cue._cueNumber.length()>0;
}

void MSCCue::Fire(ref<MIDIOutputDevice> mo, const MSC::DeviceID& devid, const MSC::CommandFormat& cf, const std::wstring& defList, const std::wstring& defPath) {
	if(mo) {
		if(NeedToSendCue()) {
			ref<DataWriter> cw = GC::Hold(new DataWriter(128));

			// When no cue list or path is given, use the default
			MSC::CueID cid = _cue;
			if(cid._cueList.length()<1) {
				cid._cueList = defList;
			}
			
			if(cid._cuePath.length()<1) {
				cid._cuePath = defPath;
			}

			cid.Serialize(cw);
			mo->SendMSCCommand(devid, cf, _command, (unsigned int)cw->GetSize(), cw->GetBuffer());
		}
		else {
			mo->SendMSCCommand(devid, cf, _command, 0, 0);
		}
	}
}

ref<PropertySet> MSCCue::GetProperties(ref<Playback> pb, strong< CueTrack<MSCCue> > track) {
	ref<PropertySet> ps = GC::Hold(new PropertySet());
	ps->Add(GC::Hold(new GenericProperty<Time>(TL(midi_msc_cue_time), this, &_time, _time)));
	ps->Add(GC::Hold(new TextProperty(TL(midi_msc_cue_description), this, &_description)));

	ref<GenericListProperty<MSC::Command> > pc = GC::Hold(new GenericListProperty<MSC::Command>(TL(midi_msc_cue_command), this, &_command, _command));
	for(int a=0;a<MSC::KMaxCommand;a++) {
		pc->AddOption(MSC::Commands[a], (MSC::Command)a);
	}
	ps->Add(pc);
	
	_cue.AddProperties(ps, this);
	return ps;
}

Time MSCCue::GetTime() const {
	return _time;
}

void MSCCue::SetTime(Time t) {
	_time = t;
}

void MSCCue::Paint(tj::shared::graphics::Graphics* g, ref<Theme> theme, float pixelLeft, unsigned int y, unsigned int h, ref< CueTrack<MSCCue> > track, bool focus) {
	std::wstring str = MSC::GetCommandName(_command);
	if(_cue._cueNumber.length()>0) {
		str = str + L" " + _cue._cueNumber;
	}

	const static Pixels KCueHeight = 4;
	graphics::Pen pn(theme->GetColor(Theme::ColorCurrentPosition), focus? 2.0f : 1.0f);
	graphics::SolidBrush cueBrush = theme->GetColor(Theme::ColorCurrentPosition);
	graphics::SolidBrush onBrush(graphics::Color(1.0, 0.0, 0.0));
	graphics::SolidBrush offBrush(graphics::Color(0.5, 0.5, 0.5));
	g->DrawLine(&pn, pixelLeft, (float)y, pixelLeft, float(y+(KCueHeight)));
	
	graphics::StringFormat sf;
	sf.SetAlignment(track->IsExpanded() ? graphics::StringAlignmentNear: graphics::StringAlignmentCenter);

	graphics::GraphicsContainer gc = g->BeginContainer();
	g->ResetClip();
	g->TranslateTransform(pixelLeft+(CueTrack<MSCCue>::KCueWidth/2), float(y)+KCueHeight*1.5f);
	if(track->IsExpanded()) {
		g->RotateTransform(90.0f);
		g->TranslateTransform(0,-KCueHeight-2);
	}

	Area trc = theme->MeasureText(str, theme->GetGUIFontSmall());
	if(!track->IsExpanded()) {
		trc.SetX(-trc.GetWidth()/2);
	}
	trc.Widen(2,1,2,1);
	g->DrawString(str.c_str(), (INT)str.length(), theme->GetGUIFontSmall(), graphics::PointF(0.0f, 0.0f), &sf, &cueBrush);
	if(focus) {
		theme->DrawFocusRectangle(*g, trc);
	}
	g->EndContainer(gc);
}

// MSCTrack
MSCTrack::MSCTrack(ref<Playback> pb): _pb(pb), _expanded(false), _deviceID(0x00), _commandFormat(MSC::Lighting) {
}

MSCTrack::~MSCTrack() {
}

bool MSCTrack::IsExpandable() {
	return true;
}

Pixels MSCTrack::GetHeight() {
	return _expanded ? 61 : CueTrack<MSCCue>::GetHeight();
}

void MSCTrack::SetExpanded(bool t) {
	_expanded = t;
}

bool MSCTrack::IsExpanded() {
	return _expanded;
}


std::wstring MSCTrack::GetTypeName() const {
	return L"MSC";
}

ref<Player> MSCTrack::CreatePlayer(ref<Stream> str) {
	return GC::Hold(new MSCPlayer(this,str));
}

Flags<RunMode> MSCTrack::GetSupportedRunModes() {
	Flags<RunMode> rm;
	rm.Set(RunModeMaster, true);
	rm.Set(RunModeDont, true);
	return rm;
}

ref<PropertySet> MSCTrack::GetProperties() {
	ref<PropertySet> ps = GC::Hold(new PropertySet());
	ps->Add(_pb->CreateSelectPatchProperty(TL(midi_msc_output_device), this, &_out));
	ps->Add(GC::Hold(new GenericProperty<unsigned int>(TL(midi_msc_device_id), this, &_deviceID, _deviceID)));

	ref<GenericListProperty<MSC::CommandFormat> > pc = GC::Hold(new GenericListProperty<MSC::CommandFormat>(TL(midi_msc_command_format), this, &_commandFormat, _commandFormat));
	bool showGMPrograms = true;
	for(int a=0;a<=MSC::KMaxCommandFormat;a++) {
		pc->AddOption(MSC::CommandFormats[a], (MSC::CommandFormat)a);
	}
	ps->Add(pc);

	ps->Add(GC::Hold(new PropertySeparator(TL(midi_msc_cues))));
	ps->Add(GC::Hold(new GenericProperty<std::wstring>(TL(midi_msc_default_cue_path), this, &_defaultCuePath, _defaultCuePath)));
	ps->Add(GC::Hold(new GenericProperty<std::wstring>(TL(midi_msc_default_cue_list), this, &_defaultCueList, _defaultCueList)));

	return ps;
}

ref<LiveControl> MSCTrack::CreateControl(ref<Stream> str) {
	return 0;
}

MSC::CommandFormat MSCTrack::GetCommandFormat() const {
	return _commandFormat;
}

void MSCTrack::Save(TiXmlElement* parent) {
	SaveAttributeSmall(parent, "output", _out);
	SaveAttributeSmall<unsigned int>(parent, "device-id", _deviceID);
	SaveAttributeSmall<unsigned int>(parent, "command-format", _commandFormat);
	SaveAttributeSmall(parent, "default-cue-list", _defaultCueList);
	SaveAttributeSmall(parent, "default-cue-path", _defaultCuePath);
	CueTrack<MSCCue>::Save(parent);
}

void MSCTrack::Load(TiXmlElement* you) {
	_out = LoadAttributeSmall(you, "output", _out);
	_deviceID = (MSC::DeviceID)LoadAttributeSmall<unsigned int>(you, "device-id", _deviceID);
	_commandFormat = (MSC::CommandFormat)LoadAttributeSmall<unsigned int>(you, "command-format", _commandFormat);
	_defaultCueList = LoadAttributeSmall<>(you, "default-cue-list", _defaultCueList);
	_defaultCuePath = LoadAttributeSmall<>(you, "default-cue-path", _defaultCuePath);
	CueTrack<MSCCue>::Load(you);
}

const std::wstring& MSCTrack::GetDefaultCuePath() const {
	return _defaultCuePath;
}

const std::wstring& MSCTrack::GetDefaultCueList() const {
	return _defaultCueList;
}

ref<MIDIOutputDevice> MSCTrack::GetOutputDevice() {
	if(_pb) {
		ref<Device> dev = _pb->GetDeviceByPatch(_out);
		if(dev.IsCastableTo<MIDIOutputDevice>()) {
			return dev;
		}
	}
	Throw(L"Invalid device patched, should be a midi output device",ExceptionTypeError);
}

MSC::DeviceID MSCTrack::GetDeviceID() const {
	return _deviceID;
}

std::wstring MSCTrack::GetEmptyHintText() const {
	return TL(midi_msc_empty_hint);
}

/* MSCPlayer */
MSCPlayer::MSCPlayer(ref<MSCTrack> tr, ref<Stream> str): _track(tr), _output(false) {
	assert(tr);
	_stream = str;
	_out = tr->GetOutputDevice();
	_last = Time(-1);
}

MSCPlayer::~MSCPlayer() {
}

ref<Track> MSCPlayer::GetTrack() {
	return ref<Track>(_track);
}

void MSCPlayer::Stop() {
	_out = 0;
	_last = Time(-1);
}

void MSCPlayer::Start(Time pos, ref<Playback> pb, float speed) {
	_last = pos;
}

Time MSCPlayer::GetNextEvent(Time t) {
	ref<MSCCue> cue = _track->GetCueAfter(t);
	if(cue) {
		return cue->GetTime()+Time(1);
	}

	return -1;
}

void MSCPlayer::Tick(Time currentPosition) {
	if(_out && _output) {
		std::vector< ref<MSCCue> > cues;
		_track->GetCuesBetween(_last, currentPosition, cues);
		std::vector< ref<MSCCue> >::iterator it = cues.begin();

		while(it!=cues.end()) {
			ref<MSCCue> cur = *it;
			cur->Fire(_out, _track->GetDeviceID(), _track->GetCommandFormat(), _track->GetDefaultCueList(), _track->GetDefaultCuePath());
			++it;
		}
	}
	_last = currentPosition;
}

void MSCPlayer::Jump(Time newT, bool paused) {
	_last = newT;
}

void MSCPlayer::SetOutput(bool enable) {
	_output = enable;
}