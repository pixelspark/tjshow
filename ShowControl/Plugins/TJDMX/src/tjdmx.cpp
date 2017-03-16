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
#include "../include/tjdmx.h"
#include "../include/color/tjdmxcolor.h"
#include <algorithm>
using namespace tj::script;
using namespace tj::dmx::color;
using namespace tj::shared::graphics;

DMXPatchable::~DMXPatchable() {
}

int DMXPatchable::GetSubChannelID() const {
	return 0;
}

DMXPlugin::DMXPlugin() {
}

DMXPlugin::~DMXPlugin() {
}

ref<Property> DMXPlugin::CreateAddressProperty(const std::wstring& name, ref<Inspectable> holder, std::wstring* address) {
	ref<Property> addressProperty = GC::Hold(new GenericProperty<std::wstring>(name, holder, address, *address));
	addressProperty->SetHint(std::wstring(TL(dmx_address_hint))+std::wstring(L"\r\n25\tSingle channel\r\ngm\tGrand master\r\nsm\tSequence master\r\nm1,2\tSubmaster\r\n1,2\tGroup\r\np1\t16-bit (coarse/fine)\r\n-1\tInverse"));
	return addressProperty;
}

ref<DMXMacro> DMXPlugin::CreateMacro(std::wstring address, DMXSource source) {
	return GetController()->CreateMacro(address, source);
}

ref<Pane> DMXPlugin::GetSettingsWindow(ref<PropertyGridProxy> pg) {
	return GC::Hold(new Pane(TL(dmx_settings), GC::Hold(new DMXSettingsWnd(this, pg)), false, true, 0));
}

void DMXPlugin::Save(TiXmlElement* you, bool showSpecific) {
	if(showSpecific) {
		TiXmlElement controller("controller");
		GetController()->Save(&controller);
		you->InsertEndChild(controller);
	}
}

void DMXPlugin::Load(TiXmlElement* you, bool showSpecific) {
	if(showSpecific) {
		TiXmlElement* controller = you->FirstChildElement("controller");
		if(controller!=0) {
			ref<DMXController> dc = GetController();
			dc->Load(controller);
		}
	}
}

void DMXPlugin::Reset() {
	ref<DMXController> dc = GetController();
	if(dc) {
		dc->Reset();
	}
	CleanWeakReferencesList(_tracks);
}

std::wstring DMXPlugin::GetName() const {
	return std::wstring(L"DMX");
}

std::wstring DMXPlugin::GetFriendlyName() const {
	return TL(dmx_plugin_friendly_name);
}

std::wstring DMXPlugin::GetFriendlyCategory() const {
	return TL(dmx_category);
}

std::wstring DMXPlugin::GetDescription() const {
	return TL(dmx_plugin_description);
}

strong<DMXController> DMXPlugin::GetController() {
	return DMXEngine::GetController();
}

void DMXPlugin::GetRequiredFeatures(std::list<std::wstring>& fts) const {
	fts.push_back(L"DMX");
}

ref<Track> DMXPlugin::CreateTrack(ref<Playback> pb) {
	if(pb->IsFeatureAvailable(L"DMX")) {
		return GC::Hold(new DMXTrack(this));
	}
	return null;
}

void DMXPlugin::AddPatchable(ref<DMXPatchable> p) {
	if(p) {
		ThreadLock lock(&_tlLock);
		_tracks.push_back(p);
	}
}

void DMXPlugin::SavePatchSet(const std::wstring& fn) {
	ThreadLock lock(&_tlLock);

	TiXmlDocument doc;
	TiXmlDeclaration decl("1.0","UTF-8","no");
	doc.InsertEndChild(decl);

	TiXmlElement root("patchset");

	// Save controller settings
	TiXmlElement controller("controller");
	ref<DMXController> dc = GetController();
	dc->Save(&controller);
	root.InsertEndChild(controller);

	// Save patches
	std::vector< weak<DMXPatchable> >::iterator it = _tracks.begin();
	while(it!=_tracks.end()) {
		ref<DMXPatchable> dp = *it;
		if(dp) {
			TiXmlElement patch("patch");
			SaveAttributeSmall(&patch, "tnp", dp->GetID());
			SaveAttributeSmall(&patch, "sub", dp->GetSubChannelID());
			SaveAttributeSmall(&patch, "dmx", dp->GetDMXAddress());
			root.InsertEndChild(patch);
		}
		++it;
	}
	doc.InsertEndChild(root);
	doc.SaveFile(Mbs(fn));
}

void DMXPlugin::LoadPatchSet(const std::wstring& fn) {
	TiXmlDocument doc;
	doc.LoadFile(Mbs(fn));

	/* The MS compiler choked here when I tried to use a std::map<Channel,PatchInfo> with
	PatchInfo a locally defined struct... using a workaround for now (there is no need
	for this function to be really fast */
	std::map< TrackID, std::map<int, std::wstring> > patches;
	std::map< TrackID, std::map<int, unsigned int> > flags;
	enum { KReset = 2 };

	TiXmlElement* root = doc.FirstChildElement();

	// Load controller settings
	TiXmlElement* controller = root->FirstChildElement("controller");
	if(controller!=0) {
		GetController()->Load(controller);
	}

	// Load patches
	TiXmlElement* patch = root->FirstChildElement("patch");
	while(patch!=0) {
		TrackID tnp = LoadAttributeSmall<TrackID>(patch, "tnp", L"");
		if(tnp!=L"") {
			std::wstring address = LoadAttributeSmall<std::wstring>(patch, "dmx", L"");
			bool reset = LoadAttributeSmall<bool>(patch, "reset", false);
			int sub = LoadAttributeSmall<int>(patch, "sub", 0);

			if(address.length()>0) {
				// save this patch
				patches[tnp][sub] = address;
				flags[tnp][sub] = (reset?KReset:0);
			}
		}

		patch = patch->NextSiblingElement("patch");
	}

	ThreadLock lock(&_tlLock);
	if(patches.size()>0) {
		// enumerate all patchables and patch them. Patchables not in the list are left alone
		std::vector< weak<DMXPatchable> >::iterator it = _tracks.begin();
		while(it!=_tracks.end()) {
			ref<DMXPatchable> dp = *it;
			if(dp) {
				TrackID channel = dp->GetID();
				int sub = dp->GetSubChannelID();

				// Find the patchable in the patch list
				std::map<TrackID, std::map<int, std::wstring> >::const_iterator it = patches.find(channel);
				if(it!=patches.end()) {
					// Check if the subchannel is there too
					const std::map<int, std::wstring>& subpatches = it->second;
					std::map<int, std::wstring>::const_iterator subpatch = subpatches.find(sub);

					// If it is, assign the values to the patchable
					if(subpatch!=subpatches.end()) {
						const std::wstring& address = subpatch->second;
						unsigned int f = flags[channel][sub];
						dp->SetResetOnStop((f&KReset) != 0);
						dp->SetDMXAddress(address);
					}
				}
			}
			++it;
		}
	}
}

void DMXPlugin::SortTrackList() {
	ThreadLock lock(&_tlLock);
	
	struct TrackSorter {
		bool operator() (weak<DMXPatchable> a, weak<DMXPatchable> b) {
			ref<DMXPatchable> ra = a;
			ref<DMXPatchable> rb = b;
			if(ra==0 || rb==0) return false;
			return ra->GetInstanceName() < rb->GetInstanceName();
		}
	};

	TrackSorter ts;
	std::sort(_tracks.begin(), _tracks.end(), ts);
}

ref<StreamPlayer> DMXPlugin::CreateStreamPlayer(ref<Playback> pb, ref<Talkback> talk) {
	return 0;
}

int DMXPlugin::GetChannelResult(int channel) {
	return GetController()->GetChannelResult(channel);
}
		
std::wstring DMXPlugin::GetVersion() const {
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

std::wstring DMXPlugin::GetAuthor() const {
	return std::wstring(L"Tommy van der Vorst");
}

void DMXPlugin::Message(ref<DataReader> code) {
}

extern "C" { 
	__declspec(dllexport) std::vector<ref<Plugin> >* GetPlugins() {
		std::vector<ref<Plugin> >* plugins = new std::vector<ref<Plugin> >();
		ref<DMXPlugin> dmx = GC::Hold(new DMXPlugin());
		plugins->push_back(dmx);
		plugins->push_back(GC::Hold(new DMXViewerPlugin(dmx)));
		plugins->push_back(GC::Hold(new DMXPositionPlugin(dmx)));
		plugins->push_back(GC::Hold(new DMXColorPlugin(dmx)));
		return plugins;
	}
}