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
#include "../include/internal/tjnetwork.h"
#include "../include/internal/view/tjplayerwnd.h"
#include "../include/internal/view/tjsettingswnd.h"
#include "view/tjtexturerenderer.h"
using namespace tj::show::media;
using namespace tj::show::view;
using namespace tj::shared::graphics;

class SelectPatchPropertyWnd: public ChildWnd {
	public:
		SelectPatchPropertyWnd(PatchIdentifier* pi, ref<Inspectable> holder): ChildWnd(L""), _holder(holder), _linkIcon(L"icons/port.png"), _pi(pi) {
			assert(pi!=0);
			SetWantMouseLeave(true);
		}

		virtual ~SelectPatchPropertyWnd() {
		}

		virtual void Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme) {
			Area rc = GetClientArea();
			g.Clear(theme->GetColor(Theme::ColorBackground));

			if(IsMouseOver()) {
				theme->DrawToolbarBackground(g, 0.0f, 0.0f, float(rc.GetWidth()), float(rc.GetHeight()));
			}

			_linkIcon.Paint(g, Area(0,0,16,16));
			
			// Get patch/device information
			ref<Inspectable> holder = _holder;
			if(holder) {
				PatchIdentifier pi = *_pi;
				DeviceIdentifier di = Application::Instance()->GetModel()->GetPatches()->GetDeviceByPatch(pi);
				ref<Device> dev = PluginManager::Instance()->GetDeviceByIdentifier(di);
				
				SolidBrush text(dev?theme->GetColor(Theme::ColorText):theme->GetColor(Theme::ColorActiveStart));

				std::wstring info = pi;
				if(dev) {
					info += L" (" + dev->GetFriendlyName()+L")";
				}
				g.DrawString(info.c_str(), (int)info.length(), theme->GetGUIFont(), PointF(20.0f, 2.0f), &text);
			}
		}

	protected:	
		virtual void OnSize(const Area& ns) {
			Repaint();
		}

		virtual void OnMouse(MouseEvent ev, Pixels x, Pixels y) {
			// this fixes some painting glitches in PropertyGridWnd
			Wnd* p = GetParent();
			if(p!=0) p->Repaint();

			if(ev==MouseEventLUp) {
				ContextMenu cm;
				cm.AddItem(TL(patch_none), 0, false, (*_pi)==L"");

				ref<Patches> ps = Application::Instance()->GetModel()->GetPatches();
				if(ps) {
					std::vector<PatchIdentifier> options;

					if(ps->GetPatchCount()<1) {
						cm.AddItem(TL(patches_none_defined), -1, false, false);
					}
					else {
						int n = 1;
						const std::map<PatchIdentifier, DeviceIdentifier>& patches = ps->GetPatches();
						std::map<PatchIdentifier, DeviceIdentifier>::const_iterator it = patches.begin();
						while(it!=patches.end()) {
							++n;
							options.push_back(it->first);
							cm.AddItem(it->first, n, false, (it->first == *_pi)?MenuItem::RadioChecked:MenuItem::NotChecked);
							++it;
						}
					}

					cm.AddSeparator();
					strong<MenuItem> patchesLink = GC::Hold(new MenuItem(TL(patches_link), 1, false, MenuItem::NotChecked));
					patchesLink->SetLink(true);
					cm.AddItem(patchesLink);

					Area rc = GetClientArea();
					int r = cm.DoContextMenu(this, rc.GetLeft(), rc.GetBottom());
					if(r>=2) {
						SetPatch(options.at(r-2));
					}
					else if(r==1) {
						// go to patches stuff
						ref<View> view = Application::Instance()->GetView();
						if(view) {
							view->ShowPatcher();
						}
					}
					else if(r==0) {
						SetPatch(L"");
					}
				}
				Repaint();
			}
			else if(ev==MouseEventMove||ev==MouseEventLeave) {
				Repaint();
			}
		}

		void SetPatch(const PatchIdentifier& pi) {
			ref<Inspectable> holder = _holder;
			if(pi!=(*_pi)) {
				UndoBlock::AddAndDoChange(GC::Hold(new PropertyChange<PatchIdentifier>(holder,L"", _pi, *_pi, pi)));
			}
		}

		Icon _linkIcon;
		PatchIdentifier* _pi;
		weak<Inspectable> _holder;
};


class SelectPatchProperty: public Property {
	public:
		SelectPatchProperty(const std::wstring& name, ref<Inspectable> holder, PatchIdentifier* pi): Property(name), _pi(pi), _holder(holder) {
			assert(pi!=0);
		}

		virtual ~SelectPatchProperty() {
		}

		virtual ref<Wnd> GetWindow() {
			if(!_wnd) {
				_wnd = GC::Hold(new SelectPatchPropertyWnd(_pi, _holder));
			}
			return _wnd;
		}
		
		// Called when a repaint is about to begin and the value in the window needs to be updated
		virtual void Update() {
			ref<Inspectable> holder = _holder;
			if(_wnd && holder && _pi!=0L) {
				_wnd->Update();
			}
		}

		virtual int GetHeight() {
			return 17;
		}

	protected:
		weak<Inspectable> _holder;
		PatchIdentifier* _pi;
		ref<SelectPatchPropertyWnd> _wnd;
};

/* ScreenDevice */
class ScreenDevice: public Device {
	public:
		ScreenDevice(ref<OutputManager> om, int idx, const std::wstring& name, strong<ScreenDefinition> def);
		virtual ~ScreenDevice();
		virtual std::wstring GetFriendlyName() const;
		virtual DeviceIdentifier GetIdentifier() const;
		virtual ref<Icon> GetIcon();
		virtual bool IsMuted() const;
		virtual void SetMuted(bool t) const;
		virtual int GetIndex() const;
		virtual strong<ScreenDefinition> GetDefinition() const;

	protected:
		weak<OutputManager> _om;
		strong<ScreenDefinition> _definition;
		int _index;
		ref<Icon> _icon;
		std::wstring _name;
};

/** ScaleMode **/
void Enumeration<ScaleMode>::InitializeMapping() {
	Add(ScaleModeNone, L"none", TL(scale_mode_none));
	Add(ScaleModeAll, L"all", TL(scale_mode_all));
	Add(ScaleModeMax, L"max", TL(scale_mode_max));
	Add(ScaleModeStretch, L"stretch", TL(scale_mode_stretch));
}

Enumeration<ScaleMode> Enumeration<ScaleMode>::Instance;

/** ScreenDefinition **/
ScreenDefinition::ScreenDefinition(strong<Settings> templ) {
	_isTouchScreen = templ->GetFlag(L"is-touch", false);
	_documentWidth = StringTo<int>(templ->GetValue(L"document-width", L"1024"), 1024);
	_documentHeight = StringTo<int>(templ->GetValue(L"document-height", L"768"), 768);
	_scaleMode = Enumeration<ScaleMode>::Instance.Unserialize(templ->GetValue(L"scale", L"none"), ScaleModeNone);
	_isBuiltIn = false;

	std::wstring backColor = templ->GetValue(L"background-color", L"000000");
	std::wistringstream wis(backColor);
	unsigned int color = 0;
	wis >> std::hex >> color;
	_backgroundColor = RGBColor((unsigned char)((color >> 16) & 0xFF),(unsigned char)((color >> 8) & 0xFF),(unsigned char)(color & 0xFF));
}

ScreenDefinition::ScreenDefinition(bool isTouchScreen, unsigned int docWidth, unsigned int docHeight, ScaleMode scale):
	_isTouchScreen(isTouchScreen),
	_documentWidth(docWidth),
	_documentHeight(docHeight),
	_scaleMode(scale),
	_backgroundColor(0.0,0.0,0.0), 
	_isBuiltIn(false),
	_showGuides(false) {
}

ScreenDefinition::ScreenDefinition():
	_isTouchScreen(false),
	_documentWidth(0),
	_documentHeight(0),
	_scaleMode(ScaleModeNone),
	_backgroundColor(0.0, 0.0, 0.0),
	_isBuiltIn(false),
	_showGuides(false) {
}

ScreenDefinition::~ScreenDefinition() {
}

bool ScreenDefinition::IsBuiltIn() const {
		return _isBuiltIn;
}

bool ScreenDefinition::IsTouchScreen() const {
	return _isTouchScreen;
}

unsigned int ScreenDefinition::GetDocumentWidth() const {
	return _documentWidth;
}

unsigned int ScreenDefinition::GetDocumentHeight() const {
	return _documentHeight;
}

ScaleMode ScreenDefinition::GetScaleMode() const {
	return _scaleMode;
}

const RGBColor& ScreenDefinition::GetBackgroundColor() const {
	return _backgroundColor;
}

bool ScreenDefinition::ShowGuides() const {
	return _showGuides;
}

void ScreenDefinition::SetShowGuides(bool t) {
	_showGuides = t;
}

ref<PropertySet> ScreenDefinition::GetProperties() {
	ref<PropertySet> ps = GC::Hold(new PropertySet());
	ps->Add(GC::Hold(new GenericProperty<bool>(TL(screen_is_touch), this, &_isTouchScreen, _isTouchScreen)));
	ps->Add(GC::Hold(new GenericProperty<unsigned int>(TL(screen_document_width), this, &_documentWidth, _documentWidth)));
	ps->Add(GC::Hold(new GenericProperty<unsigned int>(TL(screen_document_height), this, &_documentHeight, _documentHeight)));

	ref<GenericListProperty<ScaleMode> > sp = GC::Hold(new GenericListProperty<ScaleMode>(TL(screen_scale_mode), this, &_scaleMode,  _scaleMode, L"icons/scale.png"));
	sp->AddOption(TL(scale_mode_none), ScaleModeNone);
	/*sp->AddOption(TL(scale_mode_max), ScaleModeMax);
	sp->AddOption(TL(scale_mode_all), ScaleModeAll);*/
	sp->AddOption(TL(scale_mode_stretch), ScaleModeStretch);
	ps->Add(sp);

	ps->Add(GC::Hold(new ColorProperty(TL(screen_background_color), this, &_backgroundColor)));
	ps->Add(GC::Hold(new GenericProperty<bool>(TL(screen_show_guides), this, &_showGuides, _showGuides)));

	return ps;
}

void ScreenDefinition::SetBuiltIn(bool b) {
	_isBuiltIn = b;
}

void ScreenDefinition::Load(TiXmlElement* el) {
	_isBuiltIn = LoadAttributeSmall(el, "built-in", _isBuiltIn);
	_isTouchScreen = LoadAttributeSmall(el, "is-touch", _isTouchScreen);
	_showGuides = LoadAttributeSmall(el, "show-guides", _showGuides);
	_documentWidth = LoadAttributeSmall(el, "document-width", _documentWidth);
	_documentHeight = LoadAttributeSmall(el, "document-height", _documentHeight);
	_scaleMode = Enumeration<ScaleMode>::Instance.Unserialize(LoadAttributeSmall<std::wstring>(el, "scale", Enumeration<ScaleMode>::Instance.Serialize(_scaleMode)), ScaleModeNone);
	
	TiXmlElement* backColor = el->FirstChildElement("background-color");
	if(backColor!=0) {
		_backgroundColor.Load(backColor);
	}
}

void ScreenDefinition::Save(TiXmlElement* el) {
	SaveAttributeSmall(el, "built-in", _isBuiltIn);
	SaveAttributeSmall(el, "is-touch", _isTouchScreen);
	SaveAttributeSmall(el, "document-width", _documentWidth);
	SaveAttributeSmall(el, "document-height", _documentHeight);
	SaveAttributeSmall(el, "show-guides", _showGuides);
	SaveAttributeSmall<std::wstring>(el, "scale", Enumeration<ScaleMode>::Instance.Serialize(_scaleMode));

	TiXmlElement backColor("background-color");
	_backgroundColor.Save(&backColor);
	el->InsertEndChild(backColor);
}

/** ScreenDefinitions **/
ScreenDefinitions::ScreenDefinitions() {
}

ScreenDefinitions::~ScreenDefinitions() {
}

ref<ScreenDefinition> ScreenDefinitions::GetDefinitionByID(const std::wstring& screenDefID) {
	std::map< std::wstring, ref<ScreenDefinition> >::iterator it = _defs.find(screenDefID);
	if(it!=_defs.end()) {
		return it->second;
	}
	return null;
}

void ScreenDefinitions::SetDefinition(const std::wstring& id, ref<ScreenDefinition> def) {
	_defs[id] = def;
}

void ScreenDefinitions::Clear() {
	_defs.clear();
}

unsigned int ScreenDefinitions::GetDefinitionCount() const {
	return (unsigned int)_defs.size();
}

void ScreenDefinitions::Save(TiXmlElement* parent) {
	std::map< std::wstring, ref<ScreenDefinition> >::iterator it = _defs.begin();
	while(it!=_defs.end()) {
		TiXmlElement def("definition");
		SaveAttributeSmall(&def, "id", it->first);
		it->second->Save(&def);
		parent->InsertEndChild(def);
		++it;
	}
}

void ScreenDefinitions::Load(TiXmlElement* parent) {
	TiXmlElement* def = parent->FirstChildElement("definition");
	while(def!=0) {
		std::wstring id = LoadAttributeSmall<std::wstring>(def, "id", L"");
		if(id.length()>0) {
			strong<ScreenDefinition> sd = GC::Hold(new ScreenDefinition());
			sd->Load(def);
			_defs[id] = sd;
		}
		def = def->NextSiblingElement("definition");
	}
}

/** ScreenDevice **/
ScreenDevice::ScreenDevice(ref<OutputManager> om, int idx, const std::wstring& name, strong<ScreenDefinition> sd): _om(om), _index(idx), _name(name), _definition(sd) {
}

ScreenDevice::~ScreenDevice() {
}

std::wstring ScreenDevice::GetFriendlyName() const {
	return TL(screen)+std::wstring(L" ")+Stringify(_index)+L": "+_name;
}

DeviceIdentifier ScreenDevice::GetIdentifier() const {
	return L"@tj:screen:"+Stringify(_index);
}

strong<ScreenDefinition> ScreenDevice::GetDefinition() const {
	return _definition;
}

ref<Icon> ScreenDevice::GetIcon() {
	if(!_icon) {
		_icon = GC::Hold(new Icon(L"icons/devices/screen.png"));
	}
	return _icon;
}

bool ScreenDevice::IsMuted() const {
	return false;
}

void ScreenDevice::SetMuted(bool t) const {
}

int ScreenDevice::GetIndex() const {
	return _index;
}

/** Playback manager **/
OutputManager::OutputManager(): _dirty(true) {
	// Initialize player window
	_d3d = Direct3DCreate9(D3D_SDK_VERSION);
	
	if(_d3d==0) {
		Log::Write(L"TJShow/OutputManager", L"Could not initialize Direct3D9. Please verify you have DirectX 9.0c on your computer.");
	}
}

OutputManager::~OutputManager() {
	if(_d3d!=0) {
		_d3d->Release();
		_d3d = 0;
	}
}

void OutputManager::ListDevices(std::vector< ref<Device> >& devs) {
	ThreadLock lock(&_lock);

	std::vector< ref<Device> > videoDevices;
	VideoDeck::ListDevices(videoDevices, _existingVideoDevices);
	_existingVideoDevices.clear();
	std::vector< ref<Device> >::iterator vit = videoDevices.begin();
	while(vit!=videoDevices.end()) {
		ref<Device> dev = *vit;
		if(dev) {
			_existingVideoDevices[dev->GetIdentifier()] = dev;
		}
		++vit;
	}

	std::vector< ref<Device> >::iterator it = _screenDevices.begin();
	while(it!=_screenDevices.end()) {
		videoDevices.push_back(*it);
		++it;
	}
}

ref<view::PlayerWnd> OutputManager::AddScreen(const std::wstring& name, strong<ScreenDefinition> sd) {
	ThreadLock lock(&_lock);

	ref<view::PlayerWnd> pw = GC::Hold(new PlayerWnd(this, sd));
	pw->Init(_d3d, 0);
	_screens.push_back(pw);

	// Add a screen device to the plugin manager's device list
	ref<ScreenDevice> screenDevice = GC::Hold(new ScreenDevice(this, int(_screens.size())-1, name, sd));
	_screenDevices.push_back(screenDevice);

	return pw;
}

void OutputManager::AddAllScreens(std::vector< ref<view::PlayerWnd> >& created) {
	ThreadLock lock(&_lock);
	
	if(_d3d!=0) {
		bool createAsTouchScreen = Application::Instance()->GetSettings()->GetFlag(L"view.client.touch-screen", true);
		if(!createAsTouchScreen) {
			Mouse::Instance()->SetCursorHidden(true);
		}

		unsigned int adapterCount = _d3d->GetAdapterCount();
		for(unsigned int a=0;a<adapterCount;a++) {
			D3DADAPTER_IDENTIFIER9 adapter;
			if(SUCCEEDED(_d3d->GetAdapterIdentifier(a, 0, &adapter))) {
				std::wstring adapterName = Wcs(adapter.Description);
				Log::Write(L"TJShow/OutputManager", L"Video adapter #"+Stringify(a)+L": "+adapterName);	

				// TODO change this to something like GetDefinition(const std::wstring& id), which fetches it from the Model or
				// creates a default definition
				strong<ScreenDefinition> def = GC::Hold(new ScreenDefinition(createAsTouchScreen, 1024, 768, ScaleModeAll));

				ref<view::PlayerWnd> pw = GC::Hold(new PlayerWnd(this, def));
				pw->Init(_d3d, a);
				created.push_back(pw);
				_screens.push_back(pw);

				// Add device to plugin manager's device list
				ref<ScreenDevice> screenDevice = GC::Hold(new ScreenDevice(this, int(_screens.size())-1, adapterName, def));
				_screenDevices.push_back(screenDevice);
			}
		}
	}
}

int OutputManager::GetScreenCount() const {
	return (int)_screens.size();
}

ref<view::PlayerWnd> OutputManager::GetScreenWindow(int screen) {
	return _screens.at(screen);
}

ref<Property> OutputManager::CreateSelectPatchProperty(const std::wstring& name, ref<Inspectable> holder, PatchIdentifier* pi) {
	return GC::Hold(new SelectPatchProperty(name, holder, pi));
}

void OutputManager::SetOutletValue(ref<Outlet> outlet, const Any& value) {
}

ref<Device> OutputManager::GetDeviceByPatch(const PatchIdentifier& t) {
	ref<Model> model = Application::Instance()->GetModel();
	if(model) {
		ref<Patches> ps = model->GetPatches();
		if(ps) {
			DeviceIdentifier di = ps->GetDeviceByPatch(t,true);
			return PluginManager::Instance()->GetDeviceByIdentifier(di);
		}
	}

	return 0;
}

PatchIdentifier OutputManager::GetPatchByDevice(ref<Device> dev) {
	ref<Model> model = Application::Instance()->GetModel();
	if(model) {
		ref<Patches> ps = model->GetPatches();
		if(ps) {
			return ps->GetPatchForDevice(dev->GetIdentifier());
		}
	}
	return L"";
}

void OutputManager::CleanMeshes() {
	ThreadLock lock(&_lock);

	std::vector< ref<view::PlayerWnd> >::iterator it = _screens.begin();
	while(it!=_screens.end()) {
		ref<view::PlayerWnd> pw = *it;
		if(pw) {
			pw->CleanMeshes();
		}
		++it;
	}
}

void OutputManager::SortMeshes() {
	ThreadLock lock(&_lock);

	std::vector< ref<view::PlayerWnd> >::iterator it = _screens.begin();
	while(it!=_screens.end()) {
		ref<view::PlayerWnd> pw = *it;
		if(pw) {
			pw->SortMeshes();
		}
		++it;
	}
}

ref<Deck> OutputManager::CreateDeck(bool visible, ref<Device> device) {
	ThreadLock lock(&_lock);
	HRESULT hr = S_OK;

	int index = 0;
	if(device && device.IsCastableTo<ScreenDevice>()) {
		index = ref<ScreenDevice>(device)->GetIndex();
	}
	
	if(int(_screens.size())<index) {
		return 0;
	}

	ref<PlayerWnd> player = _screens.at(index);
	if(!player) {
		Log::Write(L"TJShow/OutputManager", L"CreateDeck failed; player window for screen not found!");
		return 0;
	}

	if(!player->_device) {
		Log::Write(L"TJShow/OutputManager", L"CreateDeck failed; player device for screen not found!");
		return 0;
	}

	strong<Settings> st = Application::Instance()->GetSettings();
	int maxWidth = StringTo<int>(st->GetValue(L"view.video.max-texture-width"), 1024);
	int maxHeight = StringTo<int>(st->GetValue(L"view.video.max-texture-height"), 1024);

	CComPtr<TextureRenderer> renderer = new TextureRenderer(NULL, &hr, player->_device, maxWidth, maxHeight);
	if(FAILED(hr)) {
		Log::Write(L"TJShow/PlaybackManager", L"Could not create texture renderer");
	}

	ref<Deck> deck = GC::Hold(new VideoDeck(renderer, visible));
	CleanMeshes();
	player->_meshes.push_back(ref<Mesh>(deck));
	SetDirty();
	return deck;
}

strong<ResourceProvider> OutputManager::GetResources() {
	ref<Model> model = Application::Instance()->GetModel();
	if(model) {
		ref<ResourceProvider> mrm = model->GetResourceManager();
		if(mrm) {
			return mrm;
		}
	}
	
	ref<Network> net = Application::Instance()->GetNetwork();
	if(net) {
		ref<ResourceProvider> crm = net->GetResourceProvider();
		if(crm) {
			return crm;
		}
	}

	Throw(L"Couldn't find a suitable resource manager for plug-in", ExceptionTypeError);
}

bool OutputManager::IsKeyingSupported(ref<Device> device) {
	int index = 0;
	if(device && device.IsCastableTo<ScreenDevice>()) {
		index = ref<ScreenDevice>(device)->GetIndex();
	}
	
	if(int(_screens.size())<index) {
		return false;
	}

	ref<PlayerWnd> player = _screens.at(index);
	if(player) {
		return player->IsHardwareEffectsPresent();
	}
	
	return false;
}

ref<Surface> OutputManager::CreateSurface(int w,int h, bool visible, ref<Device> screenDevice) {
	ThreadLock lock(&_lock);

	strong<Settings> st = Application::Instance()->GetSettings();
	int maxWidth = StringTo<int>(st->GetValue(L"view.video.max-texture-width"), 1024);
	int maxHeight = StringTo<int>(st->GetValue(L"view.video.max-texture-height"), 1024);

	if(w > maxWidth || h > maxHeight) {
		Log::Write(L"TJShow/OutputManager", L"Surface creation failed, it is too large (w="+Stringify(w)+L" h="+Stringify(h)+L")");
		return null;
	}

	int index = 0;
	if(screenDevice && screenDevice.IsCastableTo<ScreenDevice>()) {
		index = ref<ScreenDevice>(screenDevice)->GetIndex();
	}
	
	if(int(_screens.size())<index) {
		return null;
	}

	ref<PlayerWnd> player = _screens.at(index);
	if(!player || !player->_device) {
		Log::Write(L"TJShow/OutputManager", L"Invalid screen, cannot find player window");
		return null;
	}

	IDirect3DDevice9* device = player->_device;
	if(device==0) {
		Log::Write(L"TJShow/OutputManager", L"Cannot create surface, there is no device");
		return null;
	}

	try {
		ref<TextureSurface> tsf = GC::Hold(new TextureSurface(w,h, device, this, visible));
		CleanMeshes();
		player->_meshes.push_back(ref<Mesh>(tsf));
		SetDirty();
		return tsf;
	}
	catch(Exception& e) {
		Log::Write(L"TJShow/OutputManager", L"Surface creation failed (w="+Stringify(w)+L" h="+Stringify(h)+L"): "+e.GetMsg());
	}
	return null;
}

void OutputManager::SetDirty() {
	ThreadLock lock(&_lock);
	_dirty = true;
}

bool OutputManager::IsDirty() {
	ThreadLock lock(&_lock);
	if(_dirty) {
		_dirty = false;
		return true;
	}
	return false;
}

bool OutputManager::IsFeatureAvailable(const std::wstring& ft) {
	return true;
}

std::wstring OutputManager::ParseVariables(const std::wstring& source) {
	return Variables::ParseVariables(Application::Instance()->GetModel()->GetVariables(), source);
}

strong<Property> OutputManager::CreateParsedVariableProperty(const std::wstring& name, ref<Inspectable> holder, std::wstring* source, bool ml) {
	if(source==0) Throw(L"Invalid parameter: source cannot be null!", ExceptionTypeError);
	if(ml) {
		return GC::Hold(new TextProperty(name, holder, source));
	}
	else {
		return GC::Hold(new GenericProperty<std::wstring>(name, holder, source, *source));
	}
}

void OutputManager::QueueDownload(const std::wstring &rid) {
	Application::Instance()->GetNetwork()->NeedResource(rid);
}

