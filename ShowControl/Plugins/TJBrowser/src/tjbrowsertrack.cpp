#include "../include/tjbrowser.h"
using namespace tj::shared::graphics;
using namespace tj::browser;

namespace tj {
	namespace browser {
		class BrowserTrackRange: public TrackRange {
			public:
				BrowserTrackRange(ref<BrowserTrack> tr, Time s, Time e): _track(tr), _start(s), _end(e) {
				}

				virtual ~BrowserTrackRange() {
				}

				virtual void RemoveItems() {
					std::vector< ref<BrowserCue> > goodCues;

					std::vector< ref<BrowserCue> >::iterator it = _track->_cues.begin();
					while(it!=_track->_cues.end()) {
						ref<BrowserCue> bc = *it;
						if(bc) {
							Time t = bc->GetTime();
							if(t < _start || t>_end) {
								goodCues.push_back(bc);
							}
						}
						++it;
					}

					_track->_cues = goodCues; // TODO: Not really efficient?
				}

				virtual void Paste(ref<Track> other, Time start) {
				}

				virtual void InterpolateLeft(Time left, float ratio, Time right) {
					std::vector< ref<BrowserCue> >::iterator it = _track->_cues.begin();
					while(it!=_track->_cues.end()) {
						ref<BrowserCue> bc = *it;
						if(bc) {
							Time t = bc->GetTime();
							if(t > left && t < right) {
								Time nt = Time(int((t - left).ToInt()*ratio)) + left;
								bc->SetTime(nt);
							}
						}
						++it;
					}
				}

				virtual void InterpolateRight(Time left, float ratio, Time right) {
					std::vector< ref<BrowserCue> >::iterator it = _track->_cues.begin();
					while(it!=_track->_cues.end()) {
						ref<BrowserCue> bc = *it;
						if(bc) {
							Time t = bc->GetTime();
							if(t > left && t < right) {
								Time nt = right - Time(int((right - t).ToInt()*ratio));
								bc->SetTime(nt);
							}
						}
						++it;
					}
				}

			protected:
				Time _start, _end;
				ref<BrowserTrack> _track;
		};
	}
}

BrowserCue::BrowserCue(ref<BrowserTrack> track, Time t) {
	_time = t;
	_track = track;
}

BrowserCue::~BrowserCue() {
}

Time BrowserCue::GetTime() const {
	return _time;
}

void BrowserCue::Remove() {
	ref<BrowserTrack> track(_track);
	if(track) {
		track->RemoveItemAt(_time);
	}
}

void BrowserCue::MoveRelative(Item::Direction dir) {
	switch(dir) {
		case Item::Left:
			_time = _time-Time(1);
			if(_time<Time(0)) _time = Time(0);
			break;

		case Item::Right:
			_time = _time+Time(1);
			break;
	}
}

void BrowserCue::SetTime(Time t) {
	_time = t;
}

std::wstring BrowserCue::GetURL() const {
	return _url;
}

void BrowserCue::Move(Time t, int h) {
	if(h<-25) {
		ref<BrowserTrack> track(_track);
		if(track) {
			track->RemoveItemAt(t);
		}
		return;
	}
	_time = t;
}

void BrowserCue::Save(TiXmlElement* parent) {
	TiXmlElement me("cue");
	SaveAttribute(&me, "time", _time);
	SaveAttribute(&me, "url", _url);
	parent->InsertEndChild(me);
}

void BrowserCue::Load(TiXmlElement* you) {
	_time = LoadAttribute(you, "time", _time);
	_url = LoadAttribute(you, "url", _url);
}

ref<PropertySet> BrowserCue::GetProperties() {
	ref<PropertySet> prs = GC::Hold(new PropertySet());
	prs->Add(GC::Hold(new GenericProperty<Time>(TL(browser_cue_time), this,&_time, 0)));
	prs->Add(GC::Hold(new GenericProperty<std::wstring>(TL(browser_url), this, &_url, _url)));
	return prs;
}

BrowserTrack::BrowserTrack(ref<BrowserPlugin> plug) {
	_hideOnStop = true;
}

BrowserTrack::~BrowserTrack() {
}

void BrowserTrack::Save(TiXmlElement* parent) {
	SaveAttribute(parent, "hide-on-stop", _hideOnStop);

	TiXmlElement cues("cues");
	std::vector< ref<BrowserCue> >::iterator it = _cues.begin();
	while(it!=_cues.end()) {
		ref<BrowserCue> cue = *it;
		cue->Save(&cues);
		++it;
	}
	parent->InsertEndChild(cues);
}

void BrowserTrack::Load(TiXmlElement* you) {
	_hideOnStop = LoadAttribute(you,"hide-on-stop", _hideOnStop);

	_cues.clear();
	TiXmlElement* cues = you->FirstChildElement("cues");
	if(cues!=0) {
		TiXmlElement* cue = cues->FirstChildElement("cue");
		while(cue!=0) {
			ref<BrowserCue> newCue = GC::Hold(new BrowserCue(this));
			newCue->Load(cue);
			_cues.push_back(newCue);
			cue = cue->NextSiblingElement("cue");
		}
	}
}

ref<TrackRange> BrowserTrack::GetRange(Time s, Time e) {
	return GC::Hold(new BrowserTrackRange(this, s, e));
}

ref<LiveControl> BrowserTrack::CreateControl(ref<Stream> stream) { 
	return null;
}

std::wstring BrowserTrack::GetTypeName() const {
	return TL(browser_plugin_friendly_name);
}

Flags<RunMode> BrowserTrack::GetSupportedRunModes() {
	Flags<RunMode> flags;
	flags.Set(RunModeClient, true);
	flags.Set(RunModeMaster, true);
	return flags;
}

ref<Player> BrowserTrack::CreatePlayer(ref<Stream> str) {
	return GC::Hold(new BrowserSurfacePlayer(this, str));
}

ref<PropertySet> BrowserTrack::GetProperties() {
	ref<PropertySet> ps = GC::Hold(new PropertySet());
	ps->Add(GC::Hold(new GenericProperty<bool>(TL(browser_hide_on_stop), this, &_hideOnStop, _hideOnStop)));
	
	return ps;
}

void BrowserTrack::Paint(tj::shared::graphics::Graphics* g, Time position, float pixelsPerMs, unsigned int x, unsigned int y, unsigned int h, Time start, Time end, ref<Item> focus) {
	static float cueWidth = 5.0f;

	strong<Theme> theme = ThemeManager::GetTheme();
	tj::shared::graphics::SolidBrush cueBrush = theme->GetColor(Theme::ColorCurrentPosition);
	tj::shared::graphics::SolidBrush descBrush = theme->GetColor(Theme::ColorDescriptionText);
	
	if(_cues.size()==0) {
		StringFormat sf;
		sf.SetAlignment(StringAlignmentCenter);

		std::wstring str(TL(browser_help_text));
		g->DrawString(str.c_str(), (int)str.length(), theme->GetGUIFont(), PointF((float)(((int(end-start)*pixelsPerMs)/2)+x), (float)((h/4)+y)), &sf, &descBrush);
	}
	else {
		std::vector< ref<BrowserCue> >::iterator it = _cues.begin();
		Pen pn(theme->GetColor(Theme::ColorCurrentPosition), 1.0f);
		while(it!=_cues.end()) {
			ref<BrowserCue> cue = *it;
			Time cueTime = cue->GetTime();
			if(!(cueTime>=start && cueTime <=end)) {
				++it;
				continue;
			}
			
			float pixelLeft = (pixelsPerMs * int(cueTime-start)) - (cueWidth/2.0f) + float(x) + 1.0f;
			g->DrawLine(&pn, pixelLeft, (float)y, pixelLeft, float(y+h));
			++it;
		}
	}
}

ref<Item> BrowserTrack::GetItemAt(Time pos, unsigned int h, bool right, int th, float pixelsPerMs) {
	if(right) {
		// create new cue
		ref<BrowserCue> cue = GC::Hold(new BrowserCue(this,pos));
		_cues.push_back(cue);
		return 0;
	}
	else {
		// get nearest cue
		ref<BrowserCue> nearest = 0;
		std::vector< ref<BrowserCue> >::iterator it = _cues.begin();
		while(it!=_cues.end()) {
			ref<BrowserCue> cue = *it;
			if(!nearest) {
				nearest = cue; 
			}
			else if(abs(int(cue->GetTime()-pos)) < abs(int(nearest->GetTime()-pos))) {
				nearest = cue;
			}
			++it;
		}
		
		if(nearest) {
			return nearest;
		}
		return 0;
	}
}

ref<BrowserCue> BrowserTrack::GetCueBefore(Time pos) {
	ref<BrowserCue> nearest = 0;
	std::vector< ref<BrowserCue> >::iterator it = _cues.begin();
	while(it!=_cues.end()) {
		ref<BrowserCue> cue = *it;

		if(cue->GetTime()<pos) {	
			if(!nearest) {
				nearest = cue; 
			}
			else if(abs(int(cue->GetTime()-pos)) < abs(int(nearest->GetTime()-pos))) {
				nearest = cue;
			}
		}
		++it;
	}

	return nearest;
}

ref<BrowserCue> BrowserTrack::GetCueAfter(Time pos) {
	ref<BrowserCue> nearest = 0;
	std::vector< ref<BrowserCue> >::iterator it = _cues.begin();
	while(it!=_cues.end()) {
		ref<BrowserCue> cue = *it;

		if(cue->GetTime()>pos) {	
			if(!nearest) {
				nearest = cue; 
			}
			else if(abs(int(cue->GetTime()-pos)) < abs(int(nearest->GetTime()-pos))) {
				nearest = cue;
			}
		}
		++it;
	}

	return nearest;
}

void BrowserTrack::RemoveItemAt(Time t) {
	std::vector< ref<BrowserCue> >::iterator it = _cues.begin();
	while(it!=_cues.end()) {
		ref<BrowserCue> cue = *it;

		if(cue->GetTime()==t) {	
			_cues.erase(it);
			return;
		}
		++it;
	}
}
