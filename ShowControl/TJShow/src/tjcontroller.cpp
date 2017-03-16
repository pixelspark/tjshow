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
#include "../include/internal/tjnetwork.h"
#include "../include/internal/engine/tjengine.h"
#include "../include/internal/engine/tjcuethread.h"
#include "../include/internal/tjstreams.h"

#include "../include/internal/view/tjcuelistwnd.h"
#include "../include/internal/view/tjsplittimelinewnd.h"
#include "../include/internal/view/tjoutputwnd.h"
#include "../include/internal/view/tjtimelinewnd.h"

#include <iomanip>

using namespace tj::show::view;
using namespace tj::show::engine;

/* ControllerPlayback is a facade for the OutputManager (application-wide Playback). It handles some
controller-specific functions, such as ParseVariables (which can use application-wide variables as well
as controller-local variables). */
class ControllerPlayback: public Playback {
	public:
		ControllerPlayback(ref<Controller> controller);
		virtual ~ControllerPlayback();
		virtual ref<Deck> CreateDeck(bool visible, ref<Device> screen);
		virtual ref<Surface> CreateSurface(int w, int h, bool visible, ref<Device> screen);
		virtual bool IsKeyingSupported(ref<Device> screen);
		virtual ref<Device> GetDeviceByPatch(const PatchIdentifier& t);
		virtual ref<tj::shared::Property> CreateSelectPatchProperty(const std::wstring& name, ref<Inspectable> holder, PatchIdentifier* pi);
		virtual std::wstring ParseVariables(const std::wstring& source);
		virtual void QueueDownload(const std::wstring& rid);
		virtual strong<ResourceProvider> GetResources();
		virtual strong<Property> CreateParsedVariableProperty(const std::wstring& name, ref<Inspectable> holder, std::wstring* source, bool ml);
		virtual void SetOutletValue(ref<Outlet> outlet, const Any& value);
		virtual bool IsFeatureAvailable(const std::wstring& ft);

	private:
		weak<Controller> _controller;
};

Controller::Controller(strong<Timeline> time, strong<Network> net, strong<VariableList> locals, strong<Variables> globals, bool isInstance): 
	_lastStartTime(0), 
	_lastStopTime(0),
	_variables(locals),
	_net(net),
	_time(time),
	_state(PlaybackStop),
	_startTime(0),
	_startTicks(0),
	_scopedVariables(GC::Hold(new ScopedVariables(locals, globals))),
	_engine(Engines::CreateEngineOfType(Engines::TypeThreadPooled)),
	_isInstance(isInstance) {
	
	SetTimeline(time);
	_scope = GC::Hold(new ScriptScope());
}

Controller::~Controller() {
}

void Controller::OnCreated() {
	_time->GetCueList()->SetStaticInstance(this);
	_playback = GC::Hold(new ControllerPlayback(this));
}

void Controller::Fire() {
	ThreadLock lock(&_lock);

	Time current = GetTime();
	ref<Cue> next = GetCueList()->GetNextCue(current);
	if(next) {
		Trigger(next,false);
		
		Cue::Action action = next->GetAction();
		if(action==Cue::ActionNone && _state!=PlaybackWaiting) {
			// Play anyway if the action is 'do nothing' (at least, if we're not waiting)
			SetPlaybackState(PlaybackPlay);
		}
	}

	UpdateUI();
}

void Controller::SetTimeline(strong<Timeline> t) {
	if(_state!=PlaybackStop) {
		Throw(L"Cannot change timeline while being paused or playing!", ExceptionTypeError);
	}

	_time = t;
	_speed = _time->GetDefaultSpeed();
}

void Controller::SetCueList(strong<CueList> cues) {
	if(_state!=PlaybackStop) {
		Throw(L"Cannot change cue list while being paused or playing!", ExceptionTypeError);
	}

	// This replicates what OnCreated does with the CueList with which this controller is constructed
	strong<CueList> currentCueList = _time->GetCueList();
	currentCueList->SetStaticInstance(null);
	_time->SetCueList(cues);
	cues->SetStaticInstance(this);
}

strong<VariableList> Controller::GetLocalVariables() {
	return _variables;
}

strong<Variables> Controller::GetVariables() {
	return _scopedVariables; // this is local+global
}

ref<Instance> Controller::GetChildInstance(strong<Instancer> creator) {
	PlaybackState pb = GetPlaybackState();
	if(pb==PlaybackStop) return null; // We have no players

	// Only tracks can instantiate, so cast creator to a Track and see if we have a player
	// for that track. If that's the case, the Player must be an Instance
	ref<Instancer> ci = creator;
	if(ci.IsCastableTo<Track>()) {
		ref<Track> track = ci;
		if(track) {
			std::map< strong<Track>, ref<Player> >::iterator it = _activePlayers.find(strong<Track>(track));
			if(it!=_activePlayers.end()) {
				ref<Player> player = it->second;
				if(player && ref<Player>(player).IsCastableTo<InstanceHolder>()) {
					ref<InstanceHolder> ih = ref<Player>(player);
					if(ih) {
						return ih->GetInstance();
					}
				}
			}
		}
	}

	return null;
}

ref<Instance> Controller::GetParentOf(ref<TrackWrapper> tw) {
	ref<Iterator< ref<TrackWrapper> > > it = _time->GetTracks();
	while(it->IsValid()) {
		ref<TrackWrapper> ct = it->Get();
		ref<SubTimeline> sub = ct->GetSubTimeline();
		if(sub) {
			ref<Instance> tcrl = sub->GetInstance();
			ref<Instance> parent = tcrl->GetParentOf(tw);
			if(parent) {
				return parent;
			}
		}
		else if(ct==tw) {
			return this;
		}
		it->Next();
	}

	return 0;
}

ref<Acquisition> Controller::GetAcquisitionFor(ref<Capacity> c) {
	ThreadLock lock(&_lock);

	std::map< ref<Capacity> , ref<Acquisition> >::iterator it = _acquisitions.find(c);
	if(it!=_acquisitions.end()) {
		return it->second;
	}

	ref<Acquisition> acq = c->CreateAcquisition();
	_acquisitions[c] = acq;
	return acq;
}

void Controller::Clear() {
	ThreadLock lock(&_lock);
	if(_state!=PlaybackStop) {
		Throw(L"Controller::Clear called when controller state wasn't PlaybackStop - not supported!" ,ExceptionTypeError);
	}
	_time->Clear();
	_variables->Clear();
	_scope = GC::Hold(new ScriptScope());
	_startTime = 0;
	_startTicks = 0;
	_speed = 1.0f;
}

void Controller::GetChildInstances(std::vector< ref<Instance> >& list) {
	ref<Iterator< ref<TrackWrapper> > > it = _time->GetTracks();
	while(it->IsValid()) {
		ref<TrackWrapper> tw = it->Get();
		ref<SubTimeline> sub = tw->GetSubTimeline();
		if(sub) {
			ref<Instance> tcrl = sub->GetInstance();
			if(tcrl) {
				list.push_back(tcrl);
			}
		}
		it->Next();
	}
}

bool Controller::IsPlayingOrPausedRecursive() {
	if(_state!=PlaybackStop) {
		return true;
	}

	ref<Iterator< ref<TrackWrapper> > > it = _time->GetTracks();
	while(it->IsValid()) {
		ref<TrackWrapper> tw = it->Get();
		ref<SubTimeline> sub = tw->GetSubTimeline();
		if(sub) {
			ref<Instance> tcrl = sub->GetInstance();
			if(tcrl->IsPlayingOrPausedRecursive()) {
				return true;
			}
		}
		it->Next();
	}
	return false;
}

int Controller::GetPlayerCount() const {
	return (int)_activePlayers.size();
}

ref<TimelineWnd> Controller::GetTimelineWindow() {
	ref<SplitTimelineWnd> spw = _timelineWnd;
	if(spw) {
		return spw->GetTimelineWindow();
	}
	return 0;
}

void Controller::SetPlaybackSpeed(float c) {
	ThreadLock lock(&_lock);

	if(c<=0.0f) {
		_state = PlaybackPause;
		if(_cueThread) {
			_cueThread->Pause(GetTime(), _startTicks, true);
		}
	}
	else {
		_speed = c;
		_startTime = GetTime();
		_startTicks.Now();

		if(_cueThread) {
			_cueThread->SetPlaybackSpeed(_startTime, _startTicks, c);
		}

		_engine->SetTickingSpeed(c, _startTime, _startTicks);
		_time->SetDefaultSpeed(c);
	}
}

float Controller::GetPlaybackSpeed() const {
	return _speed;
}

ref<SplitTimelineWnd> Controller::GetSplitTimelineWindow() {
	return _timelineWnd;
}

// Can be called from any thread, but take care: this should not do any GUI actions
// except asynchronously sending messages, or it will deadlock with the main thread!
// TODO: maybe do this with an Event/Listener on which the views register? Much cleaner...
void Controller::UpdateUI() {
	ref<SplitTimelineWnd> spw = _timelineWnd;
	if(spw) {
		spw->GetTimelineWindow()->UpdateThreaded();
		spw->GetOutputWindow()->Repaint();
		spw->GetCueWindow()->Update();

		// update tab window so icon is shown correctly
		Wnd* spp = spw->GetParent();
		if(spp!=0) {
			spp->Repaint();
		}
	}
}

void Controller::Timer() {
	if(GetPlaybackState()==PlaybackPlay) {
		UpdateUI();
	}

	// Update child instances
	ref< Iterator<ref<TrackWrapper> > > it = _time->GetTracks();
	while(it->IsValid()) {
		ref<TrackWrapper> tw = it->Get();
		if(tw) {
			ref<SubTimeline> sub = tw->GetSubTimeline();
			if(sub) {
				ref<Instance> inst = sub->GetInstance();
				if(inst && inst.IsCastableTo<Controller>()) {
					ref<Controller>(inst)->Timer();
				}
			}
		}
		it->Next();
	}
}

void Controller::SetSplitTimelineWindow(ref<SplitTimelineWnd> wnd) {
	_timelineWnd = wnd;
}

ref<ScriptScope> Controller::GetScope() {
	return _scope;
}

void Controller::Trigger(ref<Cue> cue, bool denyIfPrivate) {
	if(denyIfPrivate && cue->IsPrivate()) Throw(L"You cannot trigger a private cue!", ExceptionTypeError);
	
	Time cueTime = cue->GetTime();
	if(cueTime > _time->GetTimeLengthMS()) Throw(L"You cannot trigger a cue that is off the timeline (cue time is larger than timeline length)!", ExceptionTypeError);
	if(cueTime < Time(0)) Throw(L"Cannot trigger a cue which has a negative cue time", ExceptionTypeError);

	ThreadLock lock(&_lock);
	Timestamp ts;

	if(_state==PlaybackStop) {
		// Just change the time and go play!
		SetTime(cueTime, ts);
		cue->DoAction(Application::Instance(), this);
	}
	else {
		Jump(cueTime); // Cuethread won't pick up the destination cue; Jump cancels any waiting locks
		cue->DoAction(Application::Instance(), this);
	}
}

void Controller::Jump(const Time& t) {
	if(t.ToInt()<0) return;
	if(t>_time->GetTimeLengthMS()) return;

	{
		ThreadLock lock(&_lock);
		
		// If we are jumping somewhere, we do not have to wait for any lock anymore, so cancel waiting acquisitions
		if(_state==PlaybackWaiting) {
			_waitingExpression = 0;

			std::map< ref<Capacity>, ref<Acquisition> >::iterator it = _acquisitions.begin();
			while(it!=_acquisitions.end()) {
				ref<Acquisition> acq = it->second;
				if(acq) {
					acq->Cancel();
				}
				++it;
			}
			SetPlaybackState(PlaybackPause);
		}

		// Set the time
		_startTime = t;
		_startTicks.Now();

		// Notify cue thread
		if(_cueThread) {
			_cueThread->Jump(t, _startTicks);
		}

		// Notify players
		_engine->Jump(t, _startTicks, (_state!=PlaybackPlay));	
	}
	 UpdateUI();
}

void Controller::SetTime(Time t, const Timestamp& ticks) {
	_startTime = t;
	_startTicks = ticks;
}

ref<TrackWrapper> Controller::GetTrackByChannel(const Channel& ch) {
	std::map<Channel, ref<TrackWrapper> >::iterator it = _tracksByChannel.find(ch);
	if(it!=_tracksByChannel.end()) {
		return it->second;
	}
	return null;
}

void Controller::SetPlaybackState(PlaybackState pb) {
	ThreadLock lock(&_lock);

	// Don't bother changing anything if we're already in the right state
	if(pb==_state) return;

	// Going to 'pause' when already in pause means play again ('unpause')
	if(pb==PlaybackPause && (_state == PlaybackPause || _state==PlaybackWaiting)) {
		pb = PlaybackPlay;
	}

	// From any state to 'play' or from stop => pause or from stop=>wait
	if(pb==PlaybackPlay || (pb==PlaybackPause && _state==PlaybackStop) || (pb==PlaybackWaiting && _state==PlaybackStop)) {
		if(_state==PlaybackStop) {
			_cueThread = GC::Hold(new CueThread(_time->GetCueList(), this, _speed));
			_variables->ResetAll();
		}

		// If we were paused, we are resuming here
		if(_state==PlaybackPause || _state==PlaybackWaiting) {
			// Set the time
			_startTicks.Now();
			_startTime = _startTime + Time(1);

			if(_startTime >= _time->GetTimeLengthMS()) {
				// Cannot unpause when on the end of the timeline
				Log::Write(L"TJShow/Controller", L"Unpause/start time is past the end of the timeline; not starting");
				return; 
			}

			// Cancel waiting for condition
			_waitingExpression = 0;

			// cancel acquisitions that are waiting
			if(_state==PlaybackWaiting) {
				std::map< ref<Capacity>, ref<Acquisition> >::iterator it = _acquisitions.begin();
				while(it!=_acquisitions.end()) {
					ref<Acquisition> acq = it->second;
					if(acq) {
						acq->Cancel();
					}
					++it;
				}
			}

			// Tell all players to unpause
			_engine->PauseTicking(false, _startTime, _startTicks);

			if(_cueThread) {
				_cueThread->Pause(_startTime, _startTicks, false);
			}
		}
		else {
			// Cannot start playing when startTime is off the timeline
			if(_startTime >= _time->GetTimeLengthMS()) {
				Log::Write(L"TJShow/Controller", L"Start time is off the timeline; not starting");
				return;
			}

			/* Get a list of currently active channels, so we can check if all tracks are 'connected' to at least
			one client */
			std::set<GroupID> presentGroups;
			std::list< ref<TrackWrapper> > tracksWithoutClients;

			bool doClientCheck = Application::Instance()->GetSettings()->GetFlag(L"warnings.no-addressed-client");
			if(doClientCheck) {
				presentGroups = Application::Instance()->GetNetwork()->GetPresentGroups();
			}

			{
				FILETIME ft;
				SYSTEMTIME st;
				GetLocalTime(&st);
				SystemTimeToFileTime(&st,&ft);
				ULARGE_INTEGER value;
				value.HighPart = ft.dwHighDateTime;
				value.LowPart = ft.dwLowDateTime;
				_lastStartTime = value.QuadPart;
				_lastStopTime = 0;
			}
			
			ref< Iterator< ref<TrackWrapper> > > it = _time->GetTracks();
			while(it->IsValid()) {
				ref<TrackWrapper> trackWrapper = it->Get();
				strong<Track> track = trackWrapper->GetTrack();

				// Don't play at all if neither the master or clients have to play this
				RunMode runMode = trackWrapper->GetRunMode();
				if(runMode==RunModeDont) {
					it->Next();
					continue;
				}

				// If this track is to be played on a client, check if such a client exists
				if(doClientCheck && (runMode==RunModeClient||runMode==RunModeBoth) && trackWrapper->GetGroup()!=0 && presentGroups.find(trackWrapper->GetGroup())==presentGroups.end()) {
					tracksWithoutClients.push_back(trackWrapper);
				}

				try {
					ref<Player> player;
					if(runMode==RunModeBoth||runMode==RunModeClient) {
						/** For now, all tracks use their 'main channel' (channel for first instance). As soon as we start supporting
						instancing, we should allocate (or let TrackStream allocate) a separate channel number if the track is played
						as an instance (which should then be a flag in the Controller, something like '_isInstance'. **/
						ref<TrackStream> stream = GC::Hold(new TrackStream(trackWrapper, _net, ref<Instance>(this), _isInstance));
						_tracksByChannel[stream->GetChannel()] = trackWrapper;
						player = track->CreatePlayer(stream);
					}
					else {
						player = track->CreatePlayer(0);
					}

					if(player) {
						if(runMode==RunModeClient) {
							player->SetOutput(false);
						}
						else {
							player->SetOutput(true);
						}

						player->Start(_startTime, _playback, _speed);
						_engine->AddPlayer(player, trackWrapper);
						_activePlayers[track] = player;
					}
				}
				catch(Exception& e) {
					ref<EventLogger> logger = Application::Instance()->GetEventLogger();
					logger->AddEvent(e.ToString(), e.GetType(), false);
				}
				catch(...) {
					Log::Write(TL(application_name), L"Exception occurred while starting players. Could give GC error");
				}
				it->Next();
			}

			_startTicks.Now();
			unsigned int i = 0;

			if(pb==PlaybackPlay) {
				_engine->StartTicking(_startTime, _startTicks, _playback, _speed);
				_cueThread->Start(_startTime, _startTicks, _speed, _playback, null);
			}
			else if(pb==PlaybackPause || pb==PlaybackWaiting) {
				_engine->StartPaused(_startTime, _startTicks, _playback, _speed);
				_cueThread->StartPaused(_startTime, _startTicks, _speed, _playback);
			}

			if(doClientCheck && tracksWithoutClients.size()>0) {
				// Notify the user of all the tracks that did not have any client at startup time
				std::wostringstream message;
				message << TL(tracks_without_client);
				std::list< ref<TrackWrapper> >::const_iterator tit = tracksWithoutClients.begin();
				while(tit!=tracksWithoutClients.end()) {
					ref<TrackWrapper> tw = *tit;
					if(tw) {
						message << L'\'' << tw->GetInstanceName() << L'\'';
					}
					++tit;
				}

				Application::Instance()->GetEventLogger()->AddEvent(message.str(), ExceptionTypeWarning);
			}
		}
	}
	else if(pb==PlaybackStop && _state!=PlaybackStop) {
		StopPlayback();
	}
	else if((pb==PlaybackPause || pb==PlaybackWaiting) && _state==PlaybackPlay) {
		// Pause stuff
		_startTime = GetTime();
		_startTicks = 0;

		// _engine->PauseTicking()
		_engine->PauseTicking(true, _startTime, _startTicks);
		_cueThread->Pause(_startTime, _startTicks, true);
	}
	else if(pb==PlaybackWaiting && _state==PlaybackPause) {
		// Nothing special, just _state==PlaybackWaiting (which is done below)
	}
	else {
		return; // don't set _state
	}

	_state = pb;
	UpdateUI();
}

void Controller::SetPlaybackStateRecursive(PlaybackState pbs, PlaybackState previous) {
	ref<Iterator< ref<TrackWrapper> > > it = _time->GetTracks();
	while(it->IsValid()) {
		ref<TrackWrapper> tw = it->Get();
		ref<SubTimeline> sub = tw->GetSubTimeline();
		if(sub) {
			ref<Instance> tcrl = sub->GetInstance();
			tcrl->SetPlaybackStateRecursive(pbs, previous);
		}
		it->Next();
	}

	if(pbs==PlaybackStop) {
		Jump(0);
	}

	{
		ThreadLock lock(&_lock);

		if(previous==PlaybackAny || previous==GetPlaybackState()) {
			SetPlaybackState(pbs);
		}
	}
}

void Controller::StopPlayback() {
	ThreadLock lock(&_lock); 

	_startTime = min(GetTime(), Time(_time->GetTimeLengthMS()));
	_state = PlaybackStop;
	_waitingExpression = 0;
	_acquisitions.clear(); // free all capacity we have taken and any stuff we're waiting for
	_cueThread->Stop();

	// _engine->StopTicking()
	_engine->StopTicking();
	_activePlayers.clear();
	_tracksByChannel.clear();
	//_cueThread = 0; // We cannot delete the cuethread here, because this function might have been called from it

	{
		FILETIME ft;
		SYSTEMTIME st;
		GetLocalTime(&st);
		SystemTimeToFileTime(&st,&ft);
		ULARGE_INTEGER value;
		value.HighPart = ft.dwHighDateTime;
		value.LowPart = ft.dwLowDateTime;
		_lastStopTime = value.QuadPart;
	}
	UpdateUI();	
}

PlaybackState Controller::GetPlaybackState() const {
	return _state;
}

std::wstring Controller::GetLastStartTime() const {
	return FormatTime(_lastStartTime);
}

std::wstring Controller::GetLastStopTime() const {
	return FormatTime(_lastStopTime);
}

std::wstring Controller::FormatTime(__int64 t) {
	if(t==0) return L"";

	ULARGE_INTEGER value;
	value.QuadPart = t;
	FILETIME ft;
	ft.dwHighDateTime = value.HighPart;
	ft.dwLowDateTime = value.LowPart;

	SYSTEMTIME st;
	FileTimeToSystemTime(&ft, &st);

	std::wostringstream wos;
	wos.fill(L'0');
	wos << std::setw(2) << st.wDay << L'-' << std::setw(2) << st.wMonth << L'-' << std::setw(4) << st.wYear << L' ' << std::setw(2) << st.wHour << L':' << std::setw(2) << st.wMinute << L':' << std::setw(2) << st.wSecond;

	return wos.str();
}

Time Controller::GetTime() const {
	if(_state==PlaybackPlay) {
		Timestamp now(true);

		long double diff = now.ToMilliSeconds() - _startTicks.ToMilliSeconds();
		return _startTime + Time(int(diff*_speed));
	}
	return _startTime;
}

strong<Timeline> Controller::GetTimeline() {
	return _time;
}

strong<CueList> Controller::GetCueList() {
	return _time->GetCueList();
}

ref<Playback> Controller::GetPlayback() {
	return _playback;
}

ref<Waiting> Controller::GetWaitingFor() {
	return _waitingExpression;
}

void Controller::SetWaitingFor(ref<Waiting> w) {
	_waitingExpression = w;
}

SetPlaybackStateRunnable::SetPlaybackStateRunnable(ref<Controller> c, PlaybackState p) {
	_c = c;
	_p = p;
}

SetPlaybackStateRunnable::~SetPlaybackStateRunnable() {
}

void SetPlaybackStateRunnable::Run() {
	_c->SetPlaybackState(_p);
}

/* ControllerPlayback */
ControllerPlayback::ControllerPlayback(ref<Controller> controller): _controller(controller) {
}

ControllerPlayback::~ControllerPlayback() {
}

ref<Deck> ControllerPlayback::CreateDeck(bool visible, ref<Device> screen) {
	return Application::Instance()->GetOutputManager()->CreateDeck(visible, screen);
}

ref<Surface> ControllerPlayback::CreateSurface(int w, int h, bool visible, ref<Device> screen) {
	return Application::Instance()->GetOutputManager()->CreateSurface(w,h,visible,screen);
}

bool ControllerPlayback::IsFeatureAvailable(const std::wstring& ft) {
	return Application::Instance()->GetOutputManager()->IsFeatureAvailable(ft);
}

bool ControllerPlayback::IsKeyingSupported(ref<Device> screen) {
	return Application::Instance()->GetOutputManager()->IsKeyingSupported(screen);
}

ref<Device> ControllerPlayback::GetDeviceByPatch(const PatchIdentifier& t) {
	return Application::Instance()->GetOutputManager()->GetDeviceByPatch(t);
}

ref<tj::shared::Property> ControllerPlayback::CreateSelectPatchProperty(const std::wstring& name, ref<Inspectable> holder, PatchIdentifier* pi) {
	return Application::Instance()->GetOutputManager()->CreateSelectPatchProperty(name, holder, pi);
}

void ControllerPlayback::QueueDownload(const std::wstring& rid) {
	Application::Instance()->GetOutputManager()->QueueDownload(rid);
}

std::wstring ControllerPlayback::ParseVariables(const std::wstring& source) {
	ref<Controller> controller = _controller;
	if(controller) {
		ref<Variables> vars = controller->GetVariables();
		return Variables::ParseVariables(vars, source);
	}
	return source;
}

void ControllerPlayback::SetOutletValue(ref<Outlet> outlet, const Any& value) {
	ref<Controller> controller = _controller;
	if(controller) {
		if(outlet && outlet.IsCastableTo<VariableOutlet>()) {
			ref<VariableOutlet> vo = outlet;
			if(vo) {
				strong<VariableList> vars = controller->GetLocalVariables();
				ref<Variable> var = vars->GetVariableById(vo->GetVariableID());
				if(var) {
					vars->Set(var, value);
				}
			}
		}
	}
}

strong<Property> ControllerPlayback::CreateParsedVariableProperty(const std::wstring& name, ref<Inspectable> holder, std::wstring* source, bool multiLine = false) {
	if(source==0) Throw(L"Invalid parameter: source cannot be null!", ExceptionTypeError);
	strong<SuggestionProperty> sp = GC::Hold(new SuggestionProperty(name, holder, source, multiLine));
	sp->SetSuggestionMode(SuggestionEditWnd::SuggestionModeInsert);

	ref<Controller> controller = _controller;
	if(controller) {
		// Put the variables in the list
		ref<Menu> sm = sp->GetSuggestionMenu();
		ref<Variables> vars = controller->GetVariables();

		if(vars->GetVariableCount()>0) {
			sm->AddSeparator(TL(variable_property_insert));
			
			for(unsigned int a=0; a<vars->GetVariableCount(); a++) {
				ref<Variable> v = vars->GetVariableByIndex(a);
				if(v) {
					sm->AddItem(strong<MenuItem>(GC::Hold(new SuggestionMenuItem(L"{" + v->GetName() + L"}", v->GetName()))));
				}
			}
		}
	}

	return strong<Property>(sp);
}

strong<ResourceProvider> ControllerPlayback::GetResources() {
	return Application::Instance()->GetOutputManager()->GetResources();
}