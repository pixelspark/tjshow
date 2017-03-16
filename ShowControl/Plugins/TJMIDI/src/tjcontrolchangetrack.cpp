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

class ControlChangeTrack: public Track {
	friend class ControlChangePlayer;

	public:
		ControlChangeTrack(ref<Playback> pb);
		virtual ~ControlChangeTrack();
		virtual Pixels GetHeight();
		virtual void Save(TiXmlElement* parent);
		virtual void Load(TiXmlElement* you);
		virtual Flags<RunMode> GetSupportedRunModes();
		virtual std::wstring GetTypeName() const;
		virtual ref<Player> CreatePlayer(ref<Stream> str);
		virtual ref<PropertySet> GetProperties();
		virtual void Paint(tj::shared::graphics::Graphics* g, Time position, float pixelsPerMs, unsigned int x, unsigned int y, unsigned int h, Time start, Time end, ref<Item> item);
		virtual ref<Item> GetItemAt(Time position,unsigned int h, bool rightClick, int trackHeight, float pixelsPerMs);
		virtual ref<LiveControl> CreateControl(ref<Stream> stream);
		virtual void SetManual(MIDI::ControlValue val);
		virtual ref<MIDIOutputDevice> GetOutputDevice();
		virtual MIDI::Channel GetMIDIChannel() const;
		virtual MIDI::ControlID GetMIDIControlID() const;

	protected:
		void UpdateControls();
		CriticalSection _controlsLock;
		std::vector< weak<LiveControl> > _controls;
		ref< Fader<float> > _value;
		Pixels _height;
		PatchIdentifier _out;
		MIDI::Channel _channel;
		MIDI::ControlID _cc;
		ref<Playback> _pb;
};

class ControlChangePlayer: public Player {
	public:
		ControlChangePlayer(ref<ControlChangeTrack> track, ref<Stream> stream);
		virtual ~ControlChangePlayer();
		virtual ref<Track> GetTrack();
		virtual void Stop();
		virtual void Start(Time pos,ref<Playback> pb, float speed);
		virtual void Tick(Time currentPosition);
		virtual void Jump(Time newT, bool paused);
		virtual void SetOutput(bool enable);
		virtual Time GetNextEvent(Time t);

	protected:
		ref<MIDIOutputDevice> _out;
		ref<ControlChangeTrack> _track;
		FaderPlayer<float> _fader;
		bool _output;
		ref<Stream> _stream;
};

class ControlChangeControl: public LiveControl, public Listener<SliderWnd::NotificationChanged>, public Endpoint {
	public:
		ControlChangeControl(ref<ControlChangeTrack> tr, ref<Stream> ms);
		virtual ref<tj::shared::Wnd> GetWindow();
		virtual void Notify(ref<Object> source, const SliderWnd::NotificationChanged& evt);
		virtual void Update();
		virtual std::wstring GetGroupName();
		virtual int GetWidth();
		virtual ref<tj::shared::Endpoint> GetEndpoint(const std::wstring& name);

		virtual void Set(const Any& f);

	protected:
		virtual void OnCreated();

		ref<ControlChangeTrack> _track;
		ref<SliderWnd> _slider;
		ref<Stream> _stream;
};

class ControlChangeStreamPlayer: public StreamPlayer {
	public:
		ControlChangeStreamPlayer(strong<Plugin> plugin);
		virtual ~ControlChangeStreamPlayer();
		virtual ref<Plugin> GetPlugin();
		virtual void Message(ref<tj::shared::DataReader> msg, ref<Playback> pb);

		enum ControlAction {
			ControlActionNone = 0,
			ControlActionUpdate = 1,	// Message format: [uchar ControlAction] [PatchIdentifier patch] [uchar channel] [uchar control] [uchar value]
		};

	protected:
		strong<Plugin> _plugin;
};

/* ControlChangeStreamPlayer */
ControlChangeStreamPlayer::ControlChangeStreamPlayer(strong<Plugin> plugin): _plugin(plugin) {
}

ControlChangeStreamPlayer::~ControlChangeStreamPlayer() {
}

void ControlChangeStreamPlayer::Message(ref<DataReader> msg, ref<Playback> pb) {
	unsigned int pos = 0;
	ControlAction action = (ControlAction)msg->Get<unsigned char>(pos);

	if(action==ControlActionUpdate) {
		PatchIdentifier patch = msg->Get<PatchIdentifier>(pos);
		MIDI::Channel channel = (MIDI::Channel)msg->Get<unsigned char>(pos);
		MIDI::ControlID control = (MIDI::ControlID)msg->Get<unsigned char>(pos);
		MIDI::ControlValue value = (MIDI::ControlValue)msg->Get<unsigned char>(pos);

		ref<Device> dev = pb->GetDeviceByPatch(patch);
		if(dev) {
			if(dev.IsCastableTo<MIDIOutputDevice>()) {
				ref<MIDIOutputDevice> out = dev;
				out->SendControlChange(channel, control, value);
			}
			else {
				Throw(L"Invalid device patched", ExceptionTypeError);
			}
		}
	}
}

ref<Plugin> ControlChangeStreamPlayer::GetPlugin() {
	return _plugin;
}

/* ControlChangeControl */
ControlChangeControl::ControlChangeControl(ref<ControlChangeTrack> tr, ref<Stream> str): _track(tr), _stream(str) {
	_slider = GC::Hold(new SliderWnd(L""));
	_slider->SetColor(Theme::SliderNormal); // TODO: special color for direct sliders
}

ref<tj::shared::Wnd> ControlChangeControl::GetWindow() {
	return _slider;
}

void ControlChangeControl::OnCreated() {
	_slider->EventChanged.AddListener(ref< Listener<SliderWnd::NotificationChanged> >(this));
	LiveControl::OnCreated();
}

void ControlChangeControl::Set(const Any& f) {
	float val = Clamp((float)f, 0.0f, 1.0f);
	_slider->SetValue(val, false);
	_track->SetManual((MIDI::ControlValue)(val*127.0f));
	_slider->Repaint();
}

void ControlChangeControl::Notify(ref<Object> source, const SliderWnd::NotificationChanged& evt) {
	_track->SetManual((MIDI::ControlValue)(_slider->GetValue()*127.0f));
}

void ControlChangeControl::Update() {
	ref<MIDIOutputDevice> od = _track->GetOutputDevice();
	float val = 0.0f;
	if(od) {
		val = float(od->GetTrackedControlStatus(_track->GetMIDIChannel(), _track->GetMIDIControlID())) / float(MIDI::KMaxControlValue);
	}
	
	_slider->SetDisplayValue(val, false);
	_slider->Repaint();
}

std::wstring ControlChangeControl::GetGroupName() {
	return TL(midi_cc_control_group_name);
}

int ControlChangeControl::GetWidth() {
	return 26;
}

ref<tj::shared::Endpoint> ControlChangeControl::GetEndpoint(const std::wstring& name) {
	return this;
}

/** ControlChangePlayer */
ControlChangePlayer::ControlChangePlayer(ref<ControlChangeTrack> track, ref<Stream> stream): _track(track), _stream(stream), _output(false), _fader(_track->_value) {
}

ControlChangePlayer::~ControlChangePlayer() {
}

ref<Track> ControlChangePlayer::GetTrack() {
	return _track;
}

void ControlChangePlayer::Stop() {
	_out = 0;
}

void ControlChangePlayer::Start(Time pos,ref<Playback> pb, float speed) {
	ref<Device> out = _track->_pb->GetDeviceByPatch(_track->_out);
	if(out) {
		if(out.IsCastableTo<MIDIOutputDevice>()) {
			_out = out;
		}
		else {
			// Wrong device!
			Throw(TL(midi_cc_wrong_device), ExceptionTypeError);
		}
	}
}

void ControlChangePlayer::Tick(Time currentPosition) {
	if(_fader.UpdateNecessary(currentPosition)) {
		_fader.Tick(currentPosition, 0);

		// Update value here
		float value = _track->_value->GetValueAt(currentPosition);
		MIDI::ControlValue mvalue = (MIDI::ControlValue)(value * 127);

		if(_output && _out) {
			_out->SendControlChange(_track->_channel, _track->_cc, mvalue);
		}

		if(_stream) {
			ref<Message> msg = _stream->Create();
			msg->Add<unsigned char>(ControlChangeStreamPlayer::ControlActionUpdate);
			msg->Add(_track->_out);
			msg->Add<unsigned char>(_track->_channel);
			msg->Add<unsigned char>(_track->_cc);
			msg->Add<unsigned char>(mvalue);
			_stream->Send(msg);
		}

		_track->UpdateControls();
	}
}

void ControlChangePlayer::Jump(Time newT, bool paused) {
	Tick(newT);
}

void ControlChangePlayer::SetOutput(bool enable) {
	_output = enable;
}

Time ControlChangePlayer::GetNextEvent(Time t) {
	return _track->_value->GetNextEvent(t);
}

/** ControlChangeTrack **/
ControlChangeTrack::ControlChangeTrack(ref<Playback> pb): _pb(pb), _height(MultifaderTrack::KDefaultFaderHeight), _channel(0), _cc(0) {
	_value = GC::Hold(new Fader<float>(0.0f, 0.0f, 1.0f));
}

ControlChangeTrack::~ControlChangeTrack() {
}

MIDI::Channel ControlChangeTrack::GetMIDIChannel() const {
	return _channel;
}

MIDI::ControlID ControlChangeTrack::GetMIDIControlID() const {
	return _cc;
}

Pixels ControlChangeTrack::GetHeight() {
	return max(_height, MultifaderTrack::KDefaultFaderHeight);
}

void ControlChangeTrack::SetManual(MIDI::ControlValue val) {
	ref<Device> dev = _pb->GetDeviceByPatch(_out);
	if(dev && dev.IsCastableTo<MIDIOutputDevice>()) {
		ref<MIDIOutputDevice> mdo = dev;
		mdo->SendControlChange(_channel, _cc, val);
	}
}

ref<MIDIOutputDevice> ControlChangeTrack::GetOutputDevice() {
	ref<Device> dev = _pb->GetDeviceByPatch(_out);
	if(dev && dev.IsCastableTo<MIDIOutputDevice>()) {
		return dev;
	}
	return 0;
}

void ControlChangeTrack::Save(TiXmlElement* parent) {
	SaveAttributeSmall(parent, "midi-channel", _channel);
	SaveAttributeSmall(parent, "cc", _cc);
	SaveAttributeSmall(parent, "device", _out);
	SaveAttributeSmall(parent, "height", _height);

	TiXmlElement fader("fader");
	_value->Save(&fader);
	parent->InsertEndChild(fader);
}

void ControlChangeTrack::Load(TiXmlElement* parent) {
	_channel = LoadAttributeSmall(parent, "midi-channel", _channel);
	_cc = LoadAttributeSmall(parent, "cc", _cc);
	_out = LoadAttributeSmall(parent, "device", _out);
	_height = LoadAttributeSmall(parent, "height", _height);

	TiXmlElement* fader = parent->FirstChildElement("fader");
	if(fader!=0) {
		TiXmlElement* ff = fader->FirstChildElement("fader");
		if(ff!=0) {
			_value->Load(ff);
		}
	}
}

Flags<RunMode> ControlChangeTrack::GetSupportedRunModes() {
	Flags<RunMode> rm;
	rm.Set(RunModeMaster, true);
	rm.Set(RunModeClient, true);
	rm.Set(RunModeBoth, true);
	return rm;
}

std::wstring ControlChangeTrack::GetTypeName() const {
	return L"MIDI CC";
}

ref<Player> ControlChangeTrack::CreatePlayer(ref<Stream> str) {
	return GC::Hold(new ControlChangePlayer(this, str));
}

ref<PropertySet> ControlChangeTrack::GetProperties() {
	ref<PropertySet> ps = GC::Hold(new PropertySet());
	
	ps->Add(_pb->CreateSelectPatchProperty(TL(midi_cc_device), this, &_out));
	ps->Add(GC::Hold(new GenericProperty<MIDI::Channel>(TL(midi_cc_channel), this, &_channel, _channel)));
	
	ref<GenericListProperty<MIDI::ControlID> > cc = GC::Hold(new GenericListProperty<MIDI::ControlID>(TL(midi_cc_control), this, &_cc, _cc));

	for(int a=0;a<128;a++) {
		cc->AddOption(Stringify(a)+L": "+MIDI::Controls[a], a);
	}
	ps->Add(cc);
	ps->Add(GC::Hold(new GenericProperty<Pixels>(TL(midi_cc_track_height), this, &_height, _height)));
	ps->Add(GC::Hold(new FaderProperty<float>(_value, TL(fader_data), TL(fader_data_change), L"icons/fader.png")));
	return ps;
}

void ControlChangeTrack::Paint(tj::shared::graphics::Graphics* g, Time position, float pixelsPerMs, unsigned int x, unsigned int y, unsigned int h, Time start, Time end, ref<Item> focus) {
	strong<Theme> theme = ThemeManager::GetTheme();

	FaderPainter<float> painterd(_value);
	Pen pnd(theme->GetColor(Theme::ColorFader),1.5f);
	painterd.Paint(g, start, end-start, h, int(float(end-start)*pixelsPerMs), x, y, &pnd,0, true, focus);	
}

ref<Item> ControlChangeTrack::GetItemAt(Time position,unsigned int h, bool rightClick, int trackHeight, float pixelsPerMs) {
	if(rightClick) {
		float valueAtTime = _value->GetValueAt(position);
		_value->AddPoint(position, valueAtTime);
		return 0;
	}
	else {
		// If we are allowed to 'stick', create a new point if we're too far from the nearest point
		Time s = _value->GetNearest(position);
		if(MultifaderTrack::IsStickingAllowed() && ((abs(s.ToInt()-position.ToInt())*pixelsPerMs)>MultifaderTrack::KFaderStickLimit)) {
			return GC::Hold(new FaderItem<float>(_value, position,trackHeight ));
		}
		else {
			return GC::Hold(new FaderItem<float>(_value, s,trackHeight ));
		}
	}
}

ref<LiveControl> ControlChangeTrack::CreateControl(ref<Stream> stream) {
	ThreadLock lock(&_controlsLock);
	ref<LiveControl> lc = GC::Hold(new ControlChangeControl(this,stream));
	_controls.push_back(lc);
	return lc;
}

void ControlChangeTrack::UpdateControls() {
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

/** ControlChangePlugin **/
ControlChangePlugin::ControlChangePlugin() {
}

ControlChangePlugin::~ControlChangePlugin() {
}

std::wstring ControlChangePlugin::GetName() const {
	return L"MIDI CC";
}

void ControlChangePlugin::GetRequiredFeatures(std::list<std::wstring>& fts) const {
	fts.push_back(L"MIDI");
}

std::wstring ControlChangePlugin::GetFriendlyName() const {
	return TL(midi_cc_plugin_friendly_name);
}

std::wstring ControlChangePlugin::GetFriendlyCategory() const {
	return TL(midi_category);
}

std::wstring ControlChangePlugin::GetVersion() const {
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

std::wstring ControlChangePlugin::GetAuthor() const {
	return L"Tommy van der Vorst";
}

std::wstring ControlChangePlugin::GetDescription() const {
	return TL(midi_cc_plugin_description);
}

ref<Track> ControlChangePlugin::CreateTrack(ref<Playback> playback) {
	return GC::Hold(new ControlChangeTrack(playback));
}

ref<StreamPlayer> ControlChangePlugin::CreateStreamPlayer(ref<Playback> playback, ref<Talkback> talk) {
	return GC::Hold(new ControlChangeStreamPlayer(ref<Plugin>(this)));
}