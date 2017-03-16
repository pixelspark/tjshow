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
using namespace tj::script;
using namespace tj::shared;

ScriptParameterList::ScriptParameterList() {
	_namelessCount = 0;
}

ScriptParameterList::~ScriptParameterList() {
}

void ScriptParameterList::AddNamelessParameter(ref<Scriptable> t) {
	std::wstring parameterName = Stringify(_namelessCount);
	Set(parameterName, t);
	_namelessCount++;
}
