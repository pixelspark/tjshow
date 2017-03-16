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
#ifndef _TJSHOW_INPUT_H
#define _TJSHOW_INPUT_H

namespace tj {
	namespace show {
		namespace view {
			class InputChannelListWnd;
		}

		namespace input {
			class Rule: public virtual Object, public Serializable, public Inspectable {
				friend class Rules;
				friend class tj::show::view::InputChannelListWnd;

				public:
					Rule();
					virtual ~Rule();
					virtual void Fire(const PatchIdentifier& pi, const InputID& path, const Any& value);
					virtual void Save(TiXmlElement* you);
					virtual void Load(TiXmlElement* you);
					ref<Endpoint> GetEndpoint();
					virtual InputID GetPath() const;
					virtual ref<PropertySet> GetProperties();

				protected:
					PatchIdentifier _patch;
					InputID _path;
					EndpointCategoryID _cat;
					EndpointID _id;
			};

			class Rules: public virtual Dispatcher, public Serializable, public virtual Object {
				public:
					Rules();
					virtual ~Rules();
					virtual void Save(TiXmlElement* you);
					virtual void Load(TiXmlElement* you);
					virtual void Clear();
					virtual unsigned int GetRuleCount() const;
					virtual void AddRule(ref<Rule> rule);
					virtual void RemoveRule(ref<Rule> rule);
					virtual ref<Rule> GetRuleByIndex(unsigned int idx);
					virtual ref<Rule> DoBindDialog(ref<Wnd> parent, const EndpointCategoryID& cat, const EndpointID& eid);
					virtual void FindRulesForPatch(const PatchIdentifier& pi, std::vector< ref<Rule> >& lst);

					virtual void Dispatch(const PatchIdentifier& pi, const InputID& path, const Any& value);
					virtual void Dispatch(ref<Device> device, const InputID& path, const Any& value);

				protected:
					mutable CriticalSection _lock;
					std::vector< ref<Rule> > _rules;

					const static unsigned int KRememberPaths = 10;
					std::deque<InputID> _lastPaths;
					PatchIdentifier _lastPatch;
			};
		}
	}
}

#endif