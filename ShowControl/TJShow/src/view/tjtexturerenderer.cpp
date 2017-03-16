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
#include "../../include/internal/tjshow.h"
#undef DLLEXPORT
#include "tjtexturerenderer.h"
#include <dvdmedia.h>
using namespace tj::show::media;

struct __declspec(uuid("{71771540-2017-11cf-ae26-0020afd77887}")) CLSID_TextureRenderer;

TextureRenderer::TextureRenderer(LPUNKNOWN pUnk, HRESULT* phr, CComPtr<IDirect3DDevice> dev, int maxWidth, int maxHeight): CBaseVideoRenderer(__uuidof(CLSID_TextureRenderer), NAME("TJ Texture Renderer"), pUnk, phr), _dynamic(false), _dev(dev), _maxWidth(maxWidth), _maxHeight(maxHeight) {	
	_bpp = 0;
	_hasVideo = false;
}

TextureRenderer::~TextureRenderer() {
}

CComPtr<IDirect3DTexture> TextureRenderer::GetTexture() {
	return _tex;
}

HRESULT TextureRenderer::CheckMediaType(const CMediaType *pmt) {
    VIDEOINFO *pvi = 0;
    if(pmt==0) return E_POINTER;

    // Reject the connection if this is not a video type
	if(*pmt->FormatType()!=FORMAT_VideoInfo && *pmt->FormatType()!=FORMAT_VideoInfo2) {
        return E_INVALIDARG;
    }

	if(IsEqualGUID( *pmt->Type(),MEDIATYPE_Video)) {
		 if(IsEqualGUID( *pmt->Subtype(), MEDIASUBTYPE_RGB24)) {
			return S_OK;
		 }
		 else if(IsEqualGUID( *pmt->Subtype(), MEDIASUBTYPE_RGB32) || IsEqualGUID(*pmt->Subtype(), MEDIASUBTYPE_ARGB32)) {
			//Log::Write(L"TJShow/TextureRenderer", L"Experimental RGB32 support used");
			return S_OK;
		 }
		 else if(IsEqualGUID(*pmt->Subtype(), MEDIASUBTYPE_UYVY)) {
			Log::Write(L"TJShow/TextureRenderer", L"Experimental UYVY support attempted, disabled in this version");
			return E_FAIL;
			//return S_OK;
		 }
		 else {
			return E_FAIL;
		 }
    }
	else {
		Log::Write(L"TJShow/TextureRenderer", L"Sorry, but you cannot show anything other than video on a screen ;)");
	}

	return E_FAIL;
}

bool TextureRenderer::HasVideo() const {
	return _hasVideo;
}

HRESULT TextureRenderer::SetMediaType(const CMediaType *pmt) {
    HRESULT hr;
	//Log::Write(L"TJShow/TextureDeck", L"SetMediaType");
	_hasVideo = true;

	if(*pmt->FormatType() == FORMAT_VideoInfo2) {
		VIDEOINFOHEADER2* pvi = (VIDEOINFOHEADER2*)pmt->Format();
		_width = pvi->bmiHeader.biWidth;
		_height = pvi->bmiHeader.biHeight;
	}
	else if(*pmt->FormatType() == FORMAT_VideoInfo) {
		VIDEOINFO *pviBmp = (VIDEOINFO*)pmt->Format();
		_width  = pviBmp->bmiHeader.biWidth;
		_height = abs(pviBmp->bmiHeader.biHeight);	
	}

	D3DFORMAT format;

	if(IsEqualGUID( *pmt->Subtype(), MEDIASUBTYPE_RGB32) || IsEqualGUID(*pmt->Subtype(), MEDIASUBTYPE_ARGB32)) {
		_pitch  = (_width * 4);
		_bpp = 4;
		format = D3DFMT_X8R8G8B8;
	}
	else if(IsEqualGUID( *pmt->Subtype(), MEDIASUBTYPE_RGB24)) {
		_pitch  = (_width * 3 + 3) & ~(3);
		_bpp = 3;
		format = D3DFMT_X8R8G8B8;
	}
	else if(IsEqualGUID(*pmt->Subtype(), MEDIASUBTYPE_UYVY)) {
		_pitch = (_width*2);
		_bpp = 2;
		format = D3DFMT_UYVY;
	}

	// If video is too large, limit the size (but do not touch the stride)
	if(_maxWidth!=0 && _width > _maxWidth) {
		Log::Write(L"TJShow/TextureRenderer", L"Video is too large, limiting width to "+Stringify(_maxWidth)+L" (width was "+Stringify(_width)+L")");
		_width = _maxWidth;
	}

	if(_maxHeight!=0 && _height > _maxHeight) {
		Log::Write(L"TJShow/TextureRenderer", L"Video is too large, limiting height to "+Stringify(_maxHeight)+L" (height was "+Stringify(_height)+L")");
		_height = _maxHeight;
	}

    _dynamic = true;

    // Create the texture that maps to this media type
    hr = E_UNEXPECTED;
    if(_dynamic) {
        hr = _dev->CreateTexture(_width, _height, 1, D3DUSAGE_DYNAMIC,format,D3DPOOL_DEFAULT,&_tex, NULL);
        if(FAILED(hr)) {
			Log::Write(L"TJShow/TextureRenderer", L"Trying non-dynamic textures, device won't create dynamic texture");
            _dynamic = false;
        }
    }
   
	if(!_dynamic) {
        hr = _dev->CreateTexture(_width, _height, 1, 0,format,D3DPOOL_MANAGED, &_tex, NULL);
    }

    if(FAILED(hr)) {
		Log::Write(L"TJShow/TextureRenderer", L"Couldn't create texture (size "+Stringify(_width)+L"x"+Stringify(_height)+L")");
        return hr;
    }

    // CreateTexture can silently change the parameters on us
    D3DSURFACE_DESC ddsd;
    ZeroMemory(&ddsd, sizeof(ddsd));

    if(FAILED(hr = _tex->GetLevelDesc(0, &ddsd))) {
		Log::Write(L"TJShow/TextureRenderer", L"Could not get texture level description");
        return hr;
    }

    CComPtr<IDirect3DSurface9> pSurf;
	if(SUCCEEDED(hr = _tex->GetSurfaceLevel(0, &pSurf))) {
        pSurf->GetDesc(&ddsd);
	}
	else {
		Log::Write(L"TJShow/TextureRenderer", L"Could not get texture surface");
	}

    // Save format info
	_format = ddsd.Format;
	_height = ddsd.Height;
	_width = ddsd.Width;
	
    return S_OK;
}

HRESULT TextureRenderer::DoRenderSample(IMediaSample * pSample) {
	BYTE  *pBmpBuffer, *pTxtBuffer;
    LONG  lTxtPitch;

    BYTE  * pbS = NULL;
    DWORD * pdwS = NULL;
    DWORD * pdwD = NULL;
    UINT row, col, dwordWidth;

	if(pSample==0) {
		Log::Write(L"TJShow/TextureRenderer", L"Sample equals 0, not drawing anything!");
		return E_POINTER;
	}

	if(!_tex) {
		Log::Write(L"TJShow/TextureRenderer", L"No texture to draw on in DoRenderSample");
		return E_UNEXPECTED;
	}

    // Get the video bitmap buffer
	if(FAILED(pSample->GetPointer(&pBmpBuffer))) {
		Log::Write(L"TJShow/TextureRenderer", L"Could not get video bitmap buffer pointer!");
		return E_UNEXPECTED;
	}

    // Lock the Texture
    D3DLOCKED_RECT d3dlr;
    if(_dynamic) {
		if(FAILED(_tex->LockRect(0, &d3dlr, 0, D3DLOCK_DISCARD))) {
			Log::Write(L"TJShow/TextureRenderer", L"Could not lock texture rectangle with discard");
            return E_FAIL;
		}
    }
	else {
		if(FAILED(_tex->LockRect(0, &d3dlr, 0, 0))) {
			Log::Write(L"TJShow/TextureRenderer", L"Could not lock texture rectangle without discard");
            return E_FAIL;
		}
    }

    // Get the texture buffer & pitch
    pTxtBuffer = static_cast<byte *>(d3dlr.pBits);
	lTxtPitch = d3dlr.Pitch;

	if(_format==D3DFMT_UYVY) {
		DWORD* textureBits = (DWORD*)pTxtBuffer;
		DWORD* bitmapBits = (DWORD*)pBmpBuffer;
			
		int lineWidth = _width/2;
		for(unsigned int y=0;y<(unsigned int)_height;y++) {
			DWORD* txtLine = textureBits + y*lineWidth;
			DWORD* bmpLine = bitmapBits + y*lineWidth;

			for(unsigned int x=0;x<(unsigned int)(_width/2);x++) {
				txtLine[x] = bmpLine[x];
			}
		}
	}
	// TODO pointer arithmetic probably problematic on 64 bit
	else if (_format == D3DFMT_X8R8G8B8 || _format==D3DFMT_A8R8G8B8) {
        dwordWidth = _width / 4;

        for(row = 0;row< (UINT)_height; row++) {
            pdwS = ( DWORD*)pBmpBuffer;
            pdwD = ( DWORD*)pTxtBuffer;

            for(col = 0;col < dwordWidth; col ++) {
				if(_bpp==3) {
					pdwD[0] =  pdwS[0] | 0xFF000000;
					pdwD[1] = ((pdwS[1]<<8)  | 0xFF000000) | (pdwS[0]>>24);
					pdwD[2] = ((pdwS[2]<<16) | 0xFF000000) | (pdwS[1]>>16);
					pdwD[3] = 0xFF000000 | (pdwS[2]>>8);
				}
				else {
					pdwD[0] = pdwS[0];
					pdwD[1] = pdwS[1];
					pdwD[2] = pdwS[2];
					pdwD[3] = pdwS[3];
				}
                pdwD += 4;
                pdwS += _bpp;
            }

            // we might have remaining (misaligned) bytes here
            pbS = (BYTE*) pdwS;
            for( col = 0; col < (UINT)_width % 4; col++) {
                *pdwD = 0xFF000000 | (pbS[2] << 16) | (pbS[1] <<  8) | (pbS[0]);
                pdwD++;
                pbS += _bpp;
            }

            pBmpBuffer  += _pitch;
            pTxtBuffer += lTxtPitch;
        }
    }
	else if(_format == D3DFMT_A1R5G5B5) {
        for(int y = 0; y < _height; y++) {
            BYTE *pBmpBufferOld = pBmpBuffer;
            BYTE *pTxtBufferOld = pTxtBuffer;

            for(int x = 0; x < _width; x++) {
                *(WORD *)pTxtBuffer = (WORD)(0x8000 + ((pBmpBuffer[2] & 0xF8) << 7) +((pBmpBuffer[1] & 0xF8) << 2) + (pBmpBuffer[0] >> 3));
                pTxtBuffer += 2;
                pBmpBuffer += 3;
            }

            pBmpBuffer = pBmpBufferOld + _pitch;
            pTxtBuffer = pTxtBufferOld + lTxtPitch;
        }
    }
	else {
		Log::Write(L"TJShow/TextureRenderer", L"RenderSample with an unsupported format!");
	}

    // Unlock the Texture
	if (FAILED(_tex->UnlockRect(0))) {
		Log::Write(L"TJShow/TextureRenderer", L"Could not unlock texture rectangle");
        return E_FAIL;
	}

	_tex->PreLoad();
	return S_OK;
}
