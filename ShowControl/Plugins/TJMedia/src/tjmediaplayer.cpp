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
using namespace tj::media;

MediaPlayer::MediaPlayer(strong<Track> tr, strong<MediaPlugin> pl, ref<Stream> stream): _plugin(pl), _masters(pl->GetMasters()), _track(tr), _stream(stream) {
	_outputAudioEnabled = false;
	_outputVideoEnabled = false;
	_speed = 1.0f;
}

MediaPlayer::~MediaPlayer() {
}

ref<Track> MediaPlayer::GetTrack() {
	return _track;
}
 
void MediaPlayer::Pause(Time pos) {
	_deck->Pause();

	if(_stream) {
		ref<Message> msg = _stream->Create();
		msg->Add((unsigned char)MediaActionStop);
		_stream->Send(msg);
	}
}

void MediaPlayer::Notify(ref<Object> src, const master::MediaMasters::MasterChangedNotification& me) {
	UpdateMasterValues();
}

void MediaPlayer::Notify(ref<Object> src, const Interactive::MouseNotification& mo) {
	if(mo._event==MouseEventLDown) {
		if(_track->_clickedOutlet) _pb->SetOutletValue(_track->_clickedOutlet, Any(1));
		if(_track->_clickXOutlet) _pb->SetOutletValue(_track->_clickXOutlet, Any(mo._x));
		if(_track->_clickYOutlet) _pb->SetOutletValue(_track->_clickYOutlet, Any(mo._y));
	}
}

void MediaPlayer::Stop() {
	// Notify clients that we shouldn't be playing anymore
	if(_stream) {
		ref<Message> msg = _stream->Create();
		msg->Add((unsigned char)MediaActionStop);
		_stream->Send(msg);
	}
	_outputAudioEnabled = false;
	_outputVideoEnabled = false;

	// Stop playing on this server
	if(_deck) {
		_deck->Stop();
	}
	_deck = 0;
	_playing = 0;
}

void MediaPlayer::Start(Time pos, ref<Playback> playback, float speed) {
	_pb = playback;
	_deck = playback->CreateDeck(true, playback->GetDeviceByPatch(_track->_screen));
	_deck->SetAudioOutput(_outputAudioEnabled);
	_deck->SetVideoOutput(_outputVideoEnabled);
	_deck->SetMixVolume(MediaControl::KMixName, _track->_lastControlVolume);
	_deck->SetMixAlpha(MediaControl::KMixName, _track->_lastControlOpacity);
	_deck->EventMouse.AddListener(this);
	_masters->EventMasterChanged.AddListener(this);

	_track->_lastDeck = _deck;
	_speed = speed;
	_preloadAttempted = L"";

	// Set the correct screen on the client
	if(_stream) {
		ref<Message> str = _stream->Create();
		str->Add((unsigned char)MediaActionSetScreen);
		str->Add<PatchIdentifier>(_track->_screen);
		_stream->Send(str);
	}

	/* Preload can take long (if the file found is on some network drive that cannot be reached, etc) and plug-ins shouldn't
	do any slow stuff in Start, since that delays starting the timeline and (more importantly) hangs up the application if
	a timeline happens to be started from the main thread). So don't preload here - instead do it on the first tick (which,
	through PlayerThread::OnStart, should also happen after being started). */
	//Preload(pos);
	_deck->SetPlaybackSpeed(speed);
}

void MediaPlayer::SetOutput(bool m) {
	_outputAudioEnabled = m;
	_outputVideoEnabled = m;
	
	if(_deck) {
		_deck->SetAudioOutput(_outputAudioEnabled);
		_deck->SetVideoOutput(_outputVideoEnabled);
	}
}

Time MediaPlayer::GetNextEvent(Time t) {
	return _track->GetNextEvent(t);
}

void MediaPlayer::Preload(Time pos) {
	ref<MediaBlock> next = _track->GetBlockAt(pos);
	if(!next) {
		next = _track->GetNextBlock(pos);
		if(!next) return;
	}

	std::wstring nextFile = _pb->ParseVariables(next->GetFile());
	if(!next->IsLiveSource() && _preloadAttempted != nextFile) {
		_preloadAttempted = nextFile;

		// send message to client so it can load the file before starting
		if(_stream) {
			ref<Message> str = _stream->Create();
			str->Add((unsigned char)MediaActionStart);
			str->Add(int(-1));
			str->Add(_track->GetVolume()->GetValueAt(pos));
			str->Add(_track->GetOpacity()->GetValueAt(pos));
			str->Add(_track->GetCardPatch());
			str->Add<Vector>(_track->_translate);
			str->Add<Vector>(_track->_rotate);
			str->Add<Vector>(_track->_scale);
			str->Add(nextFile);
			str->Add<PatchIdentifier>(L"");
			
			_stream->Send(str);
		}

		// load the file for the next block so it can be played
		if(_deck && (_outputVideoEnabled||_outputVideoEnabled)) {
			std::wstring path;
			if(_pb->GetResources()->GetPathToLocalResource(nextFile, path)) {
				if(_deck->GetFileName()!=path) {
					_deck->Load(path, _track->GetCardDevice());
				}
				else {
					_deck->Pause();
				}
			}
			_deck->SetAudioOutput(_outputAudioEnabled);
			_deck->SetVideoOutput(_outputVideoEnabled);
		}
	}
}

void MediaPlayer::Tick(Time t) {
	UpdateMasterValues();

	Vector currentTranslate = _track->_itranslate + _track->_translate.Scale(_track->GetTranslateAt(t));
	Vector currentRotate = _track->_irotate + _track->_rotate.Scale(_track->GetRotateAt(t));
	Vector currentScale = _track->_iscale + _track->_scale.Scale(_track->GetScaleAt(t));
	_deck->SetTranslate(currentTranslate);
	_deck->SetRotate(currentRotate);
	_deck->SetScale(currentScale);
	_deck->SetBalance(_track->GetBalance());

	ref<MediaBlock> block = _track->GetBlockAt(t);
	if(!block && _playing) {
		_deck->Pause();

		if(_stream) {
			ref<Message> str = _stream->Create();
			str->Add((unsigned char)MediaActionStop);
			_stream->Send(str);
		}
		_deck->SetVisible(false);
		_playing = 0;
		return;
	}

	if(block && t>=block->GetTime() && block!=_playing) {
		_playing = block;

		int song_offset = int(t) - int(block->GetTime());
		if(song_offset<0) song_offset = 0;
		PatchIdentifier card = _track->GetCardPatch();
		if(_stream) {
			ref<Message> str = _stream->Create();
			str->Add((unsigned char)MediaActionStart);
			str->Add(song_offset);
			str->Add(_track->GetVolumeAt(t));
			str->Add(_track->GetOpacity()->GetValueAt(t));
			str->Add(card);
			str->Add(currentTranslate);
			str->Add(currentRotate);
			str->Add(currentScale);
			str->Add(_pb->ParseVariables(block->GetFile()));
			str->Add(block->GetLiveSource());
			
			_stream->Send(str);
		}

		if(_outputAudioEnabled || _outputVideoEnabled) {
			std::wstring file = _pb->ParseVariables(block->GetFile());
			if(block->IsLiveSource()) {
				_deck->LoadLive(_pb->GetDeviceByPatch(block->GetLiveSource()), _track->GetCardDevice());
			}
			else {
				std::wstring filePath;
				if(_pb->GetResources()->GetPathToLocalResource(file, filePath)) {
					_deck->Load(filePath, _track->GetCardDevice());
				}
			}

			_deck->SetAudioOutput(_outputAudioEnabled);
			_deck->SetVideoOutput(_outputVideoEnabled);

			_deck->Jump(song_offset);
			_deck->SetPlaybackSpeed(_speed);
			_deck->Play();
			_deck->Jump(song_offset);
			_deck->SetVisible(true);
		}
	}
	else {
		// preload file
		if(!block && _deck) {
			_deck->SetVisible(false);
			_playing = 0;
			Preload(t);
		}
	}

	// Update keying data
	if(_playing) {
		_deck->SetKeyingEnabled(_playing->IsKeyingEnabled());
		_deck->SetKeyColor(_playing->GetKeyingColor());
		_deck->SetKeyingTolerance(_playing->GetKeyingTolerance());
		
		if(_stream) {
			ref<Message> str = _stream->Create();
			str->Add((unsigned char)MediaActionKeyingParameters);
			str->Add(_playing->IsKeyingEnabled());
			if(_playing->IsKeyingEnabled()) {
				str->Add(_playing->GetKeyingColor());
				str->Add(_playing->GetKeyingTolerance());
			}
			_stream->Send(str);
		}
	}

	// Update volume and opacity here and on the client (and send rotation stuff too)
	float currentVolume = _track->GetVolumeAt(t);
	float currentOpacity = _track->GetOpacityAt(t);

	if(_stream) {
		ref<Message> str = _stream->Create();
		str->Add((unsigned char)MediaActionVolume);
		str->Add(currentVolume);
		str->Add(currentOpacity);
		str->Add(currentTranslate);
		str->Add(currentRotate);
		str->Add(currentScale);
		_stream->Send(str);
	}

	_deck->SetVolume(currentVolume);
	_deck->SetMixAlpha(L"tjmedia-player", currentOpacity);
}

void MediaPlayer::UpdateMasterValues() {
	const static std::wstring KMixIdent = L"tjmedia-masters";

	if(_deck || _stream) {
		std::map<master::MediaMaster::Flavour, float> flavouredValues;
		_masters->GetMasterValues(_track->_masters, flavouredValues);

		std::map<master::MediaMaster::Flavour, float>::const_iterator it = flavouredValues.begin();
		std::set<Mesh::EffectParameter> restricted;

		ref<Message> msg;
		if(_stream) {
			msg = _stream->Create();
			msg->Add<unsigned char>((int)MediaActionMasterValues);
		}

		while(it!=flavouredValues.end()) {
			const master::MediaMaster::Flavour& flavour = it->first;

			if(flavour==master::MediaMaster::FlavourVolume) {
				if(_deck) {
					_deck->SetMixVolume(KMixIdent, it->second);
				}
			}
			else if(flavour==master::MediaMaster::FlavourAlpha) {
				if(_deck) {
					_deck->SetMixAlpha(KMixIdent, it->second);
				}
			}
			else if(flavour>=master::MediaMaster::FlavourEffect) {
				if(_deck) {
					Mesh::EffectParameter ep = (Mesh::EffectParameter)(flavour - master::MediaMaster::FlavourEffect);
					_deck->SetMixEffect(KMixIdent, ep, it->second);
					restricted.insert(ep);
				}
			}

			if(msg) {
				msg->Add<int>(flavour);
				msg->Add<float>(it->second);
			}
			++it;
		}

		if(_stream) {
			_stream->Send(msg);
		}

		// Restrict effect values to the ones we have a value for
		if(_deck) {
			_deck->RestrictMixEffects(KMixIdent, restricted);
		}

	}
}

void MediaPlayer::SetPlaybackSpeed(Time t, float c) {
	_speed = c;
	_deck->SetPlaybackSpeed(c);

	if(_stream) {
		ref<Message> msg = _stream->Create();
		msg->Add<unsigned char>((int)MediaActionSpeed);
		msg->Add<float>(c);
		_stream->Send(msg);
	}
}

void MediaPlayer::Jump(Time t, bool paused) {
	/* Check if we are jumping to a position in the currently playing file, if any.
	If it is another file or no file playing, just use Tick to handle all other changes 
	(Tick will not do the in-file jumping anyway, so we must do that here) */
	if(_playing) {
		ref<MediaBlock> block = _track->GetBlockAt(t);

		if(block && block==_playing) {
			int song_offset = t - block->GetTime();
			if(song_offset<0) song_offset = 0;

			if(_stream) {
				ref<Message> str = _stream->Create();
				str->Add((unsigned char)MediaActionJump);
				str->Add(song_offset);
				str->Add((unsigned char)paused?1:0);
				_stream->Send(str);
			}

			_deck->Jump(song_offset);

			if(!paused) {
				_deck->Play();
			}
		}
	}
	
	if(!paused) {
		Tick(t);
	}
}

bool MediaPlayer::Paint(tj::shared::graphics::Graphics* g, unsigned int w, unsigned int h) {
	return false;
}

MediaStreamPlayer::MediaStreamPlayer(ref<MediaPlugin> plug, ref<Playback> pb) {
	_plugin = plug;
	_deck = pb->CreateDeck(false);
	_pb = pb;
	assert(_deck);
}

MediaStreamPlayer::~MediaStreamPlayer() {
}

ref<Plugin> MediaStreamPlayer::GetPlugin() {
	return _plugin;
}

void MediaStreamPlayer::Message(ref<DataReader> msg, ref<Playback> playback) {
	unsigned int pos = 0;
	MediaAction ac = (MediaAction)msg->Get<unsigned char>(pos);

	if(ac==MediaActionSetScreen) {
		PatchIdentifier screen = msg->Get<PatchIdentifier>(pos);
		_deck = _pb->CreateDeck(false, _pb->GetDeviceByPatch(screen));
	}
	if(ac==MediaActionStart) {
		int song_offset = msg->Get<int>(pos);
		float volume = msg->Get<float>(pos);
		float opacity = msg->Get<float>(pos);
		PatchIdentifier card = msg->Get<PatchIdentifier>(pos);
		Vector translate = msg->Get<Vector>(pos);
		Vector rotate = msg->Get<Vector>(pos);
		Vector scale = msg->Get<Vector>(pos);
		
		std::wstring fn;
		if(msg->GetSize()>pos) {
			fn = msg->Get<std::wstring>(pos);
		}

		PatchIdentifier live;
		if(msg->GetSize()>pos) {
			live = msg->Get<PatchIdentifier>(pos);
		}

		ref<Device> liveDevice = playback->GetDeviceByPatch(live);
		if(liveDevice) {
			_deck->Stop();
			_deck->LoadLive(liveDevice, playback->GetDeviceByPatch(card));
			_deck->SetVisible(true);
		}
		else {
			std::wstring path;
			if(playback->GetResources()->GetPathToLocalResource(fn,path)) {
				if(_deck->GetFileName()!=path) {
					_deck->Stop();

					if(fn.length()>0 && !_deck->IsLoaded()) {
						_deck->Load(path, playback->GetDeviceByPatch(card));
						_deck->SetVisible(true);
					}
				}
			}
		}

		_deck->SetVolume(volume);
		_deck->SetMixAlpha(L"tjmedia-player", opacity);
		_deck->SetTranslate(translate);
		_deck->SetRotate(rotate);
		_deck->SetScale(scale);
		
		if(song_offset>-1) {
			_deck->Play();
			if(song_offset>0) {
				_deck->Jump(song_offset);
			}
		}
	}
	else if(ac==MediaActionStop) {
		_deck->Pause();
		_deck->SetVisible(false);
	}
	else if(ac==MediaActionJump) {
		int t = msg->Get<int>(pos);
		_deck->Jump(t);
	}
	else if(ac==MediaActionVolume) {
		float v = msg->Get<float>(pos);
		float o = msg->Get<float>(pos);
		Vector translate = msg->Get<Vector>(pos);
		Vector rotate = msg->Get<Vector>(pos);
		Vector scale = msg->Get<Vector>(pos);

		_deck->SetVolume(v);
		_deck->SetMixAlpha(L"tjmedia-player", o);
		_deck->SetTranslate(translate);
		_deck->SetRotate(rotate);
		_deck->SetScale(scale);
	}
	else if(ac==MediaActionKeyingParameters) {
		bool enabled = msg->Get<bool>(pos);
		if(enabled) {
			RGBColor color = msg->Get<RGBColor>(pos);
			float tolerance = msg->Get<float>(pos);
			_deck->SetKeyColor(color);
			_deck->SetKeyingTolerance(tolerance);
		}
		_deck->SetKeyingEnabled(enabled);
	}
	else if(ac==MediaActionMasterValues) {
		while(msg->GetSize()>pos) {
			master::MediaMaster::Flavour fl = (master::MediaMaster::Flavour)msg->Get<int>(pos);
			float fv = msg->Get<float>(pos);

			switch(fl) {
				case master::MediaMaster::FlavourVolume:
					_deck->SetMixVolume(L"media-masters", fv);
					break;

				case master::MediaMaster::FlavourAlpha:
					_deck->SetMixAlpha(L"media-masters", fv);
					break;

				default:
					_deck->SetMixEffect(L"media-masters", (Mesh::EffectParameter)(fl - master::MediaMaster::FlavourEffect), fv);
					break;

			}
		}
	}
	else if(ac==MediaActionSpeed) {
		float s = msg->Get<float>(pos);
		_deck->SetPlaybackSpeed(s);
	}
	else if(ac==MediaActionLiveParameters) {
		float v = msg->Get<float>(pos);
		float o = msg->Get<float>(pos);

		_deck->SetMixAlpha(L"live", o);
		_deck->SetMixVolume(L"live", v);
	}
}