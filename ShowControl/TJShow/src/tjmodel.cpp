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
#include "../include/internal/tjsubtimeline.h"
#include "../include/internal/tjdashboard.h"
#include "../include/internal/tjnetwork.h"
#include <TJScript/include/types/tjscriptmap.h>

#include "../include/internal/view/tjcapacitywnd.h"

using namespace tj::show::view;

Model::Model(strong<Network> net): 
	_version(0),
	_globals(GC::Hold(new ScriptMap())),
	_resources(GC::Hold(new Resources())),
	_caps(GC::Hold(new Capacities())),
	_variables(GC::Hold(new VariableList())),
	_groups(GC::Hold(new Groups())),
	_patches(GC::Hold(new Patches())),
	_rules(GC::Hold(new input::Rules())),
	_rmg(GC::Hold(new ResourceManager())),
	_timeline(GC::Hold(new Timeline())),
	_screens(GC::Hold(new ScreenDefinitions())),
	_dashboard(GC::Hold(new Dashboard())) {

	_rmg->AddProvider(strong<ResourceProvider>(GC::Hold(new AbsoluteLocalFileResourceProvider())));
	_rmg->AddProvider(net->GetResourceProvider());
	_showID = (unsigned int)Util::RandomInt();
}

Model::~Model() {
}

void Model::Clear() {
	_showID = (unsigned int)Util::RandomInt();
	_globals = GC::Hold(new ScriptMap());
	_resources->Clear();
	_caps->Clear();
	_variables->Clear();
	_patches->Clear();
	_rules->Clear();
	_groups->Clear();
	_timeline->Clear();
	_screens->Clear();
	_dashboard->Clear();
	
	_filename = L"";
	_fileHash = "";
	
	_title = L"";
	_author = L"";
	_notes = L"";
	_version = 0;

	Application::Instance()->OnClearShow();
}

strong<Dashboard> Model::GetDashboard() {
	return _dashboard;
}

std::wstring Model::GetAuthor() const {
	return _author;
}

std::wstring Model::GetTitle() const {
	return _title;
}

strong<Capacities> Model::GetCapacities() {
	return _caps;
}

strong<input::Rules> Model::GetInputRules() {
	return _rules;
}

strong<VariableList> Model::GetVariables() {
	return _variables;
}

strong<ScriptMap> Model::GetGlobals() {
	return _globals;
}

strong<ScreenDefinitions> Model::GetScreens() {
	return _screens;
}

namespace tj {
	namespace show {
		class ModelCrumb: public Crumb {
			public:
				ModelCrumb(ref<Model> tl, const std::wstring& name): Crumb(name, L"icons/show.png"), _model(tl) {
				}

				virtual ~ModelCrumb() {
				}

				virtual ref<Inspectable> GetSubject() {
					return ref<Model>(_model);
				}

				virtual void GetChildren(std::vector< ref<Crumb> >& cs) {
					ref<Model> model = _model;
					if(model) {
						cs.push_back(GC::Hold(new BasicCrumb(TL(model_properties), L"icons/toolbar/properties.png", model)));
						cs.push_back(GC::Hold(new BasicCrumb(TL(timeline), L"icons/timeline.png", model->GetTimeline())));
					}
				}

			protected:
				weak<Model> _model;
		};
	}
}

ref<Crumb> Model::CreateModelCrumb() {
	std::wstring name = TL(show);

	if(_title.length()!=0) {
		name = _title;
	}
	return GC::Hold(new ModelCrumb(this, name));
}

std::wstring Model::GetFileName() const {
	return _filename;
}

bool Model::IsSaved() const {
	return _filename.length()!=0;
}

ref<ResourceManager> Model::GetResourceManager() {
	return _rmg;
}

int Model::IncrementVersion() {
	++_version;
	return _version;
}

void Model::Save(TiXmlElement* parent) {
	TiXmlElement me("model");
	SaveAttribute(&me, "author", _author);
	SaveAttribute(&me, "title", _title);
	SaveAttribute(&me, "notes", _notes);
	// Model version is saved in SaveFileAction, because of the hasing stuff

	TiXmlElement groups("groups");
	_groups->Save(&groups);
	me.InsertEndChild(groups);
	
	if(_caps->GetCapacityCount()>0) {
		TiXmlElement capacities("capacities");
		_caps->Save(&capacities);
		me.InsertEndChild(capacities);
	}

	if(_variables->GetVariableCount()>0) {
		TiXmlElement variables("variables");
		_variables->Save(&variables);
		me.InsertEndChild(variables);
	}

	if(_resources->GetResourceCount()>0) {
		TiXmlElement resources("resources");
		_resources->Save(&resources);
		me.InsertEndChild(resources);
	}

	if(_patches->GetPatchCount()>0) {
		TiXmlElement patches("patches");
		_patches->Save(&patches);
		me.InsertEndChild(patches);
	}

	if(_rules->GetRuleCount()>0) {
		TiXmlElement rules("input-rules");
		_rules->Save(&rules);
		me.InsertEndChild(rules);
	}

	// Save the timeline
	_timeline->Save(&me);
	_timeline->GetCueList()->Save(&me);

	strong<VariableList> localVariables = _timeline->GetLocalVariables();
	if(localVariables->GetVariableCount()>0) {
		TiXmlElement locals("locals");
		localVariables->Save(&locals);
		me.InsertEndChild(locals);
	}

	// Cached network settings
	ref<Network> nw = Application::Instance()->GetNetwork();
	if(nw) {
		TiXmlElement net("network");
		nw->Save(&net);
		me.InsertEndChild(net);
	}

	// Dashboard
	TiXmlElement db("dashboard");
	_dashboard->Save(&db);
	me.InsertEndChild(db);

	// Plug-in settings
	strong<PluginManager> pm = PluginManager::Instance();
	TiXmlElement plugins("plugins");
	pm->SaveShowSpecificSettings(&plugins);
	me.InsertEndChild(plugins);

	// Screens
	if(_screens->GetDefinitionCount()>0) {
		TiXmlElement screens("screens");
		_screens->Save(&screens);
		me.InsertEndChild(screens);
	}

	parent->InsertEndChild(me);
}

void Model::SetFileHash(const std::string& sh) {
	_fileHash = sh;
}

std::string Model::GetFileHash() const {
	return _fileHash;
}

strong<ScreenDefinition> Model::GetPreviewScreenDefinition(strong<Settings> templ) {
	ref<ScreenDefinition> def = _screens->GetDefinitionByID(L"preview");
	if(!def) {
		strong<ScreenDefinition> sd = GC::Hold(new ScreenDefinition(templ));
		sd->SetBuiltIn(true);
		_screens->SetDefinition(L"preview", sd);
		return sd;
	}
	return def;
}

void Model::New() {
	Clear();
}

void Model::Save(TiXmlElement* parent, const std::wstring& path) {
	Save(parent);
	SetFileName(path);
}

void Model::Load(TiXmlElement* me, const std::wstring& path) {
	Load(me);
	SetFileName(path);
}

void Model::SetFileName(const std::wstring& path) {
	_filename = path;
	_rb = GC::Hold(new ResourceBundle(_rmg, strong<ResourceProvider>(GC::Hold(new LocalFileResourceProvider(File::GetDirectory(path)))), true));
}

ref<Resources> Model::GetResources() {
	return _resources;
}

void Model::Load(TiXmlElement* me) {
	_version = LoadAttributeSmall<int>(me, "version", 0);

	// We don't take version in to account when we are computing the hashes, so remove it here
	me->RemoveAttribute("version");

	// Compute a hash of the whole XML document. When we are saving, we can compare the newly
	// generated document with the loaded document to see whether it is different. If it is not,
	// we don't have to bother the user with 'do you want to save'-dialogs.
	SecureHash sh;
	XML::GetElementHash(me, sh);
	_fileHash = sh.GetHashAsString();

	_author = LoadAttribute<std::wstring>(me, "author", L"");
	_title = LoadAttribute<std::wstring>(me, "title", L"");
	_notes = LoadAttribute<std::wstring>(me,"notes", L"");

	ref<Timeline> timeline = GetTimeline();

	TiXmlElement* groups = me->FirstChildElement("groups");
	if(groups!=0) {
		_groups->Load(groups);
	}

	TiXmlElement* caps = me->FirstChildElement("capacities");
	if(caps!=0) {
		_caps->Load(caps);
	}

	TiXmlElement* vars = me->FirstChildElement("variables");
	if(vars!=0) {
		_variables->Load(vars);
	}
	
	TiXmlElement* patches = me->FirstChildElement("patches");
	if(patches!=0) {
		_patches->Load(patches);
	}

	TiXmlElement* rules = me->FirstChildElement("input-rules");
	if(rules!=0) {
		_rules->Load(rules);
	}

	// older versions store the time length in seconds in a separate <time> element, use it if it's present
	TiXmlElement* time = me->FirstChildElement("time");
	if(time!=0) {
		timeline->SetTimeLengthMS(1000* StringTo<unsigned int>(time->Attribute("length"),Timeline::DefaultTimelineLength));
	}

	// load tracks, or single track (ttx)
	TiXmlElement* tracks = me->FirstChildElement("tracks");
	if(tracks==0) {
		Throw(TL(file_format_invalid), ExceptionTypeError);
	}
	else {
		timeline->Load(tracks, Application::Instance()->GetInstances()->GetRootInstance());
	}
	
	// load cues
	ref<CueList> cuelist = _timeline->GetCueList();
	if(cuelist) {
		TiXmlElement* cues = me->FirstChildElement("cues");
		if(cues!=0) {
			cuelist->Load(cues);
		}
	}

	// primary timeline locals
	ref<VariableList> varlist = _timeline->GetLocalVariables();
	TiXmlElement* locals = me->FirstChildElement("locals");
	if(locals!=0) {
		varlist->Load(locals);
	}

	TiXmlElement* resources = me->FirstChildElement("resources");
	if(resources!=0) {
		_resources->Load(resources);
	}

	ref<Network> network = Application::Instance()->GetNetwork();
	if(network) {
		TiXmlElement* net = me->FirstChildElement("network");
		if(net!=0) {
			network->Load(net);
		}
	}

	TiXmlElement* dashboardElement = me->FirstChildElement("dashboard");
	if(dashboardElement!=0) {
		_dashboard->Load(dashboardElement);
	}

	// Show-specific plug-in settings
	TiXmlElement* plugins = me->FirstChildElement("plugins");
	if(plugins!=0) {
		strong<PluginManager> pm = PluginManager::Instance();
		pm->LoadShowSpecificSettings(plugins);
	}
	
	// Screen definitions
	TiXmlElement* screens = me->FirstChildElement("screens");
	if(screens!=0) {
		_screens->Load(screens);
	}
}

std::wstring Model::GetWebDirectory() const {
	return File::GetDirectory(GetFileName());
}

ref<PropertySet> Model::GetProperties() {
	ref<PropertySet> prs = GC::Hold(new PropertySet());
	prs->Add(GC::Hold(new PropertySeparator(TL(model_properties))));
	prs->Add(GC::Hold(new GenericProperty<std::wstring>(TL(model_title), this, &_title, _title)));
	prs->Add(GC::Hold(new GenericProperty<std::wstring>(TL(model_author), this, &_author, _author)));
	prs->Add(GC::Hold(new GenericReadOnlyProperty<int>(TL(model_version), this, &_version)));
	prs->Add(GC::Hold(new TextProperty(TL(model_notes), this, &_notes, 150)));
	return prs;
}

ref<Timeline> Model::GetTimelineByID(const TimelineIdentifier& tid, ref<Timeline> parent) {
	if(!parent) {
		parent = _timeline;
	}

	if(parent && parent->GetID()==tid) {
		return parent;
	}

	ref< Iterator< ref<TrackWrapper> > > it = parent->GetTracks();
	while(it->IsValid()) {
		ref<TrackWrapper> tw = it->Get();
		if(tw) {
			ref<SubTimeline> sub = tw->GetSubTimeline();
			if(sub) {
				ref<Timeline> tl = GetTimelineByID(tid, sub->GetTimeline());
				if(tl) {
					return tl;
				}
			}
		}
		it->Next();
	}

	return null;
}

strong<Groups> Model::GetGroups() {
	return _groups;
}

strong<Timeline> Model::GetTimeline() {
	return _timeline;
}

strong<CueList> Model::GetCueList() {
	return _timeline->GetCueList();
}

strong<Patches> Model::GetPatches() {
	return _patches;
}

/** Patches **/
Patches::Patches() {
}

Patches::~Patches() {
}

void Patches::Load(TiXmlElement* you) {
	ThreadLock lock(&_lock);
	TiXmlElement* patch = you->FirstChildElement("patch");
	while(patch!=0) {
		PatchIdentifier pi = LoadAttributeSmall<PatchIdentifier>(patch, "id", L"");
		DeviceIdentifier di = LoadAttributeSmall<DeviceIdentifier>(patch, "device", L"");
		_patches[pi] = di;
		patch = patch->NextSiblingElement("patch");
	}
}

bool Patches::DoesPatchExist(const PatchIdentifier& pi) {
	ThreadLock lock(&_lock);
	return _patches.find(pi)!=_patches.end();
}

void Patches::SetPatch(const PatchIdentifier& pi, ref<Device> di) {
	ThreadLock lock(&_lock);
	if(di) {
		_patches[pi] = di->GetIdentifier();
	}
	else {
		_patches[pi] = L"";
	}
}

void Patches::SetPatch(const PatchIdentifier& pi, const DeviceIdentifier& di) {
	ThreadLock lock(&_lock);
	_patches[pi] = di;
}

PatchIdentifier Patches::GetPatchByIndex(unsigned int i) {
	ThreadLock lock(&_lock);
	std::map<PatchIdentifier, DeviceIdentifier>::const_iterator it = _patches.begin();
	for(unsigned int a=0; a<i && it!=_patches.end();a++) {
		++it;
	}

	if(it!=_patches.end()) {
		return it->first;
	}
	return L"";
}

void Patches::Save(TiXmlElement* me) {
	ThreadLock lock(&_lock);
	std::map<PatchIdentifier, DeviceIdentifier>::const_iterator it = _patches.begin();
	while(it!=_patches.end()) {
		TiXmlElement patch("patch");
		SaveAttributeSmall<PatchIdentifier>(&patch, "id", it->first);
		SaveAttributeSmall<DeviceIdentifier>(&patch, "device", it->second);
		me->InsertEndChild(patch);
		++it;
	}
}

const std::map< PatchIdentifier, DeviceIdentifier >& Patches::GetPatches() {
	return _patches;
}

void Patches::AddPatch(const PatchIdentifier& pi) {
	_patches[pi] = L"";
}

void Patches::RemovePatch(const PatchIdentifier& pi) {
	ThreadLock lock(&_lock);

	std::map<PatchIdentifier, DeviceIdentifier>::iterator it = _patches.find(pi);
	if(it!=_patches.end()) {
		_patches.erase(it);
	}
}

PatchIdentifier Patches::GetPatchForDevice(const DeviceIdentifier& di) {
	ThreadLock lock(&_lock);

	std::map<PatchIdentifier,DeviceIdentifier>::iterator it = _patches.begin();
	while(it!=_patches.end()) {
		if(it->second==di) {
			return it->first;
		}
		++it;
	}

	return L"";
}

DeviceIdentifier Patches::GetDeviceByPatch(const PatchIdentifier& pi, bool create) {
	ThreadLock lock(&_lock);

	std::map<PatchIdentifier,DeviceIdentifier>::iterator it = _patches.find(pi);
	if(it!=_patches.end()) {
		return it->second;
	}
	else {
		if(create && pi!=L"") {
			_patches[pi] = L"";
		}
		return L"";
	}
}

void Patches::Clear() {
	ThreadLock lock(&_lock);
	_patches.clear();
}

unsigned int Patches::GetPatchCount() const {
	ThreadLock lock(&_lock);
	return (unsigned int)_patches.size();
}