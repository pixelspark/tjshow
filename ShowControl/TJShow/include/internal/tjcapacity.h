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
#ifndef _TJCAPACITY_H
#define _TJCAPACITY_H

namespace tj {
	namespace show {
		class Acquisition;
		typedef std::wstring CapacityIdentifier;

		namespace view {
			class CapacitiesWnd;
			class CapacityListWnd;
		}

		class Waiting {
			public:
				virtual ~Waiting();
				virtual void Acquired(int n) = 0;
				virtual std::wstring GetName();
		};

		class Capacity: public virtual Object, public Inspectable, public Serializable {
			friend class Acquisition;

			public:
				Capacity();
				virtual ~Capacity();

				virtual void Load(TiXmlElement* you);
				virtual void Save(TiXmlElement* parent);
				virtual const CapacityIdentifier& GetID() const;
				virtual const std::wstring& GetName() const;
				virtual const std::wstring& GetDescription() const;
				virtual int GetValue() const;
				virtual int GetInitialValue() const;
				virtual void SetName(const std::wstring& n);
				virtual void SetDescription(const std::wstring& d);
				virtual void Reset();
				virtual std::wstring GetWaitingList();
				virtual ref<PropertySet> GetProperties();
				virtual ref<Scriptable> GetScriptable();
				virtual ref<Acquisition> CreateAcquisition();
				virtual void Clone();

			protected:
				// Called by Acquisition
				virtual bool Acquire(int n, ref<Waiting> w);
				virtual void Release(int n);
				virtual void Cancel(ref<Waiting> w);

				struct WaitingInfo {
					weak<Waiting> _w;
					int _amount;
				};

				std::deque< WaitingInfo > _queue;
				CriticalSection _lock;
				CapacityIdentifier _id;
				std::wstring _name;
				std::wstring _description;
				int _initial;
				volatile int _outstanding; // Always: _value + _outstanding == _initial
				volatile int _value;
		};

		
		// This class manages automated releasing of capacities
		class Acquisition: public virtual Object, public Waiting {
			friend class Capacity;

			public:
				virtual ~Acquisition();
				virtual ref<Capacity> GetCapacity();
				virtual void Release(int n);
				virtual bool Ensure(int n, ref<Waiting> w);
				virtual bool Acquire(int n, ref<Waiting> w);
				virtual void Cancel(); // Cancels waiting

			protected:
				virtual void Acquired(int n);
				Acquisition(ref<Capacity> c);
				CriticalSection _lock;
				volatile int _n;
				ref<Capacity> _cap;
				ref<Waiting> _waitingFor;
		};

		namespace script {
			class ScriptCapacitiesIterator;
		}

		class Capacities: public virtual Object, public Serializable {
			friend class view::CapacitiesWnd;
			friend class view::CapacityListWnd;
			friend class script::ScriptCapacitiesIterator;

			public:
				Capacities();
				virtual ~Capacities();

				virtual void Load(TiXmlElement* you);
				virtual void Save(TiXmlElement* parent);

				virtual ref<Capacity> GetCapacityByID(CapacityIdentifier id);
				virtual int GetCapacityCount() const;
				virtual void AddCapacity(ref<Capacity> cap);
				virtual void RemoveCapacity(int r);
				virtual void RemoveCapacity(ref<Capacity> c);
				virtual ref<Capacity> GetCapacityByIndex(int index);
				virtual ref<Capacity> GetCapacityByName(const std::wstring& name);
				virtual void Clear();
				virtual ref<Crumb> CreateCapacityCrumb(ref<Capacity> c);
				virtual std::vector< ref<Capacity> >::const_iterator GetBegin();
				virtual std::vector< ref<Capacity> >::const_iterator GetEnd();
				virtual void Reset();
				virtual ref<Scriptable> GetScriptable();
				virtual void MoveUp(ref<Capacity> c);
				virtual void MoveDown(ref<Capacity> c);

			protected:
				CriticalSection _lock;
				std::vector< ref<Capacity> > _caps;
		};
	}
}

#endif