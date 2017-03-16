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
#ifndef _TJINSTANCE_H
#define _TJINSTANCE_H

namespace tj {
	namespace show {
		/** Instancer is an interface for tracks that 'instantiate' another timeline. Currently, this is only
		the InstancerTrack (as defined in tjinstancer.h). The TrackWrapper::IsInstance method uses this to determine
		whether the track is an instancer. **/
		class Instancer {
			public:
				virtual ~Instancer();
				virtual ref<Timeline> GetTimeline() = 0;
		};

		/** SubTimeline is an interface for tracks that 'instantiate' another timeline (like Instancer), but do this only once (i.e.
		the 'instance' always exists and is not dynamically created). Currently, this is only the SubTimelineTrack (as defined in 
		tjsubtimeline.h). The TrackWrapper::IsSubTimeline method uses this to determine whether the track is a sub-timeline. **/
		class SubTimeline: public Instancer {
			public:
				virtual ~SubTimeline();
				virtual ref<Instance> GetInstance() = 0;
				virtual std::wstring GetInstanceName() const = 0;
		};

		class InstanceHolder {
			public:
				virtual ~InstanceHolder();
				virtual ref<Instance> GetInstance() = 0;
		};

		/** An instance is essentially 'a playing timeline' with associated local/global variables, a Timeline and CueList. Instance
		is implemented by the Controller class, which implements timeline playing with threads. **/
		class Instance: public Lockable {
			public:
				virtual ~Instance();
				virtual void SetPlaybackSpeed(float c) = 0;
				virtual float GetPlaybackSpeed() const = 0;
				virtual void SetPlaybackState(PlaybackState pb) = 0;
				virtual PlaybackState GetPlaybackState() const = 0;
				virtual void SetPlaybackStateRecursive(PlaybackState pb, PlaybackState previous = PlaybackAny) = 0;
				virtual void Jump(const Time& t) = 0;
				virtual void Fire() = 0; // if paused, unpauses; otherwise, triggers next cue (if any)
				virtual void Trigger(ref<Cue> cue, bool denyIfPrivate = true) = 0;
				virtual Time GetTime() const = 0;

				virtual strong<CueList> GetCueList() = 0;
				virtual strong<VariableList> GetLocalVariables() = 0;
				virtual strong<Variables> GetVariables() = 0;
				virtual strong<Timeline> GetTimeline() = 0;
				virtual ref<Instance> GetParentOf(ref<TrackWrapper> tw) = 0;

				virtual ref<Acquisition> GetAcquisitionFor(ref<Capacity> c) = 0;
				virtual void SetWaitingFor(ref<Waiting> wait) = 0;
				virtual ref<Waiting> GetWaitingFor() = 0;
				virtual bool IsPlayingOrPausedRecursive() = 0;
				virtual ref<Instance> GetChildInstance(strong<Instancer> creator) = 0;
				virtual ref<Playback> GetPlayback() = 0;
				virtual void Clear() = 0;
				virtual ref<TrackWrapper> GetTrackByChannel(const Channel& ch) = 0;
		};

		class NamedInstance {
			public:
				NamedInstance(const std::wstring& name, ref<Instance> c);
				virtual ~NamedInstance();
				
				std::wstring _name;
				ref<Instance> _controller;
		};

		/** Instances holds all controllers and instances of timelines (i.e. everything that is not saved
		to the tsx; all stuff that is saved belongs in Model) **/
		class Instances {
			public:
				Instances(strong<Model> model, strong<Network> net);
				virtual ~Instances();
				virtual void Clear();
				virtual strong<Instance> GetRootInstance();
				virtual void GetStaticInstances(std::vector< ref<NamedInstance> >& nis);
				virtual ref<Instance> GetInstanceByTimelineID(const std::wstring& tlid, ref<Instance> parent = null);

			protected:
				void RecursiveGetStaticInstances(strong<Instance> parent, std::vector< ref<NamedInstance> >& list, int level);
				strong<Model> _model;
				ref<Instance> _root;
		};


	}
}

#endif