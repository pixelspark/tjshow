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
#include <TJNP/include/tjpattern.h>
using namespace tj::shared;
using namespace tj::show;
using namespace tj::show::input;
using namespace tj::np::pattern;

/* Rules */
Rules::Rules() {
}

Rules::~Rules() {
}

void Rules::Save(TiXmlElement* you) {
	ThreadLock lock(&_lock);
	std::vector< ref<Rule> >::iterator it = _rules.begin();
	while(it!=_rules.end()) {
		ref<Rule> inputRule = *it;
		if(inputRule) {
			TiXmlElement rule("rule");
			inputRule->Save(&rule);
			you->InsertEndChild(rule);
		}
		++it;
	}
}

void Rules::Load(TiXmlElement* you) {
	ThreadLock lock(&_lock);
	TiXmlElement* rule = you->FirstChildElement("rule");
	while(rule!=0) {
		ref<Rule> inputRule = GC::Hold(new Rule());
		inputRule->Load(rule);
		_rules.push_back(inputRule);
		rule = rule->NextSiblingElement("rule");
	}
}

void Rules::FindRulesForPatch(const PatchIdentifier& pi, std::vector< ref<Rule> >& lst) {
	ThreadLock lock(&_lock);
	std::vector< ref<Rule> >::iterator it = _rules.begin();
	while(it!=_rules.end()) {
		ref<Rule> inputRule = *it;
		if(inputRule && inputRule->_patch==pi) {
			lst.push_back(inputRule);
		}
		++it;
	}
}

void Rules::Dispatch(const PatchIdentifier& pi, const InputID& path, const Any& value) {
	if(pi!=L"") {
		// Fire here
		ThreadLock lock(&_lock);

		std::vector< ref<Rule> >::iterator it = _rules.begin();
		while(it!=_rules.end()) {
			ref<Rule> rule = *it;
			if(rule) {
				rule->Fire(pi, path, value);
			}
			++it;
		}
	}

	if(_lastPaths.size()>KRememberPaths-1) {
		_lastPaths.pop_front();
	}
	_lastPaths.push_back(path);
	_lastPatch = pi;
}

void Rules::Dispatch(ref<Device> device, const InputID& path, const Any& value) {
	// Find patch identifier (TODO: what to do with multiply patched devices?)
	PatchIdentifier pi = Application::Instance()->GetOutputManager()->GetPatchByDevice(device);
	Dispatch(pi, path, value);
}

void Rules::AddRule(ref<Rule> rule) {
	if(rule) {
		ThreadLock lock(&_lock);
		_rules.push_back(rule);
	}
}

void Rules::RemoveRule(ref<Rule> rule) {
	ThreadLock lock(&_lock);
	std::vector< ref<Rule> >::iterator it = _rules.begin();
	while(it!=_rules.end()) {
		ref<Rule> frule = *it;
		if(rule==frule) {
			it = _rules.erase(it);
		}
		else {
			++it;
		}
	}
}

ref<Rule> Rules::GetRuleByIndex(unsigned int idx) {
	ThreadLock lock(&_lock);
	return _rules.at(idx);
}

void Rules::Clear() {
	ThreadLock lock(&_lock);
	_rules.clear();
}

unsigned int Rules::GetRuleCount() const {
	ThreadLock lock(&_lock);
	return (unsigned int)_rules.size();
}

ref<Rule> Rules::DoBindDialog(ref<Wnd> parent, const EndpointCategoryID& cat, const EndpointID& eid) {
	// Check if there are patches. If there are no patches, tell the user to create one first
	ref<Model> model = Application::Instance()->GetModel();
	if(model->GetPatches()->GetPatchCount()>0) {
		class AddRuleData: public Inspectable {
			public:
				AddRuleData(strong<Rules> rs): _rs(rs) {}
				virtual ~AddRuleData() {}
				virtual ref<PropertySet> GetProperties() {
					ref<PropertySet> ps = GC::Hold(new PropertySet());
					ref<OutputManager> om = Application::Instance()->GetOutputManager();
					if(om) {
						ps->Add(om->CreateSelectPatchProperty(TL(input_patch), this, &_patch));
						ref<SuggestionProperty> sp = GC::Hold(new SuggestionProperty(TL(input_channel_id), this, &_path));
						strong<Menu> sm = sp->GetSuggestionMenu();
						std::deque<InputID>::const_reverse_iterator it = _rs->_lastPaths.rbegin();
						while(it!=_rs->_lastPaths.rend()) {
							const InputID& id = *it;
							sm->AddItem(strong<MenuItem>(GC::Hold(new SuggestionMenuItem(id, id))));
							++it;
						}
						ps->Add(sp);
					}
					return ps;
				} 

				PatchIdentifier _patch;
				InputID _path;
				strong<Rules> _rs;
		};

		ref<AddRuleData> data = GC::Hold(new AddRuleData(strong<Rules>(this)));
		data->_patch = _lastPatch;
		if(_lastPaths.size()>0) {
			data->_path = *(_lastPaths.rbegin());
		}

		ref<PropertyDialogWnd> dw = GC::Hold(new PropertyDialogWnd(TL(input_attach), TL(input_attach_question)));
		dw->SetSize(300,170);
		dw->GetPropertyGrid()->Inspect(data);

		while(true) {
			if(dw->DoModal(parent)) {
				// check data; if valid, return a rule; if not, repeat
				if(data->_patch!=L"") {
					ref<Rule> rule = GC::Hold(new Rule());
					rule->_cat = cat;
					rule->_id = eid;
					rule->_patch = data->_patch;
					rule->_path = data->_path;
					return rule;
				}
				else {
					// TODO nice error message?
				}
			}
			else {
				return 0;
			}
		}
	}
	else {
		Throw(TL(error_create_patches_before_binding), ExceptionTypeError);
	}

	return 0;
}

/* Rule */
Rule::Rule(): _path(L"") {
}

Rule::~Rule() {
}

void Rule::Save(TiXmlElement* you) {
	SaveAttributeSmall(you, "patch", _patch);
	SaveAttributeSmall(you, "path", _path);
	SaveAttributeSmall(you, "to-type", _cat);
	SaveAttributeSmall(you, "to-id", _id);
}

void Rule::Load(TiXmlElement* you) {
	_patch = LoadAttributeSmall(you, "patch", _patch);
	_path = LoadAttributeSmall(you, "path", _path);
	_cat = LoadAttributeSmall(you, "to-type", _cat);
	_id = LoadAttributeSmall(you, "to-id", _id);
}

ref<PropertySet> Rule::GetProperties() {
	ref<PropertySet> ps = GC::Hold(new PropertySet());
	ps->Add(GC::Hold(new PropertySeparator(TL(input_details))));
	ps->Add(GC::Hold(new GenericProperty<InputID>(TL(input_channel_id), this, &_path, _path)));
	return ps;
}

ref<Endpoint> Rule::GetEndpoint() {
	ref<EndpointCategory> cat = PluginManager::Instance()->GetEndpointCategoryById(_cat);
	if(cat) {
		return cat->GetEndpointById(_id);
	}
	return 0;
}

void Rule::Fire(const PatchIdentifier& pi, const InputID& path, const Any& value) {
	// If this message is for our patch, do some pattern matching magic (like OSC does)
	if(pi==_patch && Pattern::Match(path.c_str(), _path.c_str())) {
		// Get endpoint and fire!
		ref<Endpoint> ep = GetEndpoint();
		if(ep) {
			ep->Set(value);
		}
	}
}

InputID Rule::GetPath() const {
	return _path;
}