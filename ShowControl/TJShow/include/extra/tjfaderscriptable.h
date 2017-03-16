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
#ifndef _TJFADERSCRIPTABLE_H
#define _TJFADERSCRIPTABLE_H

namespace tj {
	namespace show {
		template<typename T> class FaderScriptable: public tj::script::Scriptable {
			public:
				FaderScriptable(ref< tj::show::Fader<T> > fader): _fader(fader) {
				}

				virtual ~FaderScriptable() {
				}

				virtual ref<tj::script::Scriptable> Execute(tj::script::Command c, ref<tj::script::ParameterList> p) {
					static const Parameter<int> PTime(L"time", 0);
					static const Parameter<int> PEndTime(L"end", 1);
					static const Parameter<T> PValue(L"value", 1);

					if(c==L"add") {
						Time time = PTime.Require(p, 0);
						T value = PValue.Require(p, _fader->GetDefaultValue());
						_fader->AddPoint(time,value);
						return tj::script::ScriptConstants::Null;
					}
					else if(c==L"remove") {
						Time time = PTime.Require(p, 0);
						_fader->RemovePoint(time);
						return tj::script::ScriptConstants::Null;
					}
					else if(c==L"removeAll") {
						_fader->RemoveItemsBetween(Time(0), Time(INT_MAX));
						return tj::script::ScriptConstants::Null;
					}
					else if(c==L"removeBetween") {
						Time time = PTime.Require(p, 0);
						Time end = PEndTime.Require(p,0);
						_fader->RemoveItemsBetween(time,end);
						return tj::script::ScriptConstants::Null;
					}
					else if(c==L"defaultValue") {
						return GC::Hold(new tj::script::ScriptValue<T>(_fader->GetDefaultValue()));
					}
					else if(c==L"minimumValue") {
						return GC::Hold(new tj::script::ScriptValue<T>(_fader->GetMinimum()));
					}
					else if(c==L"maximumValue") {
						return GC::Hold(new tj::script::ScriptValue<T>(_fader->GetMaximum()));
					}
					if(c==L"toString") {
						return GC::Hold(new tj::script::ScriptString(L"[FaderScriptable]"));
					}
					return 0;
				}

			protected:
				ref< tj::show::Fader<T> > _fader;
		};
	}
}

#endif