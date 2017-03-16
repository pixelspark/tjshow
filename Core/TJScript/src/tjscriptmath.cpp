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
#include <math.h>
#include <typeinfo>
using namespace tj::script;
using namespace tj::shared;

const static double PI = 3.14159265358979323846;
const static double EULER = 2.7182818284590452354;

ref<Scriptable> ScriptMathType::Construct(ref<ParameterList> p) {
	return 0;
}

ScriptMathType::~ScriptMathType() {
}

ref<Scriptable> ScriptMathType::Execute(Command c, ref<ParameterList> p) {
	if(c==L"class") {
		return tj::shared::GC::Hold(new ScriptString(Wcs(typeid(ScriptMathType).name())));	
	}

	if(!_instance) {
		_instance = GC::Hold(new ScriptMath());
	}

	return _instance->Execute(c,p);
}

ScriptMath::~ScriptMath() {
}

ScriptMath::ScriptMath() {
}

void ScriptMath::Initialize() {
	Bind(L"sin", &ScriptMath::Sin);
	Bind(L"cos", &ScriptMath::Cos);
	Bind(L"tan", &ScriptMath::Tan);
	Bind(L"atan", &ScriptMath::Atan);
	Bind(L"atan2", &ScriptMath::Atan2);
	Bind(L"acos", &ScriptMath::Acos);
	Bind(L"asin", &ScriptMath::Asin);
	Bind(L"random", &ScriptMath::Random);
	Bind(L"pi", &ScriptMath::Pi);
	Bind(L"pow", &ScriptMath::Pow);
	Bind(L"fmod", &ScriptMath::Fmod);
	Bind(L"e", &ScriptMath::E);
}

ref<Scriptable> ScriptMath::Sin(ref<ParameterList> p) {
	static const Parameter<double> PAngle(L"angle", 0);
	return GC::Hold(new ScriptDouble(sin(PAngle.Require(p,0.0))));
}

ref<Scriptable> ScriptMath::Cos(ref<ParameterList> p) {
	static const Parameter<double> PAngle(L"angle", 0);
	return GC::Hold(new ScriptDouble(cos(PAngle.Require(p,0.0))));
}

ref<Scriptable> ScriptMath::Tan(ref<ParameterList> p) {
	static const Parameter<double> PAngle(L"angle", 0);
	return GC::Hold(new ScriptDouble(tan(PAngle.Require(p,0.0))));
}

ref<Scriptable> ScriptMath::Atan2(ref<ParameterList> p) {
	static const Parameter<double> PX(L"x", 0);
	static const Parameter<double> PY(L"y", 1);

	return GC::Hold(new ScriptDouble(atan2(PY.Require(p,0.0),PX.Require(p,0.0))));
}

ref<Scriptable> ScriptMath::Atan(ref<ParameterList> p) {
	static const Parameter<double> PAngle(L"angle", 0);
	return GC::Hold(new ScriptDouble(atan(PAngle.Require(p, 0.0))));
}

ref<Scriptable> ScriptMath::Random(ref<ParameterList> p) {
	static const Parameter<int> PFrom(L"from", 0);
	static const Parameter<int> PTo(L"to", 1);

	int to = PTo.Require(p,0);
	int from = PFrom.Require(p,0);

	if(to-from<=0) {
		throw ScriptException(L"Invalid argument values for from and to");
	}

	int r = (rand()%(to-from+1))+from;
	return GC::Hold(new ScriptInt(r));
}

ref<Scriptable> ScriptMath::Acos(ref<ParameterList> p) {
	static const Parameter<double> PAngle(L"angle", 0);
	return GC::Hold(new ScriptDouble(acos(PAngle.Require(p,0.0))));
}

ref<Scriptable> ScriptMath::Asin(ref<ParameterList> p) {
	static const Parameter<double> PAngle(L"angle", 0);
	return GC::Hold(new ScriptDouble(asin(PAngle.Require(p,0.0))));
}

ref<Scriptable> ScriptMath::Fmod(ref<ParameterList> p) {
	static const Parameter<double> PA(L"a", 0);
	static const Parameter<double> PB(L"b", 1);

	return GC::Hold(new ScriptDouble(fmod(PA.Require(p,0.0),PB.Require(p,0.0))));
}

ref<Scriptable> ScriptMath::Pow(ref<ParameterList> p) {
	static const Parameter<double> PA(L"a", 0);
	static const Parameter<double> PB(L"b", 1);

	return GC::Hold(new ScriptDouble(pow(PA.Require(p,0.0), PB.Require(p,0.0))));
}

ref<Scriptable> ScriptMath::Pi(ref<ParameterList> p) {
	return GC::Hold(new ScriptDouble(PI));
}

ref<Scriptable> ScriptMath::E(ref<ParameterList> p) {
	return GC::Hold(new ScriptDouble(EULER));
}