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
#include "../include/internal/tjstreams.h"

// Special property class that holds a reference to the outlet, so it won't be destroyed while
// we are editing the property.
class OutletProperty: public GenericListProperty<std::wstring> {
	public:
		OutletProperty(strong<VariableOutlet> o, strong<Variables> vars);
		virtual ~OutletProperty();

	private:
		ref<Outlet> _out;
};

/* Track wrapper */
TrackWrapper::TrackWrapper(strong<Track> track, strong<PluginWrapper> plugin, ref<Network> net, ref<Instance> c):
	_track(track), 
	_plugin(plugin),
	_network(net), 
	_instance(c),
	_locked(false),
	_group(0),
	_runMode(RunModeDont),
	_lastMessageTime(0),
	_lastTickTime(0) {
	
	RegisterOutlets();
	SetID(L"");

	Flags<RunMode> rm = _track->GetSupportedRunModes();
	if(rm.IsSet(RunModeMaster)) {
		_runMode = RunModeMaster;
	}
	else if(rm.IsSet(RunModeClient)) {
		_runMode = RunModeClient;
	}
}

TrackWrapper::~TrackWrapper() {
}

Channel TrackWrapper::GetMainChannel() {
	ref<Group> grp = _mainChannel.GetGroup();
	if(!grp || grp->GetID()!=_group) {
		_mainChannel.Deallocate();
		ref<Group> group = Application::Instance()->GetModel()->GetGroups()->GetGroupById(_group);
		if(group) {
			_mainChannel.Allocate(group, null);
		}
	}
	return _mainChannel.GetChannel();
}

void TrackWrapper::UpdateOutlets() {
	class UpdatingTrackOutletFactory: public OutletFactory {
		public:
			UpdatingTrackOutletFactory(TrackWrapper& tw, ref<Variables> vars): _tw(tw), _vars(vars) {}
			virtual ~UpdatingTrackOutletFactory() {}

			virtual ref<Outlet> CreateOutlet(const std::wstring& id, const std::wstring& name) {
				Hash hash;
				OutletHash oh = hash.Calculate(id);
				std::map<OutletHash, weak<Outlet> >::iterator it = _tw._outlets.find(oh);
				if(it!=_tw._outlets.end()) {
					// Outlet already exists, just update the name and return it
					ref<Outlet> out = it->second;
					if(out) {
						out->SetName(name);
						return out;
					}
				}
				
				ref<VariableOutlet> out = GC::Hold(new VariableOutlet(name,id));
				_tw._outlets[oh] = ref<Outlet>(out);
				return out;
			}

			TrackWrapper& _tw;
			ref<Variables> _vars;
	};

	try {
		ref<Instance> inst = _instance;
		if(inst) {
			ref<Variables> vars = inst->GetLocalVariables();
			if(vars) {
				UpdatingTrackOutletFactory to(*this, vars);
				_track->CreateOutlets(to);
			}
		}
	}
	catch(Exception& e) {
		Log::Write(L"TrackWrapper/UpdateOutlets", e.GetMsg());
	}
	catch(...) {
		Log::Write(L"TrackWrapper/UpdateOutlets", L"Unknown error occurred");
	}
}

void TrackWrapper::RegisterOutlets() {
	class TrackOutletFactory: public OutletFactory {
		public:
			TrackOutletFactory(TrackWrapper& tw, ref<Variables> vars): _tw(tw), _vars(vars) {}
			virtual ~TrackOutletFactory() {}

			virtual ref<Outlet> CreateOutlet(const std::wstring& id, const std::wstring& name) {
				Hash hash;
				ref<VariableOutlet> out = GC::Hold(new VariableOutlet(name,id));
				_tw._outlets[hash.Calculate(id)] = ref<Outlet>(out);
				return out;
			}

			TrackWrapper& _tw;
			ref<Variables> _vars;
	};

	try {
		ref<Instance> instance = _instance;
		if(instance) {
			ref<Variables> vars = instance->GetLocalVariables();
			if(vars) {
				TrackOutletFactory to(*this, vars);
				_track->CreateOutlets(to);
			}
		}
	}
	catch(Exception& e) {
		Log::Write(L"TrackWrapper/RegisterOutlets", e.GetMsg());
	}
	catch(...) {
		Log::Write(L"TrackWrapper/RegisterOutlets", L"Unknown error occurred");
	}
}

ref<Outlet> TrackWrapper::GetOutletById(const std::wstring& id) {
	std::map< OutletHash, weak<Outlet> >::iterator it = _outlets.begin();
	while(it!=_outlets.end()) {
		ref<Outlet> out = it->second;
		if(out && out->GetID()==id) {
			return out;
		}
		++it;
	}
	return 0;
}

ref<Outlet> TrackWrapper::GetOutletByHash(const OutletHash& hash) {
	std::map< OutletHash, weak<Outlet> >::iterator it = _outlets.find(hash);
	if(it!=_outlets.end()) {
		return it->second;
	}
	return 0;
}

void TrackWrapper::Clone() {
	try {
		_track->Clone();
	}
	catch(Exception& e) {
		Log::Write(L"TrackWrapper/Clone", e.GetMsg());
	}
	catch(...) {
		Log::Write(L"TrackWrapper/Clone", L"Unknown error occurred");
	}

	SetID(L"");
}

ref<LiveControl> TrackWrapper::GetLiveControl() {
	ref<LiveControl> control = _control;
	if(!control) {
		try {
			ref<Network> nw = _network;
			control = _track->CreateControl(GC::Hold(new LiveStream(this, nw)));
		}
		catch(Exception& e) {
			Log::Write(L"TrackWrapper/GetLiveControl", e.GetMsg());
		}
		catch(...) {
			Log::Write(L"TrackWrapper/GetLiveControl", L"Unknown error occurred");
		}
		_control = control;
	}
	return control;
}

std::wstring TrackWrapper::GetInstanceName() {
	if(IsSubTimeline()) {
		return GetSubTimeline()->GetInstanceName();
	}
	return _instanceName;
}

void TrackWrapper::SetInstanceName(const std::wstring& wn) {
	_instanceName = wn;
	try {
		_track->OnSetInstanceName(_instanceName);
	}
	catch(Exception& e) {
		Log::Write(L"TrackWrapper/OnSetInstanceName", e.GetMsg());
	}
	catch(...) {
		Log::Write(L"TrackWrapper/OnSetInstanceName", L"Unknown error occurred");
	}
}

Pixels TrackWrapper::GetHeight() {
	return max(StringTo<int>(Application::Instance()->GetSettings()->GetValue(L"view.min-track-height"),20), _track->GetHeight());
}

std::wstring TrackWrapper::GetTypeName() {
	return _track->GetTypeName();
}

ref<PluginWrapper> TrackWrapper::GetPlugin() {
	return _plugin;
}

RunMode TrackWrapper::GetRunMode() const {
	return _runMode;
}

void TrackWrapper::SetRunMode(RunMode r) {
	// verify if the selected run mode is supported
	Flags<RunMode> modes = GetSupportedRunModes();
	if(modes.IsSet(r) || r==RunModeDont) {
		_runMode = r;
	}
}

void TrackWrapper::SetID(const TrackID& tid) {
	if(tid==L"") {
		_id = Util::RandomIdentifier(L'T');
	}
	else {
		_id = tid;
	}

	try {
		_track->OnSetID(_id);
	}
	catch(...) {
	}
}

TrackID TrackWrapper::GetID() const {
	return _id;
}

void TrackWrapper::SetLocked(bool l) {
	_locked = l;
}

bool TrackWrapper::IsLocked() const {
	return _locked;
}

strong<Track> TrackWrapper::GetTrack() {
	return _track;
}

unsigned int TrackWrapper::GetLastMessageTime() const {
	return _lastMessageTime;
}

unsigned int TrackWrapper::GetLastTickTime() const {
	return _lastTickTime;
}

void TrackWrapper::SetLastTickTime(unsigned int t) {
	_lastTickTime = t;
}

/** Please note that RunModeDont cannot be in the Flags (=0) but is implicitly always supported */
Flags<RunMode> TrackWrapper::GetSupportedRunModes() {
	try {
		return _track->GetSupportedRunModes();
	}
	catch(...) {
		return Flags<RunMode>(RunModeDont);
	}
}

ref<PropertySet> TrackWrapper::GetProperties() {
	UpdateOutlets();
	ref<PropertySet> prs = GC::Hold(new PropertySet());

	// Common properties
	if(!IsSubTimeline()) {
		prs->Add(GC::Hold(new PropertySeparator(TL(properties_general))));
		prs->Add(GC::Hold(new GenericProperty<std::wstring>(TL(track_name), this, &_instanceName, _instanceName)));

		strong<Groups> groups = Application::Instance()->GetModel()->GetGroups();
		prs->Add(groups->CreateGroupProperty(TL(track_group), this, &_group));
	}

	// The track's properties
	prs->Add(GC::Hold(new PropertySeparator( _track->GetTypeName() )));

	try {
		ref<PropertySet> tp = _track->GetProperties();
		if(tp) {
			prs->MergeAdd(tp);
		}
	}
	catch(...) {
		Log::Write(L"TJShow/TrackWrapper/GetProperties", L"Error occurred while trying to retrieve the properties for track "+_instanceName);
	}

	// Outlet properties
	if(_outlets.size()>0) {
		// Find out if outlets are actually used for this track
		bool outletsUsed = false;
		std::map<OutletHash, weak<Outlet> >::iterator at = _outlets.begin();
		while(at!=_outlets.end()) {
			ref<Outlet> out = at->second;
			if(out && out->IsConnected()) {
				outletsUsed = true;
				break;
			}
			++at;
		}

		// Add a property for each outlet
		ref<Instance> inst = _instance;
		if(inst) {
			strong<Variables> vars = ref<Variables>(inst->GetLocalVariables());
			prs->Add(GC::Hold(new PropertySeparator(TL(outlets), !outletsUsed)));
			std::map< OutletHash, weak<Outlet> >::iterator it = _outlets.begin();
			while(it!=_outlets.end()) {
				ref<Outlet> out = it->second;
				if(out && out.IsCastableTo<VariableOutlet>()) {
					prs->Add(GC::Hold(new OutletProperty(ref<VariableOutlet>(out), vars)));
				}
				++it;
			}
		}
	}

	return prs;
}

bool TrackWrapper::IsSubTimeline() const {
	return ref<Track>(_track).IsCastableTo<SubTimeline>();
}

bool TrackWrapper::IsInstancer() const {
	return ref<Track>(_track).IsCastableTo<Instancer>();
}

ref<SubTimeline> TrackWrapper::GetSubTimeline() {
	if(IsSubTimeline()) {
		return ref<SubTimeline>(ref<Track>(_track));
	}
	return null;
}

ref<Instancer> TrackWrapper::GetInstancer() {
	if(IsInstancer()) {
		return ref<Instancer>(ref<Track>(_track));
	}
	return null;
}

bool TrackWrapper::RequiresResource(const ResourceIdentifier& rid) {
	std::vector<ResourceIdentifier> res;
	_track->GetResources(res);
	if(res.size()>0) {
		std::vector<ResourceIdentifier>::iterator it = res.begin();
		while(it!=res.end()) {
			const ResourceIdentifier& mrid = *it;
			if(_wcsicmp(mrid.c_str(),rid.c_str())==0) return true;
			++it;
		}
	}
	return false;
}

void TrackWrapper::Save(TiXmlElement* me) {
	SaveAttributeSmall(me, "id", GetID());
	SaveAttributeSmall(me, "name", GetInstanceName());
	SaveAttributeSmall(me, "plugin", GetPlugin()->GetHash());
	SaveAttributeSmall(me, "locked", IsLocked());
	SaveAttributeSmall(me, "group", _group);

	std::string runmode("none");
	switch(_runMode) {
		case RunModeClient:
			runmode = "client";
			break;
		case RunModeMaster:
			runmode = "master";
			break;
		case RunModeBoth:
			runmode = "both";
			break;
		default:
		case RunModeDont:
			runmode = "none";
	}

	SaveAttributeSmall(me, "runat", runmode);

	if(_outlets.size()>0) {
		TiXmlElement outlets("outlets");
		std::map< OutletHash, weak<Outlet> >::iterator it = _outlets.begin();
		while(it!=_outlets.end()) {
			ref<Outlet> out = it->second;
			if(out) {
				TiXmlElement outlet("outlet");
				out->Save(&outlet);
				outlets.InsertEndChild(outlet);
			}
			++it;
		}
		me->InsertEndChild(outlets);
	}

	_track->Save(me);
}

void TrackWrapper::Load(TiXmlElement* me) {
	if(me->Attribute("id")!=0) {
		SetID(LoadAttributeSmall(me, "id", _id));
	}
	else {
		SetID(L"");
	}
	SetInstanceName(LoadAttributeSmall<std::wstring>(me, "name", L""));
	_group = LoadAttributeSmall(me, "group", _group);

	const char* ls = me->Attribute("locked");
	SetLocked(false);
	if(ls!=0 && std::string(ls)=="yes") {
		SetLocked(true);
	}

	// Load the 'run mode', which identifies where the track will be played back
	const char* runat = me->Attribute("runat");
	if(runat==0) {
		_runMode = RunModeDont;
	}
	else {
		std::string runats(runat);
		if(runats=="client") {
			_runMode = RunModeClient;
		}
		else if(runats=="master") {
			_runMode = RunModeMaster;
		}
		else if(runats=="both") {
			_runMode = RunModeBoth;
		}
		else {
			_runMode = RunModeDont;
		}
	}

	/* Check if the loaded run mode is actually supported, or else change it to don't run.
	Note that this doesn't require tracks to set 'RunModeDont' in their supported
	run modes; if RunModeDont is not in this list, it will simply be set anyway! */
	if(!_track->GetSupportedRunModes().IsSet(_runMode)) {
		_runMode = RunModeDont;
	}

	// this just does loading for the contained track
	_track->Load(me);
	
	// After the track has loaded, it might have found that it needs more outlets, so update them here
	UpdateOutlets();

	// If there are outlets, load mappings
	if(_outlets.size()>0) {
		TiXmlElement* outlets = me->FirstChildElement("outlets");
		if(outlets!=0) {
			TiXmlElement* outlet = outlets->FirstChildElement("outlet");
			while(outlet!=0) {
				std::wstring id = LoadAttributeSmall<std::wstring>(outlet, "id", L"");
				Hash h;
				std::map<OutletHash, weak<Outlet> >::iterator found = _outlets.find(h.Calculate(id));
				if(found!=_outlets.end()) {
					ref<Outlet> out = found->second;
					if(out) {
						out->Load(outlet);
					}
				}
				outlet = outlet->NextSiblingElement("outlet");
			}
		}
	}
}

ref<TrackWrapper> TrackWrapper::Duplicate(strong<Instance> controller) {
	ref<Network> network = _network;
	if(!network) {
		return 0;
	}

	// Create a track of our type and 'Clone' to generate new id number etc.
	ref<TrackWrapper> tw = _plugin->CreateTrack(controller->GetPlayback(), network, controller);
	tw->Clone();

	// Use the serialized form of the implementation track to transfer contents to the new track
	if(tw) {
		TiXmlElement track("track");
		_track->Save(&track);	
		tw->_track->Load(&track);
		return tw;
	}

	return 0;
}

void TrackWrapper::SetGroup(GroupID g) {
	_group = g;
}

GroupID TrackWrapper::GetGroup() const {
	return _group;
}

/** LiveControlEndpoint **/
class LiveControlEndpoint: public Endpoint {
	public:
		LiveControlEndpoint(const TrackID& tid, const std::wstring& endpoint): _tid(tid), _endpoint(endpoint) {
		}

		virtual ~LiveControlEndpoint() {
		}

		virtual void Set(const Any& f) {
			strong<Instance> controller = Application::Instance()->GetInstances()->GetRootInstance();

			ref<TrackWrapper> track = controller->GetTimeline()->GetTrackByID(_tid, true);
			if(track) {
				ref<LiveControl> lc = track->GetLiveControl();
				if(lc) {
					ref<Endpoint> ep = lc->GetEndpoint(_endpoint);
					if(ep) {
						ep->Set(f);
					}
				}
			}
		}

		virtual std::wstring GetName() const {
			return TL(live_control_endpoint)+_tid;
		}

	private:
		TrackID _tid;
		std::wstring _endpoint;
};

/** LiveControlEndpointCategory */
const EndpointCategoryID LiveControlEndpointCategory::KLiveControlEndpointCategoryID = L"control";

LiveControlEndpointCategory::LiveControlEndpointCategory(): EndpointCategory(KLiveControlEndpointCategoryID) {
}

LiveControlEndpointCategory:: ~LiveControlEndpointCategory() {
}

ref<Endpoint> LiveControlEndpointCategory::GetEndpointById(const EndpointID& id) {
	std::wistringstream wis(id);
	TrackID tid;
	wchar_t separator = 0;
	std::wstring endpoint = L"";
	wis >> tid;
	if(!wis.eof()) {
		wis >> separator;
		wis >> endpoint;
	}
	return GC::Hold(new LiveControlEndpoint(tid, endpoint));
}

/** Outlet **/
Outlet::~Outlet() {
}

/** OutletFactory **/
OutletFactory::~OutletFactory() {
}

/** VariableOutlet **/
VariableOutlet::VariableOutlet(const std::wstring& name, const std::wstring& id): _name(name), _id(id) {
}

VariableOutlet::~VariableOutlet() {
}

const std::wstring& VariableOutlet::GetVariableID() const {
	return _varid;
}

bool VariableOutlet::IsConnected() {
	return _varid.length()>0;
}

void VariableOutlet::Load(TiXmlElement* e) {
	_varid = LoadAttributeSmall(e, "var", _varid);
}

void VariableOutlet::Save(TiXmlElement* e) {
	SaveAttributeSmall(e, "id", _id);
	SaveAttributeSmall(e, "var", _varid);
}

std::wstring VariableOutlet::GetID() const {
	return _id;
}

std::wstring VariableOutlet::GetName() const {
	return _name;
}

void VariableOutlet::SetName(const std::wstring& name) {
	_name = name;
}

/** OutletProperty **/
OutletProperty::OutletProperty(strong<VariableOutlet> o, strong<Variables> vars): GenericListProperty<std::wstring>(o->GetName(), o, &(o->_varid), L""), _out(o) {
	AddOption(TL(outlet_not_connected), L"");
	unsigned int i = vars->GetVariableCount();

	for(unsigned int a=0;a<i;a++) {
		ref<Variable> var = vars->GetVariableByIndex(a);
		if(var) {
			AddOption(var->GetName(), var->GetID());
		}
	}
}

OutletProperty::~OutletProperty() {
}

Instancer::~Instancer() {
}

SubTimeline::~SubTimeline() {
}