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
#ifndef _TJCUE_H
#define _TJCUE_H

namespace tj {
	namespace show {
		/* These classes cannot be used by plug-in developers, but are needed for internal use of the Cue-class */
		class Application;
		class CueList;
		class Instance;
		class Capacity;
		class WaitingCue;
		class Expression;
		class NullExpression;
		class Assignments;

		typedef std::wstring CueIdentifier;

		/** A Cue is a marker on the timeline at a specified time. A cue is defined by the user
		and has a name, color and sometimes an action. When the timeline is played and a cue is
		encountered, the action is performed. The action can be one of the Cue::Action enumeration.
		**/
		class Cue: public virtual tj::shared::Object, public Item, public tj::shared::Serializable {
			friend class WaitingCue;

			public:
				enum Action {
					ActionNone=0,
					ActionStop,
					ActionStart,
					ActionPause,
				};

				enum ExpressionType {
					ExpressionTypeWait = 1,
					ExpressionTypePause,
					ExpressionTypeSkip,
				};

				Cue(ref<CueList> ctrl);
				Cue(const std::wstring& name, const tj::shared::RGBColor& col, Time t, ref<CueList> ctrl);
				virtual ~Cue();
				Time GetTime() const;
				void SetTime(Time t);
				std::wstring GetName() const;
				std::wstring GetDescription() const;
				void SetName(const std::wstring& ws);
				tj::shared::RGBColor GetColor();
				void SetColor(const tj::shared::RGBColor& c);
				bool DoAction(Application* app, ref<Instance> controller);
				Action GetAction() const;
				const CueIdentifier& GetID() const;
				bool HasAssignments() const;
				bool HasExpression() const;
				virtual ref<Expression> GetCondition();
				
				virtual void Save(TiXmlElement* parent);
				virtual void Load(TiXmlElement* you);
				virtual ref<tj::shared::PropertySet> GetProperties();
				virtual void Move(Time t, int h);
				virtual std::wstring GetTooltipText();
				virtual ref<tj::shared::Crumb> CreateCrumb();
				bool IsLinkedToDistantCue() const;
				virtual void Clone(); // changes ID's
				bool IsPrivate() const;
				virtual void Remove();
				virtual void MoveRelative(Item::Direction dir);


				bool IsUsingCapacity() const;
				bool IsReleasingCapacity() const;
				ref<Capacity> GetCapacityUsed();
				ref<Capacity> GetCapacityReleased();
				int GetCapacityCount() const;
				void DoBindInputDialog(ref<Wnd> w);

				static const tj::shared::RGBColor DefaultColor;

			protected:
				static ExpressionType ExpressionTypeFromString(const std::wstring& ct);
				static std::wstring ExpressionTypeToString(ExpressionType ct);

				// When isCapacity==false, we were waiting for a condition and we may need to wait again for a capacity
				void DoReleaseAction(ref<Instance> c, bool isCapacity); // called from WaitingCue when a capacity we've been waiting on has been released for us
				static void SetPlaybackStateToAction(ref<Instance> c, Action a);
				bool DoLeaveAction(ref<Instance> c);
				bool DoAcquireCapacity(ref<Instance> c);

				Time _t;
				bool _private;
				ExpressionType _conditionType;
				Action _action;
				std::wstring _name;
				tj::shared::RGBColor _color;
				std::wstring _description;
				std::wstring _id;
				std::wstring _takeCapacityID;
				std::wstring _releaseCapacityID;
				int _capacityCount;

				TimelineIdentifier _distantTimelineID;
				std::wstring _distantCueID;
				strong<NullExpression> _condition;
				ref<Assignments> _assignments;
				weak<CueList> _cueList;
		};

		class CueEndpointCategory: public EndpointCategory {
			public:
				CueEndpointCategory();
				virtual ~CueEndpointCategory();
				virtual ref<Endpoint> GetEndpointById(const EndpointID& id);
				static EndpointID GetEndpointID(const std::wstring& timelineID, const std::wstring& cueID);

				const static EndpointCategoryID KCueEndpointCategoryID;
		};
	}
}

#endif