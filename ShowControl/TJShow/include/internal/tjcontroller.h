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
#ifndef _TJCONTROLLER_H
#define _TJCONTROLLER_H

namespace tj {
	namespace show {
		class Application;

		namespace view {
			class SplitTimelineWnd;
			class TimelineWnd;
		}

		namespace engine {
			class Engine;
			class CueThread;
		}

		/* Takes a timeline and plays it */
		class Controller: public virtual tj::shared::Object, public Instance {
			friend class Application;

			public:
				Controller(strong<Timeline> time, strong<Network> net, strong<VariableList> variables, strong<Variables> globals, bool isInstance);
				virtual ~Controller();
				virtual void OnCreated();
				
				// Instance
				virtual void SetPlaybackSpeed(float c);
				virtual float GetPlaybackSpeed() const;
				virtual void SetPlaybackState(PlaybackState pb);
				virtual PlaybackState GetPlaybackState() const;
				virtual void SetPlaybackStateRecursive(PlaybackState pb, PlaybackState previous=PlaybackAny);
				virtual void Jump(const Time& t);
				virtual void Fire();
				virtual void Trigger(ref<Cue> cue, bool denyIfPrivate = true);
				virtual Time GetTime() const;
				virtual strong<CueList> GetCueList();
				virtual strong<VariableList> GetLocalVariables();
				virtual strong<Variables> GetVariables();
				virtual strong<Timeline> GetTimeline();
				virtual ref<Instance> GetChildInstance(strong<Instancer> creator);
				virtual ref<Acquisition> GetAcquisitionFor(ref<Capacity> c);
				virtual ref<Waiting> GetWaitingFor();
				virtual void SetWaitingFor(ref<Waiting> w);
				virtual bool IsPlayingOrPausedRecursive(); // returns true if this controller or one of its children are still playing or paused
				virtual ref<Instance> GetParentOf(ref<TrackWrapper> tw);
				virtual ref<Playback> GetPlayback();
				virtual void Clear();
				virtual ref<TrackWrapper> GetTrackByChannel(const Channel& ch);

				void Update(Time now, Time diff);
				void SetCueList(strong<CueList> c);
				void SetTimeline(strong<Timeline> t);
				
				void Timer(); // to be called by internal timer function
				void SetSplitTimelineWindow(ref<view::SplitTimelineWnd> wnd);
				ref<view::TimelineWnd> GetTimelineWindow();
				ref<view::SplitTimelineWnd> GetSplitTimelineWindow();
				void UpdateUI();
				int GetPlayerCount() const;
				ref<ScriptScope> GetScope(); // Scope for variables in scripts executing from this timeline (e.g. from cues)
				
				std::wstring GetLastStartTime() const;
				std::wstring GetLastStopTime() const;
				void GetChildInstances(std::vector< ref<Instance> >& list);
				

			protected:
				static std::wstring FormatTime(__int64 t);
				void StopPlayback();
				void SetTime(Time t, const Timestamp& ticks);
				
				std::map< strong<Track>, ref<Player> > _activePlayers;
				std::map< Channel, ref<TrackWrapper> > _tracksByChannel;
				strong<Timeline> _time;
				strong<engine::Engine> _engine;
				strong<VariableList> _variables;
				strong<ScopedVariables> _scopedVariables;
				ref<engine::CueThread> _cueThread;
				strong<Network> _net;
				ref<ScriptScope> _scope;
				ref<Playback> _playback;
				ref<Waiting> _waitingExpression; // This is the waiting object for the currently waiting cue; destroy when stopping, jumping etc.
				weak<view::SplitTimelineWnd> _timelineWnd;
				std::map< ref<Capacity>, ref<Acquisition> > _acquisitions;

				float _speed;
				bool _isInstance;
				PlaybackState _state;
				Time _startTime;
				Timestamp _startTicks;
				CriticalSection _lock; // locked when controller state is changing

				__int64 _lastStartTime; // only used for display purposes
				__int64 _lastStopTime; // only used for display purposes
		};

		class SetPlaybackStateRunnable: public Runnable {
			public:
				SetPlaybackStateRunnable(ref<Controller> c, PlaybackState p);
				virtual ~SetPlaybackStateRunnable();
				virtual void Run();

			protected:
				ref<Controller> _c;
				PlaybackState _p;
		};
	}
}

#endif