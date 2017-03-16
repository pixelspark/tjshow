/* This file is part of TJShow. TJShow is free software: you 
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
 
 #include "../include/internal/tjscript.h"
#include <limits>
using namespace tj::shared;
using namespace tj::script;

ScriptContext::ScriptContext(ref<Scriptable> global) {
	_vm = GC::Hold(new VM());
	_global = GC::Hold(new ScriptScope());
	_global->SetPrevious(global);
	_optimize = true;
}

ScriptContext::~ScriptContext() {
}

void ScriptContext::SetDispatcher(ref<Dispatcher> d) {
	_dispatcher = d;
}

strong<tj::shared::Dispatcher> ScriptContext::GetDispatcher() {
	if(_dispatcher) {
		return _dispatcher;
	}
	return Dispatcher::CurrentOrDefaultInstance();
}

Any ScriptContext::GetValue(ref<Scriptable> s) {
	if(!s) return Any();
	
	if(s.IsCastableTo<ScriptAny>()) {
		return ref<ScriptAny>(s)->Unbox();
	}
	else {
		ref<Scriptable> stringRepresentation = s->Execute(L"toString", null);
		if(stringRepresentation.IsCastableTo<ScriptAny>()) {
			return ref<ScriptAny>(stringRepresentation)->Unbox();
		}
	}
	return Any();
}

void ScriptContext::SetOptimize(bool o) {
	_optimize = o;
}

ref<Scriptable> ScriptContext::GetGlobal() {
	return _global;
}

ref<Scriptable> ScriptContext::Execute(ref<CompiledScript> scr, ref<ScriptScope> scope) {
	ThreadLock lock(&_running);
	assert(scr);

	if(scr->_creatingContext!=0 && scr->_creatingContext!=this) {
		throw ScriptException(L"Script cannot be executed, because it was not created by this context");
	}

	if(scope) {
		// Get the scope at the very end of the chain and set the previous scope of that scope to _global
		ref<ScriptScope> last = scope;
		while(true) {
			ref<ScriptScope> sc = last->GetPrevious();
			if(sc) {
				last = sc;
			}
			else {
				break;
			}
		}
		
		last->SetPrevious(_global);
		ref<Scriptable> val = _vm->Execute(this, scr, scope);
		last->SetPrevious(null);
		return val;
	}
	else {
		return _vm->Execute(this, scr, _global);
	}
}

ref<ScriptThread> ScriptContext::CreateExecutionThread(ref<CompiledScript> scr) {
	if(scr->_creatingContext!=this) {
		throw ScriptException(L"Cannot create execution thread for this script, because it was not created by this context");
	}
	ref<ScriptThread> thread = GC::Hold(new ScriptThread(this));
	thread->SetScript(scr);
	return thread;
}

void ScriptContext::SetDebug(bool d) {
	_vm->SetDebug(d);
}

void ScriptContext::AddType(const std::wstring& type, ref<ScriptType> stype) {
	_types[type] = stype;
}

ref<ScriptType> ScriptContext::GetType(const std::wstring& type) {
	if(_types.find(type)!=_types.end()) {
		return _types[type];
	}
	else {
		ref<ScriptType> st = ScriptPackage::DefaultInstance()->GetType(type);
		if(!st) {
			throw ScriptException(std::wstring(L"The type ")+type+L" does not exist.");
		}
		return st;
	}
}
