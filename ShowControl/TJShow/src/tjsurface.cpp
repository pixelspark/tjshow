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
#include "../include/internal/tjshow.h"
#include "../include/internal/tjsurface.h"
#include <d3d9.h>
using namespace tj::show;
using namespace tj::show::media;

TextureSurface::TextureSurface(int w, int h, IDirect3DDevice9* dev, ref<OutputManager> pm, bool visible): _scale(1.0f, 1.0f, 1.0f), _rotate(0.0f, 0.0f, 0.0f), _translate(0.0f, 0.0f, 0.0f), _visible(visible) {
	assert(pm && dev!=0);
	_device = dev;
	_texture = 0;
	_pm = pm;
	Resize(w,h);
}

TextureSurface::~TextureSurface() {
	if(_texture!=0) {
		_texture->Release();
		_texture = 0;
	}
}

volatile bool TextureSurface::IsVisible() const {
	return _visible;
}

void TextureSurface::SetVisible(bool t) {
	ThreadLock lock (&_lock);
	_visible = t;
}

bool TextureSurface::IsHorizontallyFlipped() const {
	return false;
}

bool TextureSurface::IsVerticallyFlipped() const {
	return false;
}

void TextureSurface::Resize(int w, int h) {
	ThreadLock lock(&_lock);

	if(w==0 || h==0) {
		_wt = 0.0f;
		_ht = 0.0f;
		return;
	}

	// We always create textures with sizes that are a power of 2, so it works with old video cards
	unsigned int adjW = GetNextPowerOfTwo<unsigned int>((unsigned int)w);
	unsigned int adjH = GetNextPowerOfTwo<unsigned int>((unsigned int)h);

	if(_texture) {
		// If the texture we have is as large or larger than we need, just use it
		// We do need to change the scaling factors
		if(adjW <= _tW && adjH <= _tH) {
			// Subtract 1 from w and h, so we do not get ugly edges
			_wt = float(w-1)/float(_tW);
			_ht = float(h-1)/float(_tH);
			_pm->SetDirty();
			///Log::Write(L"TJShow/TextureSurface", L"Re-using old texture for new image since it is large enough");
			return;
		}

		_texture->Release();
		_texture = 0;
	}

	_tH = adjH;
	_tW = adjW;
	// Well, just create a new texture then...
	HRESULT hr = _device->CreateTexture(adjW, adjH, 1, 0 /* D3DUSAGE_DYNAMIC was here */, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &_texture, 0);

	if(FAILED(hr)) {
		_texture = 0;
		Log::Write(L"TJShow/TextureSurface", L"Could not create texture");
		if(hr==D3DERR_OUTOFVIDEOMEMORY) {
			Throw(L"Could not create texture for TextureSurface (out of video memory)", ExceptionTypeError);
		}
		else if(hr==D3DERR_INVALIDCALL) {
			Throw(L"Could not create texture for TextureSurface (invalid call)", ExceptionTypeError);
		}
		else if(hr==E_OUTOFMEMORY) {
			Throw(L"Could not create texture for TextureSurface (out of memory)", ExceptionTypeError);
		}
		else {
			Throw(L"Could not create texture for TextureSurface (cause unknown)", ExceptionTypeError);
		}
	}

	// Subtract 1 from w and h, so we do not get ugly edges
	_wt = float(w-1)/float(_tW);
	_ht = float(h-1)/float(_tH);
	_pm->SetDirty();
}

float TextureSurface::GetTextureWidth() const {
	return _wt;
}

float TextureSurface::GetTextureHeight() const {
	return _ht;
}

void TextureSurface::SetTranslate(const tj::shared::Vector& v) {
	ThreadLock lock (&_lock);
	_translate = v;
}

void TextureSurface::SetScale(const tj::shared::Vector& v) {
	ThreadLock lock (&_lock);
	_scale = v;
}

void TextureSurface::SetRotate(const tj::shared::Vector& v) {
	ThreadLock lock (&_lock);
	_rotate = v;
}

void TextureSurface::SetMixAlpha(const std::wstring& ident, float f) {
	ThreadLock lock(&_lock);
	_alpha.SetMixValue(ident, f);
}

float TextureSurface::GetMixAlpha(const std::wstring& ident) {
	return _alpha.GetMixValue(ident, 1.0f);
}

float TextureSurface::GetOpacity() const {
	return _alpha.GetMultiplyMixValue(1.0f);
}

const Vector& TextureSurface::GetTranslate() const {
	return _translate;
}

const Vector& TextureSurface::GetRotate() const {
	return _rotate;
}

const Vector& TextureSurface::GetScale() const {
	return _scale;
}

namespace tj {
	namespace show {
		namespace media {
			class TextureSurfacePainter: public SurfacePainter {
				public:
					TextureSurfacePainter(ref<TextureSurface> ts, graphics::Bitmap* bmp): _graphics(0), _bitmap(bmp), _surface(ts) {
						_graphics = new graphics::Graphics(dynamic_cast<graphics::Image*>(_bitmap));
						if(_graphics==0) {
							Throw(L"Could not create graphics for painting a surface", ExceptionTypeError);
						}
					}

					virtual ~TextureSurfacePainter() {
						delete _graphics;
						delete _bitmap;
						_surface->EndPaint();
					}

					virtual tj::shared::graphics::Graphics* GetGraphics() {
						return _graphics;
					}

					virtual tj::shared::graphics::Bitmap* GetBitmap() {
						return _bitmap;
					}

				protected:
					ref<TextureSurface> _surface;
					tj::shared::graphics::Graphics* _graphics;
					tj::shared::graphics::Bitmap* _bitmap;
			};
		}
	}
}

float TextureSurface::GetAspectRatio() const {
	return float(_tW*_wt) / float(_tH*_ht);
}

ref<SurfacePainter> TextureSurface::GetPainter() {
	/* Create bitmap */
	ThreadLock lock (&_lock);
	if(_texture==0) return 0;

	D3DLOCKED_RECT bits;
	if(FAILED(_texture->LockRect(0, &bits, NULL, D3DLOCK_DISCARD))) {
		Throw(L"Could not lock texture bits", ExceptionTypeError);
	}

	tj::shared::graphics::Bitmap* bmp = new tj::shared::graphics::Bitmap(_tW, _tH, bits.Pitch, (BYTE*)bits.pBits);
	if(bmp==0) {
		Throw(L"Could not create bitmap", ExceptionTypeError);
	}

	return GC::Hold(new TextureSurfacePainter(this, bmp));
}

void TextureSurface::EndPaint() {
	ThreadLock lock (&_lock);
	_texture->UnlockRect(0);
}

IDirect3DTexture9* TextureSurface::GetTexture() {
	if(_texture!=0) {
		_texture->AddRef();
		return _texture;
	}
	return 0;
}
