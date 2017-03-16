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
#include <TJDB/include/tjdb.h>
using namespace tj::show;
using namespace tj::shared;
using namespace tj::db;

namespace tj {
	namespace show {
		namespace script {
			class ScriptDatabaseType: public ScriptType {
				public:
					virtual tj::shared::ref<Scriptable> Construct(tj::shared::ref<ParameterList> p);
					virtual ~ScriptDatabaseType();
			};

			class ScriptDatabase: public ScriptObject<ScriptDatabase> {
				public:
					ScriptDatabase(ref<ParameterList> p);
					ScriptDatabase(ref<Database> db);
					virtual ~ScriptDatabase();
					static void Initialize();

					virtual ref<Scriptable> SGetVersion(ref<ParameterList> p);
					virtual ref<Scriptable> SGetName(ref<ParameterList> p);
					virtual ref<Scriptable> SQuery(ref<ParameterList> p);

				protected:
					ref<Database> _db;
			};

			class ScriptQuery: public ScriptObject<ScriptQuery> {
				public:
					ScriptQuery(ref<Query> q);
					virtual ~ScriptQuery();
					static void Initialize();

					virtual ref<Scriptable> SBind(ref<ParameterList> p);
					virtual ref<Scriptable> SGet(ref<ParameterList> p);
					virtual ref<Scriptable> SExecute(ref<ParameterList> p);
					virtual ref<Scriptable> SHasRow(ref<ParameterList> p);
					virtual ref<Scriptable> SNext(ref<ParameterList> p);
					virtual bool Set(Field field, tj::shared::ref<Scriptable> value);

				protected:
					ref<Query> _q;
			};
		}
	}
}

using namespace tj::show::script;

ScriptDatabaseType::~ScriptDatabaseType() {
}

ref<Scriptable> ScriptDatabaseType::Construct(ref<ParameterList> p) {
	return GC::Hold(new ScriptDatabase(p));
}

ScriptDatabase::ScriptDatabase(ref<ParameterList> p) {
	static Parameter<std::wstring> PPath(L"path", 0);

	std::wstring path = PPath.Require(p, L"");
	_db = Database::Open(path);
}

ScriptDatabase::ScriptDatabase(ref<Database> db): _db(db) {
}

ScriptDatabase::~ScriptDatabase() {
}

void ScriptDatabase::Initialize() {
	Bind(L"version", &SGetVersion);
	Bind(L"name", &SGetName);
	Bind(L"query", &SQuery);
}

ref<Scriptable> ScriptDatabase::SGetVersion(ref<ParameterList> p) {
	return GC::Hold(new ScriptString(_db->GetVersion()));
}

ref<Scriptable> ScriptDatabase::SGetName(ref<ParameterList> p) {
	return GC::Hold(new ScriptString(_db->GetName()));
}

ref<Scriptable> ScriptDatabase::SQuery(ref<ParameterList> p) {
	static Parameter<std::wstring> PSQL(L"sql", 0);
	std::wstring sql = PSQL.Require(p, L"");
	return GC::Hold(new ScriptQuery(_db->CreateQuery(sql)));
}

ScriptQuery::ScriptQuery(ref<Query> q): _q(q) {
}

ScriptQuery::~ScriptQuery() {
}

void ScriptQuery::Initialize() {
	Bind(L"bind", &SBind);
	Bind(L"set", &SBind);
	Bind(L"execute", &SExecute);
	Bind(L"next", &SNext);
	Bind(L"hasRow", &SHasRow);
	Bind(L"get", &SGet);
}

bool ScriptQuery::Set(Field field, ref<Scriptable> value) {
	int i = StringTo<int>(field, -1);

	if(i!=-1) {
		// Use parameter indices
		if(value.IsCastableTo<ScriptInt>()) {
			_q->Set(i, ref<ScriptInt>(value)->GetValue());
		}
		else if(value.IsCastableTo<ScriptDouble>()) {
			_q->Set(i, ref<ScriptDouble>(value)->GetValue());
		}
		else if(value.IsCastableTo<ScriptBool>()) {
			_q->Set(i, ref<ScriptBool>(value)->GetValue());
		}
		else {
			_q->Set(i, ScriptContext::GetValue<std::wstring>(value, L""));
		}
	}
	else {
		// Use parameter names
		if(value.IsCastableTo<ScriptInt>()) {
			_q->Set(field, ref<ScriptInt>(value)->GetValue());
		}
		else if(value.IsCastableTo<ScriptDouble>()) {
			_q->Set(field, ref<ScriptDouble>(value)->GetValue());
		}
		else if(value.IsCastableTo<ScriptBool>()) {
			_q->Set(field, ref<ScriptBool>(value)->GetValue());
		}
		else {
			_q->Set(field, ScriptContext::GetValue<std::wstring>(value, L""));
		}
	}

	return true;
}

ref<Scriptable> ScriptQuery::SBind(ref<ParameterList> p) {
	std::map<std::wstring,ref<Scriptable> >::iterator it = p->_vars.begin();
	while(it!=p->_vars.end()) {
		Set(it->first, it->second);
		++it;
	}
	return ScriptConstants::Null;
}

ref<Scriptable> ScriptQuery::SGet(ref<ParameterList> p) {
	static Parameter<std::wstring> PKey(L"key", 0);

	std::wstring k = PKey.Require(p, L"");
	int i = StringTo<int>(k,-1);

	if(i>=0) {
		return GC::Hold(new ScriptString(_q->GetText(i)));
	}
	return ScriptConstants::Null;
}

ref<Scriptable> ScriptQuery::SExecute(ref<ParameterList> p) {
	_q->Execute();
	return ScriptConstants::Null;
}

ref<Scriptable> ScriptQuery::SHasRow(ref<ParameterList> p) {
	return _q->HasRow() ? ScriptConstants::True : ScriptConstants::False;
}

ref<Scriptable> ScriptQuery::SNext(ref<ParameterList> p) {
	_q->Next();
	// Iterator support
	if(_q->HasRow()) {
		return this;
	}
	else {
		return ScriptConstants::Null; // last row
	}
}