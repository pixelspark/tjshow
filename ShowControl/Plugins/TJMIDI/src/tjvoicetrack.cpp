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
#include "../include/tjmidi.h"
#include <TJShow/include/extra/tjextra.h>
#include <limits>
using namespace tj::show;
using namespace tj::shared;

class VoiceCue: public Serializable, public virtual Inspectable {
	public:
		VoiceCue(Time pos = 0): _time(pos), _program(0), _programChange(false), _noteOn(false), _noteOff(false), _velocity(127), _note(60) {
		}

		virtual ~VoiceCue() {
		}

		virtual void Load(TiXmlElement* you) {
			_time = LoadAttributeSmall<Time>(you, "time", _time);
			_program = LoadAttributeSmall<MIDI::ProgramID>(you, "program", _program);
			_programChange = LoadAttributeSmall<bool>(you, "pc", _programChange);
			_noteOn = LoadAttributeSmall<bool>(you, "note-on", _noteOn);
			_noteOff = LoadAttributeSmall<bool>(you, "note-off", _noteOff);
			_note = LoadAttributeSmall<MIDI::NoteID>(you, "note", _note);
			_velocity = LoadAttributeSmall<MIDI::Velocity>(you, "velocity", _velocity);
		}

		virtual void Save(TiXmlElement* me) {
			TiXmlElement cue("cue");
			SaveAttributeSmall(&cue, "time", _time);
			SaveAttributeSmall(&cue, "pc", _programChange);
			SaveAttributeSmall(&cue, "program", _program);
			SaveAttributeSmall(&cue, "note-on", _noteOn);
			SaveAttributeSmall(&cue, "note-off", _noteOff);
			SaveAttributeSmall(&cue, "note", _note);
			SaveAttributeSmall(&cue, "velocity", _velocity);
			me->InsertEndChild(cue);
		}

		MIDI::ProgramID GetProgram() const {
			return _program;
		}

		virtual void Move(Time t, int h) {
			_time = t;
		}

		virtual ref<VoiceCue> Clone() {
			ref<VoiceCue> vc = GC::Hold(new VoiceCue(_time));
			vc->_program = _program;
			vc->_programChange = _programChange;
			vc->_noteOn = _noteOn;
			vc->_noteOff = _noteOff;
			vc->_velocity = _velocity;
			vc->_note = _note;
			return vc;
		}

		virtual void Fire(ref<MIDIOutputDevice> out, MIDI::Channel channel) {
			if(_programChange) out->SendProgramChange(channel, _program);
			if(_noteOn) out->SendNoteOn(channel, _note, _velocity);
			if(_noteOff) out->SendNoteOff(channel, _note);
		}

		virtual ref<PropertySet> GetProperties(ref<Playback> pb, strong< CueTrack<VoiceCue> > track) {
			bool nothing = !(_noteOff || _noteOn || _programChange);

			ref<PropertySet> ps = GC::Hold(new PropertySet());
			ps->Add(GC::Hold(new PropertySeparator(TL(midi_voice_program_change), !(_programChange||nothing))));
			ps->Add(GC::Hold(new GenericProperty<bool>(TL(midi_voice_program_change), this, &_programChange, _programChange)));
			
			ref<GenericListProperty<MIDI::ControlID> > pc = GC::Hold(new GenericListProperty<MIDI::ProgramID>(TL(midi_voice_program), this, &_program,  _program));

			// TODO: use some pref here (per track or plugin-wide)
			bool showGMPrograms = true;
			for(int a=0;a<128;a++) {
				if(showGMPrograms) {
					pc->AddOption(Stringify(a)+L": "+MIDI::Programs[a], a);
				}
				else {
					pc->AddOption(Stringify(a)+L": "+MIDI::Programs[a], a);
				}
			}
			ps->Add(pc);

			ps->Add(GC::Hold(new PropertySeparator(TL(midi_voice_note), !(_noteOn || _noteOff || nothing))));
			ps->Add(GC::Hold(new GenericProperty<bool>(TL(midi_voice_note_on), this, &_noteOn, _noteOn)));
			ps->Add(GC::Hold(new GenericProperty<bool>(TL(midi_voice_note_off), this, &_noteOff, _noteOff)));
			ps->Add(GC::Hold(new GenericProperty<MIDI::NoteID>(TL(midi_voice_note_id), this, &_note, _note)));
			ps->Add(GC::Hold(new GenericProperty<MIDI::Velocity>(TL(midi_voice_note_velocity), this, &_velocity, _velocity)));

			return ps;
		}

		bool IsProgramChange() const {
			return _programChange;
		}

		bool IsNoteOn() const {
			return _noteOn;
		}

		bool IsNoteOff() const {
			return _noteOff;
		}

		MIDI::NoteID GetNote() const {
			return _note;
		}

		MIDI::Velocity GetVelocity() const {
			return _velocity;
		}

		Time GetTime() const {
			return _time;
		}

		void SetTime(Time t) {
			_time = t;
		}

		virtual void Paint(tj::shared::graphics::Graphics* g, ref<Theme> theme, float pixelLeft, unsigned int y, unsigned int h, ref< CueTrack<VoiceCue> > track, bool focus);

	protected:
		Time _time;
		MIDI::ProgramID _program;
		bool _programChange;

		// Note on/off
		bool _noteOn;
		bool _noteOff;
		MIDI::Velocity _velocity;
		MIDI::NoteID _note;
};

class VoiceTrack: public CueTrack<VoiceCue> {
	public:
		VoiceTrack(ref<Playback> pb): _channel(0), _pb(pb), _offOnStop(true), _showNoteNames(false) {
		}

		virtual ~VoiceTrack() {
		}

		virtual std::wstring GetTypeName() const {
			return L"MIDI";
		}

		virtual Flags<RunMode> GetSupportedRunModes() {
			Flags<RunMode> rm;
			rm.Set(RunModeMaster, true);
			rm.Set(RunModeDont, true);
			rm.Set(RunModeBoth, true);
			rm.Set(RunModeClient, true);
			return rm;
		}

		virtual ref<Player> CreatePlayer(ref<Stream> str);

		virtual void Save(TiXmlElement* you) {
			SaveAttributeSmall(you, "device", _out);
			SaveAttributeSmall(you, "off-stop", _offOnStop);
			SaveAttributeSmall(you, "note-names", _showNoteNames);
			SaveAttributeSmall(you, "midi-channel", _channel);
			CueTrack<VoiceCue>::Save(you);
		}

		virtual void Load(TiXmlElement* me) {
			_out = LoadAttributeSmall(me, "device", _out);
			_offOnStop = LoadAttributeSmall(me, "off-stop", _offOnStop);
			_showNoteNames = LoadAttributeSmall(me, "note-names", _showNoteNames);
			_channel = LoadAttributeSmall(me, "midi-channel", _channel);
			CueTrack<VoiceCue>::Load(me);
		}

		bool GetOffOnStop() const {
			return _offOnStop;
		}

		bool GetShowNoteNames() const {
			return _showNoteNames;
		}

		virtual ref<PropertySet> GetProperties() {
			ref<PropertySet> ps = GC::Hold(new PropertySet());
			ps->Add(_pb->CreateSelectPatchProperty(TL(midi_voice_output_device), this, &_out));
			ps->Add(GC::Hold(new GenericProperty<MIDI::Channel>(TL(midi_voice_channel), this, &_channel, _channel)));
			ps->Add(GC::Hold(new GenericProperty<bool>(TL(midi_voice_notes_off_on_stop), this, &_offOnStop, _offOnStop)));

			ps->Add(GC::Hold(new PropertySeparator(TL(midi_voice_view_preferences))));
			ps->Add(GC::Hold(new GenericProperty<bool>(TL(midi_voice_show_note_names), this, &_showNoteNames, _showNoteNames)));
			return ps;
		}

		virtual ref<MIDIOutputDevice> GetOutputDevice() {
			return _pb->GetDeviceByPatch(_out);
		}

		virtual const PatchIdentifier& GetOutputDevicePatch() const {
			return _out;
		}

		MIDI::Channel GetMIDIChannel() const {
			return _channel;
		}

		virtual std::wstring GetEmptyHintText() const {
			return TL(midi_voice_empty_hint);
		}

		void UpdateControls() {
			ThreadLock lock(&_controlsLock);

			std::vector< weak<LiveControl> >::iterator it = _controls.begin();
			while(it!=_controls.end()) {
				ref<LiveControl> wlc = *it;
				if(!wlc) {
					it = _controls.erase(it);
				}
				else {
					wlc->Update();
					++it;
				}
			}
		}

		virtual ref<LiveControl> CreateControl(ref<Stream> stream);
		
	protected:
		PatchIdentifier _out;
		MIDI::Channel _channel;
		ref<Playback> _pb;
		bool _offOnStop;
		bool _showNoteNames;

		CriticalSection _controlsLock;
		std::vector< weak<LiveControl> > _controls;
};

class VoicePianoWnd: public ChildWnd {
	public:
		VoicePianoWnd(ref<VoiceTrack> track, ref<Stream> stream): ChildWnd(true), _track(track), _stream(stream) {
			SetVerticallyScrollable(true);
			_overNote = -1;
			_downNote = -1;
			_downVelocity = 0.0f;
		}

		virtual ~VoicePianoWnd() {
		}

		virtual void OnSize(const Area& ns) {
			SetVerticalScrollInfo(Range<int>(0,127*KNoteHeight - ns.GetHeight()), ns.GetHeight()/KNoteHeight);
		}

		virtual void OnMouse(MouseEvent ev, Pixels x, Pixels y) {
			if(ev==MouseEventMove) {
				SetWantMouseLeave(true);
				_overNote = GetNoteAt(y);
				Repaint();
			}
			else if(ev==MouseEventLeave) {
				Repaint();
			}
			else if(ev==MouseEventLDown) {
				ref<MIDIOutputDevice> od = _track->GetOutputDevice();
				MIDI::Channel channel = _track->GetMIDIChannel();
				MIDI::NoteID note = GetNoteAt(y);

				if(_downNote!=-1) {
					// send stop note
					if(od) {
						od->SendNoteOff(channel, _downNote);
					}
				}

				Area rc = GetClientArea();
				if(x > rc.GetRight()-KFullMargin) {
					_downVelocity = 1.0f;
				}
				else {
					_downVelocity = float(x-rc.GetLeft()) / float(rc.GetRight()-KFullMargin-rc.GetLeft());
				}

				if(od) {
					od->SendNoteOn(channel, note, MIDI::Velocity(_downVelocity*127));
				}
				_downNote = note;
				Repaint();
			}
			else if(ev==MouseEventLUp) {
				ref<MIDIOutputDevice> od = _track->GetOutputDevice();
				MIDI::Channel channel = _track->GetMIDIChannel();

				if(od) {
					od->SendNoteOff(channel, _downNote);
				}
				_downNote = -1;
				Repaint();
			}
		}

		virtual int GetNoteAt(Pixels y) {
			y += GetVerticalPos();
			return y/KNoteHeight;
		}

		virtual void Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme) {
			ref<MIDIOutputDevice> od = _track->GetOutputDevice();

			Area rc = GetClientArea();
			SolidBrush back(theme->GetColor(Theme::ColorBackground));
			SolidBrush key(theme->GetColor(Theme::ColorActiveStart));
			SolidBrush disabled(theme->GetColor(Theme::ColorDisabledOverlay));
			SolidBrush tbr(theme->GetColor(Theme::ColorText));
			SolidBrush line(theme->GetColor(Theme::ColorLine));
			Pen linePen(&line, 1.0f);

			LinearGradientBrush down(PointF(0.0f, (float)rc.GetTop()-GetVerticalPos()), PointF(0.0f, (float)rc.GetTop()-GetVerticalPos()+KNoteHeight), theme->GetColor(Theme::ColorHighlightStart), theme->GetColor(Theme::ColorHighlightEnd));
			LinearGradientBrush tracked(PointF(0.0f, (float)rc.GetTop()-GetVerticalPos()), PointF(0.0f, (float)rc.GetTop()-GetVerticalPos()+KNoteHeight), theme->GetSliderColorStart(Theme::SliderMaster), theme->GetSliderColorEnd(Theme::SliderMaster));
			
			StringFormat sf;
			sf.SetAlignment(StringAlignmentNear);
			g.FillRectangle(&back, rc);

			bool over = IsMouseOver();

			Pixels y = -GetVerticalPos();
			for(int a=0;a<128;a++) {
				if(y>-KNoteHeight) {
					Area note(rc.GetLeft(), rc.GetTop()+y, rc.GetWidth(), KNoteHeight);
					Area narrowed = note;
					narrowed.Narrow(1,1,1,1);

					// Get the value we sent to this note, if any
					Area velocityNarrowed = narrowed;
					if(_downVelocity > 1.0f - std::numeric_limits<float>::epsilon()) {	
					}
					else {
						velocityNarrowed.SetWidth(Pixels((narrowed.GetWidth()-KFullMargin)*_downVelocity));
					}

					// Get the value that was sent by somebody else, if any
					Area trackedNarrowed = narrowed;
					if(od) {
						float trackedVelocity = float(od->GetTrackedNoteStatus(_track->GetMIDIChannel(), a)) / float(MIDI::KMaxVelocity);
						if(trackedVelocity < (1.0f - std::numeric_limits<float>::epsilon())) {
							trackedNarrowed.SetWidth(Pixels((narrowed.GetWidth()-KFullMargin)*trackedVelocity));
						}
					}
					else {
						trackedNarrowed = Area(0,0,0,0);
					}

					if(_downNote==a) {
						g.FillRectangle(&down, velocityNarrowed);
					}
					else {
						g.FillRectangle(&key, narrowed);
					}
					
					if(!over || _overNote != a) {
						g.FillRectangle(&disabled, narrowed);
					}

					if(_downNote!=a) {
						g.FillRectangle(&tracked, trackedNarrowed);
					}

					if(_track->GetShowNoteNames()) {
						g.DrawString(MIDI::Notes[a], (int)wcslen(MIDI::Notes[a]), theme->GetGUIFontSmall(), narrowed, &sf, &tbr);
					}
					else {
						std::wstring num = Stringify(a+1);
						g.DrawString(num.c_str(), (int)num.length(), theme->GetGUIFontSmall(), narrowed, &sf, &tbr);
					}
				}
				y += KNoteHeight;
			}

			// 'full' velocity margin
			g.DrawLine(&linePen, PointF(float(rc.GetRight()-KFullMargin), (float)rc.GetTop()), PointF(float(rc.GetRight()-KFullMargin), (float)rc.GetBottom()));

		}

	protected:
		const static Pixels KNoteHeight = 13;
		const static Pixels KFullMargin = 13;
		ref<VoiceTrack> _track;
		ref<Stream> _stream;
		int _overNote;
		int _downNote;
		float _downVelocity;
};

class VoiceProgramWnd: public ChildWnd {
	public:
		VoiceProgramWnd(ref<VoiceTrack> track, ref<Stream> stream): ChildWnd(L""), _track(track), _stream(stream) {
			SetVerticallyScrollable(true);
			_overProgram = -1;
		}

		virtual ~VoiceProgramWnd() {
		}

		virtual void OnSize(const Area& ns) {
			int npr = ns.GetWidth() / KButtonWidth;
			SetVerticalScrollInfo(Range<int>(0, (MIDI::KMaxProgramID/npr)*KButtonHeight), ns.GetHeight());
			Repaint();
		}

		virtual void OnMouse(MouseEvent ev, Pixels x, Pixels y) {
			if(ev==MouseEventMove) {
				SetWantMouseLeave(true);
				//_overNote = GetNoteAt(y);
				Repaint();
			}
			else if(ev==MouseEventLeave) {
				Repaint();
			}
			else if(ev==MouseEventLDown) {
				y += GetVerticalPos();
				ref<MIDIOutputDevice> od = _track->GetOutputDevice();
				MIDI::Channel channel = _track->GetMIDIChannel();
				
				Area rc = GetClientArea();
				int nr = GetButtonsPerRow();
				int row = (y - rc.GetTop()) / KButtonHeight;
				int idx = (row * nr) + (x/KButtonWidth);
				if(od && idx >= 0 && idx <=MIDI::KMaxProgramID) {
					od->SendProgramChange(channel, idx);
				}
				Repaint();
			}
		}

		virtual int GetButtonsPerRow() const {
			Area rc = GetClientArea();
			return rc.GetWidth() / KButtonWidth;
		}

		virtual void Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme) {
			ref<MIDIOutputDevice> od = _track->GetOutputDevice();
			MIDI::ProgramID current = -1;
			if(od) {
				current = od->GetTrackedProgram(_track->GetMIDIChannel());
			}

			Area rc = GetClientArea();
			SolidBrush back(theme->GetColor(Theme::ColorBackground));
			SolidBrush btn(theme->GetColor(Theme::ColorActiveStart));
			SolidBrush disabled(theme->GetColor(Theme::ColorDisabledOverlay));
			SolidBrush tbr(theme->GetColor(Theme::ColorText));
			StringFormat sf;
			sf.SetAlignment(StringAlignmentNear);

			g.FillRectangle(&back, rc);

			Pixels y = rc.GetTop() - GetVerticalPos();
			int ri = 0;
			int rn = GetButtonsPerRow();

			for(int a=0;a<=MIDI::KMaxProgramID;a++) {
				if((ri+1)>rn) {
					y += KButtonHeight;
					ri = 0;
				}

				if(y > rc.GetTop()-KButtonHeight) {
					Pixels x = rc.GetLeft() + ri * KButtonWidth;
					Area button(x,y,KButtonWidth,KButtonHeight);
					Area narrowed = button;
					narrowed.Narrow(1,1,1,1);

					g.FillRectangle(&btn, narrowed);
					if(a != current) {
						g.FillRectangle(&disabled, narrowed);
					}

					std::wstring text = Stringify(a);
					g.DrawString(text.c_str(),(int)text.length(), theme->GetGUIFontSmall(), narrowed, &sf, &tbr);
				}
				++ri;
			}
		}

	protected:
		const static Pixels KButtonHeight = 13;
		const static Pixels KButtonWidth = 26;
		ref<VoiceTrack> _track;
		ref<Stream> _stream;
		int _overProgram;
};

class VoiceControlWnd: public TabWnd {
	public:
		VoiceControlWnd(ref<VoiceTrack> vt, ref<Stream> stream): TabWnd(0), _track(vt), _stream(stream) {
			_piano = GC::Hold(new VoicePianoWnd(vt, stream));
			_program = GC::Hold(new VoiceProgramWnd(vt, stream));
			AddPane(GC::Hold(new Pane(L"", _piano, false, false, 0, GetPlacement(), L"icons/midi/piano.png")), true);
			AddPane(GC::Hold(new Pane(L"", _program, false, false, 0, GetPlacement(), L"icons/midi/program.png")), false);
			SetDetachAttachAllowed(false);
		}

		virtual ~VoiceControlWnd() {
		}

		virtual void UpdateThreaded() {
			_piano->Repaint();
			_program->Repaint();
		}

	protected:
		ref<VoiceTrack> _track;
		ref<VoicePianoWnd> _piano;
		ref<VoiceProgramWnd> _program;
		ref<Stream> _stream;
};

class VoiceControl: public LiveControl {
	public:
		VoiceControl(ref<VoiceTrack> vt, ref<Stream> stream): _track(vt), _stream(stream) {
		}

		virtual ~VoiceControl() {
		}

		virtual ref<tj::shared::Wnd> GetWindow() {
			if(!_wnd) {
				_wnd = GC::Hold(new VoiceControlWnd(_track, _stream));
			}
			return _wnd;
		}

		virtual void Update() {
			_wnd->UpdateThreaded();
		}

		virtual int GetWidth() {
			return 72;
		}
		
		virtual std::wstring GetGroupName() {
			return TL(midi_category);
		}

	protected:
		ref<VoiceControlWnd> _wnd;
		ref<VoiceTrack> _track;
		ref<Stream> _stream;
};

ref<LiveControl> VoiceTrack::CreateControl(ref<Stream> stream) {
	ThreadLock lock(&_controlsLock);
	ref<LiveControl> control = GC::Hold(new VoiceControl(this, stream));
	_controls.push_back(control);
	return control;
}

class VoicePlayer: public Player {
	public:
		VoicePlayer(ref<VoiceTrack> tr, ref<Stream> str);
		virtual ~VoicePlayer();

		ref<Track> GetTrack();
		virtual void Stop();
		virtual void Start(Time pos, ref<Playback> playback, float speed);
		virtual void Tick(Time currentPosition);
		virtual void Jump(Time newT, bool paused);
		virtual void SetOutput(bool enable);
		virtual Time GetNextEvent(Time t);

	protected:
		ref<VoiceTrack> _track;
		ref<MIDIOutputDevice> _out;
		ref<Stream> _stream;
		volatile bool _output;
		Time _last;
};

class VoiceStreamPlayer: public StreamPlayer {
	public:
		VoiceStreamPlayer(strong<Plugin> plugin);
		virtual ~VoiceStreamPlayer();
		virtual ref<Plugin> GetPlugin();
		virtual void Message(ref<tj::shared::DataReader> msg, ref<Playback> pb);

		enum VoiceAction {
			VoiceActionNone = 0,
			VoiceActionUpdate = 1,	// Message format: [uchar VoiceAction] [PatchIdentifier patch] [uchar channel] [char noteon] [char noteoff] [char pc] [uchar velocity]	
			VoiceActionPanic = 2,	// Message format: [uchar VoiceAction] [PatchIdentifier patch] [uchar channel]
		};

	protected:
		strong<Plugin> _plugin;
};

VoiceStreamPlayer::VoiceStreamPlayer(strong<Plugin> pg): _plugin(pg) {
}

VoiceStreamPlayer::~VoiceStreamPlayer() {
}

ref<Plugin> VoiceStreamPlayer::GetPlugin() {
	return _plugin;
}

void VoiceStreamPlayer::Message(ref<DataReader> msg, ref<Playback> pb) {
	unsigned int pos = 0;
	VoiceAction action = (VoiceAction)msg->Get<unsigned char>(pos);

	if(action==VoiceActionUpdate) {
		// Extract data from message
		PatchIdentifier pi = msg->Get<PatchIdentifier>(pos);
		MIDI::Channel channel = (MIDI::Channel)msg->Get<unsigned char>(pos);
		char noteOn = msg->Get<char>(pos);
		char noteOff = msg->Get<char>(pos);
		char pc = msg->Get<char>(pos);
		MIDI::Velocity velocity = (MIDI::Velocity)msg->Get<unsigned char>(pos);

		// Execute commands
		ref<Device> dev = pb->GetDeviceByPatch(pi);
		if(dev) {
			if(dev.IsCastableTo<MIDIOutputDevice>()) {
				ref<MIDIOutputDevice> out = dev;

				if(pc!=-1) {
					out->SendProgramChange(channel, (MIDI::ProgramID)pc);
				}
				if(noteOn!=-1) {
					out->SendNoteOn(channel, (MIDI::NoteID)noteOn, velocity);
				}
				if(noteOff!=-1) {
					out->SendNoteOff(channel, (MIDI::NoteID)noteOff);
				}
			}
			else {
				Throw(L"Wrong device type patched", ExceptionTypeError);
			}
		}
	}
	else if(action==VoiceActionPanic) {
		PatchIdentifier pi = msg->Get<PatchIdentifier>(pos);
		MIDI::Channel channel = msg->Get<unsigned char>(pos);
		ref<Device> dev = pb->GetDeviceByPatch(pi);
		if(dev) {
			if(dev.IsCastableTo<MIDIOutputDevice>()) {
				ref<MIDIOutputDevice> out = dev;
				out->SendAllNotesOff(channel);
			}
			else {
				Throw(L"Wrong device type patched", ExceptionTypeError);
			}
		}
	}
}

void VoiceCue::Paint(tj::shared::graphics::Graphics* g, ref<Theme> theme, float pixelLeft, unsigned int y, unsigned int h, ref< CueTrack<VoiceCue> > track, bool focus) {
	Pen pn(theme->GetColor(Theme::ColorCurrentPosition), focus ? 2.0f : 1.0f);
	tj::shared::graphics::SolidBrush cueBrush = theme->GetColor(Theme::ColorCurrentPosition);
	tj::shared::graphics::SolidBrush onBrush(Color(1.0, 0.0, 0.0));
	tj::shared::graphics::SolidBrush offBrush(Color(0.5, 0.5, 0.5));
	g->DrawLine(&pn, pixelLeft, (float)y, pixelLeft, float(y+(h/4)));

	StringFormat sf;
	sf.SetAlignment(StringAlignmentCenter);

	if(_programChange) {
		std::wstring str = Stringify(_program);	
		g->DrawString(str.c_str(), (int)str.length(), theme->GetGUIFontSmall(), PointF(pixelLeft+(CueTrack<VoiceCue>::KCueWidth/2), float(y+h/2)), &sf, &cueBrush);
	}

	if(_noteOn || _noteOff) {
		ref<VoiceTrack> vc = track;
		if(vc) {
			std::wstring str = (vc->GetShowNoteNames() && _note < 128) ? MIDI::Notes[_note] : Stringify(_note);	
			g->DrawString(str.c_str(), (int)str.length(), theme->GetGUIFontSmall(), PointF(pixelLeft+(CueTrack<VoiceCue>::KCueWidth/2), float(y+h/2)), &sf, _noteOn?&onBrush:&offBrush);
		}
	}
}

VoicePlayer::VoicePlayer(ref<VoiceTrack> tr, ref<Stream> str): _track(tr), _output(false) {
	assert(tr);
	_stream = str;
	_out = tr->GetOutputDevice();
	_last = Time(-1);
}

VoicePlayer::~VoicePlayer() {
}

ref<Track> VoicePlayer::GetTrack() {
	return ref<Track>(_track);
}

void VoicePlayer::Stop() {
	_last = Time(0);
	if(_track->GetOffOnStop()) {
		if(_output && _out) {
			_out->SendAllNotesOff(_track->GetMIDIChannel());
		}

		if(_stream) {
			ref<Message> msg = _stream->Create();
			msg->Add<unsigned char>(VoiceStreamPlayer::VoiceActionPanic);
			msg->Add(_track->GetOutputDevicePatch());
			msg->Add<unsigned char>(_track->GetMIDIChannel());
			_stream->Send(msg);
		}
	}
	_out = 0;
	_track->UpdateControls();
}

void VoicePlayer::Start(Time pos, ref<Playback> pb, float speed) {
	_last = pos;
}

Time VoicePlayer::GetNextEvent(Time t) {
	ref<VoiceCue> cue = _track->GetCueAfter(t);
	if(cue) {
		Time tr = cue->GetTime()+Time(1);
		Log::Write(L"TJMIDI/VoicePlayer", L"GetNextEvent() "+Stringify(t.ToInt())+L" -> "+Stringify(tr.ToInt()));
		return tr;
	}

	return -1;
}

void VoicePlayer::Tick(Time currentPosition) {
	Log::Write(L"TJMIDI/VoicePlayer", L"Tick() "+Stringify(_last.ToInt())+L" - "+Stringify(currentPosition.ToInt()));
	std::vector< ref<VoiceCue> > cues;
	_track->GetCuesBetween(_last, currentPosition,cues);
	std::vector< ref<VoiceCue> >::iterator it = cues.begin();

	while(it!=cues.end()) {
		ref<VoiceCue> cur = *it;
		if(_out && _output) {
			cur->Fire(_out, _track->GetMIDIChannel());
		}

		if(_stream) {
			ref<Message> msg = _stream->Create();
			msg->Add<unsigned char>(VoiceStreamPlayer::VoiceActionUpdate);
			msg->Add(_track->GetOutputDevicePatch());
			msg->Add<unsigned char>(_track->GetMIDIChannel());
			msg->Add<char>(cur->IsNoteOn() ? cur->GetNote() : -1);
			msg->Add<char>(cur->IsNoteOff() ? cur->GetNote() : -1);
			msg->Add<char>(cur->IsProgramChange() ? cur->GetProgram() : -1);
			msg->Add<unsigned char>(cur->GetVelocity());
			_stream->Send(msg);
		}
		++it;
	}

	_last = currentPosition;
	_track->UpdateControls();
}

void VoicePlayer::Jump(Time newT, bool paused) {
	 ref<VoiceCue> c = _track->GetCueBefore(newT);
	_last = newT;
}

void VoicePlayer::SetOutput(bool enable) {
	_output = enable;
}

VoicePlugin::VoicePlugin() {
}

VoicePlugin::~VoicePlugin() {
}

void VoicePlugin::GetRequiredFeatures(std::list<std::wstring>& fts) const {
	fts.push_back(L"MIDI");
}

std::wstring VoicePlugin::GetName() const {
	return L"MIDI";
}

std::wstring VoicePlugin::GetFriendlyName() const {
	return TL(midi_voice_plugin_friendly_name);
}

std::wstring VoicePlugin::GetFriendlyCategory() const {
	return TL(midi_category);
}

std::wstring VoicePlugin::GetVersion() const {
	std::wostringstream os;
	os << __DATE__ << " @ " << __TIME__;
	#ifdef UNICODE
	os << L" Unicode";
	#endif

	#ifdef NDEBUG
	os << " Release";
	#endif

	return os.str();
}

std::wstring VoicePlugin::GetAuthor() const {
	return std::wstring(L"Tommy van der Vorst");
}

std::wstring VoicePlugin::GetDescription() const {
	return TL(midi_voice_plugin_description);
}

ref<Track> VoicePlugin::CreateTrack(ref<Playback> playback) {
	return GC::Hold(new VoiceTrack(playback));
}

ref<StreamPlayer> VoicePlugin::CreateStreamPlayer(ref<Playback> playback, ref<Talkback> talk) {
	return GC::Hold(new VoiceStreamPlayer(ref<Plugin>(this)));
}

ref<Player> VoiceTrack::CreatePlayer(ref<Stream> str) {
	return GC::Hold(new VoicePlayer(this, str));
}