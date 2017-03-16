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
#ifndef _TJPLAYBACK_H
#define _TJPLAYBACK_H

interface IDirect3DTexture9;

namespace tj {
	namespace shared {
		class Inspectable;
	}
}

namespace tj {
	namespace show {
		namespace view {
			class PlayerWnd;
		}
	}
}

namespace tj {
	namespace show {
		/** This is the interface for media playback (DirectShow) through TJShow. Everything is 3D, and the
		base class for all objects in the 3D space is 'Mesh'.
		
		The 'Deck' can be seen as a plain old CD player. Decks can be created using the Playback class instance
		that is given to each player when it is created (through Track::CreatePlayer). Deck may also be implemented
		as non-3D playback. 
		
		A 'Surface' is like a drawing canvas: plug-ins can paint anything on them. They too can both be implemented
		in 3D as well as 2D.

		Both Deck and Surface implement the 'Interactive' interface. When a mesh implements this interface, the user
		can use mouse and keyboard to trigger events which the plug-in can listen to. This allows for interactive user
		interface elements such as buttons, text fields and much more.
		**/
		class Keyable {
			public:
				virtual ~Keyable();
				virtual bool IsKeyingEnabled() const = 0;
				virtual void SetKeyingEnabled(bool t) = 0;
				virtual float GetKeyingTolerance() const = 0;
				virtual void SetKeyingTolerance(float t) = 0;
				virtual tj::shared::RGBColor GetKeyColor() const = 0;
				virtual void SetKeyColor(const tj::shared::RGBColor& col) = 0;
		};

		class Interactive {
			public:
				virtual ~Interactive();
				virtual bool CanFocus() const;
				virtual void SetFocus(bool f);
				virtual void SetCanFocus(bool can);

				// Input events
				struct MouseNotification {
					tj::shared::MouseEvent _event;
					float _x;
					float _y;
				};

				struct FocusNotification {
					bool _focused;
				};

				struct KeyNotification {
					tj::shared::Key _key;
					wchar_t _code;
					bool _down;
					bool _isAccelerator;
				};

				tj::shared::Listenable<MouseNotification> EventMouse;
				tj::shared::Listenable<FocusNotification> EventFocusChanged;
				tj::shared::Listenable<KeyNotification> EventKey;

			protected:
				Interactive();

			private:
				bool _canFocus;
		};

		class Mesh: public virtual tj::shared::Object {
			friend class tj::show::view::PlayerWnd;

			public:
				virtual ~Mesh();

				virtual void SetTranslate(const tj::shared::Vector& v) = 0;
				virtual void SetScale(const tj::shared::Vector& v) = 0;
				virtual void SetRotate(const tj::shared::Vector& v) = 0;

				virtual const tj::shared::Vector& GetTranslate() const = 0;
				virtual const tj::shared::Vector& GetRotate() const = 0;
				virtual const tj::shared::Vector& GetScale() const = 0;

				virtual float GetOpacity() const = 0;
				virtual void SetMixAlpha(const std::wstring& ident, float v) = 0;
				virtual float GetMixAlpha(const std::wstring& ident) = 0;

				virtual volatile bool IsVisible() const = 0;
				virtual void SetVisible(bool v) = 0;

				virtual float GetTextureWidth() const = 0;
				virtual float GetTextureHeight() const = 0;

				virtual bool IsHorizontallyFlipped() const = 0;
				virtual bool IsVerticallyFlipped() const = 0;
				virtual float GetAspectRatio() const = 0; // equals w/h

				// Effects
				enum EffectParameter {
					EffectNone = 0,
					EffectBalance,
					EffectBlur,
					EffectGamma,
					EffectVignet,
					EffectMosaic,
					EffectSaturation,
					_EffectLast,
				};

				virtual void SetMixEffect(const std::wstring& ident, EffectParameter ep, float value);
				virtual float GetMixEffect(const std::wstring& ident, EffectParameter ep, float defaultValue);
				virtual float GetEffectValue(EffectParameter ep, float source = 1.0f);
				virtual void RemoveMixEffect(const std::wstring& ident, EffectParameter ep);
				virtual void RestrictMixEffects(const std::wstring& ident, std::set<EffectParameter> eps);
				virtual bool IsEffectPresent(EffectParameter ep);

			private:
				tj::shared::CriticalSection _effectsLock;
				/* This method returns the texture for this mesh. It can return 0 (for example, when there
				is no video playing or the mesh is hidden). In that case, nothing will be drawn. The returned
				pointer is AddRef'ed and the caller should Release() it. */
				virtual IDirect3DTexture9* GetTexture() = 0; 
				std::map< EffectParameter, tj::shared::Mixed<float> > _effects;
		};

		class Deck: public Mesh, public Keyable, public Interactive {
			public:
				virtual ~Deck();

				virtual void Load(const std::wstring& fn, ref<Device> card) = 0;
				virtual void LoadLive(ref<Device> source, ref<Device> card) = 0;
				virtual bool IsLoaded() const = 0;
				virtual void Stop() = 0;
				virtual void Play() = 0;
				virtual bool IsPlaying() const = 0;
				virtual bool IsPaused() const = 0;
				virtual void Pause() = 0;
				virtual void Jump(Time t) = 0;
				virtual void SetVolume(float v) = 0;
				virtual float GetVolume() const = 0;
				virtual void SetMixVolume(const std::wstring& ident, float v) = 0; // v is [0.0f, 1.0f]
				virtual float GetMixVolume(const std::wstring& ident) = 0;
				virtual void SetBalance(int b) = 0; // b is [-10000, 10000]
				virtual void SetAudioOutput(bool a) = 0;
				virtual void SetVideoOutput(bool v) = 0;
				virtual std::wstring GetFileName() const = 0;
				virtual void SetPlaybackSpeed(float c) = 0;
				virtual bool HasVideo() = 0;
				virtual int GetLength() = 0;
				virtual int GetPosition() = 0;
				virtual volatile float GetCurrentAudioLevel() const = 0;
		};

		/* Interface for painting on 3D-'surfaces' (currently quads) */
		class SurfacePainter {
			public:
				virtual tj::shared::graphics::Graphics* GetGraphics() = 0;
				virtual tj::shared::graphics::Bitmap* GetBitmap() = 0;
				virtual ~SurfacePainter();
		};

		class Surface: public Mesh, public Interactive {
			public:
				virtual ~Surface();
				virtual ref<SurfacePainter> GetPainter() = 0;
				virtual void Resize(int w, int h) = 0;
		};

		class Outlet;

		/** Interface for playback engine. It facilitates creation of Decks, and behind the scenes
		it handles some other playback related things. Implemented by TJShow's OutputManager **/
		class Playback: public virtual tj::shared::Object {
			public:
				virtual ~Playback() {
				}

				// Resources
				virtual strong<tj::shared::ResourceProvider> GetResources() = 0;

				// Decks, surfaces and screens
				virtual ref<Deck> CreateDeck(bool visible = true, ref<Device> screen = 0) = 0;
				virtual ref<Surface> CreateSurface(int w, int h, bool visible = true, ref<Device> screen = 0) = 0;
				virtual bool IsKeyingSupported(ref<Device> screen = 0) = 0;

				// Patching
				virtual ref<Device> GetDeviceByPatch(const tj::np::PatchIdentifier& t) = 0;
				virtual ref<tj::shared::Property> CreateSelectPatchProperty(const std::wstring& name, ref<tj::shared::Inspectable> holder, tj::np::PatchIdentifier* pi) = 0;

				template<class T> ref<T> GetDeviceByPatch(const tj::np::PatchIdentifier& t) {
					ref<Device> dev = GetDeviceByPatch(t);
					if(dev && dev.IsCastableTo<T>()) {
						return ref<T>(dev);
					}
					return 0; // wrong type
				}

				// Other stuff
				virtual strong<tj::shared::Property> CreateParsedVariableProperty(const std::wstring& name, ref<tj::shared::Inspectable> holder, std::wstring* source, bool multiLine = false) = 0;
				virtual std::wstring ParseVariables(const std::wstring& source) = 0;
				virtual void QueueDownload(const std::wstring& rid) = 0;
				virtual void SetOutletValue(ref<Outlet> outlet, const tj::shared::Any& value) = 0;
				virtual bool IsFeatureAvailable(const std::wstring& feature) = 0;
		};
	}
}

#endif
