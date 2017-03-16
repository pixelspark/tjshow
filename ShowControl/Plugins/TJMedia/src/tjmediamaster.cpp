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
#include "../include/tjmedia.h"
using namespace tj::shared;
using namespace tj::show;
using namespace tj::media;
using namespace tj::media::master;

namespace tj {
	namespace media {
		namespace master {
			class MediaMasterPlayer: public Player {
				public:
					MediaMasterPlayer(ref<MediaMasterTrack> track, ref<Stream> stream);
					virtual ~MediaMasterPlayer();
					ref<Track> GetTrack();
					virtual void Stop();
					virtual void Start(Time pos,ref<Playback> pb, float speed);
					virtual void Tick(Time currentPosition);
					virtual void Jump(Time newT, bool paused);
					virtual bool Paint(tj::shared::graphics::Graphics* g, unsigned int w, unsigned int h);
					virtual void SetOutput(bool enable);
					virtual Time GetNextEvent(Time t);

				protected:
					ref<MediaMasterTrack> _track;
					FaderPlayer<float> _fader;
					bool _output;
					ref<Stream> _stream;
			};

			class MediaMasterTrackRange: public TrackRange {
				public:
					MediaMasterTrackRange(ref<MediaMasterTrack> track, Time start, Time end);
					virtual ~MediaMasterTrackRange();
					virtual void RemoveItems();
					virtual void Paste(ref<Track> other, Time start);
					virtual void InterpolateLeft(Time left, float ratio, Time right);
					virtual void InterpolateRight(Time left, float ratio, Time right);

				private:
					ref<MediaMasterTrack> _track;
					Time _start, _end;
			};

			class MediaMasterControl: public LiveControl, public Listener<SliderWnd::NotificationChanged> {
				friend class MediaMasterControlWnd;

				public:
					MediaMasterControl(ref<MediaMasterTrack> tr);
					virtual void OnCreated();
					virtual ref<tj::shared::Wnd> GetWindow();
					virtual void Notify(ref<Object> source, const SliderWnd::NotificationChanged& evt);
					virtual void Update();
					virtual std::wstring GetGroupName();
					virtual int GetWidth();
					virtual ref<tj::shared::Endpoint> GetEndpoint(const std::wstring& name);

				protected:
					static std::map< master::MediaMaster::Flavour, ref<Icon> > _icons;
					static ref<Icon> GetIconForFlavour(master::MediaMaster::Flavour fv);
					ref<MediaMasterTrack> _track;
					ref<MediaMasterControlWnd> _wnd;
					ref<SliderWnd> _slider;
			};

			
			std::map< master::MediaMaster::Flavour, ref<Icon> > MediaMasterControl::_icons;

			class MediaMasterControlWnd: public ChildWnd {
				public:
					MediaMasterControlWnd(ref<MediaMasterTrack> mtt, ref<SliderWnd> sw);
					virtual ~MediaMasterControlWnd();
					virtual void Paint(graphics::Graphics& g, strong<Theme> theme);
					virtual void Layout();
					virtual void OnSize(const Area& ns);

				protected:
					ref<MediaMasterTrack> _track;
					ref<SliderWnd> _slider;
			};

			class MediaMasterEndpoint: public Endpoint {
				public:
					MediaMasterEndpoint(ref<MediaMasterTrack> mmt);
					virtual ~MediaMasterEndpoint();
					virtual void Set(const Any& f);
					virtual EndpointType GetType() const;
					virtual std::wstring GetName() const;

				protected:
					weak<MediaMasterTrack> _mmt;
			};
		}
	}
}

using namespace tj::media::master;

/** MediaMasterTrack **/
MediaMasterTrack::MediaMasterTrack(ref<MediaMasters> mm): _mm(mm), _flavour(MediaMaster::FlavourVolume) {
	_value = GC::Hold(new Fader<float>(1.0f,0.0f,1.0f));
	_height = 19;
}

MediaMasterTrack::~MediaMasterTrack() {
}

void MediaMasterTrack::OnCreated() {
	ref<MediaMasters> mm = _mm;
	if(mm) {
		mm->AddMaster(this);
	}
	Track::OnCreated();
}

MediaMasterID MediaMasterTrack::GetMasterID() const {
	return _mid;
}

MediaMaster::Flavour MediaMasterTrack::GetFlavour() const {
	return _flavour;
}

Pixels MediaMasterTrack::GetHeight() {
	return max(19,_height);
}

ref<LiveControl> MediaMasterTrack::CreateControl(ref<Stream> stream) {
	if(!_control) {
		_control = GC::Hold(new MediaMasterControl(this));
	}
	return _control;
}

void MediaMasterTrack::Save(TiXmlElement* parent) {
	SaveAttributeSmall(parent, "height", _height);
	SaveAttributeSmall(parent, "mid", _mid);
	SaveAttributeSmall(parent, "flavour", (int)_flavour);
	TiXmlElement valueElement("volume-fader");
	_value->Save(&valueElement);
	parent->InsertEndChild(valueElement);
}

void MediaMasterTrack::Load(TiXmlElement* you) {
	_height = LoadAttributeSmall(you, "height", _height);
	_mid = LoadAttributeSmall(you, "mid", _mid);
	_flavour = (MediaMaster::Flavour)LoadAttributeSmall<int>(you, "flavour", (int)_flavour);

	TiXmlElement* valueElement = you->FirstChildElement("volume-fader");
	if(valueElement!=0) {
		TiXmlElement* fader = valueElement->FirstChildElement("fader");
		if(fader!=0) {
			_value->Load(fader);
		}
	}
}

Flags<RunMode> MediaMasterTrack::GetSupportedRunModes() {
	return Flags<RunMode>(RunModeMaster);
}

std::wstring MediaMasterTrack::GetTypeName() const { 
	return std::wstring(L"Media Master");
}

ref<Player> MediaMasterTrack::CreatePlayer(ref<Stream> str) {
	return GC::Hold(new MediaMasterPlayer(this, str));
}

void MediaMasterTrack::OnSetInstanceName(const std::wstring& name) {
	if(_mid.length()<1) {
		_mid = name;
	}
}

ref<PropertySet> MediaMasterTrack::GetProperties() {
	ref<PropertySet> ps = GC::Hold(new PropertySet());
	ps->Add(GC::Hold(new GenericProperty<int>(TL(track_height), this, &_height, _height)));
	ps->Add(GC::Hold(new GenericProperty<std::wstring>(TL(media_master_id), this, &_mid, _mid)));

	ref<GenericListProperty<MediaMaster::Flavour> > fp = GC::Hold(new GenericListProperty<MediaMaster::Flavour>(TL(media_master_flavour), this, &_flavour, _flavour));
	fp->AddOption(TL(media_master_flavour_volume), MediaMaster::FlavourVolume);
	fp->AddOption(TL(media_master_flavour_alpha), MediaMaster::FlavourAlpha);
	fp->AddOption(TL(media_master_flavour_blur), MediaMaster::Flavour(MediaMaster::FlavourEffect + Mesh::EffectBlur));
	fp->AddOption(TL(media_master_flavour_vignet), MediaMaster::Flavour(MediaMaster::FlavourEffect + Mesh::EffectVignet));
	fp->AddOption(TL(media_master_flavour_gamma), MediaMaster::Flavour(MediaMaster::FlavourEffect + Mesh::EffectGamma));
	fp->AddOption(TL(media_master_flavour_mosaic), MediaMaster::Flavour(MediaMaster::FlavourEffect + Mesh::EffectMosaic));
	fp->AddOption(TL(media_master_flavour_saturation), MediaMaster::Flavour(MediaMaster::FlavourEffect + Mesh::EffectSaturation));
	ps->Add(fp);
	
	return ps;
}

void MediaMasterTrack::Paint(tj::shared::graphics::Graphics* g, Time position, float pixelsPerMs, unsigned int x, unsigned int y, unsigned int h, Time start, Time end, ref<Item> focus) {
	strong<Theme> theme = ThemeManager::GetTheme();

	FaderPainter<float> painterd(_value);
	Pen pnd(theme->GetColor(Theme::ColorFader),1.5f);
	painterd.Paint(g, start, end-start, h, int(float(end-start)*pixelsPerMs), x, y, &pnd,_selectedFadePoint, true, focus);	
}

ref<Item> MediaMasterTrack::GetItemAt(Time position,unsigned int h, bool rightClick, int trackHeight, float pixelsPerMs) {
	if(rightClick) {
		float valueAtTime = _value->GetValueAt(position);
		_value->AddPoint(position, valueAtTime);
		return 0;
	}
	else {
		// If we are allowed to 'stick', create a new point if we're too far from the nearest point
		_selectedFadePoint = _value->GetNearest(position);
		if(MultifaderTrack::IsStickingAllowed() && ((abs(_selectedFadePoint.ToInt()-position.ToInt())*pixelsPerMs)>MultifaderTrack::KFaderStickLimit)) {
			return GC::Hold(new FaderItem<float>(_value, position,trackHeight ));
		}
		else {
			return GC::Hold(new FaderItem<float>(_value, _selectedFadePoint,trackHeight ));
		}
	}
}

ref<TrackRange> MediaMasterTrack::GetRange(Time start, Time end) {
	return GC::Hold(new MediaMasterTrackRange(this, start, end));
}

/** MediaMasterEndpoint **/
MediaMasterEndpoint::MediaMasterEndpoint(ref<MediaMasterTrack> mmt) {
	_mmt = mmt;
}

MediaMasterEndpoint::~MediaMasterEndpoint() {
}

void MediaMasterEndpoint::Set(const Any& f) {
	ref<MediaMasterTrack> mmt = _mmt;
	if(mmt) {
		ref<MediaMasters> mm = mmt->_mm;
		if(mm) {
			float val = Clamp((float)f, 0.0f, 1.0f);
			mm->SetMasterValue(mmt->_mid, val, MediaMaster::MasterSourceManual);
		}
		ref<MediaMasterControl> mmc = mmt->_control;
		if(mmc) {
			mmc->Update();
		}
	}
}

Endpoint::EndpointType MediaMasterEndpoint::GetType() const {
	return EndpointTypeThreaded;
}

std::wstring MediaMasterEndpoint::GetName() const {
	return TL(media_master_endpoint);
}

/** MediaMasterControlWnd **/
MediaMasterControlWnd::MediaMasterControlWnd(ref<MediaMasterTrack> mtt, ref<SliderWnd> sw): ChildWnd(true), _track(mtt), _slider(sw) {
	Add(sw);
}

MediaMasterControlWnd::~MediaMasterControlWnd() {
}

void MediaMasterControlWnd::Paint(graphics::Graphics& g, strong<Theme> theme) {
	Area rc = GetClientArea();
	rc.SetHeight(16+4);

	SolidBrush back(theme->GetColor(Theme::ColorBackground));
	g.FillRectangle(&back, rc);

	ref<Icon> icon = MediaMasterControl::GetIconForFlavour(_track->GetFlavour());
	if(icon) {
		Area iconRC = rc;
		iconRC.Narrow((iconRC.GetWidth()-16)/2,2,0,0);
		iconRC.SetWidth(16);
		iconRC.SetHeight(16);
		icon->Paint(g, iconRC);
	}
}

void MediaMasterControlWnd::Layout() {
	Area rc = GetClientArea();
	rc.Narrow(0,16+4, 0, 0);
	_slider->Fill(LayoutFill,rc);
}
void MediaMasterControlWnd::OnSize(const Area& ns) {
	Layout();
}

/** MediaMasterControl **/
MediaMasterControl::MediaMasterControl(ref<MediaMasterTrack> tr): _track(tr) {
	_slider = GC::Hold(new SliderWnd(L""));
	_slider->SetColor(Theme::SliderSubmix);
	_wnd = GC::Hold(new MediaMasterControlWnd(tr, _slider));
}

void MediaMasterControl::OnCreated() {
	_slider->EventChanged.AddListener(ref< Listener<SliderWnd::NotificationChanged> >(this));
	LiveControl::OnCreated();
}

ref<tj::shared::Wnd> MediaMasterControl::GetWindow() {
	return _wnd;
}

void MediaMasterControl::Notify(ref<Object> source, const SliderWnd::NotificationChanged& evt) {
	ref<MediaMasters> mm = _track->_mm;
	if(mm) {
		mm->SetMasterValue(_track->_mid, _slider->GetValue(), MediaMaster::MasterSourceManual);
		Update();
	}
}

ref<Icon> MediaMasterControl::GetIconForFlavour(master::MediaMaster::Flavour fv) {
	if(_icons.size()<1) {
		_icons[master::MediaMaster::FlavourVolume] = GC::Hold(new Icon(L"icons/media/master-volume.png"));
		_icons[master::MediaMaster::FlavourAlpha] = GC::Hold(new Icon(L"icons/media/master-alpha.png"));
		_icons[(master::MediaMaster::Flavour)(master::MediaMaster::FlavourEffect + Mesh::EffectBlur)] = GC::Hold(new Icon(L"icons/media/master-blur.png"));
		_icons[(master::MediaMaster::Flavour)(master::MediaMaster::FlavourEffect + Mesh::EffectVignet)] = GC::Hold(new Icon(L"icons/media/master-vignet.png"));
		_icons[(master::MediaMaster::Flavour)(master::MediaMaster::FlavourEffect + Mesh::EffectMosaic)] = GC::Hold(new Icon(L"icons/media/master-mosaic.png"));
		_icons[(master::MediaMaster::Flavour)(master::MediaMaster::FlavourEffect + Mesh::EffectGamma)] = GC::Hold(new Icon(L"icons/media/master-gamma.png"));
	}

	std::map<master::MediaMaster::Flavour, ref<Icon> >::iterator it = _icons.find(fv);
	if(it!=_icons.end()) {
		return it->second;
	}
	return null;
}

void MediaMasterControl::Update() {
	ref<MediaMasters> mm = _track->_mm;
	if(mm) {
		float mvalSlider = _slider->GetValue();
		float dvalSlider = _slider->GetDisplayValue();
		float mval = mm->GetMasterValue(_track->_mid, MediaMaster::MasterSourceManual);
		float dval = mm->GetMasterValue(_track->_mid);

		if(mvalSlider!=mval || dvalSlider!=dval) {
			_slider->SetValue(mval,false);
			_slider->SetDisplayValue(dval,false);
			_slider->Update();
		}
	}
}

std::wstring MediaMasterControl::GetGroupName() {
	return TL(media_control_group_name);
}

int MediaMasterControl::GetWidth() {
	return 26;
}

ref<tj::shared::Endpoint> MediaMasterControl::GetEndpoint(const std::wstring& name) {
	return GC::Hold(new MediaMasterEndpoint(_track));
}

/** MediaMasterPlayer **/
MediaMasterPlayer::MediaMasterPlayer(ref<MediaMasterTrack> track, ref<Stream> stream): _fader(track->_value) {
	_track = track;
	_output = false;
	_stream = stream;
}

MediaMasterPlayer::~MediaMasterPlayer() {
}

ref<Track> MediaMasterPlayer::GetTrack() {
	return _track;
}

void MediaMasterPlayer::Stop() {
	ref<MediaMasters> mm = _track->_mm;
	if(mm) {
		mm->SetMasterValue(_track->_mid, 1.0f, MediaMaster::MasterSourceSequence);
	}
}

void MediaMasterPlayer::Start(Time pos,ref<Playback> pb, float speed) {
}

void MediaMasterPlayer::Tick(Time currentPosition) {
	if(_fader.UpdateNecessary(currentPosition)) {
		_fader.Tick(currentPosition, 0);

		// Update value here
		float value = _track->_value->GetValueAt(currentPosition);

		if(_output) {
			ref<MediaMasters> mm = _track->_mm;
			if(mm) {
				mm->SetMasterValue(_track->_mid, value, MediaMaster::MasterSourceSequence);
			}
		}

		if(_track->_control) {
			_track->_control->Update();
		}
	}
}

void MediaMasterPlayer::Jump(Time newT, bool paused) {
	Tick(newT);
}

bool MediaMasterPlayer::Paint(tj::shared::graphics::Graphics* g, unsigned int w, unsigned int h) {
	return false;
}

void MediaMasterPlayer::SetOutput(bool enable) {
	_output = enable;
}

Time MediaMasterPlayer::GetNextEvent(Time t) {
	return _track->_value->GetNextEvent(t);
}

/** MediaMasterPlugin **/
MediaMasterPlugin::MediaMasterPlugin(strong<master::MediaMasters> ms): _masters(ms) {
}

std::wstring MediaMasterPlugin::GetFriendlyCategory() const {
	return TL(media_category);
}

MediaMasterPlugin::~MediaMasterPlugin() {
}

std::wstring MediaMasterPlugin::GetName() const {
	return L"Media Master";
}

ref<Track> MediaMasterPlugin::CreateTrack(ref<Playback> pb) {
	return GC::Hold(new MediaMasterTrack(_masters));
}

ref<StreamPlayer> MediaMasterPlugin::CreateStreamPlayer(ref<Playback> pb, ref<Talkback> talk) {
	return null;
}

std::wstring MediaMasterPlugin::GetVersion() const {
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

std::wstring MediaMasterPlugin::GetAuthor() const {
	return std::wstring(L"Tommy van der Vorst");
}

std::wstring MediaMasterPlugin::GetFriendlyName() const {
	return TL(media_master_plugin_friendly_name);
}

std::wstring MediaMasterPlugin::GetDescription() const {
	return TL(media_master_plugin_description);
}

/** MediaMasterTrackRange **/
MediaMasterTrackRange::MediaMasterTrackRange(ref<MediaMasterTrack> track, Time start, Time end): _track(track), _start(start), _end(end) {
}

MediaMasterTrackRange::~MediaMasterTrackRange() {
}

void MediaMasterTrackRange::RemoveItems() {
	_track->_value->RemoveItemsBetween(_start, _end);
}

void MediaMasterTrackRange::Paste(ref<Track> other, Time start) {
	if(other.IsCastableTo<MediaMasterTrack>()) {
		ref<MediaMasterTrack> mmt = other;
		if(mmt) {
			mmt->_value->CopyTo(_track->_value, _start, _end, start);
		}
	}
}

void MediaMasterTrackRange::InterpolateLeft(Time left, float ratio, Time right) {
	_track->_value->InterpolateLeft(left,ratio,right);
}

void MediaMasterTrackRange::InterpolateRight(Time left, float ratio, Time right) {
	_track->_value->InterpolateRight(left,ratio,right);
}
