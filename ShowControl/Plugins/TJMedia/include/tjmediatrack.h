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
#ifndef _TJMEDIATRACK_H
#define _TJMEDIATRACK_H

namespace tj {
	namespace media {
		namespace analyzer {
			class AudioAnalysis;
		}

		class MediaBlock: public Serializable, public virtual Inspectable {
			public:
				MediaBlock(Time start, const std::wstring& file, bool isVideo);
				virtual ~MediaBlock();
				virtual ref<PropertySet> GetProperties(bool withKeying, ref<Playback> pb);
				virtual void Move(Time t, int h);
				virtual void Save(TiXmlElement* parent);
				virtual void Load(TiXmlElement* you);
				Time GetDuration(strong<Playback> pb);
				Time GetTime() const;
				void SetTime(Time start);
				std::wstring GetFile() const;
				void SetDuration(Time t);
				const PatchIdentifier& GetLiveSource() const;
				bool IsLiveSource() const;
				bool IsKeyingEnabled() const;
				bool IsValid(strong<Playback> pb);
				float GetKeyingTolerance() const;
				const RGBColor& GetKeyingColor() const;
				virtual void Paint(graphics::Graphics& g, Area rc, strong<Theme> theme, const Time& start, float pixelsPerMs);

			protected:
				void CalculateDuration(strong<Playback> pb);
				const static Time KMinimumDuration;

				Time _start;
				PatchIdentifier _liveSource;
				std::wstring _file;
				std::wstring _analysisFor;
				ref<analyzer::AudioAnalysis> _analysis;
				Time _userDuration;
				RGBColor _keyColor;
				float _keyTolerance;
				bool _keyEnabled;
				bool _isVideo;

		};

		class MediaTrack: public MultifaderTrack {
			friend class MediaPlayer;
			friend class MediaControl;
			friend class MediaTrackItem;

			public:
				MediaTrack(ref<Playback> pb, ref<MediaPlugin> mp, bool isVideoTrack);
				virtual ~MediaTrack();

				virtual void Save(TiXmlElement* parent);
				virtual void Load(TiXmlElement* you);
				virtual std::wstring GetTypeName() const;
				virtual Flags<RunMode> GetSupportedRunModes();

				virtual ref<Player> CreatePlayer(ref<Stream> str);
				virtual ref<LiveControl> CreateControl(ref<Stream> str);
				virtual ref<PropertySet> GetProperties();
				virtual void Paint(tj::shared::graphics::Graphics* g, Time position, float pixelsPerMs, unsigned int x, unsigned int y, unsigned int h, Time start, Time end, ref<Item> focus);
				virtual ref<Item> GetItemAt(Time pos, unsigned int h, bool rightClick, int th, float pixelsPerMs);
				ref<Device> GetCardDevice();
				ref<Device> GetScreenDevice();
				const PatchIdentifier& GetCardPatch();
				virtual ref<TrackRange> GetRange(Time start, Time end);
				virtual void RemoveItemsBetween(Time start, Time end);

				virtual Pixels GetHeight();
				ref<Fader<float> > GetVolume();
				ref<Fader<float> > GetOpacity();
				int GetBalance();
				ref<MediaBlock> GetBlockAt(Time t);
				ref<MediaBlock> GetNextBlock(Time t);
				void RemoveBlock(ref<MediaBlock> block);
				virtual Time GetNextEvent(Time t);
				float GetVolumeAt(Time t);
				float GetOpacityAt(Time t);
				float GetScaleAt(Time t);
				float GetRotateAt(Time t);
				float GetTranslateAt(Time t);
				virtual void GetResources(std::vector< ResourceIdentifier >& rids);
				virtual void InsertFromControl(Time t, ref<LiveControl> control, bool fade);
				ref<Deck> CreateDeck();
				virtual void CreateOutlets(OutletFactory& of);
				void SetLastControlValues(float volume, float opacity);
				virtual bool PasteItemAt(const Time& position, TiXmlElement* item);

				static std::wstring KClickedOutletID;
				static std::wstring KClickedXOutletID;
				static std::wstring KClickedYOutletID;

			protected:
				void SaveFader(TiXmlElement* parent, int id, const char* name);
				void LoadFader(TiXmlElement* parent, int id, const char* name);

				enum WhichFader { FaderVolume, FaderOpacity, FaderRotate, FaderTranslate, FaderScale };

				ref<Playback> _pb;
				ref<MediaPlugin> _plugin;
				ref<MediaControl> _control;
				weak<Deck> _lastDeck;
				std::vector< ref<MediaBlock> > _blocks;
				
				int _balance;
				RGBColor _color;
				PatchIdentifier _card;
				PatchIdentifier _screen;
				Time _selectedFadePoint;
				std::wstring _defaultFile;
				bool _isVideo;
				master::MediaMasterList _masters;

				// video stuff
				Vector _scale, _translate, _rotate;		// values that define the 'scale' of the faders
				Vector _iscale, _itranslate, _irotate;	// initial values

				// Outlets
				ref<Outlet> _clickedOutlet;
				ref<Outlet> _clickXOutlet;
				ref<Outlet> _clickYOutlet;

				// Last control values
				float _lastControlVolume;
				float _lastControlOpacity;
		};
	}
}

#endif