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
#include "../include/internal/tjcontroller.h"
#include "../include/internal/tjinstancer.h"
#include "../include/extra/tjcuetrack.h"
#include "../include/internal/view/tjtimelinewnd.h" // for playback state icons (PlaybackStateIcons)
using namespace tj::shared;
using namespace tj::show;
using namespace tj::show::instancer;

namespace tj {
	namespace show {
		namespace instancer {
			class InstancerCue: public virtual Object, public Serializable, public virtual Inspectable {
				friend class InstancerPlayer;

				public:
					InstancerCue(const Time& pos = Time(0));
					virtual ~InstancerCue();
					virtual void Load(TiXmlElement* you);
					virtual void Save(TiXmlElement* me);
					virtual void Move(Time t, int h);
					virtual ref<InstancerCue> Clone();
					virtual ref<PropertySet> GetProperties(ref<Playback> pb, strong<CueTrack<InstancerCue> > track);
					virtual ref<PropertySet> GetProperties();
					Time GetTime() const;
					void SetTime(Time t);
					virtual void Paint(tj::shared::graphics::Graphics* g, ref<Theme> theme, float pixelLeft, unsigned int y, unsigned int h, ref< CueTrack<InstancerCue> > track, bool focus);
					
				protected:
					CueIdentifier _cid;
					Time _time;
			};

			class InstancerTrack: public CueTrack<InstancerCue>, public Instancer {
				friend class InstancerCue;
				friend class InstancerPlayer;
				friend class InstancerTimelineProperty;

				public:
					InstancerTrack(ref<Playback> pb, ref<Model> model, ref<Instances> inst);
					virtual ~InstancerTrack();
					virtual std::wstring GetTypeName() const;
					virtual ref<Player> CreatePlayer(ref<Stream> str);
					virtual Flags<RunMode> GetSupportedRunModes();
					virtual ref<PropertySet> GetProperties();
					virtual ref<LiveControl> CreateControl(ref<Stream> str);
					virtual void Save(TiXmlElement* parent);
					virtual void Load(TiXmlElement* you);
					virtual void CreateOutlets(OutletFactory& of);
					virtual std::wstring GetEmptyHintText() const;
					virtual void GetResources(std::vector<tj::shared::ResourceIdentifier>& rids);
					virtual void Clone();
					virtual bool IsExpandable();
					virtual void SetExpanded(bool e);
					virtual bool IsExpanded();
					virtual Pixels GetHeight();

					virtual ref<Timeline> GetTimeline();
					virtual void SetTimeline(const TimelineIdentifier& tlid);

				protected:
					CriticalSection _lock;
					ref<Playback> _pb;
					weak<Model> _model;
					weak<Instances> _instances;
					TimelineIdentifier _tlid;
					std::map< std::wstring, std::wstring > _marshalIn;
					std::map< std::wstring, ref<Outlet> > _marshalOut;
					Icon _targetIcon;
					bool _expanded;
					volatile int _playerCount;

					ref<Timeline> _cachedTimeline;
					TimelineIdentifier _cachedFor;
			};

			class InstancerPlayer: public Player, public InstanceHolder, public Listener<Variables::ChangedVariables> {
				public:
					InstancerPlayer(strong<InstancerTrack> tr, ref<Stream> str);
					virtual ~InstancerPlayer();

					ref<Track> GetTrack();
					virtual void Stop();
					virtual void Start(Time pos, ref<Playback> playback, float speed);
					virtual void Tick(Time currentPosition);
					virtual void Jump(Time newT, bool paused);
					virtual void SetOutput(bool enable);
					virtual Time GetNextEvent(Time t);
					virtual void SetPlaybackSpeed(Time t, float c);
					virtual ref<Instance> GetInstance();
					virtual void Notify(ref<Object> source, const Variables::ChangedVariables& data);
					virtual void Pause(Time pos);

				private:
					void MarshalVariables(strong<VariableList> locals, bool initial);
					strong<InstancerTrack> _track;
					ref<Stream> _stream;
					ref<Playback> _pb;
					ref<Instance> _controller;
					Time _last;
					PlaybackState _stateBeforePause;
					bool _isPaused;
			};

			class InstancerTimelineProperty: public LinkProperty {
				public:
					class TimelineMenuItem: public MenuItem {
						public:
							TimelineMenuItem(strong<Timeline> tl, bool current, int level, const std::wstring& name): MenuItem(name, 0, false, current ? MenuItem::Checked : MenuItem::NotChecked), _tl(tl) {
								SetIndent(level);
							}

							virtual ~TimelineMenuItem() {
							}

							strong<Timeline> _tl;
					};

					InstancerTimelineProperty(const std::wstring& name, ref<InstancerTrack> track): LinkProperty(name, L"", L"icons/timeline.png"), _track(track) {
						ref<Timeline> current = _track->GetTimeline();
						SetText(current ? current->GetName() : TL(instancer_no_timeline));
					}

					virtual ~InstancerTimelineProperty() {
					}

					virtual void OnClicked() {
						if(_track->_playerCount > 0) {
							Alert::Show(TL(instancer_name), TL(instancer_cannot_change_timeline), Alert::TypeWarning);
						}
						else {
							ref<Model> model = _track->_model;
							if(model) {
								// TODO: change this to use Model and the 'root timeline' instead of the root instance
								strong<Instance> root = Application::Instance()->GetInstances()->GetRootInstance();
								ContextMenu cm;
								Populate(cm, root->GetTimeline(), 0);
								
								ref<Wnd> lpw = GetWindow();
								Area lpwRC = lpw->GetClientArea();
								ref<MenuItem> res = cm.DoContextMenuByItem(lpw, lpwRC.GetLeft(), lpwRC.GetBottom());
								if(res && res.IsCastableTo<TimelineMenuItem>()) {
									ref<TimelineMenuItem> tlm = res;
									if(tlm) {
										TimelineIdentifier tlid = tlm->_tl->GetID();
										if(tlid!=_track->_tlid) {
											if(_track->_tlid==L"" || Alert::ShowOKCancel(TL(instancer_name), TL(instancer_change_timeline_warning), Alert::TypeWarning)) {
												std::wstring name = tlm->_tl->GetName();
												if(name.length()<1) {
													name = TL(timeline);
												}
												GetWindow()->SetText(name.c_str());
												_track->SetTimeline(tlid);
											}
										}
									}
								}
							}
						}
					}

					void Populate(ContextMenu& cm, strong<Timeline> tl, int level) {
						if(!tl->IsSingleton()) {
							std::wstring name = tl->GetName();
							if(name.length()<1) {
								name = TL(timeline);
							}
							cm.AddItem(GC::Hold(new TimelineMenuItem(tl, _track->_tlid==tl->GetID(), level, name)));
						}

						ref< Iterator< ref<TrackWrapper> > > it = tl->GetTracks();
						while(it && it->IsValid()) {
							ref<TrackWrapper> tw = it->Get();
							if(tw) {
								ref<SubTimeline> sub = tw->GetSubTimeline();
								if(sub) {
									Populate(cm, sub->GetTimeline(), level+1);
								}
							}
							it->Next();
						}
					}

				private:
					ref<InstancerTrack> _track;
			};
		}
	}
}

/** InstancerPlayer **/
InstancerPlayer::InstancerPlayer(strong<InstancerTrack> tr, ref<Stream> str): _track(tr), _stream(str), _last(-1), _stateBeforePause(PlaybackAny), _isPaused(false) {
}

InstancerPlayer::~InstancerPlayer() {
}

ref<Track> InstancerPlayer::GetTrack() {
	return ref<InstancerTrack>(_track);
}

void InstancerPlayer::Stop() {
	--_track->_playerCount;
	if(_controller) {
		_controller->SetPlaybackStateRecursive(PlaybackStop);
		strong<VariableList> locals = _controller->GetLocalVariables();
		locals->EventVariablesChanged.RemoveListener(this);
	}
	_stream = null;
	_controller = null;
	_stateBeforePause = PlaybackAny;
	_isPaused = false;
}

void InstancerPlayer::Pause(Time pos) {
	if(_controller) {
		_isPaused = true;
		_stateBeforePause = _controller->GetPlaybackState();
		if(_stateBeforePause==PlaybackWaiting || _stateBeforePause==PlaybackPlay) {
			_controller->SetPlaybackState(PlaybackPause);
		}
	}
}

void InstancerPlayer::Notify(ref<Object> source, const Variables::ChangedVariables& data) {
	if(_pb) {
		std::set< strong<Variable> >::const_iterator it = data.which.begin();
		while(it!=data.which.end()) {
			strong<Variable> var = *it;
			if(var->IsOutput()) {
				ThreadLock lock(&(_track->_lock));
				std::map<std::wstring, ref<Outlet> >::iterator mit = _track->_marshalOut.find(var->GetID());
				if(mit!=_track->_marshalOut.end() && mit->second) {
					_pb->SetOutletValue(mit->second, var->GetValue());
				}
			}
			++it;
		}
	}
}

void InstancerPlayer::MarshalVariables(strong<VariableList> vl, bool initial) {
	ThreadLock lock(&(_track->_lock));
	std::map<std::wstring, std::wstring>::const_iterator it = _track->_marshalIn.begin();
	while(it!=_track->_marshalIn.end()) {
		ref<Variable> var = vl->GetVariableById(it->first);
		if(var && var->IsInput()) {
			if(initial) {
				var->SetInitialValue(it->second);
			}
			else {
				vl->Set(var, Any(var->GetType(), it->second));
			}
		}
		++it;
	}
}

void InstancerPlayer::Start(Time pos, ref<Playback> playback, float speed) {
	++_track->_playerCount;
	_last = pos;
	_pb = playback;
	ref<Timeline> tl = _track->GetTimeline();
	ref<Model> model = _track->_model;
	ref<Instances> instances = _track->_instances;
	_isPaused = false;
	_stateBeforePause = PlaybackAny;

	if(tl && model && instances) {
		if(tl->IsSingleton()) {
			Throw(L"An attempt was made by an instancer track to instantiate a singleton timeline", ExceptionTypeError);
		}
		/* 'Clone' the local variables; we look up the main controller for this timeline, and then get its local variables.
		Then, we create a new VariableList, which will be filled with shared variables (reference simply copies from the
		original local list) and instance variables (which we 'clone') */
		strong<VariableList> localVariables = tl->GetLocalVariables()->CreateInstanceClone();

		/* Overwrite variable initial values with our own 'parameters'. We cannot set the variable values directly, since
		when SetPlaybackState is called on the child controller (_controller), it will call ResetAll on the variable list.
		So instead, we overwrite the initial value (and only overwrite if we have some value for it and if the variable
		indicates that it is an 'input' variable) */
		MarshalVariables(localVariables, true);
		localVariables->EventVariablesChanged.AddListener(this);

		strong<Variables> globalVariables = model->GetVariables();
		_controller = GC::Hold(new Controller(tl, Application::Instance()->GetNetwork(), localVariables, globalVariables, true));
	}
}

void InstancerPlayer::Tick(Time currentPosition) {
	if(_controller) {
		ref<Timeline> tl = _controller->GetTimeline();
		if(tl) {
			// If we're resuming from pause, then unpause the timeline if it was playing
			if(_isPaused) {
				_isPaused = false;
				if(_stateBeforePause==PlaybackPlay) {
					_controller->SetPlaybackState(PlaybackPlay);
				}
				// Don't change anything when state was PlaybackWaiting (it will be paused), PlaybackPaused (still paused) or PlaybackStop (still stopped)
			}

			// Process cues
			strong<CueList> cuelist = tl->GetCueList();
			std::vector< ref<InstancerCue> > cues;
			_track->GetCuesBetween(_last, currentPosition+Time(1), cues);
			std::vector< ref<InstancerCue> >::iterator it = cues.begin();
			while(it!=cues.end()) {
				ref<InstancerCue> ic = *it;
				if(ic) {
					// fire cue
					ref<Cue> cue = cuelist->GetCueByID(ic->_cid);
					if(cue) {
						_controller->Trigger(cue, true); // Will throw when cue is marked private
					}
				}
				++it;
			}
		}
	}
	_last = currentPosition;
}

void InstancerPlayer::Jump(Time newT, bool paused) {
	_last = newT;
}

void InstancerPlayer::SetOutput(bool enable) {
	// TODO implement (through some Controller::SetOutput method which mutes all active players? or just pause and do not trigger anything?)
}

void InstancerPlayer::SetPlaybackSpeed(Time t, float c) {
	/* TODO relay this to the controller, but only if the user indicated that speeds should
	be linked (in some cases this is desirable, in somes cases,	it is not) */
}

Time InstancerPlayer::GetNextEvent(Time t) {
	ref<InstancerCue> ic = _track->GetCueAfter(t);
	if(ic) {
		return ic->GetTime();
	}
	return Time(-1);
}

ref<Instance> InstancerPlayer::GetInstance() {
	return ref<Instance>(_controller);
}

/** InstancerCue **/
InstancerCue::InstancerCue(const Time& pos): _time(pos) {
}

InstancerCue::~InstancerCue() {
}

void InstancerCue::Load(TiXmlElement* you) {
	_time = LoadAttributeSmall(you, "time", _time);
	_cid = LoadAttributeSmall(you, "target", _cid);
}

void InstancerCue::Save(TiXmlElement* me) {
	TiXmlElement cue("cue");
	SaveAttributeSmall(&cue, "time", _time);
	SaveAttributeSmall(&cue, "target", _cid);
	me->InsertEndChild(cue);
}

void InstancerCue::Move(Time t, int h) {
	_time = t;
}

ref<InstancerCue> InstancerCue::Clone() {
	ref<InstancerCue> ic = GC::Hold(new InstancerCue(_time));
	ic->_cid = _cid;
	return ic;
}

ref<PropertySet> InstancerCue::GetProperties() {
	// The InstancerCue::GetProperties is never called; the CueTrackItem<InstancerCue>::GetProperties will call the
	// GetProperties(ref<Playback>,strong<CueTrack<InstancerCue> >) method instead. However, because GetProperties(...)
	// returns properties, InstancerCue needs to be Inspectable (since otherwise it cannot set the holder of the returned
	// properties). Thus, InstancerCue has to implement GetProperties(). Just never ever call it.
	Throw(L"GetProperties called on InstancerCue; shouldn't happen!", ExceptionTypeError);
}

ref<PropertySet> InstancerCue::GetProperties(ref<Playback> pb, strong<CueTrack<InstancerCue> > tr) {
	ref<PropertySet> ps = GC::Hold(new PropertySet());
	ps->Add(GC::Hold(new GenericProperty<Time>(TL(instancer_cue_time), this, &_time, _time)));

	// Cue selection property
	ref<InstancerTrack> itr = ref<CueTrack<InstancerCue>>(tr);
	if(itr) {
		ref<Timeline> tl = itr->GetTimeline();
		if(tl) {
			strong<CueList> cues = tl->GetCueList();
			ref< GenericListProperty<CueIdentifier> > cil = GC::Hold(new GenericListProperty< CueIdentifier >(TL(instancer_target_cue), this, &_cid, _cid));
			cil->AddOption(TL(instancer_cue_none), L"");
			std::vector< ref<Cue> >::iterator it = cues->GetCuesBegin();
			while(it!=cues->GetCuesEnd()) {
				ref<Cue> cue = *it;
				if(cue && !cue->IsPrivate()) {
					std::wstring name = cue->GetName();
					if(name.length()>0) {
						cil->AddOption(cue->GetName(), cue->GetID());
					}
				}
				++it;
			}
			ps->Add(cil);
		}
	}

	return ps;
}

Time InstancerCue::GetTime() const {
	return _time;
}

void InstancerCue::SetTime(Time t) {
	_time = t;
}

void InstancerCue::Paint(tj::shared::graphics::Graphics* g, ref<Theme> theme, float pixelLeft, unsigned int y, unsigned int h, ref< CueTrack<InstancerCue> > track, bool focus) {
	graphics::Pen pn(theme->GetColor(Theme::ColorCurrentPosition), focus? 2.0f : 1.0f);
	graphics::SolidBrush cueBrush = theme->GetColor(Theme::ColorCurrentPosition);

	ref<InstancerTrack> itr = ref< CueTrack<InstancerCue> >(track);
	if(itr) {
		Area iconRC(Pixels(pixelLeft-8), y, 16, 16);

		if(focus) {
			Area focusRC(iconRC);
			focusRC.Narrow(2,2,3,3);
			theme->DrawFocusEllipse(*g, focusRC);
		}

		itr->_targetIcon.Paint(*g, iconRC, true);

		graphics::StringFormat sf;
		sf.SetAlignment(graphics::StringAlignmentNear);

		std::wstring str = L"??";
	
		ref<Timeline> tl = itr->GetTimeline();
		if(tl) {
			strong<CueList> cues = tl->GetCueList();
			ref<Cue> cue = cues->GetCueByID(_cid);
			if(cue) {
				str = cue->GetName();
			}
		}

		if(track->IsExpanded()) {
			graphics::GraphicsContainer gc = g->BeginContainer();
			g->ResetClip();
			g->TranslateTransform(float(pixelLeft+(16/2)), float(y)+16);
			g->RotateTransform(90.0f);
			
			g->DrawString(str.c_str(), (INT)str.length(), theme->GetGUIFontSmall(), graphics::PointF(0.0f, 0.0f), &sf, &cueBrush);
			g->EndContainer(gc);
		}
		else {
			g->DrawString(str.c_str(), (INT)str.length(), theme->GetGUIFontSmall(), graphics::PointF(float(iconRC.GetRight()), float(iconRC.GetTop()+3)), &sf, &cueBrush);
		}
	}
}

/** InstancerTrack **/
InstancerTrack::InstancerTrack(ref<Playback> pb, ref<Model> model, ref<Instances> inst): _pb(pb), _model(model), _instances(inst), _targetIcon(L"icons/target.png"), _expanded(false), _playerCount(0) {
}

InstancerTrack::~InstancerTrack() {
}

std::wstring InstancerTrack::GetTypeName() const {
	return TL(instancer_name);
}

ref<Player> InstancerTrack::CreatePlayer(ref<Stream> str) {
	return GC::Hold(new InstancerPlayer(ref<InstancerTrack>(this), str));
}

Flags<RunMode> InstancerTrack::GetSupportedRunModes() {
	Flags<RunMode> rm;
	rm.Set(RunModeMaster, true);
	return rm;
}

Pixels InstancerTrack::GetHeight() {
	if(_expanded) {
		return 100;
	}
	return CueTrack<InstancerCue>::GetHeight();
}

ref<PropertySet> InstancerTrack::GetProperties() {
	ref<PropertySet> ps = GC::Hold(new PropertySet());
	ps->Add(GC::Hold(new InstancerTimelineProperty(TL(instancer_timeline), this)));

	ref<Timeline> tl = GetTimeline();
	if(tl) {
		bool first = true;
		strong<VariableList> locals = tl->GetLocalVariables();
		ThreadLock lock(&_lock);
		for(unsigned int i = 0; i < locals->GetVariableCount(); ++i) {
			ref<Variable> var = locals->GetVariableByIndex(i);
			if(var && var->IsInput()) {
				if(first) {
					ps->Add(GC::Hold(new PropertySeparator(TL(instancer_input_variables))));
					first = false;
				}
				ps->Add(_pb->CreateParsedVariableProperty(var->GetName(), this, &(_marshalIn[var->GetID()]), false));
			}
		}
	}
	return ps;
}

ref<Timeline> InstancerTrack::GetTimeline() {
	if(!_cachedTimeline || _cachedFor!=_tlid) {
		ref<Model> model = _model;
		if(model) {
			_cachedTimeline = model->GetTimelineByID(_tlid);
			if(_cachedTimeline) {
				_cachedFor = _tlid;
			}
			else {
				_cachedFor = L"";
			}
		}
	}

	return _cachedTimeline;
}

ref<LiveControl> InstancerTrack::CreateControl(ref<Stream> str) {
	return null;
}

void InstancerTrack::Save(TiXmlElement* parent) {
	CueTrack<InstancerCue>::Save(parent);
	SaveAttributeSmall(parent, "timeline", _tlid);

	if(_marshalIn.size()>0) {
		TiXmlElement parameters("parameters");
		ThreadLock lock(&_lock);
		std::map<std::wstring, std::wstring>::const_iterator it = _marshalIn.begin();
		while(it!=_marshalIn.end()) {
			TiXmlElement param("param");
			SaveAttributeSmall(&param, "id", it->first);
			SaveAttributeSmall(&param, "value", it->second);
			parameters.InsertEndChild(param);
			++it;
		}
		parent->InsertEndChild(parameters);
	}
}

void InstancerTrack::Load(TiXmlElement* you) {
	CueTrack<InstancerCue>::Load(you);
	_tlid = LoadAttributeSmall(you, "timeline", _tlid);

	TiXmlElement* parameters = you->FirstChildElement("parameters");
	if(parameters!=0) {
		ThreadLock lock(&_lock);
		TiXmlElement* param = parameters->FirstChildElement("param");
		while(param!=0) {
			std::wstring k = LoadAttributeSmall<std::wstring>(param, "id", L"");
			std::wstring v = LoadAttributeSmall<std::wstring>(param, "value", L"");
			_marshalIn[k] = v;
			param = param->NextSiblingElement("param");
		}
	}
}

void InstancerTrack::SetTimeline(const TimelineIdentifier& tlid) {
	if(tlid!=_tlid && _playerCount == 0) {
		_tlid = tlid;
		_marshalIn.clear();
		_marshalOut.clear();
		RemoveAllCues();
	}
}

void InstancerTrack::CreateOutlets(OutletFactory& of) {
	ref<Timeline> tl = GetTimeline();
	if(tl) {
		std::map<std::wstring, ref<Outlet> >newMarshalOut;
		strong<VariableList> vars = tl->GetLocalVariables();
		for(unsigned int a = 0; a < vars->GetVariableCount(); a++) {
			ref<Variable> var = vars->GetVariableByIndex(a);
			if(var /* && var->IsOutput() */) {
				const std::wstring& id = var->GetID();
				newMarshalOut[id] = of.CreateOutlet(id, var->GetName());
			}
		}
		_marshalOut = newMarshalOut;
	}
}

bool InstancerTrack::IsExpandable() {
	return true;
}

void InstancerTrack::SetExpanded(bool e) {
	_expanded = e;
}

bool InstancerTrack::IsExpanded() {
	return _expanded;
}

std::wstring InstancerTrack::GetEmptyHintText() const {
	return TL(instancer_no_cues);
}

void InstancerTrack::GetResources(std::vector< ResourceIdentifier>& rids) {
}

void InstancerTrack::Clone() {
}

/** InstancerPlugin **/
InstancerPlugin::InstancerPlugin() {
}

InstancerPlugin::~InstancerPlugin() {
}

std::wstring InstancerPlugin::GetName() const {
	return L"Instancer";
}

ref<Track> InstancerPlugin::CreateTrack(ref<Playback> pb) {
	return GC::Hold(new InstancerTrack(pb, Application::Instance()->GetModel(), Application::Instance()->GetInstances()));
}

ref<StreamPlayer> InstancerPlugin::CreateStreamPlayer(ref<Playback> pb, ref<Talkback> talk) {
	return null;
}

std::wstring InstancerPlugin::GetVersion() const {
	return L"";
}

std::wstring InstancerPlugin::GetAuthor() const {
	return L"";
}

std::wstring InstancerPlugin::GetFriendlyName() const {
	return TL(instancer_name);
}

std::wstring InstancerPlugin::GetFriendlyCategory() const {
	return L"";
}

std::wstring InstancerPlugin::GetDescription() const {
	return TL(instancer_description);
}