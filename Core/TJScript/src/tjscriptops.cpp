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
#include "../include/types/tjstatictypes.h"
#include <limits>
using namespace tj::script;
using namespace tj::shared;

void OpNopHandler(VM* vm) {
}

void OpPushStringHandler(VM* vm) {
	StackFrame* sf = vm->GetStackFrame();
	//std::wstring value(sf->_scriptlet->Get<wchar_t*>(sf->_pc));
	LiteralIdentifier id = sf->_scriptlet->Get<LiteralIdentifier>(sf->_pc);
	vm->GetStack().Push(sf->_scriptlet->GetLiteral(id));
}

void OpPushDoubleHandler(VM* vm) {
	StackFrame* sf = vm->GetStackFrame();
	LiteralIdentifier id = sf->_scriptlet->Get<LiteralIdentifier>(sf->_pc);
	vm->GetStack().Push(sf->_scriptlet->GetLiteral(id));
}

void OpPushDelegateHandler(VM* vm) {
	StackFrame* sf = vm->GetStackFrame();
	LiteralIdentifier id = sf->_scriptlet->Get<LiteralIdentifier>(sf->_pc);
	vm->GetStack().Push(sf->_scriptlet->GetLiteral(id));
}

void OpPushTrueHandler(VM* vm) {
	vm->GetStack().Push(ScriptConstants::True);
}

void OpPushFalseHandler(VM* vm) {
	vm->GetStack().Push(ScriptConstants::False);
}

void OpPushIntHandler(VM* vm) {
	StackFrame* sf = vm->GetStackFrame();
	LiteralIdentifier id = sf->_scriptlet->Get<LiteralIdentifier>(sf->_pc);
	vm->GetStack().Push(sf->_scriptlet->GetLiteral(id));
}

void OpPushNullHandler(VM* vm) {
	vm->GetStack().Push(ScriptConstants::Null);
}

void OpPopHandler(VM* vm) {
	vm->GetStack().Pop();
}

void OpCallHandler(VM* vm) {
	ScriptStack& stack = vm->GetStack();
	ref<Scriptable> parameterList = stack.Pop();
	ref<ScriptParameterList> list;
	if(parameterList.IsCastableTo<ScriptParameterList>()) {
		list = parameterList;
	}
	else {
		stack.Push(parameterList);
	}

	ref< ScriptValue<std::wstring> > funcName = stack.Pop();
	ref<Scriptable> target = stack.Pop();

	ref<Scriptable> result = target->Execute(funcName->GetValue(), list);
	if(!result) {
		throw ScriptException(L"Variable does not exist on object or scope: '"+funcName->GetValue()+L"'");
	}

	// only call functions when there is a parameter list given
	if(result.IsCastableTo<ScriptFunction>() && list) {
		ref<Scriptlet> scriptlet = ref<ScriptFunction>(result)->_scriptlet;
		vm->Call(scriptlet, list);
	}
	else {
		stack.Push(result);
	}
}

void OpTypeHandler(VM* vm) {
	ScriptStack& stack = vm->GetStack();
	ref<ScriptString> typeName = stack.Pop();
	stack.Push(vm->GetContext()->GetType(typeName->GetValue()));
}

void OpCallGlobalHandler(VM* vm) {
	ScriptStack& stack = vm->GetStack();
	ref<Scriptable> parameterList = stack.Pop();
	ref<ScriptParameterList> list;
	if(parameterList.IsCastableTo<ScriptParameterList>()) {
		list = parameterList;
	}
	else {
		stack.Push(parameterList);
	}

	ref< ScriptValue<std::wstring> > funcName = stack.Pop();
	ref<Scriptable> target = vm->GetCurrentScope();

	ref<Scriptable> result = target->Execute(funcName->GetValue(), list);
	if(!result) {
		throw ScriptException(L"Variable does not exist on object or scope: '"+funcName->GetValue()+L"'");
	}

	// only call functions when there is a parameter list given
	if(result.IsCastableTo<ScriptFunction>() && list) {
		ref<Scriptlet> scriptlet = ref<ScriptFunction>(result)->_scriptlet;
		vm->Call(scriptlet, list);
	}
	else {
		stack.Push(result);
	}
}

void OpNewHandler(VM* vm) {
	ScriptStack& stack = vm->GetStack();
	ref<Scriptable> parameterList = stack.Pop();
	ref<ScriptParameterList> list;
	if(parameterList.IsCastableTo<ScriptParameterList>()) {
		list = parameterList;
	}
	else {
		stack.Push(parameterList);
	}

	ref< ScriptString > funcName = stack.Pop();
	
	ref<Scriptable> instance = vm->GetContext()->GetType(funcName->GetValue())->Construct(list);
	stack.Push(instance);
}

void OpSaveHandler(VM* vm) {
	ScriptStack& stack = vm->GetStack();
	ref<Scriptable> object = stack.Pop();
	ref< ScriptValue<std::wstring> > varName = stack.Pop();
	vm->GetCurrentScopeForWriting()->Set(varName->GetValue(), object);
}

void OpEqualsHandler(VM* vm) {
	ScriptStack& stack = vm->GetStack();
	ref<Scriptable> a = stack.Pop();
	ref<Scriptable> b = stack.Pop();
	
	if(a.IsCastableTo<ScriptAny>() && b.IsCastableTo<ScriptAny>()) {
		Any aa = ref<ScriptAny>(a)->Unbox();
		Any ab = ref<ScriptAny>(b)->Unbox();
		stack.Push((aa==ab) ? ScriptConstants::True : ScriptConstants::False);
	}
	else {
		stack.Push(ScriptConstants::Null);
	}
}

void OpNegateHandler(VM* vm) {
	ScriptStack& stack = vm->GetStack();
	ref<Scriptable> a = stack.Pop();
	
	if(a.IsCastableTo<ScriptAny>()) {
		Any aa = ref<ScriptAny>(a)->Unbox();
		strong<ScriptAnyValue> sav = Recycler<ScriptAnyValue>::Create();
		sav->SetValue(-aa);
		stack.Push(sav);
	}
	else {
		stack.Push(ScriptConstants::Null);
	}
}

void OpAddHandler(VM* vm) {
	ScriptStack& stack = vm->GetStack();
	ref<Scriptable> a = stack.Pop();
	ref<Scriptable> b = stack.Pop();

	if(a.IsCastableTo<ScriptAny>() && b.IsCastableTo<ScriptAny>()) {
		Any aa = ref<ScriptAny>(a)->Unbox();
		Any ab = ref<ScriptAny>(b)->Unbox();
		strong<ScriptAnyValue> sav = Recycler<ScriptAnyValue>::Create();
		sav->SetValue(ab+aa);
		stack.Push(sav);
	}
	else {
		stack.Push(ScriptConstants::Null);
	}
}

void OpSubHandler(VM* vm) {
	ScriptStack& stack = vm->GetStack();
	ref<Scriptable> a = stack.Pop();
	ref<Scriptable> b = stack.Pop();
	
	if(a.IsCastableTo<ScriptAny>() && b.IsCastableTo<ScriptAny>()) {
		Any aa = ref<ScriptAny>(a)->Unbox();
		Any ab = ref<ScriptAny>(b)->Unbox();
		strong<ScriptAnyValue> sav = Recycler<ScriptAnyValue>::Create();
		sav->SetValue(ab-aa);
		stack.Push(sav);
	}
	else {
		stack.Push(ScriptConstants::Null);
	}
}

void OpMulHandler(VM* vm) {
	ScriptStack& stack = vm->GetStack();
	ref<Scriptable> a = stack.Pop();
	ref<Scriptable> b = stack.Pop();
	
	if(a.IsCastableTo<ScriptAny>() && b.IsCastableTo<ScriptAny>()) {
		Any aa = ref<ScriptAny>(a)->Unbox();
		Any ab = ref<ScriptAny>(b)->Unbox();
		strong<ScriptAnyValue> sav = Recycler<ScriptAnyValue>::Create();
		sav->SetValue(aa*ab);
		stack.Push(sav);
	}
	else {
		stack.Push(ScriptConstants::Null);
	}
}

void OpDivHandler(VM* vm) {
	ScriptStack& stack = vm->GetStack();
	ref<Scriptable> a = stack.Pop();
	ref<Scriptable> b = stack.Pop();
	
	if(a.IsCastableTo<ScriptAny>() && b.IsCastableTo<ScriptAny>()) {
		Any aa = ref<ScriptAny>(a)->Unbox();
		Any ab = ref<ScriptAny>(b)->Unbox();
		strong<ScriptAnyValue> sav = Recycler<ScriptAnyValue>::Create();
		sav->SetValue(ab/aa);
		stack.Push(sav);
	}
	else {
		stack.Push(ScriptConstants::Null);
	}
}

void OpAndHandler(VM* vm) {
	ref<Scriptable> a = vm->GetStack().Pop();
	ref<Scriptable> b = vm->GetStack().Pop();

	bool ba = ScriptContext::GetValue<bool>(a, false);
	bool bb = ScriptContext::GetValue<bool>(b, false);

	vm->GetStack().Push((ba&&bb) ? ScriptConstants::True : ScriptConstants::False);
}

void OpOrHandler(VM* vm) {
	ref<Scriptable> a = vm->GetStack().Pop();
	ref<Scriptable> b = vm->GetStack().Pop();

	bool ba = ScriptContext::GetValue<bool>(a, false);
	bool bb = ScriptContext::GetValue<bool>(b, false);

	vm->GetStack().Push((ba||bb) ? ScriptConstants::True : ScriptConstants::False);
}

void OpBranchIfHandler(VM* vm) {
	ScriptStack& stack = vm->GetStack();
	StackFrame* frame = vm->GetStackFrame();
	int  scriptlet = frame->_scriptlet->Get<int>(frame->_pc);
	ref<Scriptable> top = stack.Top();
	
	bool r = ScriptContext::GetValue<bool>(top, false);
	if(r) {
		vm->Call(scriptlet);
	}
}

void OpParameterHandler(VM* vm) {
	ScriptStack& stack = vm->GetStack();
	ref<Scriptable> value = stack.Pop();
	ref<Scriptable> key = stack.Pop();
	ref<ScriptParameterList> parameter = stack.Pop();

	ref< ScriptValue<std::wstring> > keyString = key;

	parameter->Set(keyString->GetValue(), value);
	stack.Push(parameter);
}

void OpNamelessParameterHandler(VM* vm) {
	ScriptStack& stack = vm->GetStack();
	ref<Scriptable> value = stack.Pop();
	ref<ScriptParameterList> parameter = stack.Pop();
	parameter->AddNamelessParameter(value);
	stack.Push(parameter);
}

void OpLoadScriptletHandler(VM* vm) {
	StackFrame* frame = vm->GetStackFrame();
	int id = frame->_scriptlet->Get<int>(frame->_pc);

	ref<Scriptlet> sc = vm->GetScript()->GetScriptlet(id);
	vm->GetStack().Push(GC::Hold(new ScriptFunction(sc)));
}

void OpReturnHandler(VM* vm) {
	vm->Return(false);
}

void OpReturnValueHandler(VM* vm) {
	vm->Return(true);
}

void OpEndScriptletHandler(VM* vm) {
	vm->ReturnFromScriptlet();
}

void OpGreaterThanHandler(VM* vm) {
	ScriptStack& stack = vm->GetStack();
	ref<Scriptable> a = stack.Pop();
	ref<Scriptable> b = stack.Pop();
	
	if(a.IsCastableTo<ScriptAny>() && b.IsCastableTo<ScriptAny>()) {
		Any aa = ref<ScriptAny>(a)->Unbox();
		Any ab = ref<ScriptAny>(b)->Unbox();
		
		bool result = ab > aa;
		stack.Push(result ? ScriptConstants::True : ScriptConstants::False);
	}
	else {
		stack.Push(ScriptConstants::Null);
	}
}

void OpLessThanHandler(VM* vm) {
	ScriptStack& stack = vm->GetStack();
	ref<Scriptable> a = stack.Pop();
	ref<Scriptable> b = stack.Pop();
	
	if(a.IsCastableTo<ScriptAny>() && b.IsCastableTo<ScriptAny>()) {
		Any aa = ref<ScriptAny>(a)->Unbox();
		Any ab = ref<ScriptAny>(b)->Unbox();
		
		bool result = ab < aa;
		stack.Push(result ? ScriptConstants::True : ScriptConstants::False);
	}
	else {
		stack.Push(ScriptConstants::Null);
	}
}

void OpXorHandler(VM* vm) {
	ref<Scriptable> a = vm->GetStack().Pop();
	ref<Scriptable> b = vm->GetStack().Pop();

	bool ba = ScriptContext::GetValue<bool>(a, false);
	bool bb = ScriptContext::GetValue<bool>(b, false);
	bool result = ((ba||bb) && !(ba==bb));
	vm->GetStack().Push(result ? ScriptConstants::True : ScriptConstants::False);
}

void OpBreakHandler(VM* vm) {
	vm->Break();
}

void OpIndexHandler(VM* vm) {
	ScriptStack& stack = vm->GetStack();
	ref<Scriptable> index = stack.Pop();
	ref<Scriptable> object = stack.Pop();

	ref<ParameterList> pl = GC::Hold(new ParameterList());
	pl->Set(L"key", index);
	ref<Scriptable> result = object->Execute(L"get", pl);
	if(result==0) {
		throw ScriptException(L"Object does not support get(key=...) method, array index cannot be used");
	}
	stack.Push(result);
}

void OpIterateHandler(VM* vm) {
	ref<Scriptable> iterable = vm->GetStack().Pop();
	ref<Scriptable> varName = vm->GetStack().Pop();
	vm->GetStack().Push(varName);
	vm->GetStack().Push(iterable);

	StackFrame* frame = vm->GetStackFrame();
	int mypc = frame->_pc; // previous instruction
	int scriptlet = frame->_scriptlet->Get<int>(frame->_pc);
	
	ref<Scriptable> value = iterable->Execute(L"next", 0);
	if(value==0) {
		throw ScriptException(L"Object does not support iteration");
	}
	else if(!value.IsCastableTo<ScriptNull>()) {
		// set pc of this stack frame one back, so this instruction will be called again
		StackFrame* frame = vm->GetStackFrame();
		frame->_pc = mypc-(sizeof(int)/sizeof(char)); // move back one instruction, so that this instruction will be called again when the scriptlet returns

		//set variable
		vm->GetCurrentScopeForWriting()->Set(ref<ScriptString>(varName)->GetValue(), value);
		vm->Call(scriptlet);
	}
	else {
		vm->GetStack().Pop();
		vm->GetStack().Pop();
	}
}

void OpPushParameterHandler(VM* vm) {
	vm->GetStack().Push(GC::Hold(new ScriptParameterList()));
}

void OpSetFieldHandler(VM* vm) {
	ScriptStack& st = vm->GetStack();
	ref<Scriptable> value = st.Pop();
	ref<Scriptable> ident = st.Pop();
	ref<Scriptable> target = st.Pop();

	ref<ScriptString> identString = ident;
	if(!identString) {
		throw ScriptException(L"Identifier is not a string; cannot set field!");
	}

	if(!target->Set(identString->GetValue(), value)) {
		throw ScriptException(L"Object is not mutable; cannot set field "+identString->GetValue()+L"!");
	}

	st.Push(value);
}

void OpPushArrayHandler(VM* vm) {
	ScriptStack& st = vm->GetStack();
	st.Push(GC::Hold(new ScriptArray()));
}

void OpAddToArrayHandler(VM* vm) {
	ScriptStack& st = vm->GetStack();
	ref<Scriptable> data = st.Pop();
	ref<ScriptArray> lst = st.Top();
	if(lst) {
		lst->Push(data);
	}
}

const wchar_t* Ops::Names[Ops::_OpLast] = {L"OpNop", L"OpPushString", L"OpPushDouble", L"OpPushTrue", L"OpPushFalse", L"OpPushInt", L"OpPushNull", L"OpPop", 
L"OpCall", L"OpCallGlobal", L"OpNew", L"OpSave", L"OpEquals", L"OpNegate", L"OpAdd", L"OpSub", 
L"OpMul", L"OpDiv", L"OpAnd",L"OpOr", L"OpBranchIf", L"OpParameter", L"OpNamelessParameter", L"OpPushParameter",
L"OpLoadScriptlet", L"OpReturn", L"OpReturnValue", L"OpGreaterThan", L"OpLessThan", L"OpXor",
L"OpBreak", L"OpIndex", L"OpIterate", L"OpPushDelegate", L"OpSetField", L"OpAddToArray", L"OpPushArray", L"OpType", L"OpEndScriptlet" };

Ops::OpHandler Ops::Handlers[Ops::_OpLast] = {OpNopHandler,OpPushStringHandler,OpPushDoubleHandler,
OpPushTrueHandler, OpPushFalseHandler,OpPushIntHandler, OpPushNullHandler, OpPopHandler,OpCallHandler,OpCallGlobalHandler,OpNewHandler,
OpSaveHandler,OpEqualsHandler,OpNegateHandler,OpAddHandler,OpSubHandler,
OpMulHandler,OpDivHandler,OpAndHandler,OpOrHandler,OpBranchIfHandler,
OpParameterHandler,OpNamelessParameterHandler,OpPushParameterHandler, OpLoadScriptletHandler, OpReturnHandler,
OpReturnValueHandler,OpGreaterThanHandler,OpLessThanHandler,OpXorHandler,OpBreakHandler
,OpIndexHandler,OpIterateHandler, OpPushDelegateHandler, OpSetFieldHandler, OpAddToArrayHandler, OpPushArrayHandler, OpTypeHandler, OpEndScriptletHandler };

