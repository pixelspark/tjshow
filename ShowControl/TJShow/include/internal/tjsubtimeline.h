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
#ifndef _TJSUBTIMELINE_H
#define _TJSUBTIMELINE_H

namespace tj {
	namespace show {
		namespace view {
			class SplitTimelineWnd;
		}

		class SubTimelinePlugin: public OutputPlugin {
			public:
				SubTimelinePlugin();
				virtual ~SubTimelinePlugin();
				virtual std::wstring GetName() const;
				virtual ref<Track> CreateTrack(ref<Playback> pb);
				virtual ref<StreamPlayer> CreateStreamPlayer(ref<Playback> pb, ref<Talkback> talk);
				virtual std::wstring GetVersion() const;
				virtual std::wstring GetAuthor() const;
				virtual std::wstring GetFriendlyName() const;
				virtual std::wstring GetFriendlyCategory() const;
				virtual std::wstring GetDescription() const;
		};

		class SubTimelineTrack;

		class SubTimelineControl: public LiveControl {
			public:
				SubTimelineControl(ref<SubTimelineTrack> track, bool iv);
				virtual ~SubTimelineControl();
				virtual std::wstring GetGroupName();
				virtual ref<Wnd> GetWindow();
				virtual bool IsSeparateTab();
				virtual void Update();
				virtual bool IsInitiallyVisible();
				
			protected:
				weak<view::SplitTimelineWnd> _wnd;
				weak<SubTimelineTrack> _track;
				bool _initiallyVisible;
		};

		class SubTimelineTrack: public Track, public virtual Scriptable, public SubTimeline {
			friend class SubTimelineControl;

			public:
				SubTimelineTrack();
				virtual ~SubTimelineTrack();
				virtual Pixels GetHeight();
				virtual void Save(TiXmlElement* parent);
				virtual void Load(TiXmlElement* you);
				virtual Flags<RunMode> GetSupportedRunModes();
				virtual std::wstring GetTypeName() const;
				virtual ref<Player> CreatePlayer(ref<Stream> str);
				virtual ref<PropertySet> GetProperties();
				virtual void Paint(tj::shared::graphics::Graphics* g, Time position, float pixelsPerMs, unsigned int x, unsigned int y, unsigned int h, Time start, Time end);
				virtual ref<Item> GetItemAt(Time position,unsigned int height, bool rightClick, int trackHeight, float pixelsPerMs);
				virtual ref<LiveControl> CreateControl(ref<Stream> str);

				virtual ref<Controller> GetController();
				virtual ref<Instance> GetInstance(); // Same as GetController, from SubTimeline class
				virtual ref<Timeline> GetTimeline();
				ref<Scriptable> GetScriptable();
				virtual ref<Scriptable> Execute(Command c, ref<ParameterList> p);
				virtual void GetResources(std::vector <ResourceIdentifier>& rids);

				virtual std::wstring GetInstanceName() const;
				virtual void OnSetInstanceName(const std::wstring& name);
				virtual void Clone();

			protected:
				ref<Controller> _controller;
				ref<Timeline> _time;
				bool _initiallyVisible;
		};
	}
}

#endif