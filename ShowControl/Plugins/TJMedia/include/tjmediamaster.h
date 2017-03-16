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
#ifndef _TJMEDIAMASTER_H
#define _TJMEDIAMASTER_H

#include "tjmediamasters.h"

namespace tj {
	namespace media {
		class MediaPlugin;

		namespace master {
			class MediaMasterPlugin: public OutputPlugin {
				public:
					MediaMasterPlugin(strong<master::MediaMasters> ms);
					virtual ~MediaMasterPlugin();
					virtual std::wstring GetName() const;
					virtual ref<Track> CreateTrack(ref<Playback> pb);
					virtual ref<StreamPlayer> CreateStreamPlayer(ref<Playback> pb, ref<Talkback> talk);
					virtual std::wstring GetVersion() const;
					virtual std::wstring GetAuthor() const;
					virtual std::wstring GetFriendlyName() const;
					virtual std::wstring GetFriendlyCategory() const;
					virtual std::wstring GetDescription() const;

				protected:
					strong<master::MediaMasters> _masters;
			};

			class MediaMasterControl;

			class MediaMasterTrack: public Track, public MediaMaster {
				friend class MediaMasterPlayer;
				friend class MediaMasterEndpoint;
				friend class MediaMasterTrackRange;
				friend class MediaMasterControl;
				friend class tj::media::MediaPlugin;

				public:
					MediaMasterTrack(ref<MediaMasters> mp);
					virtual ~MediaMasterTrack();
					virtual Pixels GetHeight();
					virtual void Save(TiXmlElement* parent);
					virtual void Load(TiXmlElement* you);
					virtual Flags<RunMode> GetSupportedRunModes();
					virtual std::wstring GetTypeName() const;
					virtual ref<Player> CreatePlayer(ref<Stream> str);
					virtual ref<PropertySet> GetProperties();
					virtual void Paint(tj::shared::graphics::Graphics* g, Time position, float pixelsPerMs, unsigned int x, unsigned int y, unsigned int h, Time start, Time end, ref<Item> focus);
					virtual ref<Item> GetItemAt(Time position,unsigned int h, bool rightClick, int trackHeight, float pixelsPerMs);
					virtual ref<TrackRange> GetRange(Time start, Time end);
					virtual ref<LiveControl> CreateControl(ref<Stream> stream);
					virtual void OnSetInstanceName(const std::wstring& name);
					virtual MediaMasterID GetMasterID() const;
					virtual MediaMaster::Flavour GetFlavour() const;

				protected:
					virtual void OnCreated();
					ref< Fader<float> > _value;
					int _height;
					MediaMaster::Flavour _flavour;
					std::wstring _mid;
					weak<MediaMasters> _mm;
					Time _selectedFadePoint;
					ref<MediaMasterControl> _control;
			};
		}
	}
}

#endif