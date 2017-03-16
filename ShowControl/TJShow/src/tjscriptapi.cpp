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
#include "../include/internal/view/tjactions.h"
#include "../include/internal/tjscriptapi.h"
#include "../include/internal/tjnetwork.h"
#include <TJScript/include/types/tjscripthash.h>
#include <TJScript/include/types/tjscriptmap.h>

using namespace tj::show;
using namespace tj::show::script;

namespace tj { 
	namespace show {
		namespace script {
			class CommandRunnable: public Runnable {
				public:
					CommandRunnable(ref<view::View> view, int command) {
						_view = view;
						_cmd = command;
					}

					virtual ~CommandRunnable() {
					}

					virtual void Run() {
						_view->Command(_cmd);
					}

					int _cmd;
					ref<view::View> _view;
			};

			class ViewScriptable: public ScriptObject<ViewScriptable> {
				public:
					ViewScriptable(ref<view::View> vw): _view(vw) {
					}

					virtual ~ViewScriptable() {
					}

					static void Initialize() {
						Bind(L"new", &SNew);
						Bind(L"open", &SOpen);
						Bind(L"save", &SSave);
					}

					virtual ref<Scriptable> SNew(ref<ParameterList> p) {
						_view->Command(ID_NEW);
						return ScriptConstants::Null;
					}

					virtual ref<Scriptable> SOpen(ref<ParameterList> p) {
						static const Parameter<std::wstring> PFile(L"file", 0);

						if(PFile.Exists(p)) {
							std::wstring file = PFile.Require(p,L"");
							ref<Action> t = GC::Hold(new view::OpenFileAction(Application::Instance(), file, Application::Instance()->GetModel(), false));
							Application::Instance()->ExecuteAction(t);
						}
						else {
							ref<Action> t = GC::Hold(new CommandRunnable(_view, ID_OPEN));
							Application::Instance()->ExecuteAction(t);
						}
						return ScriptConstants::Null;
					}

					virtual ref<Scriptable> SSave(ref<ParameterList> p) {
						static const Parameter<std::wstring> PFile(L"file", 0);

						if(PFile.Exists(p)) {
							std::wstring file = PFile.Require(p,L"");
							ref<Action> t = GC::Hold(new view::SaveFileAction(Application::Instance(), Application::Instance()->GetModel(), view::SaveFileAction::TypeSaveAs, file));
							Application::Instance()->ExecuteAction(t);
						}
						else {
							ref<Action> t = GC::Hold(new CommandRunnable(_view, ID_SAVE));
							Application::Instance()->ExecuteAction(t);
						}
						
						return ScriptConstants::Null;
					}

				protected:
					strong<view::View> _view;
			};

			class ResourceProviderScriptable: public ScriptObject<ResourceProviderScriptable> {
				public:
					ResourceProviderScriptable(strong<ResourceProvider> rp): _rp(rp) {
					}

					virtual ~ResourceProviderScriptable() {
					}

					static void Initialize() {
						Bind(L"get", &SGet);
						Bind(L"exists", &SExists);
						Bind(L"toString", &SToString);
					}

					virtual ref<Scriptable> SGet(ref<ParameterList> p) {
						static const Parameter<std::wstring> PName(L"name", 0);

						std::wstring name = PName.Require(p,L"");
						std::wstring fn;
						if(_rp->GetPathToLocalResource(name,fn)) {
							return GC::Hold(new ScriptString(fn));
						}
						return ScriptConstants::Null;
					}

					virtual ref<Scriptable> SToString(ref<ParameterList> p) {
						return GC::Hold(new ScriptString(L"[ResourceProviderScriptable]"));
					}

					virtual ref<Scriptable> SExists(ref<ParameterList> p) {
						static const Parameter<std::wstring> PName(L"name", 0);

						std::wstring name = PName.Require(p, L"");
						ref<Resource> res = _rp->GetResource(name);
						return GC::Hold(new ScriptBool(res && res->Exists()));
					}

				protected:
					strong<ResourceProvider> _rp;
			};

			class NetworkScriptable: public ScriptObject<NetworkScriptable> {
				public:
					NetworkScriptable(ref<Network> nw): _network(nw) {
					}

					virtual ~NetworkScriptable() {
					}

					virtual ref<Scriptable> SToString(ref<ParameterList> p) {
						return GC::Hold(new ScriptString(L"[NetworkScriptable]"));
					}

					virtual ref<Scriptable> SIsPrimaryMaster(ref<ParameterList> p) {
						return _network->IsPrimaryMaster() ? ScriptConstants::True : ScriptConstants::False;
					}

					virtual ref<Scriptable> SIsPromotionInProgress(ref<ParameterList> p) {
						return _network->IsPromotionInProgress() ? ScriptConstants::True : ScriptConstants::False;
					}

					virtual ref<Scriptable> SIsConnected(ref<ParameterList> p) {
						return _network->IsConnected() ? ScriptConstants::True : ScriptConstants::False;
					}

					virtual ref<Scriptable> SIsServer(ref<ParameterList> p) {
						return (_network->GetRole() == RoleMaster) ? ScriptConstants::True : ScriptConstants::False;
					}

					virtual ref<Scriptable> SIsClient(ref<ParameterList> p) {
						return (_network->GetRole() == RoleClient) ? ScriptConstants::True : ScriptConstants::False;
					}

					virtual ref<Scriptable> SGetRole(ref<ParameterList> p) {
						return GC::Hold(new ScriptString(Roles::GetRoleName(_network->GetRole())));
					}

					virtual ref<Scriptable> SCancelPromotion(ref<ParameterList> p) {
						_network->CancelPromotion();
						return ScriptConstants::Null;
					}
					
					virtual ref<Scriptable> SPromote(ref<ParameterList> p) {
						_network->Promote();
						return ScriptConstants::Null;
					}

					virtual ref<Scriptable> SDemote(ref<ParameterList> p) {
						_network->Demote(false);
						return ScriptConstants::Null;
					}

					virtual ref<Scriptable> SBytesSent(ref<ParameterList> p) {
						return GC::Hold(new ScriptInt(_network->GetBytesSent()));
					}

					virtual ref<Scriptable> SBytesReceived(ref<ParameterList> p) {
						return GC::Hold(new ScriptInt(_network->GetBytesReceived()));
					}

					virtual ref<Scriptable> SFindResource(ref<ParameterList> p) {
						static Parameter<std::wstring> PRid(L"name", 0);
						_network->SendFindResource(PRid.Require(p, L""));

						return ScriptConstants::Null;
					}


					static void Initialize() {
						Bind(L"toString", &SToString);
						Bind(L"isPrimaryServer", &SIsPrimaryMaster);
						Bind(L"isPromotionInProgress", &SIsPromotionInProgress);
						Bind(L"isConnected", &SIsConnected);
						Bind(L"isClient", &SIsClient);
						Bind(L"isServer", &SIsServer);
						Bind(L"role", &SGetRole);
						Bind(L"cancelPromotion", &SCancelPromotion);
						Bind(L"promote", &SPromote);
						Bind(L"demote", &SDemote);
						Bind(L"bytesReceived", &SBytesReceived);
						Bind(L"bytesSent", &SBytesSent);
						Bind(L"findResource" , &SFindResource);
					}

				protected:
					strong<Network> _network;
			};

			class StatsScriptable: public ScriptObject<StatsScriptable> {
				public:
					StatsScriptable(ref<Application> app) {
					}

					virtual ~StatsScriptable() {
					}

					static void Initialize() {
						Bind(L"toString", &SToString);
						Bind(L"gcCount", &SGCCount);
						Bind(L"revisionID", &SRevisionID);
						Bind(L"revisionDate", &SRevisionDate);
					}

					virtual ref<Scriptable> SToString(ref<ParameterList> p) {
						return GC::Hold(new ScriptString(L"[StatsScriptable]"));
					}

					virtual ref<Scriptable> SGCCount(ref<ParameterList> p) {
						return GC::Hold(new ScriptInt(tj::shared::intern::Resource::GetResourceCount()));
					}

					virtual ref<Scriptable> SRevisionID(ref<ParameterList> p) {
						return GC::Hold(new ScriptInt(Version::GetRevisionID()));
					}

					virtual ref<Scriptable> SRevisionDate(ref<ParameterList> p) {
						return GC::Hold(new ScriptString(Version::GetRevisionDate()));
					}
			};

			class InstanceRunnable: public Task {
				public:
					InstanceRunnable(std::wstring c, ref<ParameterList> p, ref<Instance> controller): _command(c) {
						_params = p;
						_controller = controller;
					}

					virtual void Run() {
						if(_command==L"stop") {
							_controller->SetPlaybackState(PlaybackStop);
						}
						else if(_command==L"pause") {
							_controller->SetPlaybackState(PlaybackPause);
						}
						else if(_command==L"play") {
							_controller->SetPlaybackState(PlaybackPlay);
						}
						else if(_command==L"stopAll") {
							_controller->SetPlaybackStateRecursive(PlaybackStop);
						}
						else if(_command==L"pauseAll") {
							_controller->SetPlaybackStateRecursive(PlaybackPause, PlaybackPlay);
						}
						else if(_command==L"resumeAll") {
							_controller->SetPlaybackStateRecursive(PlaybackPlay, PlaybackPause);
						}
						else if(_command==L"set") {
							static const Parameter<double> PSpeed(L"speed", 0);
							if(PSpeed.Exists(_params)) {
								double speed = PSpeed.Require(_params,1.0);
								_controller->SetPlaybackSpeed((float)speed);
							}
						}
						else if(_command==L"jump") {
							static const Parameter<int> PTime(L"time", 0);
							static const Parameter<std::wstring> PCueName(L"cue", -1);

							if(PTime.Exists(_params)) {
								Time t(PTime.Require(_params, 0));
								JumpToTime(t);
							}
							else if(PCueName.Exists(_params)) {
								if(PCueName.Get(_params).IsCastableTo<CueScriptable>()) {
									JumpToCue(ref<CueScriptable>(PCueName.Get(_params))->GetCue());
								}
								else {
									ref<CueList> cues = _controller->GetCueList();
									ref<Cue> theCue = cues->GetCueByName(PCueName.Require(_params, L""));
									if(theCue) {
										JumpToCue(theCue);
									}
								}
							}
						}
						else if(_command==L"trigger") {
							static const Parameter<int> PCue(L"cue", 0);

							ref<Scriptable> s = PCue.Get(_params);
							if(s && s.IsCastableTo<CueScriptable>()) {
								ref<Cue> tc = ref<CueScriptable>(s)->GetCue();
								if(tc) {
									_controller->Trigger(tc, true);
								}
							}
						}
					}

					void JumpToCue(ref<Cue> cs) {
						_controller->Jump(cs->GetTime());
					}

					void JumpToTime(Time t) {
						_controller->Jump(t);
					}

					std::wstring _command;
					ref<ParameterList> _params;
					ref<Instance> _controller;
			};

			TracksIterator::TracksIterator(ref<Timeline> t) {
				_it = t->GetTracks();
			}

			ref<Scriptable> TracksIterator::Next() {
				if(_it->IsValid()) {
					ref<Scriptable> sc = GC::Hold(new TrackScriptable(_it->Get()));
					_it->Next();
					return sc;
				}
				return ScriptConstants::Null;
			}

			class PluginManagerScriptable: public Scriptable {
				public:
					PluginManagerScriptable(): _it(PluginManager::Instance()->GetPluginsByHash()->begin()) {
					}

					virtual ~PluginManagerScriptable() {
					}

					virtual ref<Scriptable> Execute(Command c, ref<ParameterList> p) {
						if(c==L"toString") {
							return GC::Hold(new ScriptString(L"[PluginScriptable]"));
						}
						else if(c==L"next") {
							if(_it!=PluginManager::Instance()->GetPluginsByHash()->end()) {
								std::pair< PluginHash, ref<PluginWrapper> > data = *_it;
								ref<PluginWrapper> pw = data.second;
								++_it;	
								return GC::Hold(new PluginScriptable(pw));
								
							}
							return ScriptConstants::Null;
						}
						else if(c==L"get") {
							static const Parameter<int> PHash(L"hash", -1);
							static const Parameter<std::wstring> PKey(L"key", -1);

							if(PHash.Exists(p)) {
								PluginHash hash = (PluginHash)PHash.Require(p,0);
								ref<PluginWrapper> pw = PluginManager::Instance()->GetPluginByHash(hash);
								return GC::Hold(new PluginScriptable(pw));
							}
							else if(PKey.Exists(p)) {
								std::wstring key = PKey.Require(p,L"");

								Hash h;
								int hash = h.Calculate(key);
								ref<PluginWrapper> pw = PluginManager::Instance()->GetPluginByHash((PluginHash)hash);
								return GC::Hold(new PluginScriptable(pw));
							}
						}
						return 0;
					}
				
				protected:
					std::map<PluginHash, ref<PluginWrapper> >::iterator _it;
			};

			class ScriptDelegateRunnable: public Task {
				public:
					ScriptDelegateRunnable(ref<ScriptDelegate> dlg) {
						_script = ref<ScriptDelegate>(dlg)->GetScript();
						if(!_script) {
							throw ScriptException(L"Delegate has no script!"); // shouldn't happen
						}
					}

					virtual ~ScriptDelegateRunnable() {
					}

					virtual void Run() {
						Application::Instance()->GetScriptContext()->Execute(_script);
					}

					ref<CompiledScript> _script;
			};

			void ApplicationScriptable::Initialize() {
				Bind(L"alert", &SAlert);
				Bind(L"notify", &SNotify);
				Bind(L"log", &SLog);
				Bind(L"info", &SInfo);
				Bind(L"defer", &SDefer);
				Bind(L"view", &SView);
				Bind(L"debug", &SDebug);
				Bind(L"show", &SShow);
				Bind(L"guards", &SGuards);
				Bind(L"variables", &SVariables);
				Bind(L"plugins", &SPlugins);
				Bind(L"output", &SOutput);
				Bind(L"root", &SRoot);
				Bind(L"timeline", &STimeline);
				Bind(L"resources", &SResources);
				Bind(L"network", &SNetwork);
			}
			ref<Scriptable> ApplicationScriptable::SAlert(ref<ParameterList> p) {
				static const Parameter<std::wstring> PMessage(L"message", 0);
				static const Parameter<std::wstring> PTitle(L"title", 1);

				std::wstring message = PMessage.Require(p, L"");
				std::wstring title = PTitle.Get(p, TL(application_name));
				Alert::Show(title, message, Alert::TypeInformation);
				return ScriptConstants::Null;
			}

			ref<Scriptable> ApplicationScriptable::SNotify(ref<ParameterList> p) {
				static const Parameter<std::wstring> PMessage(L"message", 0);

				std::wstring message = PMessage.Require(p, L"");
				Application::Instance()->GetEventLogger()->AddEvent(message, ExceptionTypeMessage);
				return ScriptConstants::Null;
			}

			ref<Scriptable> ApplicationScriptable::SLog(ref<ParameterList> p) {
				static const Parameter<std::wstring> PMessage(L"message", 0);
				std::wstring message = PMessage.Require(p, L"");
				Log::Write(L"TJShow/Script", message);

				return ScriptConstants::Null;
			}

			ref<Scriptable> ApplicationScriptable::SInfo(ref<ParameterList> p) {
				return GC::Hold(new StatsScriptable(Application::InstanceReference()));
			}

			ref<Scriptable> ApplicationScriptable::SDefer(ref<ParameterList> p) {
				static const Parameter<int> PDelegate(L"delegate", 0);

				ref<Scriptable> dlg = PDelegate.Get(p);
				if(!dlg || !dlg.IsCastableTo<ScriptDelegate>()) {
					throw ParameterException(L"Delegate should be a ScriptDelegate");
				}
				Dispatcher::CurrentOrDefaultInstance()->Dispatch(ref<Task>(GC::Hold(new ScriptDelegateRunnable(dlg))));
				return ScriptConstants::Null;
			}

			ref<Scriptable> ApplicationScriptable::SView(ref<ParameterList> p) {
				return GC::Hold(new ViewScriptable(Application::InstanceReference()->GetView()));
			}

			ref<Scriptable> ApplicationScriptable::SDebug(ref<ParameterList> p) {
				static const Parameter<bool> PEnable(L"enable", 0);

				bool val = PEnable.Require(p, false);
				Application::Instance()->GetScriptContext()->SetDebug(val);
				return ScriptConstants::Null;
			}

			ref<Scriptable> ApplicationScriptable::SShow(ref<ParameterList> p) {
				return Application::Instance()->GetModel()->GetGlobals();
			}

			ref<Scriptable> ApplicationScriptable::SGuards(ref<ParameterList> p) {
				return Application::Instance()->GetModel()->GetCapacities()->GetScriptable();
			}

			ref<Scriptable> ApplicationScriptable::SVariables(ref<ParameterList> p) {
				return Application::Instance()->GetModel()->GetVariables();
			}

			ref<Scriptable> ApplicationScriptable::SPlugins(ref<ParameterList> p) {
				return GC::Hold(new PluginManagerScriptable());
			}

			ref<Scriptable> ApplicationScriptable::SOutput(ref<ParameterList> p) {
				ref<OutputManager> output = Application::Instance()->GetOutputManager();
				return GC::Hold(new OutputManagerScriptable(output));
			}

			ref<Scriptable> ApplicationScriptable::SRoot(ref<ParameterList> p) {
				return GC::Hold(new InstanceScriptable(Application::Instance()->GetInstances()->GetRootInstance()));
			}

			ref<Scriptable> ApplicationScriptable::STimeline(ref<ParameterList> p) {
				return GC::Hold(new InstanceScriptable(Application::Instance()->GetInstances()->GetRootInstance()));
			}

			// TODO make separate method for application resources and show resources
			ref<Scriptable> ApplicationScriptable::SResources(ref<ParameterList> p) {
				return GC::Hold(new ResourceProviderScriptable(ref<ResourceProvider>(Application::Instance()->GetModel()->GetResourceManager())));
			}

			ref<Scriptable> ApplicationScriptable::SNetwork(ref<ParameterList> p) {
				return GC::Hold(new NetworkScriptable(Application::Instance()->GetNetwork()));
			}

			ApplicationScriptable::~ApplicationScriptable() {
			}

			ApplicationScriptable::ApplicationScriptable() {
			}

			CueScriptable::CueScriptable(ref<Cue> cue): _cue(cue) {
			}

			CueScriptable::~CueScriptable() {
			}

			ref<Cue> CueScriptable::GetCue() {
				return _cue;
			}

			void CueScriptable::Initialize() {
				Bind(L"name", &SName);
				Bind(L"time", &STime);
				Bind(L"toString", &SToString);
			}

			ref<Scriptable> CueScriptable::SName(ref<ParameterList> p) {
				return GC::Hold(new ScriptString(_cue->GetName()));
			}

			ref<Scriptable> CueScriptable::STime(ref<ParameterList> p) {
				return GC::Hold(new ScriptInt(_cue->GetTime().ToInt()));
			}

			ref<Scriptable> CueScriptable::SToString(ref<ParameterList> p) {
				return GC::Hold(new ScriptString(L"[CueScriptable]"));
			}

			bool CueScriptable::Set(Field field, ref<Scriptable> value) {
				if(field==L"name") {
					_cue->SetName(ScriptContext::GetValue<std::wstring>(value,L""));
					return true;
				}
				if(field==L"time") {
					_cue->SetTime(Time(ScriptContext::GetValue<int>(value, 0)));
					return true;
				}

				return false;
			}

			InstanceScriptable::InstanceScriptable(ref<Instance> c): _controller(c) {
			}

			InstanceScriptable::~InstanceScriptable() {
			}

			void InstanceScriptable::Initialize() {
				Bind(L"isPlaying", &SIsPlaying);
				Bind(L"isPaused", &SIsPaused);
				Bind(L"stop", &SStop);
				Bind(L"play", &SPlay);
				Bind(L"jump", &SJump);
				Bind(L"set", &SSet);
				Bind(L"stopAll", &SStopAll);
				Bind(L"pauseAll", &SPauseAll);
				Bind(L"resumeAll", &SResumeAll);
				Bind(L"trigger", &STrigger);
				Bind(L"time", &STime);
				Bind(L"length", &SLength);
				Bind(L"name", &SName);
				Bind(L"id", &SID);
				Bind(L"get", &SGet);
				Bind(L"getById", &SGetById);
				Bind(L"tracks", &STracks);
				Bind(L"cues", &SCues);
				Bind(L"toString", &SToString);
			}

			ref<Scriptable> InstanceScriptable::SToString(ref<ParameterList> p) {
				return GC::Hold(new ScriptString(L"[InstanceScriptable]"));
			}

			ref<Scriptable> InstanceScriptable::SIsPlaying(ref<ParameterList> p) {
				return (_controller->GetPlaybackState()==PlaybackPlay) ? ScriptConstants::True : ScriptConstants::False;
			}

			ref<Scriptable> InstanceScriptable::SIsPaused(ref<ParameterList> p) {
				return (_controller->GetPlaybackState()==PlaybackPause) ? ScriptConstants::True : ScriptConstants::False;
			}

			ref<Scriptable> InstanceScriptable::SStop(ref<ParameterList> p) {
				return DeferAction(L"stop", p);
			}

			ref<Scriptable> InstanceScriptable::SPlay(ref<ParameterList> p) {
				return DeferAction(L"play", p);
			}

			ref<Scriptable> InstanceScriptable::SJump(ref<ParameterList> p) {
				return DeferAction(L"jump", p);
			}

			ref<Scriptable> InstanceScriptable::SSet(ref<ParameterList> p) {
				return DeferAction(L"set", p);
			}
			ref<Scriptable> InstanceScriptable::SStopAll(ref<ParameterList> p) {
				return DeferAction(L"stopAll", p);
			}

			ref<Scriptable> InstanceScriptable::SPauseAll(ref<ParameterList> p) {
				return DeferAction(L"pauseAll", p);
			}

			ref<Scriptable> InstanceScriptable::SResumeAll(ref<ParameterList> p) {
				return DeferAction(L"resumeAll", p);
			}

			ref<Scriptable> InstanceScriptable::STrigger(ref<ParameterList> p) {
				return DeferAction(L"trigger", p);
			}
			
			ref<Scriptable> InstanceScriptable::STime(ref<ParameterList> p) {
				return GC::Hold(new ScriptDouble(_controller->GetTime().ToInt()));
			}

			ref<Scriptable> InstanceScriptable::SLength(ref<ParameterList> p) {
				return GC::Hold(new ScriptDouble(_controller->GetTimeline()->GetTimeLengthMS().ToInt()));
			}

			ref<Scriptable> InstanceScriptable::SName(ref<ParameterList> p) {
				return GC::Hold(new ScriptString(_controller->GetTimeline()->GetName()));
			}

			ref<Scriptable> InstanceScriptable::SID(ref<ParameterList> p) {
				return GC::Hold(new ScriptString(_controller->GetTimeline()->GetID()));
			}

			ref<Scriptable> InstanceScriptable::SGet(ref<ParameterList> p) {
				static const Parameter<std::wstring> PKey(L"key",-1);
				std::wstring name = PKey.Require(p,L"");

				ref<Timeline> t = _controller->GetTimeline();
				if(!t) {
					return 0;
				}
				ref<TrackWrapper> tw = t->GetTrackByName(name);
				if(tw) {
					return GC::Hold(new TrackScriptable(tw));
				}

				return ScriptConstants::Null;
			}

			ref<Scriptable> InstanceScriptable::SGetById(ref<ParameterList> p) {
				static const Parameter<std::wstring> PID(L"id", 0);
				ref<Instance> cr = Application::Instance()->GetInstances()->GetInstanceByTimelineID(PID.Require(p, L""), _controller);

				if(cr) {
					return GC::Hold(new InstanceScriptable(cr));
				}
				else {
					return ScriptConstants::Null;
				}
			}

			ref<Scriptable> InstanceScriptable::STracks(ref<ParameterList> p) {
				return GC::Hold(new TracksIterator(_controller->GetTimeline()));
			}

			ref<Scriptable> InstanceScriptable::SCues(ref<ParameterList> p) {
				return GC::Hold(new CueListScriptable(_controller->GetCueList()));
			}

			ref<Scriptable> InstanceScriptable::DeferAction(const std::wstring& c, ref<ParameterList> p) {
				ref<Runnable> t = GC::Hold(new InstanceRunnable(c, p, _controller));
				Dispatcher::CurrentOrDefaultInstance()->Dispatch(ref<Task>(t));
				return ScriptConstants::Null;
			}

			class CueListIteratorScriptable: public ScriptIterator<Cue> {
				public:
					CueListIteratorScriptable(ref<CueList> cues): _cues(cues), _it(cues->GetCuesBegin()) {
					}

					virtual ~CueListIteratorScriptable() {
					}

					virtual ref<Scriptable> Next() {
						if(_it!=_cues->GetCuesEnd()) {
							ref<Cue> cue =  *_it;
							++_it;
							return GC::Hold(new tj::show::script::CueScriptable(cue));
						}
						return ScriptConstants::Null;
					}

				protected:
					ref<CueList> _cues;
					std::vector< ref<Cue> >::iterator _it;
			};

			CueListScriptable::CueListScriptable(ref<CueList> cl): _cueList(cl) {
			}

			CueListScriptable::~CueListScriptable() {
			}

			void CueListScriptable::Initialize() {
				Bind(L"toString", &SToString);
				Bind(L"size", &SSize);
				Bind(L"get", &SGet);
				Bind(L"next", &SNext);
				Bind(L"previous", &SPrevious);
				Bind(L"clear", &SClear);
				Bind(L"iterate", &SIterate);
				Bind(L"add", &SAdd);
			}

			ref<Scriptable> CueListScriptable::SToString(ref<ParameterList> p) {
				return GC::Hold(new ScriptString(L"[CueListScriptable]"));
			}

			ref<Scriptable> CueListScriptable::SSize(ref<ParameterList> p) {
				return GC::Hold(new ScriptInt((int)_cueList->GetCueCount()));
			}

			ref<Scriptable> CueListScriptable::SGet(ref<ParameterList> p) {
				static const Parameter<std::wstring> PKey(L"key", -1);
				static const Parameter<std::wstring> PName(L"name", -1);
				static const Parameter<int> PTime(L"time", 0);

				if(PKey.Exists(p)) {
					// bracket[] notation
					std::wstring key = PKey.Require(p,L"");
					ref<Cue> cue = _cueList->GetCueByName(key);
					if(cue) {
						return GC::Hold(new tj::show::script::CueScriptable(cue));
					}
					return ScriptConstants::Null;
				}
				else if(PName.Exists(p)) {
					ref<Cue> cue = _cueList->GetCueByName(PName.Require(p,L""));
					if(cue) {
						return GC::Hold(new tj::show::script::CueScriptable(cue));
					}
					return ScriptConstants::Null;
				}
				else if(PTime.Exists(p)) {
					ref<Cue> cue = _cueList->GetCueAt(Time(PTime.Require(p,0)));
					if(cue) {
						return GC::Hold(new tj::show::script::CueScriptable(cue));
					}
					return ScriptConstants::Null;
				}

				return ScriptConstants::Null;
			}

			ref<Scriptable> CueListScriptable::SNext(ref<ParameterList> p) {
				static const Parameter<int> PTime(L"time", 0);
				Time time(PTime.Require(p, 0));

				if(time>Time(0)) {
					ref<Cue> cue = _cueList->GetNextCue(time);
					if(cue) {
						return GC::Hold(new CueScriptable(cue));
					}
					else {
						return ScriptConstants::Null;
					}
				}
				else {
					return ScriptConstants::Null;
				}
			}

			ref<Scriptable> CueListScriptable::SPrevious(ref<ParameterList> p) {
				static const Parameter<int> PTime(L"time", 0);

				Time time(PTime.Require(p,0));
				if(time>Time(0)) {
					ref<Cue> cue = _cueList->GetPreviousCue(time);
					if(cue) {
						return GC::Hold(new CueScriptable(cue));
					}
					else {
						return ScriptConstants::Null;
					}
				}
				else {
					return ScriptConstants::Null;
				}
			}

			ref<Scriptable> CueListScriptable::SClear(ref<ParameterList> p) {
				_cueList->Clear();
				return ScriptConstants::Null;
			}

			ref<Scriptable> CueListScriptable::SIterate(ref<ParameterList> p) {
				return GC::Hold(new CueListIteratorScriptable(_cueList));
			}

			ref<Scriptable> CueListScriptable::SAdd(ref<ParameterList> p) {
				static const Parameter<int> PTime(L"time", 0);
				Time time(PTime.Require(p,0));
				ref<Cue> cue = GC::Hold(new Cue(_cueList));
				_cueList->AddCue(cue);
				return GC::Hold(new tj::show::script::CueScriptable(cue));
			}
		}
	}
}

/** OutputManagerScriptable **/
OutputManagerScriptable::~OutputManagerScriptable() {
}

OutputManagerScriptable::OutputManagerScriptable(ref<OutputManager> om): _om(om) {
}

ref<Scriptable> OutputManagerScriptable::SToString(ref<ParameterList> p) {
	return GC::Hold(new ScriptString(L"[OutputManagerScriptable]"));
}

void OutputManagerScriptable::Initialize() {
	Bind(L"toString", &SToString);
	Bind(L"getDeviceByPatch", &SGetDeviceByPatch);
}

ref<Scriptable> OutputManagerScriptable::SGetDeviceByPatch(ref<ParameterList> p) {
	static Parameter<std::wstring> PPatch(L"patch", 0);

	ref<Device> dev = _om->GetDeviceByPatch((PatchIdentifier)PPatch.Require(p, L""));
	if(dev) {
		return GC::Hold(new DeviceScriptable(dev));
	}
	else {
		return ScriptConstants::Null;
	}
}

/* DeviceScriptable */
DeviceScriptable::DeviceScriptable(ref<Device> dev): _dev(dev) {
	assert(_dev);
}

DeviceScriptable::~DeviceScriptable() {
}

void DeviceScriptable::Initialize() {
	Bind(L"friendlyName", &SFriendlyName);
	Bind(L"identifier", &SIdentifier);
	Bind(L"isMuted", &SIsMuted);
	Bind(L"mute", &SMute);
}

ref<Scriptable> DeviceScriptable::SFriendlyName(ref<ParameterList> p) {
	return GC::Hold(new ScriptString(_dev->GetFriendlyName()));
}

ref<Scriptable> DeviceScriptable::SIdentifier(ref<ParameterList> p) {
	return GC::Hold(new ScriptString(_dev->GetIdentifier()));
}
ref<Scriptable> DeviceScriptable::SIsMuted(ref<ParameterList> p) {
	return _dev->IsMuted() ? ScriptConstants::True : ScriptConstants::False;
}

ref<Scriptable> DeviceScriptable::SMute(ref<ParameterList> p) {
	static Parameter<bool> PMute(L"mute", 0);
	_dev->SetMuted(PMute.Require(p, true));
	return ScriptConstants::Null;
}

/* TrackScriptable */
TrackScriptable::TrackScriptable(ref<TrackWrapper> tw) {
	_track = tw;
}

TrackScriptable::~TrackScriptable() {
}

void TrackScriptable::Initialize() {
	Bind(L"name", &SName);
	Bind(L"locked", &SLocked);
	Bind(L"runsOnClient", &SRunsOnClient);
	Bind(L"runsOnMaster", &SRunsOnMaster);
	Bind(L"plugin", &SPlugin);
	Bind(L"control", &SControl);
	Bind(L"isTimeline", &SIsTimeline);
}

bool TrackScriptable::Set(Field f, ref<Scriptable> val) {
	if(f==L"name") {
		_track->SetInstanceName(ScriptContext::GetValue<std::wstring>(val,L""));
	}
	else if(f==L"locked") {
		_track->SetLocked(ScriptContext::GetValue<bool>(val, false));
	}
	else {
		return false;
	}
	return true;
}

ref<Scriptable> TrackScriptable::SName(ref<ParameterList> p) {
	return GC::Hold(new ScriptString(_track->GetInstanceName()));
}

ref<Scriptable> TrackScriptable::SPlugin(ref<ParameterList> p) {
	return GC::Hold(new PluginScriptable(_track->GetPlugin()));
}

ref<Scriptable> TrackScriptable::SLocked(ref<ParameterList> p) {
	return _track->IsLocked() ? ScriptConstants::True : ScriptConstants::False;
}

ref<Scriptable> TrackScriptable::SRunsOnMaster(ref<ParameterList> p) {
	RunMode rm = _track->GetRunMode();
	return GC::Hold(new ScriptBool(rm==RunModeBoth || rm==RunModeMaster));
}

ref<Scriptable> TrackScriptable::SRunsOnClient(ref<ParameterList> p) {
	RunMode rm = _track->GetRunMode();
	return GC::Hold(new ScriptBool(rm==RunModeBoth || rm==RunModeClient));
}

ref<Scriptable> TrackScriptable::SIsTimeline(ref<ParameterList> p) {
	return GC::Hold(new ScriptBool(_track->IsSubTimeline()));
}

ref<Scriptable> TrackScriptable::SControl(ref<ParameterList> p) {
	ref<LiveControl> lc = _track->GetLiveControl();
	if(!lc) {
		return ScriptConstants::Null;
	}
	else {
		return GC::Hold(new LiveControlScriptable(lc));
	}
}

ref<Scriptable> TrackScriptable::Execute(Command c, ref<ParameterList> p) {
	ref<Scriptable> val = ScriptObject<TrackScriptable>::Execute(c,p);
	if(!val) {
		ref<Scriptable> ps = _track->GetTrack()->GetScriptable();
		if(ps) {
			return ps->Execute(c,p);
		}
		return 0;
	}
	return val;
}

/* LiveControlScriptable */
LiveControlScriptable::LiveControlScriptable(ref<LiveControl> control): _control(control) {
	assert(control);
}

void LiveControlScriptable::Initialize() {
	Bind(L"toString", &SToString);
	Bind(L"groupName", &SGroupName);
	Bind(L"value", &SValue);
	Bind(L"get", &SGet);
}

ref<Scriptable> LiveControlScriptable::SToString(ref<ParameterList> p) {
	return GC::Hold(new ScriptString(L"[LiveControlScriptable]"));
}

ref<Scriptable> LiveControlScriptable::SGroupName(ref<ParameterList> p) {
	return GC::Hold(new ScriptString(_control->GetGroupName()));
}

ref<Scriptable> LiveControlScriptable::SValue(ref<ParameterList> p) {
	ref<Endpoint> endpoint = _control->GetEndpoint(L"");
	if(endpoint) {
		return GC::Hold(new EndpointScriptable(endpoint));
	}
	return ScriptConstants::Null;
}

ref<Scriptable> LiveControlScriptable::SGet(ref<ParameterList> p) {
	static const Parameter<std::wstring> PKey(L"key", 0);

	std::wstring key = PKey.Require(p,L"");
	ref<Endpoint> endpoint = _control->GetEndpoint(key);
	if(endpoint) {
		return GC::Hold(new EndpointScriptable(endpoint));
	}
	return ScriptConstants::Null;
}

LiveControlScriptable::~LiveControlScriptable() {
}

/** PluginScriptable */
PluginScriptable::PluginScriptable(ref<PluginWrapper> pw) {
	_plugin = pw;
}

PluginScriptable::~PluginScriptable() {
}

void PluginScriptable::Initialize() {
	Bind(L"toString", &SToString);
	Bind(L"name", &SName);
	Bind(L"hash", &SHash);
	Bind(L"id", &SID);
	Bind(L"version", &SVersion);
	Bind(L"author", &SAuthor);
}

ref<Scriptable> PluginScriptable::SToString(ref<ParameterList> p) {
	return GC::Hold(new ScriptString(L"[PluginWrapper]"));
}

ref<Scriptable> PluginScriptable::SName(ref<ParameterList> p) {
	return GC::Hold(new ScriptString(_plugin->GetFriendlyName()));
}

ref<Scriptable> PluginScriptable::SHash(ref<ParameterList> p) {
	return GC::Hold(new ScriptHash(_plugin->GetHash()));
}

ref<Scriptable> PluginScriptable::SID(ref<ParameterList> p) {
	return GC::Hold(new ScriptString(_plugin->GetHashName()));
}

ref<Scriptable> PluginScriptable::SVersion(ref<ParameterList> p) {
	return GC::Hold(new ScriptString(_plugin->GetPlugin()->GetVersion()));
}

ref<Scriptable> PluginScriptable::SAuthor(ref<ParameterList> p) {
	return GC::Hold(new ScriptString(_plugin->GetPlugin()->GetAuthor()));
}

ref<Scriptable> PluginScriptable::Execute(Command c, ref<ParameterList> p) {
	ref<Scriptable> val = ScriptObject<PluginScriptable>::Execute(c,p);
	if(!val) {
		ref<Scriptable> ps = _plugin->GetPlugin()->GetScriptable();
		if(ps) {
			return ps->Execute(c,p);
		}
		return 0;
	}
	return val;
}

EndpointScriptable::EndpointScriptable(ref<Endpoint> p): _p(p) {
	assert(p);
}

EndpointScriptable::~EndpointScriptable() {
}

ref<Scriptable> EndpointScriptable::Execute(Command c, ref<ParameterList> p) {
	if(c==L"toString") {
		return GC::Hold(new ScriptString(L"[EndpointScriptable]"));
	}
	else if(c==L"set") {
		ref<Scriptable> parameter;
		if(p->Exists(L"0")) {
			parameter = p->Get(L"0");
		}
		else if(p->Exists(L"value")) {
			parameter = p->Get(L"value");
		}

		if(!parameter) {
			throw ScriptException(L"Parameter 'value' is not set or invalid");
		}

		// TODO: What to do with threaded endpoints?
		// TODO: allow other value types than float (through Any)
		float fv = ScriptContext::GetValue<float>(parameter,0.0f);
		_p->Set(Any(fv));
		return ScriptConstants::Null;
	}
	return 0;
}

ref<Endpoint> EndpointScriptable::GetEndpoint() {
	return _p;
}