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
#ifndef _TJPLAYER_H
#define _TJPLAYER_H

interface IDirect3D9;
interface IDirect3DDevice9;
interface IDirect3DVertexBuffer9;
interface ID3DXFont;
interface ID3DXEffect;
struct D3DXMATRIX;

namespace tj {
	namespace show {
		class Player;
		class MediaWrapper;
		class OutputManager;

		namespace media {
			class TextureDeck;
			class HardwareEffect;

			struct D3DVERTEX {
				float x,y,z;
				float tx, ty;
			};

			struct D3DSIMPLEVERTEX {
				float x,y,z;
			};
		}

		namespace view {
			class PlayerWnd: public ChildWnd {
				friend class OutputManager;

				public:
					PlayerWnd(ref<OutputManager> mgr, strong<ScreenDefinition> def);
					virtual ~PlayerWnd();
					virtual void Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme);
					virtual LRESULT PreMessage(UINT msg, WPARAM wp, LPARAM lp);
					virtual void Update();
					virtual void SetDefinition(strong<ScreenDefinition> sd);
					virtual strong<ScreenDefinition> GetDefinition();
					
					virtual bool IsHardwareEffectsPresent();
					void SortMeshes();
					void CleanMeshes();
					
				protected:
					void Reset();
					void Init(CComPtr<IDirect3D9> d3d, unsigned int adapter);
					virtual void RebuildGeometry();
					ref<Mesh> GetMeshAt(Pixels x, Pixels y, float* localX = 0, float* localY = 0);
					void GetObjectWorldTransform(strong<Mesh> mesh, D3DXMATRIX& matrix);
					virtual void UpdateCursor();

					virtual void OnSize(const Area& newSize);
					virtual void OnMouse(MouseEvent ev, Pixels x, Pixels y);
					virtual void OnKey(Key k, wchar_t code, bool down, bool isAccelerator);
					virtual void OnCharacter(wchar_t t);
					virtual void OnTimer(unsigned int id);
					
					const static unsigned int KRepaintTimerID = 1;

				private:
					strong<ScreenDefinition> _def;
					weak<OutputManager> _manager;
					CComPtr<IDirect3DDevice9> _device;
					CComPtr<IDirect3D9> _d3d;
					CComPtr<IDirect3DVertexBuffer9> _vb;
					ref<media::HardwareEffect> _effect;
					volatile int _pmVersion;
					weak<Interactive> _focus;
					std::vector< weak<Mesh> > _meshes;
			};
		}
	}
}

#endif