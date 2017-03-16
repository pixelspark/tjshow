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
#include <TJShow/include/tjshow.h>
#include "../../../TJShow/include/extra/tjextra.h"

using namespace tj::shared::graphics;
using namespace tj::show;
using namespace tj::shared;
using namespace tj::np;

/** This is a very simple plug-in for TJShow. Every plug-in is placed in its own .dll-file
inside the plugins/ directory in the TJShow installation folder. Every DLL has one exported
function: Plugin* GetPlugin(), which should return your own subclass of the Plugin-interface
(see TJShow/include/tjplugin.h). This class provides TJShow the functions to create 'tracks'
and 'players'. The plugin instantiation can also be used to hold global data for the plug-in
(every TJShow-instance creates a plugin-object, DLL-global data might be shared between TJShow
instances).

When compiling this plug-in, check the following things:
- Always include TJShow/include/tjshow.h, as it provides plugin definitions

- For painting your track, include Gdiplus.h and link with Gdiplus.lib. The Gdiplus stuff is contained in a namespace
called 'Gdiplus', so either use a  'using namespace Gdiplus' or put tj::shared::graphics:: in front of every class/function you use
from GDI+.

- For painting, you can use colors and fonts from the Theme class. Usage is like this:
strong<Theme> theme = ThemeManager::GetTheme();
SolidBrush backBrush(theme->GetBackgroundColor());
...use backbrush here...

- Link to tjshared.lib and tjsharedui.lib (tjshared.dll/tjsharedui.dll). TJShared is the 'shared base' of TJShow. 
It holds functions for TJShow's GC (more about that below) and several other useful stuff. The TJSharedUI library
contains all the user-interface related stuff.

- If you need fading, list properties, color properties and other fancy things, look at the header
files in extra/. They provide faders and other stuff. Properties are part of TJShared (see include/properties). 

There are a few things you should know before starting your own plug-in:
- TJShow uses 'Garbage Collection' for its memory management. The 'GC' is defined in tjreference.h
and is based on reference-counting smart-pointers. A static GC-class in tjshared counts active
objects and can be used to create objects. The good thing about this is that you don't have to
think about who is going to delete objects passed between TJShow and your plugin. 
Here's a short usage reference:

- Creating objects:  ref<Track> track = GC::Hold(new MyTrack());
- Casting: ref<MyTrack> mytrack = ref<MyTrack>(track); (will throw BadCastException if there's something wrong)
- Testing if valid: if(mytrack) {...}
- Is it castable? if(track.IsCastableTo<MyTrack>()) {...}
- When references are not initalized, they are automatically null.
- When calling methods on null references, an exception will be thrown

Watch out with circular references, as the GC will not be able to delete objects if they're still
referenced. When the GC detects that not all of the objects have been deleted at program shutdown,
it will issue a warning message. When this message appears, and it doesn't appear when your plugin 
is not loaded, you probably have a memory leak somewhere in your plugin. If you really need to circularly
reference two objects, give one object a weak<OtherObject> 'reference' to the other. The weak reference does
not increment the reference count of the object, but it can be made a reference (ref<T> = someWeakReferenceToT).
Usually, the 'owner' has the reference to the child object and the child object would have a weak<Parent> reference.
When the owner is destroyed, the child object is destroyed because the reference count is equal to 0.

Please also note that storing references inside a templated container should be done like this:
std::vector< ref<SomeClas> > 
Note the space between > > above. This is *required* by the C++ standard, because otherwise some
compilers will be confused and think its a >> bitwise operator. So for most compilers, the space
between the two angle brackets is mandatory!

- TJShow uses Unicode almost everywhere, and the whole plug-in API uses Unicode. Even Gdi+ uses it.
In tjutil.h, a few functions exist to convert back and forth between 'normal' strings and Unicode
strings, but in general, you don't need them. The only thing you need to know is that you have to put
an 'L' in front of a literal that you want to be unicode: L"my string" versus "my string". 

- If you need a reference counted 'this', you can use this. But beware: it does not work inside constructors
or destructors and only works in classes that are derived from Object like this:
class MyObject: public virtual Object.
Do not store a this reference to an object inside that object (only use it temporarily) or your object will not be deleted and
you will be leaking memory.

- TJShow is currently multi-threaded. When not playing back, it will call functions like Save and Load from one
main thread. When starting playback, the player may or may not be created (CreatePlayer) from another thread. The player
is then 'played' by a separate thread (that calls Stop, Start, Tick et al). You should do proper locking if you share resources
among players (use the ThreadLock and CriticalSection classes for that). 
**/
class DummyTrack;

class DummyPlayer: public Player, public Listener<Interactive::MouseNotification> {
	public:
		DummyPlayer(ref<DummyTrack> tr, ref<Stream> str) {
			_track = tr;
			_stream = str;
			_x = _y = 0.0f;
		}

		virtual ~DummyPlayer() {
		}

		ref<Track> GetTrack() {
			return ref<DummyTrack>(_track);
		}

		virtual void Stop() {
			_surface = 0;
		}

		virtual void Notify(ref<Object> src, const Interactive::MouseNotification& ms) {
			_x = ms._x;
			_y = ms._y;
		}

		virtual void Start(Time pos,ref<Playback> pb, float speed);

		virtual void Tick(Time currentPosition);

		virtual void Jump(Time newT, bool paused) {
			_current = newT;
		}

		virtual Time GetNextEvent(Time t) {
			return t+Time(100);
		}

		virtual void SetOutput(bool t) {
		}

	protected:
		float _x, _y;
		weak<DummyTrack> _track;
		Time _current;
		ref<Stream> _stream;
		ref<Surface> _surface;
};

class DummyLiveControl: public LiveControl {
	public:
		DummyLiveControl() {
		}

		virtual ~DummyLiveControl() {
		}

		void TreePopulate(ref<SimpleTreeNode> parent, const std::wstring& text) {
			if(text.length()>0) {				
				ref<SimpleTreeNode> me = GC::Hold(new SimpleTreeNode(text));
				
				std::wstring sub = text.substr(1);
				TreePopulate(me, sub);

				ref<SimpleTreeNode> sibling = GC::Hold(new SimpleTreeNode(L"Something else"));
				parent->Add(strong<TreeNode>(me));
				parent->Add(strong<TreeNode>(sibling));
			}
		}

		virtual ref<Wnd> GetWindow() {
			if(!_tree) {
				_tree = GC::Hold(new TreeWnd());
				_tree->AddColumn(L"Key",KColKey);

				ref<SimpleTreeNode> root = GC::Hold(new SimpleTreeNode(L"root"));
				TreePopulate(root, L"TommyvanderVorst");
				_tree->SetRoot(root);


			}
			return _tree;
		}

		virtual void Update() {
		}

		virtual bool IsSeparateTab() {
			return true;
		}

		ref<TreeWnd> _tree;
		enum {
			KColKey=1,
			KColValue,
		};
};

class DummyTrack: public MultifaderTrack {
	public:
		DummyTrack(ref<Playback> pb) {
			AddFader(0, L"Fader_A");
			AddFader(1, L"Fader_B");
			AddFader(2, L"Fader_C");
			AddFader(3, L"Fader_D");
			_pb = pb;
		}
		
		virtual ~DummyTrack() {
		}

		virtual void Save(TiXmlElement* parent) {
			MultifaderTrack::Save(parent);
		}

		virtual void Load(TiXmlElement* you) {
			MultifaderTrack::Load(you);
		}

		virtual Flags<RunMode> GetSupportedRunModes() {
			return Flags<RunMode>(RunModeMaster);
		}

		virtual std::wstring GetTypeName() const { return std::wstring(L"Dummy"); }

		virtual ref<Player> CreatePlayer(ref<Stream> str) {
			return GC::Hold(new DummyPlayer(this, str));
		}

		virtual ref<LiveControl> CreateControl(ref<Stream> stream) {
			return GC::Hold(new DummyLiveControl());
		}

		/** You can safely return 0 when there are no properties you want the user to edit **/
		virtual ref<PropertySet> GetProperties() {
			ref<PropertySet> ps = GC::Hold(new PropertySet());
			ps->Add(GC::Hold(new ColorProperty(L"Kleur", this, &_color, 0)));
			ps->Add(_pb->CreateSelectPatchProperty(L"Selecteer patch", this, &_patch));
			return ps;
		}

		virtual void CreateOutlets(OutletFactory& of) {
			_myOutlet = of.CreateOutlet(L"my_outlet", L"My outlet");
		}

		virtual void Paint(tj::shared::graphics::Graphics* g, Time position, float pixelsPerMs, unsigned int x, unsigned int y, unsigned int h, Time start, Time end, ref<Item> focus) {
			MultifaderTrack::Paint(g,position,pixelsPerMs, x, y, h, start, end, focus);
			LinearGradientBrush lbr(PointF((float)x, 0.0f), PointF(x+pixelsPerMs*1000.0f, 0.0f), _color, Theme::ChangeAlpha(_color,155));
			g->FillRectangle(&lbr, RectF(float(x), float(y), float(int(end-start)*pixelsPerMs), float(GetFaderHeight())));
		}

		RGBColor _color;
		ref<Playback> _pb;
		ref<Surface> _surface;
		ref<Outlet> _myOutlet;
		PatchIdentifier _patch;
		float _x, _y;

};

class DummyPlugin: public OutputPlugin {
	public:
		virtual ~DummyPlugin() {
		}

		virtual std::wstring GetName() const {
			return std::wstring(L"Dummy");
		}

		virtual ref<Track> CreateTrack(ref<Playback> pb) {
			return GC::Hold(new DummyTrack(pb));
		}

		virtual ref<StreamPlayer> CreateStreamPlayer(ref<Playback> pb, ref<Talkback> talk) {
			return 0;
		}

		/** Normally, you would call TL(...) here so this is localized **/
		virtual std::wstring GetFriendlyName() const {
			return L"Dummy";
		}

		virtual std::wstring GetFriendlyCategory() const {
			return TL(other_category);
		}

		/** Normally, you would call TL(...) here so this is localized **/
		virtual std::wstring GetDescription() const {
			return L"Dummy is an example that programmers can use to create their own TJShow plug-in";
		}

		virtual void GetRequiredFeatures(std::list<std::wstring>& fts) const {
		}

		virtual std::wstring GetVersion() const {
			std::wostringstream os;
			os << __DATE__ << " @ " << __TIME__;
			#ifdef UNICODE
			os << L" Unicode";
			#endif

			#ifdef NDEBUG
			os << " Release";
			#endif

			return os.str();
		}

		std::wstring GetAuthor() const {
			return std::wstring(L"Tommy van der Vorst");
		}
};

extern "C" { 
	__declspec(dllexport) std::vector<ref<Plugin> >* GetPlugins() {
		std::vector<ref<Plugin> >* plugins = new std::vector<ref<Plugin> >();
		plugins->push_back(GC::Hold(new DummyPlugin()));
		return plugins;
	}
}

void DummyPlayer::Start(Time pos, ref<Playback> pb, float speed) {
	ref<DummyTrack> track = _track;
	if(track && track->_myOutlet) {
		// Setting a tuple to an outlet
		strong<Tuple> tuple = GC::Hold(new Tuple(3));
		tuple->Set(0, Any(1337));
		tuple->Set(1, Any(std::wstring(L"Tommy van der Vorst")));
		tuple->Set(2, Any(std::wstring(L"06-55543708")));
		pb->SetOutletValue(track->_myOutlet, Any(tuple));
	}

	_surface = pb->CreateSurface(512,512);

	if(_surface) {
		_surface->SetTranslate(Vector(0.0f, 0.0f, 1.0f));
		_surface->EventMouse.AddListener(this);
	}
}

void DummyPlayer::Tick(Time t)  {
	_current = t;
	if(_surface) {
		ref<SurfacePainter> sp = _surface->GetPainter();
		if(sp) {
			tj::shared::graphics::Graphics* g = sp->GetGraphics();
			SolidBrush yellow(Color(1.0, 1.0, 0.0));
			g->FillEllipse(&yellow, Area(Pixels(_x*512.0f), Pixels(_y*512.0f), 5, 5));
		}
	}
}