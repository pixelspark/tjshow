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
#ifndef _TJOSCCUETRACK_H
#define _TJOSCCUETRACK_H

#include "tjosc.h"
#include "tjoscdevice.h"
#include "tjoscarguments.h"
#include <TJShow/include/extra/tjcuetrack.h>
#include <OSCPack/OscTypes.h>

namespace oscp = osc;

namespace tj {
	namespace osc {
		class OSCOutputCue: public Serializable, public virtual Inspectable {
			public:
				OSCOutputCue(Time pos = 0);
				virtual ~OSCOutputCue();

				virtual void Load(TiXmlElement* you);
				virtual void Save(TiXmlElement* me);
				virtual void Move(Time t, int h);
				virtual ref<OSCOutputCue> Clone();
				virtual ref<PropertySet> GetProperties(ref<Playback> pb, strong< CueTrack<OSCOutputCue> > track);
				Time GetTime() const;
				void SetTime(Time t);
				virtual void Paint(tj::shared::graphics::Graphics* g, ref<Theme> theme, float pixelLeft, unsigned int y, unsigned int h, ref< CueTrack<OSCOutputCue> > track, bool focus);
				virtual void Fire(ref<OSCDevice> dev, ref<Playback> pb, const std::wstring& prefix, const std::wstring& defaultAction);

			protected:
				Time _time;
				std::wstring _description;
				std::wstring _path;
				strong<OSCArgumentList> _arguments;
		};

		class OSCOutputCueTrack: public CueTrack<OSCOutputCue> {
			friend class OSCOutputCuePlayer;
			friend class OSCOutputCue;

			public:
				OSCOutputCueTrack(ref<Playback> pb);
				virtual ~OSCOutputCueTrack();
				virtual std::wstring GetTypeName() const;
				virtual ref<Player> CreatePlayer(ref<Stream> str);
				virtual Flags<RunMode> GetSupportedRunModes();
				virtual ref<PropertySet> GetProperties();
				virtual ref<LiveControl> CreateControl(ref<Stream> str);
				virtual void Save(TiXmlElement* parent);
				virtual void Load(TiXmlElement* you);
				ref<OSCDevice> GetOutputDevice();
				virtual std::wstring GetEmptyHintText() const;
				virtual bool IsExpandable();
				virtual Pixels GetHeight();
				virtual void SetExpanded(bool t);
				virtual bool IsExpanded();

			protected:
				bool _expanded;
				PatchIdentifier _out;
				ref<Playback> _pb;
				std::wstring _prefix;
				std::wstring _default;
		};

		class OSCOutputCuePlayer: public Player {
			public:
				OSCOutputCuePlayer(strong<OSCOutputCueTrack> tr, ref<Stream> str);
				virtual ~OSCOutputCuePlayer();

				ref<Track> GetTrack();
				virtual void Stop();
				virtual void Start(Time pos, ref<Playback> playback, float speed);
				virtual void Tick(Time currentPosition);
				virtual void Jump(Time newT, bool paused);
				virtual void SetOutput(bool enable);
				virtual Time GetNextEvent(Time t);

			protected:
				strong<OSCOutputCueTrack> _track;
				ref<OSCDevice> _out;
				ref<Stream> _stream;
				bool _output;
				Time _last;
		};
	}
}

#endif