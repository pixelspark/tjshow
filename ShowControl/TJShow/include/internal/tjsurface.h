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
#ifndef _TJSURFACE_H
#define _TJSURFACE_H

interface IDirect3DDevice9;

namespace tj {
	namespace show {
		namespace media {
			class TextureSurface: public Surface {
				friend class TextureSurfacePainter;

				public:
					TextureSurface(int w, int h, IDirect3DDevice9* dev, ref<OutputManager> pm, bool visible);
					virtual ~TextureSurface();
					virtual void Resize(int w, int h);
					
					virtual void SetTranslate(const tj::shared::Vector& v);
					virtual void SetScale(const tj::shared::Vector& v);
					virtual void SetRotate(const tj::shared::Vector& v);

					virtual const Vector& GetTranslate() const;
					virtual const Vector& GetRotate() const;
					virtual const Vector& GetScale() const;
					virtual void SetMixAlpha(const std::wstring& ident, float a);
					virtual float GetMixAlpha(const std::wstring& ident);
					virtual float GetOpacity() const;
					virtual ref<SurfacePainter> GetPainter();

					virtual volatile bool IsVisible() const;
					virtual void SetVisible(bool t);

					virtual float GetTextureWidth() const;
					virtual float GetTextureHeight() const;

					virtual bool IsHorizontallyFlipped() const;
					virtual bool IsVerticallyFlipped() const;
					virtual float GetAspectRatio() const;

					// for PlayerWnd
					virtual IDirect3DTexture9* GetTexture();

				protected:
					virtual void EndPaint();

					Vector _translate, _scale, _rotate;
					CriticalSection _lock;
					CComPtr<IDirect3DDevice9> _device;
					IDirect3DTexture9* _texture;
					ref<OutputManager> _pm;
					Mixed<float> _alpha;
					float _wt, _ht;
					unsigned int _tH, _tW;
					volatile bool _visible;
			};
		}
	}
}

#endif