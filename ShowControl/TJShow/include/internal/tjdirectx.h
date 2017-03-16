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
#ifndef _TJDIRECTX_H
#define _TJDIRECTX_H

//DLLEXPORT probably defined in TinyXML and some header below redefines it...
#undef DLLEXPORT 
#include <d3d9.h>
#include <dshow.h>
#include <vmr9.h>
#include "../../../Libraries/DirectShow/Streams/streams.h"

#if DIRECT3D_VERSION == 0x0900
	typedef IDirect3DIndexBuffer9	IDirect3DIndexBuffer;
	typedef IDirect3DVertexBuffer9	IDirect3DVertexBuffer;
	typedef IDirect3DDevice9		IDirect3DDevice;
	typedef IDirect3DTexture9		IDirect3DTexture;
#else
	#error DX10 (or any other version than 9.0c) not yet supported!
#endif

#endif