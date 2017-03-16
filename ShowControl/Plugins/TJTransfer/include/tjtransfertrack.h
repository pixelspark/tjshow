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
#ifndef _TJTRANSFERTRACK_H
#define _TJTRANSFERTRACK_H

#include <TJShow/include/extra/tjextra.h>

namespace tj {
	namespace transfer {
		class TransferCue: public Inspectable {
			friend class TransferPlayer;

			public:
				TransferCue(Time t = 0);
				virtual ~TransferCue();
				virtual void Save(TiXmlElement* you);
				virtual void Load(TiXmlElement* you);
				virtual Time GetTime() const;
				virtual void Paint(tj::shared::graphics::Graphics* g, ref<Theme> theme, float pixelLeft, unsigned int y, unsigned int h, ref<CueTrack<TransferCue> > track, bool focus);
				virtual ref<PropertySet> GetProperties(ref<Playback> pb, strong< CueTrack<TransferCue> > track);
				virtual void Move(Time t, int h);
				virtual void SetTime(Time t);
				virtual ref<TransferCue> Clone();

			protected:
				Time _time;
				std::wstring _rid;
		};

		class TransferTrack: public CueTrack<TransferCue> {
			public:
				TransferTrack(ref<TransferPlugin> pl);
				virtual ~TransferTrack();
				virtual std::wstring GetTypeName() const;
				virtual Flags<RunMode> GetSupportedRunModes();
				virtual ref<Player> CreatePlayer(ref<Stream> st);
				virtual std::wstring GetEmptyHintText() const;
				virtual bool IsExpandable();
				virtual Pixels GetHeight();
				virtual void SetExpanded(bool t);
				virtual bool IsExpanded();

			protected:
				bool _expanded;
				weak<TransferPlugin> _plugin;
		};
	}
}

#endif