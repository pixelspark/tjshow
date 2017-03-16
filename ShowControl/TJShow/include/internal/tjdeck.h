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
#ifndef _TJDECK_H
#define _TJDECK_H

interface IBaseFilter;
interface IGraphBuilder;
interface TextureManager;
interface IMediaControl;
interface IMediaSeeking;
interface IBasicAudio;
interface IDirect3DTexture9;

namespace tj {
	namespace show {
		namespace media {
			class TextureRenderer;

			class MediaDevice: public Device {
				public:
					MediaDevice(const std::wstring& id, const std::wstring& friendly, IBaseFilter* filter, bool isSource);
					virtual ~MediaDevice();
					virtual std::wstring GetFriendlyName() const;
					virtual DeviceIdentifier GetIdentifier() const;
					virtual ref<tj::shared::Icon> GetIcon();
					virtual bool IsMuted() const;
					virtual void SetMuted(bool t) const;

					// Own methods
					virtual CComPtr<IBaseFilter> GetFilter();

				protected:
					std::wstring _friendlyName;
					std::wstring _id;
					IBaseFilter* _filter;
					ref<Icon> _icon;
			};

			class TextureDeck: public Deck {
				public:
					TextureDeck();
					virtual ~TextureDeck();

					// Mesh implementation
					virtual const Vector& GetScale() const;
					virtual const Vector& GetRotate() const;
					virtual const Vector& GetTranslate() const;
					virtual void SetTranslate(const Vector& v);
					virtual void SetScale(const Vector& v);
					virtual void SetRotate(const Vector& v);
					virtual float GetTextureWidth() const;
					virtual float GetTextureHeight() const;

				protected:
					Vector _translate;
					Vector _scale;
					Vector _rotate;
			};

			/** Implementation of the Deck interface for audio/video media playback as defined in tjplayback.h public header **/
			class VideoDeck: public TextureDeck {
				public:
					VideoDeck(CComPtr<TextureRenderer> renderer, bool visible);
					virtual ~VideoDeck();

					virtual void Load(const std::wstring& fn, ref<Device> card);
					virtual void LoadLive(ref<Device> source, ref<Device> card);
					virtual bool IsLoaded() const;
					virtual void Stop();
					virtual void Play();
					virtual void Pause();
					virtual void Jump(Time t);
					virtual void SetVolume(float v);
					virtual void SetBalance(int b);
					virtual void SetOpacity(float v);
					virtual void SetAudioOutput(bool a);
					virtual void SetVideoOutput(bool v);
					virtual std::wstring GetFileName() const;
					virtual void SetPlaybackSpeed(float c);
					virtual bool HasVideo();
					virtual void SetMixVolume(const std::wstring& ident, float v);
					virtual float GetMixVolume(const std::wstring& ident);
					virtual void SetMixAlpha(const std::wstring& ident, float v);
					virtual float GetMixAlpha(const std::wstring& ident);
					virtual bool IsHorizontallyFlipped() const;
					virtual bool IsVerticallyFlipped() const;

					virtual float GetVolume() const;
					virtual int GetLength();
					virtual int GetPosition();
					virtual float GetOpacity() const;
					virtual bool IsPlaying() const;
					virtual bool IsPaused() const;
					virtual IDirect3DTexture9* GetTexture();
					virtual volatile float GetCurrentAudioLevel() const;
					static void ListDevices(std::vector< ref<Device> >& devs, std::map<std::wstring, ref<Device> >& existing);
					virtual float GetAspectRatio() const;

					virtual volatile bool IsVisible() const;
					virtual void SetVisible(bool v);

					// Keyable
					virtual bool IsKeyingEnabled() const;
					virtual void SetKeyingEnabled(bool t);
					virtual float GetKeyingTolerance() const;
					virtual void SetKeyingTolerance(float t);
					virtual RGBColor GetKeyColor() const;
					virtual void SetKeyColor(const RGBColor& col);
				
				protected:
					static void ListDevices(const IID& cat, std::vector< ref<Device> >& dev, bool isSource, std::map<std::wstring, ref<Device> >& existing);
					void Load(const std::wstring& source, ref<Device> live, ref<Device> card);
					void UpdateVolume();
					static IBaseFilter* GetSoundCard(unsigned char index);

					std::wstring _file;
					CComPtr<IGraphBuilder> _graph;
					TextureRenderer* _renderer;
					CComPtr<IMediaControl> _mc;
					CComPtr<IMediaSeeking> _ms;
					CComPtr<IBasicAudio> _ba;
					mutable CriticalSection _lock;
					bool _muted;
					bool _videoOutput;
					bool _loaded;
					float _volume;
					Mixed<float> _volumeMixer;
					Mixed<float> _alphaMixer;
					ref<OutputManager> _pm;
					PlaybackState _playing;
					volatile float _alpha;
					volatile bool _visible;
					bool _keyingEnabled;
					float _keyingTolerance;
					RGBColor _keyingColor;
			};
		}
	}
}

#endif