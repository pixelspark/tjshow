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
#include <algorithm>
using namespace tj::show;
using namespace tj::shared::graphics;

Variable::Variable(): _type(Any::TypeInteger), _value(Any::TypeInteger), _isInput(false), _isOutput(false) {
	Clone();
}

Variable::~Variable() {
}

void Variable::Clone() {
	_id = Util::RandomIdentifier(L'V');
}

strong<Variable> Variable::CreateInstanceClone() {
	strong<Variable> clone = GC::Hold(new Variable());
	clone->_name = _name;
	clone->_id = _id;
	clone->_value = _initial;
	clone->_initial = _initial;
	clone->_type = _type;
	clone->_isInput = _isInput;
	clone->_isOutput = _isOutput;
	return clone;
}

bool Variable::IsInput() const {
	return _isInput;
}

void Variable::SetIsInput(bool t) {
	_isInput = t;
}

bool Variable::IsOutput() const {
	return _isOutput;
}

void Variable::SetIsOutput(bool t) {
	_isOutput = t;
}

const std::wstring& Variable::GetID() const {
	return _id;
}

void Variable::SetInitialValue(const std::wstring& init) {
	_initial = init;
}

void Variable::Save(TiXmlElement* p) {
	SaveAttributeSmall<std::wstring>(p, "id", _id);
	SaveAttributeSmall<std::wstring>(p, "name", _name);
	SaveAttributeSmall<std::wstring>(p, "initial", _initial);
	SaveAttributeSmall<bool>(p, "is-input", _isInput);
	SaveAttributeSmall<bool>(p, "is-output", _isOutput);
	SaveAttributeSmall<int>(p, "type", (int)_type);

	if(_desc.length()>0) {
		SaveAttribute<std::wstring>(p, "description", _desc);
	}
}

void Variable::Load(TiXmlElement* you) {
	_id = LoadAttributeSmall(you, "id", _id);
	_name = LoadAttributeSmall(you, "name", _name);
	_initial = LoadAttributeSmall<std::wstring>(you, "initial", _initial);
	_desc = LoadAttribute<std::wstring>(you, "description", _desc);
	_type = (Any::Type)LoadAttributeSmall<int>(you, "type", _type);
	_isInput = LoadAttributeSmall<bool>(you, "is-input", _isInput);
	_isOutput = LoadAttributeSmall<bool>(you, "is-output", _isOutput);
}

ref<PropertySet> Variable::GetProperties() {
	ref<PropertySet> ps = GC::Hold(new PropertySet());
	ps->Add(GC::Hold(new GenericProperty<std::wstring>(TL(variable_name), this, &_name, _name)));
	ps->Add(GC::Hold(new TextProperty(TL(variable_description), this, &_desc)));
	ps->Add(Properties::CreateTypeProperty(TL(variable_type), this, &_type));
	ps->Add(GC::Hold(new GenericProperty<std::wstring>(TL(variable_initial_value), this, &_initial, _initial)));
	ps->Add(GC::Hold(new GenericProperty<bool>(TL(variable_is_input), this, &_isInput, _isInput)));
	ps->Add(GC::Hold(new GenericProperty<bool>(TL(variable_is_output), this, &_isOutput, _isOutput)));
	return ps;
}

const std::wstring& Variable::GetName() const {
	return _name;
}

Any::Type Variable::GetType() const {
	return _type;
}

void Variable::SetValue(const Any& a) {
	_value = a.Force(_type);
}

Any Variable::GetInitialValue() const {
	return Any(_type, _initial);
}

Any Variable::GetValue() const {
	return _value.Force(_type);
}

const std::wstring& Variable::GetDescription() const {
	return _desc;
}

void Variable::Initialize() {
	Bind(L"name", &SGetName);
	Bind(L"id", &SGetID);
	Bind(L"description", &SGetDescription);
	Bind(L"value", &SGetValue);
	Bind(L"initialValue", &SGetInitialValue);
}

void Variable::SetName(const std::wstring& n) {
	_name = n;
}

void Variable::Reset() {
	SetValue(Any(_type, _initial));
}

ref<Scriptable> Variable::SGetName(ref<ParameterList> p) {
	return GC::Hold(new ScriptString(_name));
}

ref<Scriptable> Variable::SGetID(ref<ParameterList> p) {
	return GC::Hold(new ScriptString(_id));
}

ref<Scriptable> Variable::SGetValue(ref<ParameterList> p) {
	// TODO Create ScriptAny interface
	Any value = _value;
	return GC::Hold(new ScriptString(value.ToString()));
}

ref<Scriptable> Variable::SGetInitialValue(ref<ParameterList> p) {
	return GC::Hold(new ScriptString(_initial));
}

ref<Scriptable> Variable::SGetDescription(ref<ParameterList> p) {
	return GC::Hold(new ScriptString(_desc));
}

/* Variables */
VariableList::VariableList() {
}

VariableList::~VariableList() {
}

unsigned int VariableList::GetVariableCount() const {
	return (unsigned int)_vars.size();
}

bool VariableList::Exists(ref<Variable> v) const {
	ThreadLock lock(&_lock);
	return std::find(_vars.begin(), _vars.end(), v) != _vars.end();
}	

bool VariableList::Evaluate(ref<Expression> c, ref<Waiting> w) {
	ThreadLock lock(&_lock);

	if(c->Evaluate(ref<Variables>(this)).Force(Any::TypeBool)) {
		return true;
	}
	else if(w) {
		WaitingInfo wi;
		wi._waiting = w;
		wi._condition = c;
		// Push the 'Waiting' object on the waiting list
		_waiting.push_back(wi);
	}
	return false;
}

strong<VariableList> VariableList::CreateInstanceClone() {
	// TODO: allow the use of 'shared' variables (but, how to propagate changes to other cloned VariableLists?)
	strong<VariableList> clone = GC::Hold(new VariableList());
	ThreadLock lock(&_lock);

	std::vector< ref<Variable> >::iterator it = _vars.begin();
	while(it!=_vars.end()) {
		ref<Variable> var = *it;
		if(var) {
			clone->Add(var->CreateInstanceClone());
		}
		++it;
	}
	return clone;
}

void VariableList::Clone() {
	ThreadLock lock(&_lock);

	std::vector< ref<Variable> >::iterator it = _vars.begin();
	while(it!=_vars.end()) {
		ref<Variable> var = *it;
		if(var) {
			var->Clone();
		}
		++it;
	}
}

void VariableList::Assign(ref<Assignments> as, ref<Variables> scope) {
	if(!scope) scope = this;

	int n = 0;
	Variables::ChangedVariables changes;
	{
		ThreadLock lock(&_lock);
		std::vector< ref<Assignment> >::iterator it = as->_assignments.begin();
		while(it!=as->_assignments.end()) {
			ref<Assignment> a = *it;
			if(a) {
				ref<Variable> var = GetVariableById(a->GetVariableId());
				if(var) {
					changes.which.insert(var);
					var->SetValue(a->GetExpression()->Evaluate(scope));
					++n;
				}
			}
			++it;
		}
	}

	if(n>0) {
		OnVariablesChanged(changes);
	}
}

void VariableList::Reset(ref<Variable> v) {
	if(v) {
		{
			ThreadLock lock(&_lock);
			v->Reset();
		}
		Variables::ChangedVariables changes;
		changes.which.insert(v);
		OnVariablesChanged(changes);
	}
}

void VariableList::ResetAll() {
	Variables::ChangedVariables changes;
	{
		ThreadLock lock(&_lock);
		std::vector< ref<Variable> >::iterator it = _vars.begin();
		while(it!=_vars.end()) {
			ref<Variable> var = *it;
			if(var) {
				var->_value = var->_initial;
				changes.which.insert(var);
			}
			++it;
		}
	}

	OnVariablesChanged(changes);
}

void VariableList::Set(ref<Variable> v, const Any& value) {
	if(v) {
		{
			ThreadLock lock(&_lock);
			v->SetValue(value);
		}

		Variables::ChangedVariables changes;
		changes.which.insert(v);
		OnVariablesChanged(changes);
	}
}

void VariableList::Clear() {
	ThreadLock lock(&_lock);
	_vars.clear();
}

ref<Property> VariableList::CreateProperty(const std::wstring& name, ref<Inspectable> holder, std::wstring* id, const std::wstring& def) {
	ref<GenericListProperty<std::wstring> > lp = GC::Hold(new GenericListProperty<std::wstring>(name, holder, id, def));
	ThreadLock lock(&_lock);

	std::vector< ref<Variable> >::iterator it = _vars.begin();
	while(it!=_vars.end()) {
		ref<Variable> var = *it;
		if(var) {
			lp->AddOption(var->GetName(), var->GetID());
		}
		++it;
	}

	return lp;
}

void VariableList::Save(TiXmlElement* p) {
	ThreadLock lock(&_lock);
	std::vector< ref<Variable> >::iterator it = _vars.begin();
	while(it!=_vars.end()) {
		TiXmlElement variable("variable");
		(*it)->Save(&variable);
		p->InsertEndChild(variable);
		++it;
	}
}

void VariableList::Load(TiXmlElement* p) {
	ThreadLock lock(&_lock);

	TiXmlElement* var = p->FirstChildElement("variable");
	while(var!=0) {
		ref<Variable> v = GC::Hold(new Variable());
		v->Load(var);
		_vars.push_back(v);
		var = var->NextSiblingElement("variable");
	}
}

ref<Variable> VariableList::DoChoosePopup(Pixels x, Pixels y, ref<Wnd> w) {
	int n = 1;
	ContextMenu m;
	{
		ThreadLock lock(&_lock);
		
		if(_vars.size()>0) {
			std::vector< ref<Variable> >::iterator it = _vars.begin();
			while(it!=_vars.end()) {
				ref<Variable> var = *it;
				if(var) {
					m.AddItem(var->GetName(), n);
				}
				++n;
				++it;
			}
		}
		else {
			m.AddItem(TL(no_variables_defined), -1);
		}
	}

	int r = m.DoContextMenu(w,x,y);
	if(n>1 && r>0 && r<=n) {
		return GetVariableByIndex(r-1);
	}

	return 0;
}

ref<Variable> VariableList::GetVariableByIndex(unsigned int idx) {
	ThreadLock lock(&_lock);
	if(idx<_vars.size()) {
		return _vars.at(idx);
	}
	return 0;
}

ref<Variable> VariableList::GetVariableById(const std::wstring& id) {
	ThreadLock lock(&_lock);
	std::vector< ref<Variable> >::iterator it = _vars.begin();
	while(it!=_vars.end()) {
		ref<Variable> v = *it;
		if(v && v->GetID()==id) {
			return v;
		}
		++it;
	}
	return 0;
}

ref<Variable> VariableList::GetVariableByName(const std::wstring& name) {
	ThreadLock lock(&_lock);
	std::vector< ref<Variable> >::iterator it = _vars.begin();
	while(it!=_vars.end()) {
		ref<Variable> v = *it;
		if(v && v->GetName()==name) {
			return v;
		}
		++it;
	}
	return 0;
}

void VariableList::Add(ref<Variable> v) {
	ThreadLock lock(&_lock);
	_vars.push_back(v);
}

void VariableList::Remove(ref<Variable> v) {
	ThreadLock lock(&_lock);
	std::vector< ref<Variable> >::iterator it = std::find(_vars.begin(), _vars.end(), v);
	if(it!=_vars.end()) {
		_vars.erase(it);
	}
}

void VariableList::Initialize() {
	Bind(L"set", &SSet);
	Bind(L"count", &SGetCount);
	Bind(L"get", &SGet);
}

ref<Scriptable> VariableList::SGet(ref<ParameterList> p) {
	Parameter<std::wstring> key(L"key", 0);

	std::wstring k = key.Require(p, L"");
	ref<Variable> var = GetVariableById(k);
	if(var) {
		return var;
	}

	var = GetVariableByName(k);
	if(var) {
		return var;
	}

	return ScriptConstants::Null;
}

ref<Scriptable> VariableList::SGetCount(ref<ParameterList> p) {
	return GC::Hold(new ScriptInt(GetVariableCount()));
}

ref<Scriptable> VariableList::SSet(ref<ParameterList> p) {
	static Parameter<int> value(L"value", 1);

	ref<Variable> var = 0;

	if(p->Exists(L"0")) {
		ref<Scriptable> v = p->Get(L"0");
		if(v.IsCastableTo<Variable>()) {
			var = v;
		}
	}
	else if(p->Exists(L"variable")) {
		ref<Scriptable> v = p->Get(L"variable");
		if(v.IsCastableTo<Variable>()) {
			var = v;
		}
	}

	if(!var) {
		throw ParameterException(L"variable");
	}

	Set(var, value.Require(p, 0));
	return ScriptConstants::Null;
}

bool VariableList::Set(Field f, ref<Scriptable> s) {
	ref<Variable> var = GetVariableByName(f);
	if(var) {
		Set(var, Any(ref<Object>(s)));
		return true;
	}
	return false;
}

/* Variables */
Variables::~Variables() {
}

std::wstring Variables::ParseVariables(ref<Variables> vars, const std::wstring& source) {
	if(!vars) {
		return source;
	}
	
	std::wostringstream wos;
	std::wstring::const_iterator nextOpenBrace = std::find(source.begin(), source.end(), L'{');

	if(nextOpenBrace==source.end()) {
		// No open braces in this string, just return the original then
		return source;
	}

	std::wstring::const_iterator previous = source.begin();

	{
		///ThreadLock lock(&(vars->_lock));
		while(nextOpenBrace != source.end()) {
			wos << std::wstring(previous, nextOpenBrace);

			std::wstring::const_iterator nextCloseBrace = std::find(nextOpenBrace, source.end(), L'}');
			if(nextCloseBrace!=source.end()) {
				std::wstring var(nextOpenBrace+1, nextCloseBrace);
				// Try to find this variable
				ref<Variable> variable = vars->GetVariableByName(var);
				if(variable) {
					wos << variable->GetValue().ToString();
				}
				else {
					try {
						// Try to parse it with the condition parser
						ref<Expression> ex = GC::Hold(new NullExpression());
						ex->Parse(var, vars);
						wos << ex->Evaluate(vars).ToString();
					}
					catch(...) {
						wos << var;
					}
				}
			}

			previous = nextCloseBrace;
			if(previous!=source.end()) {
				++previous;
			}

			nextOpenBrace = std::find(previous, source.end(), L'{');
		}
	}
	wos << std::wstring(previous, source.end());
	return wos.str();
}

void Variables::OnVariablesChanged(const ChangedVariables& changes) {
	std::list< ref<Waiting> > satisfied;

	{
		ThreadLock lock(&_lock);
		std::list< WaitingInfo >::iterator it = _waiting.begin();
		while(it!=_waiting.end()) {
			WaitingInfo& wi = *it;
			if(wi._condition && (bool)(wi._condition->Evaluate(ref<Variables>(this)))) {
				// this condition is satisfied, notify, remove and move on
				ref<Waiting> waiting = wi._waiting;
				it = _waiting.erase(it); // Erase has to happen before calling 'Acquired', since the waiting (cue) can also change variables and trigger 'OnVariablesChanged'.

				if(waiting) {
					satisfied.push_back(waiting);
				}
			}
			else {
				++it;
			}
		}
	}

	std::list< ref<Waiting> >::iterator it = satisfied.begin();
	while(it!=satisfied.end()) {
		ref<Waiting> wi = *it;
		wi->Acquired(0);
		++it;
	}

	EventVariablesChanged.Fire(this, changes);
	Application::Instance()->GetView()->OnVariablesChanged();
}

/* ScopedVariables */
ScopedVariables::ScopedVariables(strong<Variables> local, strong<Variables> global): _local(local), _global(global) {
}

ScopedVariables::~ScopedVariables() {
}

void ScopedVariables::OnCreated() {
	_local->EventVariablesChanged.AddListener(ref<ScopedVariables>(this));
	_global->EventVariablesChanged.AddListener(ref<ScopedVariables>(this));
}

unsigned int ScopedVariables::GetVariableCount() const {
	return _local->GetVariableCount() + _global->GetVariableCount();
}

ref<Variable> ScopedVariables::GetVariableByIndex(unsigned int idx) {
	unsigned int localLength = _local->GetVariableCount();
	if(idx<localLength) {
		return _local->GetVariableByIndex(idx);
	}
	return _global->GetVariableByIndex(idx-localLength);
}

ref<Variable> ScopedVariables::GetVariableById(const std::wstring& id) {
	ref<Variable> vlocal = _local->GetVariableById(id);
	if(vlocal) {
		return vlocal;
	}
	else {
		return _global->GetVariableById(id);
	}
}

ref<Variable> ScopedVariables::GetVariableByName(const std::wstring& name) {
	ref<Variable> vlocal = _local->GetVariableByName(name);
	if(vlocal) {
		return vlocal;
	}
	else {
		return _global->GetVariableByName(name);
	}
}

void ScopedVariables::Notify(ref<Object> source, const Variables::ChangedVariables& data) {
	OnVariablesChanged(data);
}

void ScopedVariables::Reset(ref<Variable> v) {
	ThreadLock lock(&_lock);

	if(_local->Exists(v)) {
		_local->Reset(v);
	}
	else {
		_global->Reset(v);
	}
}

void ScopedVariables::Set(ref<Variable> v, const Any& val) {
	ThreadLock lock(&_lock);

	if(_local->Exists(v)) {
		_local->Set(v,val);
	}
	else {
		_global->Set(v,val);
	}
}

void ScopedVariables::Remove(ref<Variable> v) {
	ThreadLock lock(&_lock);

	if(_local->Exists(v)) {
		_local->Remove(v);
	}
	else {
		_global->Remove(v);
	}
}

void ScopedVariables::Add(ref<Variable> v) {
	_local->Add(v);
}

bool ScopedVariables::Exists(ref<Variable> v) const {
	ThreadLock lock(&_lock);

	return _local->Exists(v) || _global->Exists(v);
}

ref<Property> ScopedVariables::CreateProperty(const std::wstring& name, ref<Inspectable> holder, std::wstring* id, const std::wstring& def) {
	ref<GenericListProperty<std::wstring> > lp = GC::Hold(new GenericListProperty<std::wstring>(name, holder, id, def));


	{
		ThreadLock lock(&_local->_lock);
		unsigned int n = _local->GetVariableCount();
		for(unsigned int a = 0; a < n; a++) {
			ref<Variable> var = _local->GetVariableByIndex(a);
			if(var) {
				lp->AddOption(var->GetName(), var->GetID());
			}
		}
	}

	{
		ThreadLock lock(&_global->_lock);
		unsigned int n = _global->GetVariableCount();
		for(unsigned int a = 0; a < n; a++) {
			ref<Variable> var = _global->GetVariableByIndex(a);
			if(var) {
				lp->AddOption(var->GetName(), var->GetID());
			}
		}
	}

	return lp;
}

ref<Variable> ScopedVariables::DoChoosePopup(Pixels x, Pixels y, ref<Wnd> w) {
	ContextMenu cm;
	cm.AddSeparator(TL(variables_local));
	int idx = 0;

	{
		ThreadLock lock(&_local->_lock);
		unsigned int n = _local->GetVariableCount();
		for(unsigned int a = 0; a < n; a++) {
			ref<Variable> var = _local->GetVariableByIndex(a);
			if(var) {
				cm.AddItem(var->GetName(), idx);
			}
			++idx;
		}
	}

	cm.AddSeparator(TL(variables_global));
	{
		ThreadLock lock(&_global->_lock);
		unsigned int n = _global->GetVariableCount();
		for(unsigned int a = 0; a < n; a++) {
			ref<Variable> var = _global->GetVariableByIndex(a);
			if(var) {
				cm.AddItem(var->GetName(), idx);
			}
			++idx;
		}
	}

	int r = cm.DoContextMenu(w,x,y);
	if(r>=0) {
		return GetVariableByIndex(r);
	}

	return 0;
}

bool ScopedVariables::Evaluate(ref<Expression> c, ref<Waiting> w) {
	/* Since any outside change to a variable in either local or global would also cause a
	call to OnVariablesChanged here and try to lock this instance of ScopedVariables,
	threadlocking here ensures that nothing changes during the assignments */
	ThreadLock lock(&_lock);

	if(c->Evaluate(ref<Variables>(this)).Force(Any::TypeBool)) {
		return true;
	}
	else if(w) {
		WaitingInfo wi;
		wi._waiting = w;
		wi._condition = c;
		// Push the 'Waiting' object on the waiting list
		_waiting.push_back(wi);
	}
	return false;
}

void ScopedVariables::Assign(ref<Assignments> s, ref<Variables> vars) {
	/* Since any outside change to a variable in either local or global would also cause a
	call to OnVariablesChanged here and try to lock this instance of ScopedVariables,
	threadlocking here ensures that nothing changes during the assignments */
	///ThreadLock lock(&_lock);

	_local->Assign(s, vars);
	_global->Assign(s, vars);
}

/** VariableEndpoint **/
namespace tj {
	namespace show {
		class VariableEndpoint: public Endpoint {
			public:
				VariableEndpoint(ref<Variable> var, ref<Variables> vars);
				virtual ~VariableEndpoint();
				virtual void Set(const Any& f);
				virtual std::wstring GetName() const;

			protected:
				weak<Variables> _vars;
				weak<Variable> _var;
		};
	}
}

/** VariableEndpointCategory **/
VariableEndpointCategory::VariableEndpointCategory(): EndpointCategory(KVariableEndpointCategoryID) {
}

VariableEndpointCategory::~VariableEndpointCategory() {
}

ref<Endpoint> VariableEndpointCategory::GetEndpointById(const EndpointID& id) {
	ref<Variables> vars = Application::Instance()->GetModel()->GetVariables();
	if(vars) {
		ref<Variable> var = vars->GetVariableById(id);
		if(var) {
			return GC::Hold(new VariableEndpoint(var, vars));
		}
	}
	return null;
}

const EndpointCategoryID VariableEndpointCategory::KVariableEndpointCategoryID = L"variable";

/** VariableEndpoint **/
VariableEndpoint::VariableEndpoint(ref<Variable> var, ref<Variables> vars): _vars(vars), _var(var) {
}

VariableEndpoint::~VariableEndpoint() {
}

void VariableEndpoint::Set(const Any& f) {
	ref<Variables> vars = _vars;
	ref<Variable> var = _var;
	if(var && vars) {
		vars->Set(var, f);
	}
}

std::wstring VariableEndpoint::GetName() const {
	ref<Variable> var = _var;
	if(var) {
		return std::wstring(TL(variable))+L" '"+var->GetName()+L'\'';
	}
	return L"?";
}