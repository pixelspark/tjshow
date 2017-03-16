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
#include "../../include/internal/tjnetwork.h"
#include "../../include/internal/view/tjplayerwnd.h"

#undef DLLEXPORT
#include <dshow.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <vmr9.h>
#include <algorithm>
using namespace tj::shared::graphics;
using namespace tj::show::media;
using namespace tj::show::view;

namespace tj {
	namespace show {
		namespace media {
			const static int D3DFVF_CUSTOMVERTEX = (D3DFVF_XYZ | D3DFVF_TEX1);
			const static int D3DFVF_SIMPLEVERTEX = (D3DFVF_XYZ );

			class HardwareEffect: public virtual Object {
				public:
					HardwareEffect(IDirect3DDevice9* device, const std::wstring& path);
					virtual ~HardwareEffect();
					virtual void BeginFrame();
					virtual void EndFrame();
					virtual bool IsPresent() const;
					virtual void BeginMesh(ref<Mesh> m, float opacity, IDirect3DTexture9* texture);
					virtual void EndMesh();
					virtual bool IsUsingAdvancedTechnique() const;
					virtual void OnLostDevice();
					virtual void OnResetDevice();
					virtual void Release();

				protected:
					ID3DXEffect* _effect;
					bool _advanced;
					D3DXHANDLE _sourceParameter, _alphaParameter, _keyingParameter, _keyingToleranceParameter, _keyingColorParameter, _blurParameter, _gammaParameter, _vignetParameter, _mosaicParameter, _saturationParameter;
			};
		}
	}
}

/** PlayerWnd **/
PlayerWnd::PlayerWnd(ref<OutputManager> mgr, strong<ScreenDefinition> def): ChildWnd(true), _def(def), _manager(mgr), _device(0), _vb(0) {
	SetText(TL(player));
}

void PlayerWnd::Init(CComPtr<IDirect3D9> d3d, unsigned int adapter) {
	_d3d = d3d;

    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
	
	// Move our window to the right location
	HMONITOR monitor = d3d->GetAdapterMonitor(adapter);
	if(monitor!=NULL) {
		MONITORINFO mi;
		mi.cbSize = sizeof(mi);
		if(!GetMonitorInfo(monitor, &mi)) {
			Throw(L"Cannot get monitor information for monitor attached to adapter "+Stringify(adapter), ExceptionTypeError);
		}

		SetWindowPos(GetWindow(), 0L, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right-mi.rcMonitor.left, mi.rcMonitor.bottom-mi.rcMonitor.top, SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOREDRAW);
	}
	else {
		Throw(L"Monitor attached to adapter "+Stringify(adapter)+L" does not exist", ExceptionTypeError);
	}

	// try hardware first
	if(adapter==0) {
		adapter = D3DADAPTER_DEFAULT; // D3DADAPTER_DEFAULT is also 0, but you never know
	}

	HRESULT hr = d3d->CreateDevice(adapter, D3DDEVTYPE_HAL, GetWindow(), D3DCREATE_SOFTWARE_VERTEXPROCESSING|D3DCREATE_MULTITHREADED, &d3dpp, &_device );                                     
	if(_device==0) {
		Log::Write(L"TJShow/PlayerWnd", L"Hardware device creation failed with hr="+StringifyHex(hr));
		
		// try software rendering
		d3d->CreateDevice(adapter, D3DDEVTYPE_SW, GetWindow(), D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &_device );                                     
		if(_device==0) {
			return;
		}
		Log::Write(L"TJShow/PlayerWnd", L"Using software rendering");
	}
	else {
		Log::Write(L"TJShow/PlayerWnd", L"Using hardware rendering");
	}

	// check caps
	D3DCAPS9 caps;
	_device->GetDeviceCaps(&caps);
	if(caps.TextureCaps & D3DPTEXTURECAPS_POW2) {
		if(!(caps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL)) {
			Log::Write(L"TJShow/PlayerWnd", L"This computer's video device does not support non-pow2 textures, video might not work!");
		}
		else {
			Log::Write(L"TJShow/PlayerWnd", L"This computer's video device conditionally supports non-pow2 textures.");
		}
	}

	// set render parameters
    _device->SetSamplerState(0,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP);
    _device->SetSamplerState(0,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP);
    _device->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);
    _device->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);

    _device->SetRenderState(D3DRS_AMBIENT,RGB(255,255,255));
    _device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    _device->SetRenderState(D3DRS_LIGHTING, TRUE);
    _device->SetRenderState(D3DRS_ZENABLE, TRUE);
	_device->SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
    _device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	///_device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	_device->SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE);
	_device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
	_device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TEXTURE);
	
	_device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	_device->SetRenderState(D3DRS_ALPHAREF,255);
	_device->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
	_device->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
	
	_device->SetFVF(D3DFVF_CUSTOMVERTEX);

	_device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
	_device->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
	_device->SetRenderState(D3DRS_WRAP0,0);
	_device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, TRUE);

	ref<Settings> settings = Application::Instance()->GetSettings()->GetNamespace(L"view.video");

	// Create effect shader
	wchar_t programDir[MAX_PATH+1];
	GetModuleFileName(GetModuleHandle(NULL), programDir, MAX_PATH);
	PathRemoveFileSpec(programDir);

	std::wstring fxFile = settings->GetValue(L"fx");
	std::wstring fxPath = std::wstring(programDir) + L"\\" + fxFile;
	_effect = GC::Hold(new HardwareEffect(_device, fxPath));

	_pmVersion = 0;
	RebuildGeometry();

	unsigned int fps = StringTo<unsigned int>(settings->GetValue(L"fps"), 25);
	StartTimer(Time(1000/fps), KRepaintTimerID);
}

PlayerWnd::~PlayerWnd() {
}

void PlayerWnd::GetObjectWorldTransform(strong<Mesh> mesh, D3DXMATRIX& matrix) {
	D3DXMatrixIdentity(&matrix);
	D3DXMATRIX other;

	// First scale the mesh to size and additional scaling
	// TODO scale to size
	const Vector& scale = mesh->GetScale();
	D3DXMatrixScaling(&other, scale.x, scale.y, scale.z);
	D3DXMatrixMultiply(&matrix, &matrix, &other);

	// Rotate (translate first, so we rotate around the center)
	D3DXMatrixTranslation(&other, -(scale.x)/2, -(scale.y)/2, 0);
	D3DXMatrixMultiply(&matrix, &matrix, &other);
	const Vector& rotate = mesh->GetRotate();
	D3DXMatrixRotationY(&other, D3DXToRadian(rotate.y));
	D3DXMatrixMultiply(&matrix, &matrix, &other);
	D3DXMatrixRotationZ(&other, D3DXToRadian(rotate.z));
	D3DXMatrixMultiply(&matrix, &matrix, &other);
	D3DXMatrixRotationX(&other, D3DXToRadian(rotate.x));
	D3DXMatrixMultiply(&matrix, &matrix, &other);
	D3DXMatrixTranslation(&other, (scale.x)/2, (scale.y)/2, 0);
	D3DXMatrixMultiply(&matrix, &matrix, &other);
	
	// Translate
	const Vector& translate = mesh->GetTranslate();
	D3DXMatrixTranslation(&other, translate.x, translate.y - _def->GetDocumentHeight(), translate.z);
	D3DXMatrixMultiply(&matrix, &matrix, &other);
}

void PlayerWnd::SetDefinition(strong<ScreenDefinition> sd) {
	_def = sd;
	ref<OutputManager> manager = _manager;
	if(!manager) return;
	manager->SetDirty();
}

strong<ScreenDefinition> PlayerWnd::GetDefinition() {
	return _def;
}

void PlayerWnd::RebuildGeometry() {
	ref<OutputManager> manager = _manager;
	if(!manager) return;
	ThreadLock(&(manager->_lock));

	if(!_device) {
		Log::Write(L"TJShow/PlayerWnd", L"No device to create vertex buffer into");
		return;
	}

	if(_meshes.size()>0 && manager->IsDirty()) {
		/* all triangles are the same for all meshes, so just create one (previous versions of this code created separate, but identical
		triangles for each Mesh, which wasn't really necessary */
		unsigned int neededSize = (unsigned int)(6 * _meshes.size() * sizeof(D3DVERTEX));
		bool needToAllocate = true;

		if(_vb) {
			D3DVERTEXBUFFER_DESC desc;
			if(SUCCEEDED(_vb->GetDesc(&desc))) {
				if(desc.Size >= neededSize) {
					// No need to re-allocate a vertex buffer, we already have one that is sufficiently large
					needToAllocate = false;
				}
			}
		}
		
		if(needToAllocate) {
			HRESULT hr = _device->CreateVertexBuffer(neededSize, D3DUSAGE_WRITEONLY, D3DFVF_CUSTOMVERTEX, D3DPOOL_MANAGED, &_vb, NULL);
			if(FAILED(hr)) {
				Log::Write(L"TJShow/PlayerWnd", L"Could not create vertex buffer (hr="+Stringify(hr));
				return;
			}
		}

		// Create quads
		D3DVERTEX* buffer = 0;
		_vb->Lock(0,0,(void**)&buffer,0);

		std::vector< weak<Mesh> >::iterator it = _meshes.begin();
		while(it!=_meshes.end()) {
			ref<Mesh> mesh = *it;
			if(mesh) {
				float tx = 0.0f;
				float ty = 0.0f;
				float tw = mesh->GetTextureWidth();
				float th = mesh->GetTextureHeight();

				if(mesh->IsHorizontallyFlipped()) {
					ty = th;
					th = 0.0f;
				}

				if(mesh->IsVerticallyFlipped()) {
					tx = tw;
					tw = 0.0f;
				}

				float z = 0.0f;
				buffer[0].x = 0.0f;		buffer[0].y = 0.0f;		buffer[0].z = z;	buffer[0].tx = tx;		buffer[0].ty = ty;
				buffer[1].x = 0.0f;		buffer[1].y = 1.0f;		buffer[1].z = z;	buffer[1].tx = tx;		buffer[1].ty = th;
				buffer[2].x = 1.0f;		buffer[2].y = 1.0f;		buffer[2].z = z;	buffer[2].tx = tw;		buffer[2].ty = th;

				buffer[3].x = 0.0f;		buffer[3].y = 0.0f;		buffer[3].z = z;	buffer[3].tx = tx;		buffer[3].ty = ty;
				buffer[4].x = 1.0f;		buffer[4].y = 1.0f;		buffer[4].z = z;	buffer[4].tx = tw;		buffer[4].ty = th;
				buffer[5].x = 1.0f;		buffer[5].y = 0.0f;		buffer[5].z = z;	buffer[5].tx = tw;		buffer[5].ty = ty;
			}

			buffer = &(buffer[6]);
			++it;
		}

		_vb->Unlock();
		_vb->PreLoad();
	}

	// Let's do some Matrix Magic©
	D3DXMATRIX matWorld;
	D3DXMatrixIdentity(&matWorld);
	_device->SetTransform( D3DTS_WORLD, &matWorld );

	// Get the data we need
	unsigned int documentWidth = _def->GetDocumentWidth();
	unsigned int documentHeight = _def->GetDocumentHeight();
	ScaleMode scaleMode = _def->GetScaleMode();
	Area rc = GetClientArea();
	float fov = 3.14159f / 4.0f;
	strong<Theme> theme = ThemeManager::GetTheme();
	float dpi = theme->GetDPIScaleFactor();
	
	// Center the view
	D3DXMATRIX matProj, matView;
	float documentCenterX = documentWidth / 2.0f;
	float documentCenterY = documentHeight / 2.0f;
	D3DXMatrixLookAtLH(&matView, &D3DXVECTOR3(documentCenterX, -documentCenterY, -1.0f),&D3DXVECTOR3(documentCenterX, -documentCenterY, 0.0f), &D3DXVECTOR3( 0.0f, 1.0f, 0.0f ));
	_device->SetTransform(D3DTS_VIEW, &matView);

	switch(scaleMode) {
		// ScaleModeNone: each pixel corresponds to one screen pixel, aspect ratio is the same
		case ScaleModeNone:
		case ScaleModeMax:
		case ScaleModeAll:
		{
			D3DXMatrixOrthoLH(&matProj, rc.GetWidth()*dpi, -int(rc.GetHeight())*dpi, 0.0f, 10000.0f);
			break;	
		}

		// ScaleModeStretch: use all available screen pixels; the aspect ratio changes
		case ScaleModeStretch:
		default:
			D3DXMatrixOrthoLH(&matProj, float(documentWidth), -float(documentHeight), 0.0f, 10000.0f);
			break;
	}
	_device->SetTransform(D3DTS_PROJECTION, &matProj);
}

void PlayerWnd::UpdateCursor() {
}

ref<Mesh> PlayerWnd::GetMeshAt(Pixels x, Pixels y, float* localX, float* localY) {
	ref<OutputManager> manager = _manager;
	if(!manager) return null;
	ThreadLock lock(&(manager->_lock));
	Area rc = GetClientArea();
	float dpi = ThemeManager::GetTheme()->GetDPIScaleFactor();

	if(_meshes.size()>0 && _vb!=0) {
		// static triangles
		D3DXVECTOR3 triangles[6];
		triangles[0].x = 0.0f;	triangles[0].y = 0.0f;	triangles[0].z = 0.0f;
		triangles[1].x = 0.0f;	triangles[1].y = 1.0f;	triangles[1].z = 0.0f;
		triangles[2].x = 1.0f;	triangles[2].y = 1.0f;	triangles[2].z = 0.0f;
		triangles[3].x = 0.0f;	triangles[3].y = 0.0f;	triangles[3].z = 0.0f;
		triangles[4].x = 1.0f;	triangles[4].y = 1.0f;	triangles[4].z = 0.0f;
		triangles[5].x = 1.0f;	triangles[5].y = 0.0f;	triangles[5].z = 0.0f;

		// Get matrices
		D3DXMATRIX matProjection, matView, matProjectionInv;
		if(FAILED(_device->GetTransform(D3DTS_PROJECTION, &matProjection))) return 0;
		if(FAILED(_device->GetTransform(D3DTS_VIEW, &matView))) return 0;

		D3DVIEWPORT9 viewport;
		_device->GetViewport(&viewport);

		D3DXVECTOR3 mouse;
		mouse.x = (x/float(rc.GetWidth()) - viewport.X) * float(viewport.Width);
		mouse.y = (y/float(rc.GetHeight()) - viewport.Y) * float(viewport.Height);
		mouse.z =  1.0f;		

		float closest = -1.0f;
		ref<Mesh> closestMesh = 0;
		float closestU = 0.0f;
		float closestV = 0.0f;
		bool closestUVSecond = false;

		std::vector< weak<Mesh> >::iterator it = _meshes.begin();
		while(it!=_meshes.end()) {
			ref<Mesh> mesh = *it;

			if(mesh) {
				// Get the inverse view matrix
				D3DXMATRIX matWorld;
				GetObjectWorldTransform(mesh, matWorld);
				D3DXVECTOR3 v1, v0;

				// Create the intersection ray
				mouse.z = 0.0f;
				D3DXVec3Unproject(&v0, &mouse, &viewport, &matProjection, &matView, &matWorld);
				mouse.z = 1.0f;
				D3DXVec3Unproject(&v1, &mouse, &viewport, &matProjection, &matView, &matWorld);
				D3DXVec3Subtract(&v1, &v1, &v0);

				// Do some intersection
				float dist = -1.0f;
				float u = 0.0f;
				float v = 0.0f;

				if(D3DXIntersectTri(&(triangles[0]), &(triangles[1]), &(triangles[2]), &v0, &v1, &u, &v, &dist)==TRUE && dist>0.0f && (dist<closest || closest<0.0f)) {
					closestU = u;
					closestV = v;
					closest = dist;
					closestMesh = mesh;
					closestUVSecond = false;
				}

				if(D3DXIntersectTri(&(triangles[3]), &(triangles[4]), &(triangles[5]), &v0, &v1, &u, &v, &dist)==TRUE && dist>0.0f && (dist<closest || closest<0.0f)) {
					closestU = u;
					closestV = v;
					closest = dist;
					closestMesh = mesh;
					closestUVSecond = true;
				}
			}
			++it;
		}

		// Convert barycentric u,v coordinates to something we can actually understand
		// (just carthesian, with (0,0) at left top and x,y ranging from [0,1])
		if(localX!=0 && localY!=0) {
			float t = 1-closestU-closestV;
			if(!closestUVSecond) {
				*localX = closestV;
				*localY = 1.0f - (closestV + closestU);
			}
			else {
				*localX = closestU + closestV;
				*localY = 1.0f - closestU;
			}
		}
		return closestMesh;
	}
	return null;
}

void PlayerWnd::OnMouse(MouseEvent ev, Pixels x, Pixels y) {
	ref<OutputManager> manager = _manager;
	if(!manager) return;

	if(ev!=MouseEventMove) {
		ThreadLock lock(&(manager->_lock));
		Interactive::MouseNotification mn;
		ref<Mesh> closestMesh = GetMeshAt(x,y,&(mn._x), &(mn._y));

		if(closestMesh && closestMesh.IsCastableTo<Interactive>()) {
			ref<Interactive> ia = closestMesh;
			mn._event = ev;
			ia->EventMouse.Fire(null, mn);

			if(ev==MouseEventLDown && ia->CanFocus()) {
				ref<Interactive> previouslyFocused = _focus;
				if(previouslyFocused) {
					previouslyFocused->SetFocus(false);
				}

				_focus = ia;
				Focus();
				ia->SetFocus(true);
			}
		}
		else {
			ref<Interactive> previouslyFocused = _focus;
			if(previouslyFocused) {
				previouslyFocused->SetFocus(false);
			}
			_focus = ref<Interactive>();
		}
	}
	
	// Update cursor state
	if(ev == MouseEventMove) {
		SetWantMouseLeave(true);
		UpdateCursor();
	}
	else if(ev==MouseEventLeave) {
		Mouse::Instance()->SetCursorHidden(false);
	}
}

void PlayerWnd::OnCharacter(wchar_t vk) {
	ref<Interactive> focused = _focus;
	if(focused) {
		Interactive::KeyNotification kn;
		kn._key = KeyCharacter;
		kn._code = vk;
		kn._down = true;
		kn._isAccelerator = false;
		focused->EventKey.Fire(null, kn);
	}
}

void PlayerWnd::OnKey(Key k, wchar_t code, bool down, bool isAccelerator) {
	if(k!=KeyCharacter) {
		ref<Interactive> focused = _focus;
		if(focused) {
			Interactive::KeyNotification kn;
			kn._key = k;
			kn._code = code;
			kn._down = down;
			kn._isAccelerator = isAccelerator;
			focused->EventKey.Fire(null, kn);
		}
	}
}

void PlayerWnd::Paint(Graphics& g, strong<Theme> theme) {
	// This method gets called from PlayerWnd::PreMessage whenever there are no meshes to paint (or something
	// goes wrong in painting with Direct3D
	Area rc = GetClientArea();
	g.Clear(theme->GetColor(Theme::ColorBackground));

	// When there are meshes and this Paint method is still called, something must be wrong
	if(_meshes.size()>0) {
		StringFormat sf;
		sf.SetAlignment(StringAlignmentCenter);
		sf.SetLineAlignment(StringAlignmentCenter);
		SolidBrush textBr(theme->GetColor(Theme::ColorActiveStart));

		std::wstring msg = TL(video_not_supported);
		g.DrawString(msg.c_str(), (int)msg.length(), theme->GetGUIFont(), rc, &sf, &textBr);
	}
}

void PlayerWnd::Reset() {
	_effect->OnLostDevice();

	if(_device!=0) {
		// try to reset the device
		D3DPRESENT_PARAMETERS d3dpp;
		ZeroMemory(&d3dpp, sizeof(d3dpp));
		d3dpp.Windowed = TRUE;
		d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
		d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
		d3dpp.EnableAutoDepthStencil = TRUE;
		d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
		d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;

		if(FAILED(_device->Reset(&d3dpp))) {
			Log::Write(L"TJShow/PlayerWnd", L"Could not reset device");
			return;
		}
		
		_effect->OnResetDevice();
		Log::Write(L"TJShow/PlayerWnd", L"Device reset");
	}
}

void PlayerWnd::CleanMeshes() {
	std::vector< weak<Mesh> >::iterator it = _meshes.begin();
	while(it!=_meshes.end()) {
		ref<Mesh> mesh = *it;
		if(!mesh) {
			it = _meshes.erase(it);
		}
		else {
			++it;
		}
	}
}

void PlayerWnd::SortMeshes() {
	if(_meshes.size()>1) {
		struct MeshSorter {
			public:
				bool operator()(weak<Mesh> a, weak<Mesh> b) {
					ref<Mesh> ra = a;
					ref<Mesh> rb = b;
					if(ra && rb) {
						return ra->GetTranslate().z > rb->GetTranslate().z;
					}
					return false;
				}
		};

		std::sort(_meshes.begin(), _meshes.end(), MeshSorter());
	}
}

LRESULT PlayerWnd::PreMessage(UINT msg, WPARAM wp, LPARAM lp) {
	if(msg==WM_CLOSE) {
		PostQuitMessage(0);
	}
	else if(msg==WM_PAINT && _device!=0) {	
		strong<Theme> theme = ThemeManager::GetTheme();
		ref<OutputManager> manager = _manager;
		if(!manager) return 0;
		ThreadLock lock(&(manager->_lock));
		HRESULT hr = S_OK;

		if(_meshes.size()>0) {
			// check to see if the device still works
			hr = _device->TestCooperativeLevel();
			if(FAILED(hr)) {
				if(hr==D3DERR_DEVICENOTRESET) {
					// device was lost, clean up unmanaged resources
					Reset();
				}
				else {
					return 0;
				}
			}

			// Rebuild geometry stuff
			CleanMeshes();
			SortMeshes();
			RebuildGeometry();

			// Clear the background of the video window
			Role role = Application::Instance()->GetNetwork()->GetRole();
			graphics::Color back = (_meshes.size()>0 || role==RoleClient) ? _def->GetBackgroundColor() : theme->GetColor(Theme::ColorBackground); // black only when video playing
			_device->Clear(0, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,D3DCOLOR_XRGB(back.GetR(),back.GetG(),back.GetB()), 1.0f, 0);
			_device->BeginScene(); 

			// Set material for meshes
			D3DMATERIAL9 mat;
			memset(&mat, 0, sizeof(D3DMATERIAL9));

			// Draw lines around scene
			if(_def->ShowGuides()) {
				mat.Diffuse.r = mat.Ambient.r = 1.0f;
				mat.Diffuse.g = mat.Ambient.g = 1.0f;
				mat.Diffuse.b = mat.Ambient.b = 1.0f;
				mat.Diffuse.a = mat.Ambient.a = 1.0f;
				_device->SetMaterial(&mat);

				D3DSIMPLEVERTEX vertex[5] = {
					{0.0f, 0.0f, 0.0f},
					{0.0f, -float(_def->GetDocumentHeight()), 0.0f},
					{float(_def->GetDocumentWidth()), -float(_def->GetDocumentHeight()), 0.0f},
					{float(_def->GetDocumentWidth()), 0.0f, 0.0f},
					{0.0f, 0.0f, 0.0f},
				};
				_device->SetFVF(D3DFVF_SIMPLEVERTEX);
				_device->DrawPrimitiveUP(D3DPT_LINESTRIP, 4, &(vertex[0]), sizeof(D3DSIMPLEVERTEX));
			}

			// Get ready to draw the meshes
			mat.Diffuse.r = mat.Ambient.r = 0.0f;
			mat.Diffuse.g = mat.Ambient.g = 0.0f;
			mat.Diffuse.b = mat.Ambient.b = 0.0f;
			mat.Diffuse.a = mat.Ambient.a = 1.0f;
			_device->SetMaterial(&mat);
			_device->SetStreamSource(0, _vb, 0, sizeof(D3DVERTEX));
			_device->SetFVF(D3DFVF_CUSTOMVERTEX);
			_effect->BeginFrame();

			if(_vb!=0) {
				// Enumerate all meshes
				std::vector< weak<Mesh> >::iterator it = _meshes.begin();

				int n = 0;
				while(it!=_meshes.end()) {
					ref<Mesh> mesh = *it;

					if(mesh && mesh->IsVisible()) {
						IDirect3DTexture9* texture = mesh->GetTexture();

						if(texture!=0) {
							// Set parameters for this mesh
							float opacity = mesh->GetOpacity();
							mat.Diffuse.a = mat.Ambient.a = opacity;
							_device->SetMaterial(&mat);
							_device->SetTexture(0, texture);

							if(_effect->IsPresent()) {
								ThreadLock effectsLock(&(mesh->_effectsLock));
								_effect->BeginMesh(mesh, opacity, texture);
							}
								
							// Transform this object
							D3DXMATRIX matrix;
							GetObjectWorldTransform(mesh, matrix);
							_device->SetTransform(D3DTS_WORLD, &matrix);

							// And draw it
							_device->DrawPrimitive(D3DPT_TRIANGLELIST, 6*n, 2);
							_effect->EndMesh();
							texture->Release();
						}
					}
					++it;
					++n;
				}
			}

			_effect->EndFrame();
			_device->EndScene();

			if(_device->Present( NULL, NULL, NULL, NULL )==D3DERR_DEVICELOST) {
				Log::Write(L"TJShow/PlayerWnd", L"Device lost after Present");
				Reset();
			}

			ValidateRect(GetWindow(),NULL);
			return 0;
		}
		// When there are no meshes, use the normal Paint method, called through Wnd::PreMessage
		
	}
	return Wnd::PreMessage(msg, wp, lp);
}

void PlayerWnd::OnTimer(unsigned int id) {
	if(id==KRepaintTimerID && IsShown()) {
		Update();
	}
}

void PlayerWnd::Update() {
	Repaint();
}

void PlayerWnd::OnSize(const Area& newSize) {
	Update();
}

bool PlayerWnd::IsHardwareEffectsPresent() {
	// These effects depend on a pixel shader
	return _effect && _effect->IsUsingAdvancedTechnique();
}

HardwareEffect::HardwareEffect(IDirect3DDevice9* device, const std::wstring& fxPath): _effect(0), _advanced(false) {
	Log::Write(L"TJShow/HardwareEffect", L"Loading effect from "+fxPath);

	if(SUCCEEDED(D3DXCreateEffectFromFile(device, fxPath.c_str(), 0, 0, 0, NULL, &_effect, NULL))) {
		_sourceParameter = _effect->GetParameterByName(NULL, "SourceTexture");
		_alphaParameter = _effect->GetParameterByName(NULL, "Alpha");
		_keyingParameter = _effect->GetParameterByName(NULL, "KeyingEnabled");
		_keyingToleranceParameter = _effect->GetParameterByName(NULL, "KeyingTolerance");
		_keyingColorParameter = _effect->GetParameterByName(NULL, "KeyingColor");
		_blurParameter = _effect->GetParameterByName(NULL, "Blur");
		_vignetParameter = _effect->GetParameterByName(NULL, "Vignet");
		_gammaParameter = _effect->GetParameterByName(NULL, "Gamma");
		_mosaicParameter = _effect->GetParameterByName(NULL, "Mosaic");
		_saturationParameter = _effect->GetParameterByName(NULL, "Saturation");

		// Select a suitable technique
		D3DXHANDLE advancedTech = _effect->GetTechniqueByName("Advanced");
		if(SUCCEEDED(_effect->ValidateTechnique(advancedTech))) {
			_effect->SetTechnique(advancedTech);
			_advanced = true;
			Log::Write(L"TJShow/HardwareEffect", L"Using advanced shader technique");
		}
		else {
			D3DXHANDLE defaultTech = _effect->GetTechniqueByName("Default");
			if(SUCCEEDED(_effect->ValidateTechnique(defaultTech))) {
				_effect->SetTechnique(defaultTech);
				Log::Write(L"TJShow/HardwareEffect", L"Using default shader technique");
			}
			else {
				Log::Write(L"TJShow/HardwareEffect", L"Not using any shading technique");
			}
		}
	}
	else {
		Log::Write(L"TJShow/HardwareEffect", L"Effect compilation failed");
	}
}

HardwareEffect::~HardwareEffect() {
	Release();
}

void HardwareEffect::Release() {
	if(_effect!=0) {
		_effect->Release();
		_effect = 0;
	}
}

void HardwareEffect::BeginFrame() {
	// Enable effect
	if(_effect!=0) {
		unsigned int passes = 0;
		_effect->Begin(&passes, 0);
		_effect->BeginPass(0);
	}
}

void HardwareEffect::EndFrame() {
	if(_effect!=0) {
		_effect->EndPass();
		_effect->End();
	}
}

bool HardwareEffect::IsPresent() const {
	return _effect!=0;
}

void HardwareEffect::BeginMesh(ref<Mesh> mesh, float opacity, IDirect3DTexture9* texture) {
	if(_effect!=0) {
		_effect->SetTexture(_sourceParameter, texture);
		_effect->SetFloat(_alphaParameter, opacity);

		// Effect parameters
		if(mesh->IsEffectPresent(Mesh::EffectBlur)) {
			_effect->SetFloat(_blurParameter, mesh->GetEffectValue(Mesh::EffectBlur, 1.0f));
		}
		else {
			_effect->SetFloat(_blurParameter, 0.0f);
		}

		if(mesh->IsEffectPresent(Mesh::EffectGamma)) {
			_effect->SetFloat(_gammaParameter, mesh->GetEffectValue(Mesh::EffectGamma, 1.0f));
		}
		else {
			_effect->SetFloat(_gammaParameter, 0.0f);
		}

		if(mesh->IsEffectPresent(Mesh::EffectVignet)) {
			_effect->SetFloat(_vignetParameter, mesh->GetEffectValue(Mesh::EffectVignet, 1.0f));
		}
		else {
			_effect->SetFloat(_vignetParameter, 0.0f);
		}

		if(mesh->IsEffectPresent(Mesh::EffectMosaic)) {
			_effect->SetFloat(_mosaicParameter, mesh->GetEffectValue(Mesh::EffectMosaic, 1.0f));
		}
		else {
			_effect->SetFloat(_mosaicParameter, 0.0f);
		}

		if(mesh->IsEffectPresent(Mesh::EffectSaturation)) {
			_effect->SetFloat(_saturationParameter, mesh->GetEffectValue(Mesh::EffectSaturation, 1.0f));
		}
		else {
			_effect->SetFloat(_saturationParameter, 1.0f);
		}

		// Keying
		bool keyingEnabled = false;
		if(mesh.IsCastableTo<Keyable>()) {
			ref<Keyable> k = mesh;
			if(k->IsKeyingEnabled()) {
				keyingEnabled = true;
				_effect->SetFloat(_keyingToleranceParameter, k->GetKeyingTolerance());
				float kc[3];
				RGBColor keyColor = k->GetKeyColor();
				kc[0] = (float)keyColor._r;
				kc[1] = (float)keyColor._g;
				kc[2] = (float)keyColor._b;
				_effect->SetFloatArray(_keyingColorParameter, kc, 3);
			}
		}
		_effect->SetBool(_keyingParameter, keyingEnabled?TRUE:FALSE);
		_effect->CommitChanges();
	}
}

void HardwareEffect::EndMesh() {
}

bool HardwareEffect::IsUsingAdvancedTechnique() const {
	return _advanced;
}

void HardwareEffect::OnLostDevice() {
	if(_effect!=0) {
		_effect->OnLostDevice();
	}
}

void HardwareEffect::OnResetDevice() {
	if(_effect!=0) {
		_effect->OnResetDevice();
	}
}