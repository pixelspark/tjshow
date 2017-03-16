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
#include "../include/internal/view/tjcapacitywnd.h"
#include <algorithm>
using namespace tj::show;
using namespace tj::script;
using namespace tj::shared::graphics;
using namespace tj::show::view;

Waiting::~Waiting() {
}

std::wstring Waiting::GetName() {
	return TL(capacity_waiting_unknown);
}

/** Acquisition **/
Acquisition::Acquisition(ref<Capacity> c) {
	_cap = c;
	_n = 0;
}

Acquisition::~Acquisition() {
	// Make sure we cancel any waiting
	_waitingFor = 0;

	if(_cap) {
		_cap->Release(_n);
	}
}

void Acquisition::Cancel() {
	ThreadLock lock(&_lock);
	if(_waitingFor) {
		_cap->Cancel(this);
		_waitingFor = 0;
	}
}

void Acquisition::Acquired(int n) {
	ThreadLock lock(&_lock);
	_n += n;

	if(_waitingFor) {
		_waitingFor->Acquired(n);
		_waitingFor = 0;
	}
}

// Partly releasing stuff is also possible
void Acquisition::Release(int n) {
	ThreadLock lock(&_lock);
	if(n>_n) {
		n = _n;
	}

	_cap->Release(n);
	_n -= n;
}

bool Acquisition::Acquire(int n, ref<Waiting> w) {
	ThreadLock lock(&_lock);
	if(n<=_n) {
		return true; // we have enough capacity
	}

	_waitingFor = w;
	if(_cap->Acquire(n-_n,ref<Waiting>(this))) {
		Acquired(n);
		return true;
	}

	return false;
}

bool Acquisition::Ensure(int n, ref<Waiting> w) {
	ThreadLock lock(&_lock);
	if(n<=_n) {
		return true; // we have enough capacity
	}

	_waitingFor = w;
	if(_cap->Acquire(n-_n, this)) {
		Acquired(n-_n);
		return true;
	}

	return false;
}

ref<Capacity> Acquisition::GetCapacity() {
	return _cap;
}

Capacity::Capacity() {
	Clone();
	_value = 1;
	_initial = 1;
	_outstanding = 0;
}

Capacity::~Capacity() {
}

void Capacity::Load(TiXmlElement* you) {
	ThreadLock lock(&_lock);
	_id = LoadAttributeSmall<CapacityIdentifier>(you, "id", _id);
	_name = LoadAttributeSmall<std::wstring>(you, "name", _name);
	_description = LoadAttribute<std::wstring>(you, "description", _description);
	_initial = LoadAttributeSmall(you, "initial", _initial);

	// If nobody is holding this lock, we can safely set the value
	if(_outstanding==0) {
		_value = _initial;
	}
}

void Capacity::Cancel(ref<Waiting> w) {
	ThreadLock lock(&_lock);
	std::deque< WaitingInfo >::iterator it = _queue.begin();
	while(it!=_queue.end()) {
		WaitingInfo& wi = *it;
		if(wi._w ==w) {
			_queue.erase(it);
			return;
		}
		++it;
	}
}

ref<Acquisition> Capacity::CreateAcquisition() {
	return GC::Hold(new Acquisition(this));
}

void Capacity::Clone() {
	_id = Util::RandomIdentifier(L'G');
}

void Capacity::Save(TiXmlElement* parent) {
	ThreadLock lock(&_lock);
	SaveAttributeSmall<CapacityIdentifier>(parent, "id", _id);
	SaveAttributeSmall<std::wstring>(parent, "name", _name);
	SaveAttributeSmall(parent, "initial", _initial);

	if(_description.length()>0) {
		SaveAttribute<std::wstring>(parent, "description", _description);
	}
}

ref<PropertySet> Capacity::GetProperties() {
	ref<PropertySet> prs = GC::Hold(new PropertySet());
	
	prs->Add(GC::Hold(new GenericProperty<std::wstring>(TL(capacity_name), this, &_name, _name)));
	prs->Add(GC::Hold(new GenericProperty<int>(TL(capacity_initial), this, &_initial, _initial)));
	prs->Add(GC::Hold(new TextProperty(TL(capacity_description), this, &_description, 50)));
	return prs;
}

std::wstring Capacity::GetWaitingList() {
	ThreadLock lock(&_lock);

	int n = 0;
	std::deque< WaitingInfo >::iterator it = _queue.begin();
	while(it!=_queue.end()) {
		WaitingInfo& wi = *it;
		ref<Waiting> waiting = wi._w;
		if(waiting) {
			n++;
		}
		++it;
	}
	return Stringify(n);
}

void Capacity::SetName(const std::wstring& n) {
	_name = n;
}

void Capacity::SetDescription(const std::wstring& d) {
	_description = d;
}

const CapacityIdentifier& Capacity::GetID() const {
	return _id;
}

const std::wstring& Capacity::GetName() const {
	return _name;
}

const std::wstring& Capacity::GetDescription() const {
	return _description;
}

int Capacity::GetValue() const {
	return _value;
}

int Capacity::GetInitialValue() const {
	return _initial;
}

bool Capacity::Acquire(int n, ref<Waiting> w) {
	ThreadLock lock(&_lock);
	if(_value<n) {
		if(w) {
			WaitingInfo wt;
			wt._w = w;
			wt._amount = n;
			_queue.push_back(wt);
		}
		
		Application::Instance()->GetView()->OnCapacityChanged();
		return false;
	}

	_value -= n;
	_outstanding += n;

	Application::Instance()->GetView()->OnCapacityChanged();
	return true;
}

void Capacity::Release(int n) {
	{
		ThreadLock lock(&_lock);
		_value += n;
		_outstanding -= n;

		
		if(_queue.size()>0) {
			std::deque<WaitingInfo>::iterator it = _queue.begin();
			while(it!=_queue.end()) {
				const WaitingInfo& wt = *it;
				int amount = wt._amount;
				ref<Waiting> w = wt._w;

				if(w) {
					// Check if we have enough to satisfy the longest waiting timeline; if so, the timeline acquires
					// If not, we wait longer
					if(amount<=_value) {
						_value -= wt._amount;
						_outstanding += wt._amount;
						it = _queue.erase(it);
						w->Acquired(wt._amount); 
					}
					else {
						break;
					}
				}
				else {
					// If the wait object doesn't exist anymore, don't do anything and just delete the thing
					it = _queue.erase(it);
				}
			}
		}
	}
	
	ref<View> view = Application::Instance()->GetView();
	if(view) {
		view->OnCapacityChanged();
	}
}

void Capacity::Reset() {
	ThreadLock lock(&_lock);
	_queue.clear();

	// Only if there are no more Acquisitions outstanding, we can safely do this
	if(_outstanding==0) {
		_value = _initial;
	}
	else {
		Throw(L"Cannot reset capacity, some of the capacity is still in use", ExceptionTypeError);
	}
}

/* Capacities */
namespace tj {
	namespace show {
		namespace script {
			class ScriptCapacitiesIterator: public ScriptIterator<Capacity> {
				public:
					ScriptCapacitiesIterator(ref<Capacities> c) {
						_caps = c;
						_it = _caps->_caps.begin();
						_end = _caps->_caps.end();
					}

					virtual ~ScriptCapacitiesIterator() {
					}

					virtual tj::shared::ref<Scriptable> Next() {
						if(_it!=_end) {
							ref<Capacity> cap = *_it;
							++_it;
							return cap->GetScriptable();
						}
						return ScriptConstants::Null;
					}

				protected:
					ref<Capacities> _caps;
					std::vector< ref<Capacity> >::iterator _it;
					std::vector< ref<Capacity> >::iterator _end;
			};

			/* Scriptable API */
			class CapacitiesScriptable: public ScriptObject<CapacitiesScriptable> {
				public:
					CapacitiesScriptable(ref<Capacities> cp) {
						_caps = cp;
					}

					virtual ~CapacitiesScriptable() {
					}

					static void Initialize() {
						Bind(L"count", &SCount);
						Bind(L"list", &SList);
						Bind(L"get", &SGet);
					}

					ref<Scriptable> SCount(ref<ParameterList> p) {
						return GC::Hold(new ScriptInt(_caps->GetCapacityCount()));
					}

					ref<Scriptable> SList(ref<ParameterList> p) {
						return GC::Hold(new ScriptCapacitiesIterator(_caps));
					}

					ref<Scriptable> SGet(ref<ParameterList> p);
					
				protected:
					ref<Capacities> _caps;
			};

			class CapacityScriptable: public ScriptObject<CapacityScriptable> {
				public:
					CapacityScriptable(ref<Capacity> cap) {
						_cap = cap;
					}

					virtual ~CapacityScriptable() {
					}

					static void Initialize() {
						Bind(L"name", &SName);
						Bind(L"description", &SDescription);
						Bind(L"used", &SUsed);
						Bind(L"free", &SFree);
						Bind(L"initial", &SInitial);
						Bind(L"reset", &SReset);
					}

					ref<Scriptable> SName(ref<ParameterList> p) {
						return GC::Hold(new ScriptString(_cap->GetName()));
					}

					ref<Scriptable> SDescription(ref<ParameterList> p) {
						return GC::Hold(new ScriptString(_cap->GetDescription()));
					}

					ref<Scriptable> SUsed(ref<ParameterList> p) {
						return GC::Hold(new ScriptInt(_cap->GetInitialValue()-_cap->GetValue()));
					}

					ref<Scriptable> SFree(ref<ParameterList> p) {
						return GC::Hold(new ScriptInt(_cap->GetValue()));
					}

					ref<Scriptable> SInitial(ref<ParameterList> p) {
						return GC::Hold(new ScriptInt(_cap->GetInitialValue()));
					}

					ref<Scriptable> SReset(ref<ParameterList> p) {
						_cap->Reset();
						return ScriptConstants::Null;
					}

					virtual bool Set(Field field, ref<Scriptable> val) {
						if(field==L"name") {
							_cap->SetName(ScriptContext::GetValue<std::wstring>(val, L""));
							return true;
						}
						else if(field==L"description") {
							_cap->SetDescription(ScriptContext::GetValue<std::wstring>(val, L""));
							return true;
						}
						return false;
					}

				protected:
					ref<Capacity> _cap;
			};

			ref<Scriptable> CapacitiesScriptable::SGet(ref<ParameterList> p) {
				static const Parameter<std::wstring> PKey(L"key", 0);
				
				std::wstring key = PKey.Require(p,L"");
				ref<Capacity> cap = _caps->GetCapacityByName(key);
				if(cap) {
					return GC::Hold(new CapacityScriptable(cap));
				}
				return ScriptConstants::Null;	
			}
		}
	}
}

ref<Scriptable> Capacity::GetScriptable() {
	return GC::Hold(new script::CapacityScriptable(this));
}

/* Capacities */
Capacities::Capacities() {
}

Capacities::~Capacities() {
}

void Capacities::Load(TiXmlElement* you) {
	ThreadLock lock(&_lock);
	TiXmlElement* cap = you->FirstChildElement("capacity");
	while(cap!=0) {
		ref<Capacity> c = GC::Hold(new Capacity());
		c->Load(cap);
		AddCapacity(c);
		cap = cap->NextSiblingElement("capacity");
	}
}

void Capacities::Save(TiXmlElement* parent) {
	ThreadLock lock(&_lock);
	std::vector< ref<Capacity> >::iterator it = _caps.begin();

	while(it!=_caps.end()) {
		ref<Capacity> c = *it;
		TiXmlElement e("capacity");
		c->Save(&e);
		parent->InsertEndChild(e);
		++it;
	}
}

std::vector< ref<Capacity> >::const_iterator Capacities::GetBegin() {
	return _caps.begin();
}

std::vector< ref<Capacity> >::const_iterator Capacities::GetEnd() {
	return _caps.end();
}

void Capacities::MoveUp(ref<Capacity> c) {
	ThreadLock lock(&_lock);
	std::vector< ref<Capacity> >::iterator it = std::find(_caps.begin(), _caps.end(), c);
	if(it!=_caps.end() && it!=_caps.begin()) {
		ref<Capacity> a = *it;
		*it = *(it-1);
		*(it-1) = a;
	}
}

void Capacities::MoveDown(ref<Capacity> c) {
	ThreadLock lock(&_lock);
	std::vector< ref<Capacity> >::iterator it = std::find(_caps.begin(), _caps.end(), c);
	if(it!=_caps.end() && (it+1)!=_caps.end()) {
		ref<Capacity> a = *it;
		*it = *(it+1);
		*(it+1) = a;
	}
}

void Capacities::AddCapacity(ref<Capacity> c) {
	ThreadLock lock(&_lock);
	_caps.push_back(c);
}

void Capacities::RemoveCapacity(int r) {
	ThreadLock lock(&_lock);
	if(int(_caps.size())>r) {
		_caps.erase(_caps.begin()+r);
	}
}

void Capacities::RemoveCapacity(ref<Capacity> c) {
	ThreadLock lock(&_lock);
	std::vector<ref<Capacity> >::iterator it = _caps.begin();
	while(it!=_caps.end()) {
		if(*it==c) {
			it = _caps.erase(it);
		}
		else {
			++it;
		}
	}
}

ref<Scriptable> Capacities::GetScriptable() {
	return GC::Hold(new script::CapacitiesScriptable(this));
}

void Capacities::Reset() {
	ThreadLock lock(&_lock);
	std::vector< ref<Capacity> >::iterator it = _caps.begin();

	while(it!=_caps.end()) {
		ref<Capacity> cap = *it;
		cap->Reset();
		++it;
	}
}

int Capacities::GetCapacityCount() const {
	return (int)_caps.size();
}

ref<Capacity> Capacities::GetCapacityByID(CapacityIdentifier id) {
	ThreadLock lock(&_lock);
	std::vector< ref<Capacity> >::iterator it = _caps.begin();

	while(it!=_caps.end()) {
		ref<Capacity> c = *it;
		if(c->GetID()==id) return c;
		++it;
	}

	return 0;
}

ref<Capacity> Capacities::GetCapacityByName(const std::wstring& id) {
	ThreadLock lock(&_lock);
	std::vector< ref<Capacity> >::iterator it = _caps.begin();

	while(it!=_caps.end()) {
		ref<Capacity> c = *it;
		if(c->GetName()==id) return c;
		++it;
	}

	return 0;
}

void Capacities::Clear() {
	_caps.clear();
}

ref<Crumb> Capacities::CreateCapacityCrumb(ref<Capacity> cap) {
	ref<BasicCrumb> c = GC::Hold(new BasicCrumb(cap->GetName(), L"icons/capacity.png", cap));
	
	/* TODO: create a special CapacityCrumb */
	return c;
}

ref<Capacity> Capacities::GetCapacityByIndex(int idx) {
	if(idx < int(_caps.size())) {
		return _caps.at(idx);
	}
	return 0;
}