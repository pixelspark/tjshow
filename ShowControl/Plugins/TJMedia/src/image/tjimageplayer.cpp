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
#include "../../include/tjmedia.h"
#include "../../include/tjmediamasters.h"
using namespace tj::media;

ImagePlayer::ImagePlayer(ref<ImageTrack> track, ref<Stream> pb): _track(track), _stream(pb) {
	assert(_track && _pb);
	_output = false;
}

ImagePlayer::~ImagePlayer() {
}

ref<Track> ImagePlayer::GetTrack() {
	return _track;
}

void ImagePlayer::Stop() {
	_surface = 0;
	_playing = 0;
	_pb = 0;

	if(_stream && _track->_resetOnStop) {
		ref<Message> msg = _stream->Create();
		msg->Add(ImageStreamPlayer::KMessageHide);
		_stream->Send(msg);
	}
}

void ImagePlayer::Start(Time pos, ref<Playback> playback, float speed) {
	_pb = playback;
	Tick(pos);

	ref<ImagePlugin> ip = _track->_plugin;
	if(ip) {
		ip->GetMasters()->EventMasterChanged.AddListener(this);
	}
}

void ImagePlayer::Jump(Time t, bool pause) {
	Tick(t);
}

void ImagePlayer::Notify(ref<Object> src, const Interactive::MouseNotification& mo) {
	if(_playing) {
		if(mo._event==MouseEventLDown) {
			if(_track->_clickedOutlet) _pb->SetOutletValue(_track->_clickedOutlet,Any(1));
			if(_track->_clickXOutlet) _pb->SetOutletValue(_track->_clickXOutlet, Any(mo._x));
			if(_track->_clickYOutlet) _pb->SetOutletValue(_track->_clickYOutlet, Any(mo._y));
			_down = true;
			UpdateImage(_playing);
		}
		else if(mo._event==MouseEventLUp) {
			_down = false;
			UpdateImage(_playing);
		}
	}
}

void ImagePlayer::Notify(ref<Object> source, const Interactive::KeyNotification& kn) {
	if(_playing) {
		if(kn._down) {
			ref<ImageBlock> block = _playing;
			if(block) {
				block->OnKey(kn);

				if(_track->_textOutlet) {
					_pb->SetOutletValue(_track->_textOutlet, block->GetBlockText());
				}
				UpdateImage(block);
			}
		}
	}
}

void ImagePlayer::Notify(ref<Object> source, const Interactive::FocusNotification& kn) {
	if(_playing) {
		ref<ImageBlock> block = _playing;
		if(block) {
			block->OnFocus(kn._focused);
			UpdateImage(block);
		}
	}
}

void ImagePlayer::UpdateImage(ref<ImageBlock> block) {
	if(_pb && _output) {
		Bitmap* file = _down ? block->GetDownBitmap(_pb) : block->GetBitmap(_pb);
		if(file!=0) {
			if(!_surface) {
				_surface = _pb->CreateSurface(file->GetWidth(), file->GetHeight(), false, _pb->GetDeviceByPatch(_track->_screen));
				if(_surface) {
					_surface->EventMouse.AddListener(this);
					_surface->EventKey.AddListener(this);
					_surface->EventFocusChanged.AddListener(this);
					_surface->SetCanFocus(true);
				}
			}
			else {
				// Resize it to our needs (setting it to invisible causes TJShow to not display it until we're done loading)
				_surface->SetVisible(false);
				_surface->Resize(file->GetWidth(), file->GetHeight());
			}

			if(_surface) {
				ref<SurfacePainter> sp = _surface->GetPainter();
				if(sp) {
					graphics::Graphics* g = sp->GetGraphics();
					g->SetCompositingMode(CompositingModeSourceCopy);
					g->DrawImage(file, RectF(0.0f, 0.0f, (float)file->GetWidth(), (float)file->GetHeight()));
				}
				_surface->SetVisible(true);
			}
		}
	}
}

void ImagePlayer::Notify(ref<Object> src, const master::MediaMasters::MasterChangedNotification& me) {
	UpdateMasterValues();
}

void ImagePlayer::UpdateMasterValues() {
	const static std::wstring KMixIdent = L"tjmedia-masters";

	if(_surface || _stream) {
		ref<ImagePlugin> ip = _track->_plugin;
		if(!ip) {
			return;
		}

		// Get a list of applicable master values
		strong<master::MediaMasters> ms = ip->GetMasters();
		std::map<master::MediaMaster::Flavour, float> flavouredValues;
		ms->GetMasterValues(_track->_masters, flavouredValues);
		std::set<Mesh::EffectParameter> restricted;

		// Create a message to send to the client
		ref<Message> msg;
		if(_stream) {
			msg = _stream->Create();
			msg->Add(ImageStreamPlayer::KMessageMasterValues);
		}
		
		// Iterate over all master values and apply them
		std::map<master::MediaMaster::Flavour, float>::const_iterator it = flavouredValues.begin();
		while(it!=flavouredValues.end()) {
			master::MediaMaster::Flavour fv = it->first;

			if(fv==master::MediaMaster::FlavourVolume) {
				// Ignore
			}
			else if(fv==master::MediaMaster::FlavourAlpha) {
				if(_surface) {
					_surface->SetMixAlpha(KMixIdent, it->second);
				}
			}
			else if(fv>=master::MediaMaster::FlavourEffect) {
				if(_surface) {
					Mesh::EffectParameter ep = (Mesh::EffectParameter)(fv - master::MediaMaster::FlavourEffect);
					restricted.insert(ep);
					_surface->SetMixEffect(KMixIdent, ep, it->second);
				}
			}

			if(msg) {
				msg->Add<int>(fv);
				msg->Add<float>(it->second);
			}	
			++it;
		}

		// Restrict effects to the set of effects we have a value for
		if(_surface) {
			_surface->RestrictMixEffects(KMixIdent, restricted);
		}

		if(_stream) {
			_stream->Send(msg);
		}
	}
}

void ImagePlayer::Tick(Time t) {
	UpdateMasterValues();

	ref<ImageBlock> block = _track->GetBlockAt(t);
	if(block && block!=_playing) {
		// Load this block
		_playing = block;
		block->Prepare(_pb);
		
		// send load message
		if(_stream) {
			block->Send(_stream, _pb, _track->_screen);
		}
		
		// Create surface and paint!
		UpdateImage(block);
	}
	else if(!block) {
		// nothing should be playing, move it away!
		if(_stream) {
			ref<Message> msg = _stream->Create();
			msg->Add(ImageStreamPlayer::KMessageHide);
			_stream->Send(msg);
		}

		if(_surface) {
			_surface->SetVisible(false);
		}
		_playing = 0;
	}

	// Update geometry info
	Vector cTranslate = _track->_itranslate + _track->_translate.Scale(_track->GetTranslateAt(t));
	Vector cRotate = _track->_irotate + _track->_rotate.Scale(_track->GetRotateAt(t));
	Vector cScale = _track->_iscale + _track->_scale.Scale(_track->GetScaleAt(t));

	if(block) {
		graphics::Bitmap* bitmap = block->GetBitmap(_pb);
		cScale = cScale.Dot(Vector((float)bitmap->GetWidth(), (float)bitmap->GetHeight(), 1.0f));
	}

	float opacity = _track->GetOpacityAt(t);

	if(_stream) {
		ref<Message> msg = _stream->Create();
		msg->Add(ImageStreamPlayer::KMessageUpdate);
		msg->Add(opacity);
		msg->Add(cTranslate);
		msg->Add(cRotate);
		msg->Add(cScale); 
		_stream->Send(msg);
	}

	if(_surface) {
		_surface->SetMixAlpha(L"tjmedia-player", opacity);
		_surface->SetTranslate(cTranslate);
		_surface->SetRotate(cRotate);
		_surface->SetScale(cScale);
	}
}

Time ImagePlayer::GetNextEvent(Time t) {
	return _track->GetNextEvent(t);
}

void ImagePlayer::SetOutput(bool enable) {
	_output = enable;
	
	// If the output is disabled while playing and we are displaying something, move it away
	if(!enable && _surface) {
		_surface = 0;
	}
}


/* Streamplayer */
ImageStreamPlayer::ImageStreamPlayer(ref<ImagePlugin> p, ref<Talkback> t): _plugin(p), _talk(t) {
}

ImageStreamPlayer::~ImageStreamPlayer() {
}

ref<Plugin> ImageStreamPlayer::GetPlugin() {
	return _plugin;
}

void ImageStreamPlayer::Notify(ref<Object> src, const Interactive::MouseNotification& mn) {
	if(mn._event==MouseEventLDown) {
		if(_talk) {
			_talk->SendOutletChange(ImageTrack::KClickedOutletID, Any(int(1)));
			_talk->SendOutletChange(ImageTrack::KClickedXOutletID, Any(double(mn._x)));
			_talk->SendOutletChange(ImageTrack::KClickedYOutletID, Any(double(mn._y)));
		}
	}
}

void ImageStreamPlayer::Message(ref<DataReader> msg, ref<Playback> pb) {
	unsigned int pos = 0;
	int code = msg->Get<int>(pos);

	if(code==KMessageHide) {
		if(_surface) {
			_surface->SetVisible(false);
			_surface = 0;
		}
	}
	else if(code==KMessageLoad) {
		std::wstring path;
		if(pb->GetResources()->GetPathToLocalResource(msg->Get<std::wstring>(pos), path)) {
			PatchIdentifier screen = msg->Get<PatchIdentifier>(pos);
			
			Bitmap* file = (Bitmap*)Bitmap::FromFile(path.c_str(), TRUE);
			if(file!=0) {
				_surface = pb->CreateSurface(file->GetWidth(), file->GetHeight(), true, pb->GetDeviceByPatch(screen));
				if(_surface) {
					_surface->EventMouse.AddListener(this);
					ref<SurfacePainter> sp = _surface->GetPainter();
					if(sp) {
						tj::shared::graphics::Graphics* sg = sp->GetGraphics();
						sg->SetCompositingMode(CompositingModeSourceCopy);
						sg->DrawImage(file, RectF(0.0f, 0.0f, (float)file->GetWidth(), (float)file->GetHeight()));
					}
				}
				delete file;
			}
		}
	}
	else if(code==KMessageLoadText) {
		Pixels width = msg->Get<Pixels>(pos);
		Pixels height = msg->Get<Pixels>(pos);
		Pixels fontSize = msg->Get<Pixels>(pos);
		unsigned int flags = msg->Get<unsigned int>(pos);
		double red = msg->Get<double>(pos);
		double green = msg->Get<double>(pos);
		double blue = msg->Get<double>(pos);
		std::wstring translated = msg->Get<std::wstring>(pos);
		PatchIdentifier screen = msg->Get<PatchIdentifier>(pos);
		std::wstring fontName = msg->Get<std::wstring>(pos);

		_surface = pb->CreateSurface(width, height, true, pb->GetDeviceByPatch(screen));
		if(_surface) {
			_surface->EventMouse.AddListener(this);
			ref<SurfacePainter> sp = _surface->GetPainter();
			if(sp) {
				Graphics* g = sp->GetGraphics();

				g->Clear(Color(0,0,0,0));
				strong<Theme> theme = ThemeManager::GetTheme();

				Font* font = 0;

				if(fontName.length()>0) {
					font = new Font(fontName.c_str(), float(fontSize), FontStyleRegular);
				}
				else {
					strong<Theme> theme = ThemeManager::GetTheme();
					std::wstring uiFontName = theme->GetGUIFontName();
					font = new Font(uiFontName.c_str(), float(fontSize), FontStyleRegular);
				}
				SolidBrush tbr(RGBColor(red, green, blue));
				PointF origin(0.0f, 0.0f);
				
				if(font!=0) {
					g->DrawString(translated.c_str(), (int)translated.length(), font, origin, &tbr);
				}

				if((flags&1)!=0) {
					Pen pn(&tbr,2.0f);
					g->DrawRectangle(&pn, Area(2, 1, width-3, height-2));
				}
				delete font;
			}
		}
	}
	else if(code==KMessageUpdate && _surface) {
		_surface->SetMixAlpha(L"tjmedia-player", msg->Get<float>(pos));
		_surface->SetTranslate(msg->Get<Vector>(pos));
		_surface->SetRotate(msg->Get<Vector>(pos));
		_surface->SetScale(msg->Get<Vector>(pos));
	}
	else if(code==KMessageMasterValues && _surface) {
		while(msg->GetSize()>pos) {
			master::MediaMaster::Flavour fl = (master::MediaMaster::Flavour)msg->Get<int>(pos);
			float fv = msg->Get<float>(pos);

			switch(fl) {
				case master::MediaMaster::FlavourVolume:
					// ignore
					break;

				case master::MediaMaster::FlavourAlpha:
					_surface->SetMixAlpha(L"media-masters", fv);
					break;

				default:
					_surface->SetMixEffect(L"media-masters", (Mesh::EffectParameter)(fl - master::MediaMaster::FlavourEffect), fv);
					break;

			}
		}
	}
}