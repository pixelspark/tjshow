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
#include <limits>
#include <iomanip>
#include "../include/tjosccuetrack.h"
#include <OSCPack/OscOutboundPacketStream.h>

using namespace tj::show;
using namespace tj::shared;
using namespace tj::osc;
namespace oscp = osc;

/** OSCOutputCue **/
OSCOutputCue::OSCOutputCue(Time pos): _time(pos), _arguments(GC::Hold(new OSCArgumentList())) {
}

OSCOutputCue::~OSCOutputCue() {
}

void OSCOutputCue::Load(TiXmlElement* you) {
	_time = LoadAttributeSmall(you, "time", _time);
	_description = LoadAttribute(you, "description", _description);
	_path = LoadAttributeSmall(you, "action", _path);

	TiXmlElement* argument = you->FirstChildElement("argument");
	while(argument!=0) {
		OSCArgument arg;
		arg._type = (oscp::TypeTagValues)LoadAttributeSmall<char>(argument, "type", oscp::NIL_TYPE_TAG);
		arg._description = LoadAttributeSmall<std::wstring>(argument, "description", L"");
		arg._value = LoadAttributeSmall<std::wstring>(argument, "value", L"");
		 _arguments->_arguments.push_back(arg);
		argument = argument->NextSiblingElement("argument");
	}
}

void OSCOutputCue::Save(TiXmlElement* me) {
	TiXmlElement cue("cue");
	SaveAttributeSmall(&cue, "time", _time);
	SaveAttribute(&cue, "description", _description);
	SaveAttributeSmall(&cue, "action", _path);
	
	std::vector<OSCArgument>::const_iterator it = _arguments->_arguments.begin();
	while(it!= _arguments->_arguments.end()) {
		const OSCArgument& arg = *it;
		TiXmlElement argument("argument");
		SaveAttributeSmall<char>(&argument, "type", (char)arg._type);
		SaveAttributeSmall(&argument, "value", arg._value);
		SaveAttributeSmall(&argument, "description", arg._description);
		cue.InsertEndChild(argument);
		++it;
	}

	me->InsertEndChild(cue);
}

void OSCOutputCue::Move(Time t, int h) {
	_time = t;
}

ref<OSCOutputCue> OSCOutputCue::Clone() {
	ref<OSCOutputCue> nc = GC::Hold(new OSCOutputCue(_time));
	nc->_description = _description;
	return nc;
}

ref<PropertySet> OSCOutputCue::GetProperties(ref<Playback> pb, strong< CueTrack<OSCOutputCue> > track) {
	ref<PropertySet> ps = GC::Hold(new PropertySet());
	ps->Add(GC::Hold(new GenericProperty<Time>(TL(osc_cue_time), this, &_time, _time)));
	ps->Add(GC::Hold(new TextProperty(TL(osc_cue_description), this, &_description)));

	ps->Add(GC::Hold(new PropertySeparator(TL(osc_message))));
	ps->Add(pb->CreateParsedVariableProperty(TL(osc_message_path), this, &_path, false));
	ps->Add(GC::Hold(new OSCArgumentProperty(TL(osc_arguments), _arguments, pb)));
	return ps;
}

Time OSCOutputCue::GetTime() const {
	return _time;
}

void OSCOutputCue::SetTime(Time t) {
	_time = t;
}

void OSCOutputCue::Paint(tj::shared::graphics::Graphics* g, ref<Theme> theme, float pixelLeft, unsigned int y, unsigned int h, ref< CueTrack<OSCOutputCue> > track, bool focus) {
	const static Pixels KCueHeight = 4;
	graphics::Pen pn(theme->GetColor(Theme::ColorCurrentPosition), focus? 2.0f : 1.0f);
	graphics::SolidBrush cueBrush = theme->GetColor(Theme::ColorCurrentPosition);
	graphics::SolidBrush onBrush(graphics::Color(1.0, 0.0, 0.0));
	graphics::SolidBrush offBrush(graphics::Color(0.5, 0.5, 0.5));
	g->DrawLine(&pn, pixelLeft, (float)y, pixelLeft, float(y+(KCueHeight)));
	
	graphics::StringFormat sf;
	sf.SetAlignment(track->IsExpanded() ? graphics::StringAlignmentNear: graphics::StringAlignmentCenter);
	
	// TODO only show last part of path?
	std::wstring str = _path;
	if(str.length()<1) {
		str = ref<OSCOutputCueTrack>(track)->_default;
	}

	graphics::GraphicsContainer gc = g->BeginContainer();
	g->ResetClip();
	g->TranslateTransform(pixelLeft+(CueTrack<OSCOutputCue>::KCueWidth/2), float(y)+KCueHeight*1.5f);
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

void OSCOutputCue::Fire(ref<OSCDevice> dev, ref<Playback> pb, const std::wstring& pre, const std::wstring& def) {
	if(dev) {
		char buffer[4096];
		oscp::OutboundPacketStream p(buffer, 4096);
		std::wstring path = pre + ((_path.size()<1) ? def : _path);
		p << oscp::BeginMessage(Mbs(path).c_str());

		std::vector<OSCArgument>::const_iterator it = _arguments->_arguments.begin();
		while(it!=_arguments->_arguments.end()) {
			const OSCArgument& arg = *it;
			std::wstring parsedValue = pb ? pb->ParseVariables(arg._value) : arg._value;

			switch(arg._type) {
				case oscp::TRUE_TYPE_TAG: {
					if(parsedValue.length()<1) {
						p << true;
					}
					else {
						p << StringTo<bool>(parsedValue, true);
					}
					break;
				}

				case oscp::FALSE_TYPE_TAG: {
					if(parsedValue.length()<1) {
						p << false;
					}
					else {
						p << StringTo<bool>(parsedValue, false);
					}
					break;
				}

				case oscp::NIL_TYPE_TAG:
					p << oscp::Nil;
					break;

				case oscp::INFINITUM_TYPE_TAG:
					p << oscp::Infinitum;
					break;

				case oscp::INT32_TYPE_TAG:
					p << StringTo<oscp::int32>(parsedValue, 0);
					break;

				case oscp::FLOAT_TYPE_TAG:
					p << StringTo<float>(parsedValue, std::numeric_limits<float>::quiet_NaN());
					break;

				case oscp::CHAR_TYPE_TAG:
					p << (char)StringTo<unsigned int>(parsedValue, '\0');
					break;

				case oscp::INT64_TYPE_TAG:
					p << StringTo<__int64>(parsedValue, 0);
					break;

				case oscp::STRING_TYPE_TAG:
					p << Mbs(parsedValue).c_str();
					break;

				case oscp::RGBA_COLOR_TYPE_TAG: {
					// Parse color
					std::wistringstream wis(parsedValue);
					unsigned int r = 0;
					wis >> std::setw(8) >> std::setfill(L'0') >> std::hex >> r;
					p << oscp::RgbaColor(r);
					break;								
				}
				
				case oscp::MIDI_MESSAGE_TYPE_TAG: {
					std::wistringstream wis(parsedValue);
					unsigned int r = 0;
					wis >> std::setw(8) >> std::setfill(L'0') >> std::hex >> r;
					p << oscp::MidiMessage(r);
					break;
				}

				case oscp::BLOB_TYPE_TAG:
				case oscp::SYMBOL_TYPE_TAG:
				case oscp::TIME_TAG_TYPE_TAG:
				default:
					Throw(L"OSC argument type not supported when sending OSCOutputCue", ExceptionTypeError);
			}

			++it;
		}

		p << oscp::EndMessage;
		dev->Send(p.Data(), p.Size());
	}
}

/** OSCOutputCueTrack **/
OSCOutputCueTrack::OSCOutputCueTrack(ref<Playback> pb): CueTrack<OSCOutputCue>(pb), _pb(pb), _expanded(false) {
}

OSCOutputCueTrack::~OSCOutputCueTrack() {
}

std::wstring OSCOutputCueTrack::GetTypeName() const {
	return L"OSC";
}

bool OSCOutputCueTrack::IsExpandable() {
	return true;
}

Pixels OSCOutputCueTrack::GetHeight() {
	return _expanded ? 61 : CueTrack<OSCOutputCue>::GetHeight();
}

void OSCOutputCueTrack::SetExpanded(bool t) {
	_expanded = t;
}

bool OSCOutputCueTrack::IsExpanded() {
	return _expanded;
}

ref<Player> OSCOutputCueTrack::CreatePlayer(ref<Stream> str) {
	return GC::Hold(new OSCOutputCuePlayer(ref<OSCOutputCueTrack>(this), str));
}

Flags<RunMode> OSCOutputCueTrack::GetSupportedRunModes() {
	Flags<RunMode> rf;
	rf.Set(RunModeMaster, true);
	return rf;
}

ref<PropertySet> OSCOutputCueTrack::GetProperties() {
	ref<PropertySet> ps = GC::Hold(new PropertySet());
	ps->Add(_pb->CreateSelectPatchProperty(TL(osc_output_device), this, &_out));
	ps->Add(_pb->CreateParsedVariableProperty(TL(osc_path_prefix), this, &_prefix));
	ps->Add(_pb->CreateParsedVariableProperty(TL(osc_default_path), this, &_default));
	return ps;
}

ref<LiveControl> OSCOutputCueTrack::CreateControl(ref<Stream> str) {
	return null;
}

void OSCOutputCueTrack::Save(TiXmlElement* parent) {
	SaveAttributeSmall(parent, "output", _out);
	SaveAttributeSmall(parent, "path-prefix", _prefix);
	SaveAttributeSmall(parent, "path-default", _default);
	CueTrack<OSCOutputCue>::Save(parent);
}

void OSCOutputCueTrack::Load(TiXmlElement* you) {
	_out = LoadAttributeSmall(you, "output", _out);
	_prefix = LoadAttributeSmall(you, "path-prefix", _prefix);
	_default = LoadAttributeSmall(you, "path-default", _default);
	CueTrack<OSCOutputCue>::Load(you);
}

ref<OSCDevice> OSCOutputCueTrack::GetOutputDevice() {
	if(_pb) {
		ref<Device> dev = _pb->GetDeviceByPatch(_out);
		if(dev.IsCastableTo<OSCDevice>()) {
			return dev;
		}
	}
	return null;
}

std::wstring OSCOutputCueTrack::GetEmptyHintText() const {
	return TL(osc_output_track_empty_text);
}

/** OSCOutputCuePlayer **/
OSCOutputCuePlayer::OSCOutputCuePlayer(strong<OSCOutputCueTrack> tr, ref<Stream> str): _track(tr), _output(false) {
	_stream = str;
	_out = tr->GetOutputDevice();
	_last = Time(-1);
}

OSCOutputCuePlayer::~OSCOutputCuePlayer() {
}

ref<Track> OSCOutputCuePlayer::GetTrack() {
	return ref<Track>(_track);
}

void OSCOutputCuePlayer::Stop() {
	_out = 0;
	_last = Time(-1);
}

void OSCOutputCuePlayer::Start(Time pos, ref<Playback> pb, float speed) {
	_last = pos;
}

Time OSCOutputCuePlayer::GetNextEvent(Time t) {
	ref<OSCOutputCue> cue = _track->GetCueAfter(t);
	if(cue) {
		return cue->GetTime()+Time(1);
	}

	return -1;
}

void OSCOutputCuePlayer::Tick(Time currentPosition) {
	if(_out && _output) {
		try {
			std::wstring prefix = _track->_pb->ParseVariables(_track->_prefix);
			std::wstring defaultAction = _track->_pb->ParseVariables(_track->_default);

			std::vector< ref<OSCOutputCue> > cues;
			_track->GetCuesBetween(_last, currentPosition, cues);
			std::vector< ref<OSCOutputCue> >::iterator it = cues.begin();

			while(it!=cues.end()) {
				ref<OSCOutputCue> cur = *it;
				cur->Fire(_out, _track->_pb, prefix, defaultAction);
				++it;
			}
		}
		catch(const Exception& e) {
			Log::Write(L"TJOSC/CuePlayer/Tick", std::wstring(L"Exception occurred: ")+e.GetMsg());
		}
		catch(const std::exception& e) {
			Log::Write(L"TJOSC/CuePlayer/Tick", std::wstring(L"Exception occurred: ")+Wcs(e.what()));
		}
	}
	_last = currentPosition;
}

void OSCOutputCuePlayer::Jump(Time newT, bool paused) {
	_last = newT;
}

void OSCOutputCuePlayer::SetOutput(bool enable) {
	_output = enable;
}