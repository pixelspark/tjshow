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
#ifndef _TJMEDIACONTROL_H
#define _TJMEDIACONTROL_H

namespace tj {
	namespace media {
		class MediaTrack;
		class MediaControlWnd;

		class MediaControl: public LiveControl {
			friend class MediaControlWnd;
			friend class MediaControlEndpoint;

			public:
				MediaControl(MediaTrack* track, ref<Stream> stream, bool isVideo);
				virtual ~MediaControl();
				virtual void Update();
				virtual ref<Wnd> GetWindow();
				virtual std::wstring GetGroupName();
				virtual bool IsSeparateTab();
				virtual int GetWidth();
				virtual void SetPropertyGrid(ref<PropertyGridProxy> pg);
				virtual ref<tj::shared::Endpoint> GetEndpoint(const std::wstring& name);
				virtual void GetEndpoints(std::vector< LiveControl::EndpointDescription >& eps);
				virtual float GetOpacityValue() const;
				virtual float GetVolumeValue() const;
				virtual ref<Deck> GetCurrentDeck();
				virtual void SetLastControlValues(float volume, float opacity);

				const static wchar_t* KMixName;
			protected:
				bool _isVideo;
				MediaTrack* _track;
				ref<MediaControlWnd> _wnd;
		};
	}
}

#endif