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
using namespace tj::shared::graphics;
using namespace tj::media;

#include <shlwapi.h>

namespace tj {
	namespace media {
		class MediaControlWnd: public ChildWnd, public Listener<SliderWnd::NotificationChanged> {
			friend class MediaControlEndpoint; 
			friend class MediaControl;

			public:
				MediaControlWnd(MediaControl* mc, ref<Stream> stream, bool isVideo): ChildWnd(L""), _isVideo(isVideo) {
					SetStyle(WS_CLIPCHILDREN);

					assert(mc);
					_mc = mc;
					_stream = stream;

					// create sliders
					_volumeSlider = GC::Hold(new SliderWnd(L""));
					_volumeSlider->SetValue(1.0f);
					Add(_volumeSlider);

					if(_isVideo) {
						_alphaSlider = GC::Hold(new SliderWnd(L""));
						_alphaSlider->SetValue(1.0f);
						_alphaSlider->SetColor(Theme::SliderAlpha);
						Add(_alphaSlider);
					}

					Update();
				}

				virtual ~MediaControlWnd() {
				}

				virtual void OnCreated() {
					_volumeSlider->EventChanged.AddListener(ref< Listener<SliderWnd::NotificationChanged> >(this));
					if(_isVideo) {
						_alphaSlider->EventChanged.AddListener(ref< Listener<SliderWnd::NotificationChanged> >(this));
					}
					ChildWnd::OnCreated();
				}

				virtual void Notify(ref<Object> source, const SliderWnd::NotificationChanged& evt) {
					ref<Deck> deck = _mc->GetCurrentDeck();
					float vol = _volumeSlider->GetValue();
					float alpha = 0.0f;

					if(_isVideo) {
						alpha = _alphaSlider->GetValue();
					}

					if(deck) {
						deck->SetMixVolume(MediaControl::KMixName, vol);
						if(_isVideo) {
							deck->SetMixAlpha(MediaControl::KMixName, alpha);
						}
					}
					
					_mc->SetLastControlValues(vol,alpha);

					if(_stream) {
						ref<tj::np::Message> msg = _stream->Create();
						msg->Add((unsigned char)MediaActionLiveParameters);
						msg->Add(vol);
						msg->Add(alpha);
						msg->SetSendToPlugin(false);
						_stream->Send(msg);
					}
				}

				virtual LRESULT Message(UINT msg, WPARAM wp, LPARAM lp) {
					if(msg==WM_SIZE) {
						Update();
					}

					return ChildWnd::Message(msg, wp, lp);
				}

				virtual void Update() {
					ref<Deck> deck = _mc->GetCurrentDeck();
					if(deck) {
						float cal = deck->GetCurrentAudioLevel();
						if(cal<0.05f) {
							cal = -1.0f;
						}
						_volumeSlider->SetMarkValue(cal);
						_volumeSlider->SetDisplayValue(deck->GetVolume(), false);
						_volumeSlider->SetValue(deck->GetMixVolume(L"live"));
						_volumeSlider->Update();

						if(_isVideo) {
							_alphaSlider->SetDisplayValue(deck->GetOpacity(), false);
							_alphaSlider->SetValue(deck->GetMixAlpha(L"live"));
							_alphaSlider->Update();
						}
					}

					Layout();
					Repaint();
				}

				virtual void Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme) {
					// 15px title
					// 10px progress bar
					// slider
					Area rc = GetClientArea();

					SolidBrush br(theme->GetColor(Theme::ColorBackground));
					g.FillRectangle(&br, GetClientArea());

					ref<Deck> deck = _mc->GetCurrentDeck();
					if(deck) {
						std::wstring fn = deck->GetFileName();
						std::wstring filename = PathFindFileName(fn.c_str());
						SolidBrush tbr(deck->IsPlaying()?theme->GetColor(Theme::ColorText):theme->GetColor(Theme::ColorActiveEnd));
						StringFormat sf;
						sf.SetAlignment(StringAlignmentCenter);
						g.DrawString(filename.c_str(), (int)filename.length(), theme->GetGUIFont(), RectF(1.0f, 0.0f, rc.GetWidth()-2.0f, 13.0f), &sf, &tbr);
					
						// where are we in the file?
						LinearGradientBrush progress(PointF(0.0f, 15.0f), PointF(0.0f, 23.0f), theme->GetColor(Theme::ColorActiveStart), theme->GetColor(Theme::ColorActiveEnd));
						Pen progressPen(&progress, 1.0f);
						g.DrawRectangle(&progressPen, RectF(1.0f, 16.0f, rc.GetWidth()-2.0f, 6.0f));

						int length = deck->GetLength();
						int pos = deck->GetPosition();
						if(length!=0) {
							float r = float(pos)/float(length);
							g.FillRectangle(&progress, RectF(1.0f, 16.0f, r*(rc.GetWidth()-2.0f), 6.0f));
						}
					}
				}

				virtual void Layout() {
					Area rc = GetClientArea();

					_volumeSlider->Move(rc.GetLeft(), rc.GetTop()+30, 30, rc.GetHeight()-30);
					if(_isVideo) {
						_alphaSlider->Move(rc.GetLeft()+30, rc.GetTop()+30, 30, rc.GetHeight()-30);
					}
				}

				// no windows ui calls in here, because this is called from the player thread and causes lockup
				// can also be used for other threads (e.g. input) for an update
				virtual void UpdateFromPlayer() {
					ref<Deck> deck = _mc->GetCurrentDeck();
					if(deck) {
						_volumeSlider->SetDisplayValue(deck->GetVolume(), false);

						if(_isVideo) {
							_alphaSlider->SetDisplayValue(deck->GetOpacity(), false);
						}
					}
					_volumeSlider->Update();

					if(_isVideo) {
						_alphaSlider->Update();
					}

					Repaint();
				}

			protected:
				MediaControl* _mc;
				ref<SliderWnd> _volumeSlider;
				ref<SliderWnd> _alphaSlider;
				ref<Stream> _stream;
				bool _isVideo;
		};

		class MediaControlEndpoint: public Endpoint {
			public:
				MediaControlEndpoint(ref<MediaControl> mc, bool volume=true);
				virtual ~MediaControlEndpoint();
				virtual void Set(const Any& f);
				virtual std::wstring GetName() const;

			protected:
				weak<MediaControl> _mc;
				bool _volume;
		};
	}
}

const wchar_t* MediaControl::KMixName = L"live";

MediaControl::MediaControl(MediaTrack* track, ref<Stream> stream, bool isVideo): _isVideo(isVideo) {
	assert(track);
	_track = track;
	_wnd = GC::Hold(new MediaControlWnd(this, stream, isVideo));
}

MediaControl::~MediaControl() {
}

ref<Wnd> MediaControl::GetWindow() {
	return _wnd;
}

std::wstring MediaControl::GetGroupName() {
	return TL(media_control_group_name);
}

bool MediaControl::IsSeparateTab() {
	return false;
}

float MediaControl::GetVolumeValue() const {
	return _wnd->_volumeSlider->GetValue();
}

float MediaControl::GetOpacityValue() const {
	return _wnd->_alphaSlider->GetValue();
}

int MediaControl::GetWidth() {
	return _isVideo ? 60 : 30;
}

void MediaControl::SetPropertyGrid(ref<PropertyGridProxy> pg) {
}

void MediaControl::Update() {
	_wnd->UpdateFromPlayer();
}

ref<Deck> MediaControl::GetCurrentDeck() {
	return _track->_lastDeck;
}

void MediaControl::SetLastControlValues(float volume, float opacity) {
	_track->SetLastControlValues(volume, opacity);
}

void MediaControl::GetEndpoints(std::vector< LiveControl::EndpointDescription >& eps) {
	eps.push_back(LiveControl::EndpointDescription(L"alpha", TL(media_endpoint_alpha)));
	eps.push_back(LiveControl::EndpointDescription(L"volume", TL(media_endpoint_volume)));
}

ref<Endpoint> MediaControl::GetEndpoint(const std::wstring& name) {
	if(name==L"alpha") {
		return GC::Hold(new MediaControlEndpoint(this, false));
	}
	else {
		return GC::Hold(new MediaControlEndpoint(this, true));
	}
}

/** MediaControlEndpoint **/
MediaControlEndpoint::MediaControlEndpoint(ref<MediaControl> mc, bool volume) {
	_mc = mc;
	_volume = volume;
}

MediaControlEndpoint::~MediaControlEndpoint() {
}

void MediaControlEndpoint::Set(const Any& f) {
	ref<MediaControl> mc = _mc;
	if(mc) {
		float val = Clamp((float)f, 0.0f, 1.0f);

		if(_volume) {
			mc->_wnd->_volumeSlider->SetValue(val, true);
		}
		else {
			mc->_wnd->_alphaSlider->SetValue(val, true);
		}
		mc->_wnd->UpdateFromPlayer();
	}
}

std::wstring MediaControlEndpoint::GetName() const {
	return _volume?TL(media_endpoint_volume):TL(media_endpoint_opacity);
}