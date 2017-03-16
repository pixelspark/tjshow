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
#ifndef _TJSCRIPTAPI_H
#define _TJSCRIPTAPI_H

namespace tj {
	namespace show {
		namespace script {
			class EndpointScriptable: public Scriptable {
				public:
					EndpointScriptable(ref<Endpoint> p);
					virtual ~EndpointScriptable();
					virtual tj::shared::ref<Scriptable> Execute(Command c, tj::shared::ref<ParameterList> p);
					ref<Endpoint> GetEndpoint();

				protected:
					ref<Endpoint> _p;
			};

			class ApplicationScriptable: public ScriptObject<ApplicationScriptable> {
				public:
					ApplicationScriptable();
					virtual ~ApplicationScriptable();
					static void Initialize();

				protected:
					virtual ref<Scriptable> SAlert(ref<ParameterList> p);
					virtual ref<Scriptable> SNotify(ref<ParameterList> p);
					virtual ref<Scriptable> SLog(ref<ParameterList> p);
					virtual ref<Scriptable> SInfo(ref<ParameterList> p);
					virtual ref<Scriptable> SDefer(ref<ParameterList> p);
					virtual ref<Scriptable> SView(ref<ParameterList> p);
					virtual ref<Scriptable> SDebug(ref<ParameterList> p);
					virtual ref<Scriptable> SShow(ref<ParameterList> p);
					virtual ref<Scriptable> SGuards(ref<ParameterList> p);
					virtual ref<Scriptable> SVariables(ref<ParameterList> p);
					virtual ref<Scriptable> SPlugins(ref<ParameterList> p);
					virtual ref<Scriptable> SOutput(ref<ParameterList> p);
					virtual ref<Scriptable> SRoot(ref<ParameterList> p);
					virtual ref<Scriptable> STimeline(ref<ParameterList> p);
					virtual ref<Scriptable> SResources(ref<ParameterList> p);
					virtual ref<Scriptable> SNetwork(ref<ParameterList> p);
			};

			class CueScriptable: public ScriptObject<CueScriptable> {
				public:
					CueScriptable(ref<Cue> cue);
					virtual ~CueScriptable();
					virtual ref<Cue> GetCue();
					static void Initialize();

					virtual ref<Scriptable> SName(ref<ParameterList> p);
					virtual ref<Scriptable> STime(ref<ParameterList> p);
					virtual ref<Scriptable> SToString(ref<ParameterList> p);

					virtual bool Set(Field field, ref<Scriptable> value);

				private:
					strong<Cue> _cue;
			};

			class InstanceScriptable: public ScriptObject<InstanceScriptable> {
				public:
					InstanceScriptable(ref<Instance> controller);
					virtual ~InstanceScriptable();
					static void Initialize();

					virtual ref<Scriptable> SToString(ref<ParameterList> p);
					virtual ref<Scriptable> SIsPlaying(ref<ParameterList> p);
					virtual ref<Scriptable> SIsPaused(ref<ParameterList> p);
					virtual ref<Scriptable> SStop(ref<ParameterList> p);
					virtual ref<Scriptable> SPlay(ref<ParameterList> p);
					virtual ref<Scriptable> SJump(ref<ParameterList> p);
					virtual ref<Scriptable> SSet(ref<ParameterList> p);
					virtual ref<Scriptable> SStopAll(ref<ParameterList> p);
					virtual ref<Scriptable> SPauseAll(ref<ParameterList> p);
					virtual ref<Scriptable> SResumeAll(ref<ParameterList> p);
					virtual ref<Scriptable> STrigger(ref<ParameterList> p);
					virtual ref<Scriptable> STime(ref<ParameterList> p);
					virtual ref<Scriptable> SLength(ref<ParameterList> p);
					virtual ref<Scriptable> SName(ref<ParameterList> p);
					virtual ref<Scriptable> SID(ref<ParameterList> p);
					virtual ref<Scriptable> SGet(ref<ParameterList> p);
					virtual ref<Scriptable> SGetById(ref<ParameterList> p);
					virtual ref<Scriptable> STracks(ref<ParameterList> p);
					virtual ref<Scriptable> SCues(ref<ParameterList> p);
					
				protected:
					ref<Scriptable> DeferAction(const std::wstring& c, ref<ParameterList> p);
					strong<Instance> _controller;
			};

			class OutputManagerScriptable: public ScriptObject<OutputManagerScriptable> {
				public:
					OutputManagerScriptable(ref<OutputManager> controller);
					virtual ~OutputManagerScriptable();
					static void Initialize();

				protected:
					virtual ref<Scriptable> SToString(ref<ParameterList> p);
					virtual ref<Scriptable> SGetDeviceByPatch(ref<ParameterList> p);
					ref<OutputManager> _om;
			};

			class CueListScriptable: public ScriptObject<CueListScriptable> {
				public:
					CueListScriptable(ref<CueList> cl);
					virtual ~CueListScriptable();
					static void Initialize();

					virtual ref<Scriptable> SToString(ref<ParameterList> p);
					virtual ref<Scriptable> SSize(ref<ParameterList> p);
					virtual ref<Scriptable> SGet(ref<ParameterList> p);
					virtual ref<Scriptable> SNext(ref<ParameterList> p);
					virtual ref<Scriptable> SPrevious(ref<ParameterList> p);
					virtual ref<Scriptable> SClear(ref<ParameterList> p);
					virtual ref<Scriptable> SIterate(ref<ParameterList> p);
					virtual ref<Scriptable> SAdd(ref<ParameterList> p);

				protected:
					strong<CueList> _cueList;
			};

			class LiveControlScriptable: public ScriptObject<LiveControlScriptable> {
				public:
					LiveControlScriptable(ref<LiveControl> control);
					virtual ~LiveControlScriptable();
					static void Initialize();

					virtual ref<Scriptable> SToString(ref<ParameterList> p);
					virtual ref<Scriptable> SGroupName(ref<ParameterList> p);
					virtual ref<Scriptable> SValue(ref<ParameterList> p);
					virtual ref<Scriptable> SGet(ref<ParameterList> p);

				protected:
					ref<LiveControl> _control;
			};

			class PluginScriptable: public ScriptObject<PluginScriptable> {
				public:
					PluginScriptable(ref<PluginWrapper> pw);
					virtual ~PluginScriptable();
					static void Initialize();

					virtual ref<Scriptable> SToString(ref<ParameterList> p);
					virtual ref<Scriptable> SName(ref<ParameterList> p);
					virtual ref<Scriptable> SHash(ref<ParameterList> p);
					virtual ref<Scriptable> SID(ref<ParameterList> p);
					virtual ref<Scriptable> SVersion(ref<ParameterList> p);
					virtual ref<Scriptable> SAuthor(ref<ParameterList> p);

					virtual ref<Scriptable> Execute(Command c, ref<ParameterList> p);

				protected:
					ref<PluginWrapper> _plugin;
			};

			class TrackScriptable: public ScriptObject<TrackScriptable> {
				public:
					TrackScriptable(ref<TrackWrapper> tw);
					virtual ~TrackScriptable();
					static void Initialize();
					virtual bool Set(Field f, ref<Scriptable> val);

					virtual ref<Scriptable> SName(ref<ParameterList> p);
					virtual ref<Scriptable> SLocked(ref<ParameterList> p);
					virtual ref<Scriptable> SRunsOnMaster(ref<ParameterList> p);
					virtual ref<Scriptable> SRunsOnClient(ref<ParameterList> p);
					virtual ref<Scriptable> SPlugin(ref<ParameterList> p);
					virtual ref<Scriptable> SControl(ref<ParameterList> p);
					virtual ref<Scriptable> SIsTimeline(ref<ParameterList> p);

					virtual ref<Scriptable> Execute(Command c, ref<ParameterList> p);

				protected:
					ref<TrackWrapper> _track;
			};

			class TracksIterator: public ScriptIterator<TrackScriptable> {
				public:
					TracksIterator(ref<Timeline> t);
					virtual ref<Scriptable> Next();

					ref< Iterator< ref<TrackWrapper> > > _it;
			};


			class DeviceScriptable: public ScriptObject<DeviceScriptable> {
				public:
					DeviceScriptable(ref<Device> dev);
					virtual ~DeviceScriptable();
					static void Initialize();
					
					virtual ref<Scriptable> SFriendlyName(ref<ParameterList> p);
					virtual ref<Scriptable> SIdentifier(ref<ParameterList> p);
					virtual ref<Scriptable> SIsMuted(ref<ParameterList> p);
					virtual ref<Scriptable> SMute(ref<ParameterList> p);

				protected:
					ref<Device> _dev;
			};
		}
	}
}

#endif