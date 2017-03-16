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
#include "../include/types/tjscriptarray.h"
#include <algorithm>
using namespace tj::script;
using namespace tj::shared;

ScriptNull::ScriptNull() {
}

ScriptNull::~ScriptNull() {
}

ref<Scriptable> ScriptNull::Execute(Command c, ref<ParameterList> p) {
	if(c==L"toString") {
		return GC::Hold(new ScriptString(L"null"));
	}
	return 0;
}

ScriptAny::~ScriptAny() {
}

ref<Scriptable> ScriptAny::Execute(Command c, ref<ParameterList> p) {
	if(c==L"toString") {
		return GC::Hold(new ScriptString(Unbox()));
	}
	else if(c==L"toInteger") {
		return GC::Hold(new ScriptInt(Unbox()));
	}
	else if(c==L"toDouble") {
		return GC::Hold(new ScriptDouble(Unbox()));
	}
	else if(c==L"toBool") {
		return GC::Hold(new ScriptBool(Unbox()));
	}
	else if(c==L"toHexString") {
		int val = Unbox();
		std::wostringstream wos;
		wos << std::hex << std::uppercase << val;
		return GC::Hold(new ScriptString(wos.str()));
	}
	else if(c==L"toUpper") {
		std::wstring val = Unbox();
		std::transform(val.begin(), val.end(), val.begin(), toupper);
		return GC::Hold(new ScriptString(val));
	}
	else if(c==L"toLower") {
		std::wstring val = Unbox();
		std::transform(val.begin(), val.end(), val.begin(), tolower);
		return GC::Hold(new ScriptString(val));
	}
	else if(c==L"charAt") {
		static const Parameter<int> PAt(L"index", 0);
		int index = PAt.Require(p,0);
		String value = Unbox();
		if(index<0) {
			index = value.length()-index;
		}

		if(index>=int(value.length())) {
			return ScriptConstants::Null;
		}
		
		std::wostringstream wos;
		wos << value.at(index);
		return GC::Hold(new ScriptString(wos.str()));
	}
	else if(c==L"replaceAll") {
		static const Parameter<String> PFragment(L"fragment", 0);
		static const Parameter<String> PWith(L"with", 1);

		String fragment = PFragment.Require(p, L"");
		String with = PWith.Require(p, L"");
		String value = Unbox();
		ReplaceAll<String>(value, fragment, with);
		return GC::Hold(new ScriptString(value));
	}
	else if(c==L"explode") {
		static const Parameter<std::wstring> PSeparator(L"separator", 0);
		std::wstring separator = PSeparator.Require(p, L"");
		
		ref<ScriptArray> result = GC::Hold(new ScriptArray());
		std::wstring::size_type start = 0;
		std::wstring::size_type end = 0;
		std::wstring value = Unbox();
		
		while((end = value.find(separator, start)) != std::wstring::npos) {
			result->Push(GC::Hold(new ScriptString(value.substr(start, end-start))));
			start = end + separator.size();
		}
		result->Push(GC::Hold(new ScriptString(value.substr(start))));
		return result;
	}
	return null;
}
