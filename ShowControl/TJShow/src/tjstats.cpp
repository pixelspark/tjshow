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
#include "../include/internal/tjshow.h"
#include "../include/internal/tjstats.h"
#include "../include/extra/tjextra.h"
using namespace tj::shared::graphics;
using namespace tj::show::stats;

namespace tj {
	namespace show {
		namespace stats {
			class StatsTrack;

			class StatsPlayer: public Player {
				public:
					StatsPlayer(ref<StatsTrack> track);
					virtual ~StatsPlayer();
					ref<Track> GetTrack();
					virtual void Stop();
					virtual void Start(Time pos,ref<Playback> pb, float speed);
					virtual void Tick(Time currentPosition);
					virtual void Jump(Time newT, bool paused);
					virtual bool Paint(tj::shared::graphics::Graphics* g, unsigned int w, unsigned int h);
					virtual void SetOutput(bool enable);
					virtual Time GetNextEvent(Time t);

				protected:
					ref<StatsTrack> _track;
					Time _nextTick;
					int _tickCount;
			};

			class StatsTrack: public Track {
				public:
					StatsTrack() {
						_drift = GC::Hold(new Fader<int>(0,0,20));
						_interval = Time(250);
						_height = 19;
						_driftSum = 0;
						_driftCount = 0;
					}
					
					virtual ~StatsTrack() {
					}

					virtual Pixels GetHeight() {
						return max(19,_height);
					}

					virtual void Save(TiXmlElement* parent) {
						SaveAttribute(parent, "interval", _interval.ToInt());
						SaveAttribute(parent, "height", _height);
					}

					virtual void Load(TiXmlElement* you) {
						_interval = Time(LoadAttribute(you, "interval", _interval.ToInt()));
						_height = LoadAttribute(you, "height", _height);
					}

					virtual Flags<RunMode> GetSupportedRunModes() {
						return Flags<RunMode>(RunModeMaster);
					}

					virtual std::wstring GetTypeName() const { 
						return std::wstring(L"Stats");
					}

					virtual ref<Player> CreatePlayer(ref<Stream> str) {
						return GC::Hold(new StatsPlayer(this));
					}

					virtual ref<PropertySet> GetProperties() {
						ref<PropertySet> prs = GC::Hold(new PropertySet());

						prs->Add(GC::Hold(new GenericProperty<Time>(TL(stats_interval), this, &_interval, _interval)));
						prs->Add(GC::Hold(new GenericProperty<int>(TL(track_height), this, &_height, _height)));
						
						return prs;
					}

					virtual void Paint(tj::shared::graphics::Graphics* g, Time position, float pixelsPerMs, unsigned int x, unsigned int y, unsigned int h, Time start, Time end, ref<Item> focus) {
						strong<Theme> theme = ThemeManager::GetTheme();

						FaderPainter<int> painterd(_drift);
						Pen pnd(Color(255,0,0),2.0f);
						painterd.Paint(g, start, end-start, h, int(float(end-start)*pixelsPerMs), x, y, &pnd,position);	
					
						// drift average
						if(_driftCount>0) {
							std::wstring avg = TL(stats_drift_average_before) + Stringify(int(_driftSum/_driftCount)) + TL(stats_drift_average_after);
							SolidBrush tbr(theme->GetColor(Theme::ColorText));
							StringFormat sf;
							sf.SetAlignment(StringAlignmentNear);
							g->DrawString(avg.c_str(), (int)avg.length(), theme->GetGUIFont(), RectF(float(x), float(y), 200.0f, float(h)), &sf, &tbr);
						}
					}

					virtual ref<LiveControl> CreateControl() {
						return 0;
					}

					ref< Fader<int> > _drift;
					int _driftSum;
					int _driftCount;
					Time _interval;
					Pixels _height;
			};
		}
	}
}

using namespace tj::show::stats;

/** Statsplayer **/
StatsPlayer::StatsPlayer(ref<StatsTrack> track) {
	_track = track;
	_nextTick = Time(-1);
	_tickCount = 0;
}
StatsPlayer::~StatsPlayer() {
}

ref<Track> StatsPlayer::GetTrack() {
	return _track;
}

void StatsPlayer::Stop() {
	_tickCount = 0;
}

void StatsPlayer::Start(Time pos,ref<Playback> pb, float speed) {
	_track->_drift->GetPoints()->clear();
	_nextTick = Time(-1);
	_track->_driftSum = 0;
	_track->_driftCount = 0;
	_tickCount = 0;
}

std::wstring StatsPlugin::GetFriendlyName() const {
	return TL(stats_plugin_friendly_name);
}

std::wstring StatsPlugin::GetFriendlyCategory() const {
	return TL(other_category);
}


std::wstring StatsPlugin::GetDescription() const {
	return TL(stats_plugin_description);
}

void StatsPlayer::Tick(Time currentPosition) {
	_tickCount++;

	if(_nextTick>Time(0)) {
		int drift = abs(currentPosition.ToInt() - _nextTick.ToInt());
		if(drift > _track->_drift->GetMaximum()) {
			_track->_drift->SetMaximum(drift+10);
		}
		_track->_drift->AddPoint(_nextTick, drift);

		// drift average
		_track->_driftSum += drift;
		_track->_driftCount++;
	}
}

void StatsPlayer::Jump(Time newT, bool paused) {
	_tickCount = 0;
	Tick(newT);
}

bool StatsPlayer::Paint(Graphics* g, unsigned int w, unsigned int h) {
	return false;
}

void StatsPlayer::SetOutput(bool enable) {
}

Time StatsPlayer::GetNextEvent(Time t) {
	_nextTick = t + _track->_interval;
	return _nextTick;
}

/** StatsPlugin **/
StatsPlugin::StatsPlugin() {
}

StatsPlugin::~StatsPlugin() {
}

std::wstring StatsPlugin::GetName() const {
	return L"Stats";
}

ref<Track> StatsPlugin::CreateTrack(ref<Playback> pb) {
	return GC::Hold(new StatsTrack());
}

ref<StreamPlayer> StatsPlugin::CreateStreamPlayer(ref<Playback> pb, ref<Talkback> talk) {
	return 0;
}

std::wstring StatsPlugin::GetVersion() const {
	return L"";
}

std::wstring StatsPlugin::GetAuthor() const {
	return L"";
}
