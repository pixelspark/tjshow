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
#ifndef _TJMEDIAPLUGIN_H
#define _TJMEDIAPLUGIN_H

namespace tj {
	namespace media {
		class MediaPlugin: public OutputPlugin {
			friend class MediaTrackItem;

			public:
				MediaPlugin(strong<master::MediaMasters> ms);
				virtual ~MediaPlugin();
				virtual std::wstring GetName() const;
				virtual ref<Track> CreateTrack(ref<Playback> pb);
				virtual ref<StreamPlayer> CreateStreamPlayer(ref<Playback> pb, ref<Talkback> talk);
				virtual std::wstring GetVersion() const;
				virtual std::wstring GetAuthor() const;
				virtual std::wstring GetFriendlyName() const;
				virtual std::wstring GetFriendlyCategory() const;
				virtual std::wstring GetDescription() const;
				virtual void GetRequiredFeatures(std::list<std::wstring>& fts) const;
				virtual void Reset();
				bool IsKeyingSupported() const;
				strong<master::MediaMasters> GetMasters();

			protected:
				bool _keyingSupported;
				strong<master::MediaMasters> _masters;
		};

		class MediaAudioPlugin: public OutputPlugin {
			public:
				MediaAudioPlugin(ref<MediaPlugin> mp);
				virtual ~MediaAudioPlugin();
				virtual ref<Track> CreateTrack(ref<Playback> playback);
				virtual ref<StreamPlayer> CreateStreamPlayer(ref<Playback> playback, ref<Talkback> talk);
				virtual std::wstring GetName() const;
				virtual void GetRequiredFeatures(std::list<std::wstring>& fts) const;

				/* This returns a localized (same as user language) name for this track. */
				virtual std::wstring GetFriendlyName() const;
				virtual std::wstring GetFriendlyCategory() const;
				virtual std::wstring GetVersion() const;
				virtual std::wstring GetAuthor() const;
				virtual std::wstring GetDescription() const;

			private:
				strong<MediaPlugin> _mp;
		};
	}
}

#endif