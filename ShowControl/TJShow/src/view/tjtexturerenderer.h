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
#ifndef _TJTEXTURERENDERER_H
#define _TJTEXTURERENDERER_H

#include "../../include/internal/tjdirectx.h"

namespace tj {
	namespace show {
		namespace media {
			class TextureRenderer: public CBaseVideoRenderer {
				public:
					TextureRenderer(LPUNKNOWN pUnk,HRESULT *phr, CComPtr<IDirect3DDevice> dev, int maxWidth=0, int maxHeight=0);
					virtual ~TextureRenderer();
					HRESULT CheckMediaType(const CMediaType *pmt );     // Format acceptable?
					HRESULT SetMediaType(const CMediaType *pmt );       // Video format notification
					HRESULT DoRenderSample(IMediaSample *pMediaSample); // New video sample
					CComPtr<IDirect3DTexture> GetTexture();
					bool HasVideo() const;

				protected:
					bool _dynamic;
					bool _hasVideo;
					long _width;
					long _height;
					long _pitch;
					int _bpp;
					int _maxWidth, _maxHeight;
					CComPtr<IDirect3DDevice> _dev;
					CComPtr<IDirect3DTexture> _tex;
					D3DFORMAT _format;
			};
		}
	}
}

#endif