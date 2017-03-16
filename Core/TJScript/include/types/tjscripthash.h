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
 
 #ifndef _TJSCRIPTHASH_H
#define _TJSCRIPTHASH_H

#pragma warning(push)
#pragma warning(disable: 4251 4275)

namespace tj {
	namespace script {
		class ScriptHashType: public ScriptType {
			public:
				virtual tj::shared::ref<Scriptable> Construct(tj::shared::ref<ParameterList> p);
				virtual ~ScriptHashType();
		};

		class SCRIPT_EXPORTED ScriptHash: public ScriptObject<ScriptHash> {
			public:	
				ScriptHash(std::wstring x);
				ScriptHash(int h);
				virtual ~ScriptHash();
				int GetHash() const;

				static void Initialize();
			protected:
				virtual ref<Scriptable> ToString(ref<ParameterList> p);
				virtual ref<Scriptable> ToInt(ref<ParameterList> p);
				int _hash;
		};
	}
}

#pragma warning(pop)

#endif