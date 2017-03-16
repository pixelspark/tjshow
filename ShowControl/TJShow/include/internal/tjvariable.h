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
#ifndef _TJVARIABLE_H
#define _TJVARIABLE_H

namespace tj {
	namespace show {
		class Variable: public tj::script::ScriptObject<Variable>, public Serializable, public Inspectable {
			friend class VariableList;

			public:	
				Variable();
				virtual ~Variable();
				virtual void Save(TiXmlElement* p);
				virtual void Load(TiXmlElement* you);
				virtual ref<PropertySet> GetProperties();
				virtual const std::wstring& GetName() const;
				virtual Any GetInitialValue() const;
				virtual Any GetValue() const;
				virtual const std::wstring& GetDescription() const;
				virtual void SetName(const std::wstring& n);
				virtual bool IsInput() const;
				virtual void SetIsInput(bool t);
				virtual bool IsOutput() const;
				virtual void SetIsOutput(bool t);
				virtual void Clone();
				virtual strong<Variable> CreateInstanceClone();
				virtual const std::wstring& GetID() const;
				virtual Any::Type GetType() const;
				virtual void SetInitialValue(const std::wstring& initial);			
				static void Initialize();

			protected:
				// Other classes should call this through Variables, so events can be dispatched
				virtual void Reset();
				virtual void SetValue(const Any& a);
				virtual ref<Scriptable> SGetName(ref<ParameterList> p);
				virtual ref<Scriptable> SGetID(ref<ParameterList> p);
				virtual ref<Scriptable> SGetValue(ref<ParameterList> p);
				virtual ref<Scriptable> SGetInitialValue(ref<ParameterList> p);
				virtual ref<Scriptable> SGetDescription(ref<ParameterList> p);

				std::wstring _name;
				std::wstring _desc;
				std::wstring _id;
				std::wstring _initial;
				Any::Type _type;
				Any _value;
				bool _isInput;
				bool _isOutput;
		};

		class Variables: public virtual Object {
			friend class ScopedVariables; // for access to _lock

			public:
				virtual ~Variables();
				virtual unsigned int GetVariableCount() const = 0;
				virtual ref<Variable> GetVariableByIndex(unsigned int idx) = 0;
				virtual ref<Variable> GetVariableById(const std::wstring& id) = 0;
				virtual ref<Variable> GetVariableByName(const std::wstring& name) = 0;
				virtual ref<Property> CreateProperty(const std::wstring& name, ref<Inspectable> holder, std::wstring* id, const std::wstring& def) = 0;
				virtual ref<Variable> DoChoosePopup(Pixels x, Pixels y, ref<Wnd> owner) = 0;
				virtual void Reset(ref<Variable> v) = 0;
				virtual void Set(ref<Variable> v, const Any& val) = 0;
				virtual void Remove(ref<Variable> b) = 0;
				virtual void Add(ref<Variable> v) = 0;
				virtual bool Exists(ref<Variable> v) const = 0; // returns true if the variable is in this list
				virtual bool Evaluate(ref<Expression> c, ref<Waiting> w) = 0;
				virtual void Assign(ref<Assignments> s, ref<Variables> scope = null) = 0;

				static std::wstring ParseVariables(ref<Variables> vars, const std::wstring& source);

				struct ChangedVariables {
					std::set< strong<Variable> > which;
				};
				Listenable<ChangedVariables> EventVariablesChanged;

			protected:
				virtual void OnVariablesChanged(const ChangedVariables& which);

				struct WaitingInfo {
					ref<Waiting> _waiting;
					ref<Expression> _condition;
				};

				std::list< WaitingInfo > _waiting;
				mutable CriticalSection _lock;
		};

		class VariableList: public tj::script::ScriptObject<VariableList>, public Serializable, public Variables {
			public:
				VariableList();
				virtual ~VariableList();
				virtual void Clear();
				virtual void Save(TiXmlElement* p);
				virtual void Load(TiXmlElement* p);
				virtual unsigned int GetVariableCount() const;
				virtual ref<Variable> GetVariableByIndex(unsigned int idx);
				virtual ref<Variable> GetVariableById(const std::wstring& id);
				virtual ref<Variable> GetVariableByName(const std::wstring& name);
				virtual void Add(ref<Variable> v);
				virtual void Remove(ref<Variable> b);
				virtual ref<Property> CreateProperty(const std::wstring& name, ref<Inspectable> holder, std::wstring* id, const std::wstring& def);
				virtual bool Evaluate(ref<Expression> c, ref<Waiting> w);
				virtual void Reset(ref<Variable> v);
				virtual void Set(ref<Variable> v, const Any& val);
				virtual void Assign(ref<Assignments> s, ref<Variables> scope = null);
				virtual ref<Variable> DoChoosePopup(Pixels x, Pixels y, ref<Wnd> owner);
				virtual void ResetAll();
				static void Initialize();
				virtual bool Exists(ref<Variable> v) const;
				virtual void Clone();
				virtual strong<VariableList> CreateInstanceClone();

			protected:
				virtual ref<Scriptable> SSet(ref<ParameterList> p);
				virtual ref<Scriptable> SGetCount(ref<ParameterList> p);
				virtual ref<Scriptable> SGet(ref<ParameterList> p);
				virtual bool Set(Field f, ref<Scriptable> s);
				std::vector< ref<Variable> > _vars;

		};

		class ScopedVariables: public Variables, public Listener<Variables::ChangedVariables> {
			public:
				ScopedVariables(strong<Variables> local, strong<Variables> global);
				virtual ~ScopedVariables();
				virtual unsigned int GetVariableCount() const;
				virtual ref<Variable> GetVariableByIndex(unsigned int idx);
				virtual ref<Variable> GetVariableById(const std::wstring& id);
				virtual ref<Variable> GetVariableByName(const std::wstring& name);
				virtual ref<Property> CreateProperty(const std::wstring& name, ref<Inspectable> holder, std::wstring* id, const std::wstring& def);
				virtual ref<Variable> DoChoosePopup(Pixels x, Pixels y, ref<Wnd> owner);
				virtual void Reset(ref<Variable> v);
				virtual void Set(ref<Variable> v, const Any& val);
				virtual void Remove(ref<Variable> b);
				virtual void Add(ref<Variable> v);
				virtual bool Exists(ref<Variable> v) const;
				virtual bool Evaluate(ref<Expression> c, ref<Waiting> w);
				virtual void Assign(ref<Assignments> s, ref<Variables> scope = null);
				virtual void OnCreated();
				virtual void Notify(ref<Object> source, const Variables::ChangedVariables& data);

			protected:
				strong<Variables> _global, _local;
		};

		class VariableEndpointCategory: public EndpointCategory {
			public:
				VariableEndpointCategory();
				virtual ~VariableEndpointCategory();
				virtual ref<Endpoint> GetEndpointById(const EndpointID& id);
				
				const static EndpointCategoryID KVariableEndpointCategoryID;
		};
	}	
}

#endif