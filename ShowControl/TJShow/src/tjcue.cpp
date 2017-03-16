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
#include "../include/internal/tjsubtimeline.h"
#include "../include/internal/tjinput.h"
#include "../include/internal/view/tjcueproperty.h"

using namespace tj::shared::graphics;
 
namespace tj {
	namespace show {
		// Capacity properties
		class CapacityPropertyWnd: public ChildWnd {
			public:
				CapacityPropertyWnd(std::wstring* cid): ChildWnd(L""), _linkIcon(L"icons/capacity.png") {
					assert(cid!=0);
					SetWantMouseLeave(true);
					_cid = cid;
				}

				virtual ~CapacityPropertyWnd() {
				}

				virtual void Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme) {
					Area rc = GetClientArea();

					SolidBrush back(theme->GetColor(Theme::ColorBackground));
					g.FillRectangle(&back, rc);

					if(IsMouseOver()) {
						theme->DrawToolbarBackground(g, 0.0f, 0.0f, float(rc.GetWidth()), float(rc.GetHeight()));
					}

					_linkIcon.Paint(g, Area(0,0,16,16));

					// Get Cue/Timeline information
					UpdateCache();
					ref<Capacity> cap = _cachedCapacity;
					if(cap) {
						std::wostringstream info;
						info << cap->GetName() << L" [" << cap->GetValue() << L"]";
						std::wstring name = info.str();
						SolidBrush tbr(theme->GetColor(Theme::ColorText));
						g.DrawString(name.c_str(), (int)name.length(), theme->GetGUIFont(), PointF(20.0f, 2.0f), &tbr);
					}
				}

				void UpdateCache() {
					ref<Capacity> cached = _cachedCapacity;
					if(!cached || cached->GetID()!=*_cid) {
						_cachedCapacity = Application::Instance()->GetModel()->GetCapacities()->GetCapacityByID(*_cid);
					}
				}

			protected:	
				virtual void OnSize(const Area& ns) {
					Repaint();
				}

				virtual void OnMouse(MouseEvent ev, Pixels x, Pixels y) {
					if(ev==MouseEventLUp) {
						// Popup madness
						ContextMenu cm;
						std::map<int, ref<Capacity> > commands;
						int index = 1;
						cm.AddItem(TL(capacity_none), 0, false, (*_cid)==L"");
						cm.AddSeparator();

						// Add capacities
						ref<Capacities> caps = Application::Instance()->GetModel()->GetCapacities();
						std::vector< ref<Capacity> >::const_iterator it = caps->GetBegin();
						std::vector< ref<Capacity> >::const_iterator end = caps->GetEnd();

						while(it!=end) {
							ref<Capacity> cap = *it;
							cm.AddItem(cap->GetName(), index, false, (*_cid)==cap->GetID());
							commands[index] = cap;							
							++index;
							++it;
						}

						// Show menu and process response
						Area rc = GetClientArea();
						int result = cm.DoContextMenu(this, rc.GetLeft(), rc.GetBottom());

						if(result==0) {
							*_cid = L"";
						}
						else if(result>0 && commands.find(result)!=commands.end()) {
							ref<Capacity> data = commands[result];
							if(data) {
								*_cid = data->GetID();
							}
						}
						Repaint();
					}
					else if(ev==MouseEventMove||ev==MouseEventLeave) {
						Repaint();
					}
				}

				Icon _linkIcon;
				std::wstring* _cid;
				weak<Capacity> _cachedCapacity;
		};

		class CapacityProperty: public Property {
			public:
				CapacityProperty(const std::wstring& name, std::wstring* cid): Property(name) {
					assert(cid!=0);
					_cid = cid;
				}

				virtual ~CapacityProperty() {
				}

				virtual ref<Wnd> GetWindow() {
					if(!_wnd) {
						_wnd = GC::Hold(new CapacityPropertyWnd(_cid));
					}
					return _wnd;
				}
				
				// Called when a repaint is about to begin and the value in the window needs to be updated
				virtual void Update() {
					if(_wnd) {
						_wnd->Update();
					}
				}

				virtual int GetHeight() {
					return 17;
				}

			protected:
				ref<CueList> _list;
				std::wstring *_cid;
				ref<CapacityPropertyWnd> _wnd;
		};
	}
}
 
const RGBColor Cue::DefaultColor(1.0f, 0.0f, 0.0f);

Cue::Cue(const std::wstring& name, const tj::shared::RGBColor& color, Time t, ref<CueList> cueList): _name(name), _condition(GC::Hold(new NullExpression())), _color(color), _t(t), _action(ActionNone), _cueList(cueList), _capacityCount(1), _private(false), _conditionType(Cue::ExpressionTypeWait) {
	Clone();
	_assignments = GC::Hold(new Assignments());
}

Cue::Cue(ref<CueList> ctrl): _name(L""), _color(Cue::DefaultColor), _t(-1), _action(ActionNone), _cueList(ctrl),_condition(GC::Hold(new NullExpression())), _capacityCount(1), _private(false), _conditionType(Cue::ExpressionTypeWait) {
	Clone();
	_assignments = GC::Hold(new Assignments());
}

void Cue::Clone() {
	_id = Util::RandomIdentifier(L'C');
}

void Cue::Remove() {
	ref<CueList> list = _cueList;
	if(list) {
		list->RemoveCue(this);
	}
}

void Cue::MoveRelative(Item::Direction dir) {
	switch(dir) {
		case Item::Left:
			_t = _t-Time(1);
			if(_t<Time(0)) _t = Time(0);
			break;

		case Item::Right:
			_t = _t+Time(1);
			break;
	}
}

Cue::~Cue() {
}

Time Cue::GetTime() const {
	return _t;
}

const std::wstring& Cue::GetID() const {
	return _id;
}

Cue::Action Cue::GetAction() const {
	return _action;
}

std::wstring Cue::GetTooltipText() {
	if(_name.length()>0) {
		return _name+std::wstring(L" @ ")+_t.Format()+ L" (" +Stringify(int(_t))+ L")";
	}
	return _t.Format()+ L" (" +Stringify(int(_t))+ L")";
}

void Cue::SetTime(Time t) {
	_t = t;
}

bool Cue::IsPrivate() const {
	return _private;
}

ref<Crumb> Cue::CreateCrumb() {
	return GC::Hold(new BasicCrumb(_name.length()>0?_name:TL(cue), L"icons/cue.png", this));
}

std::wstring Cue::GetName() const {
	return _name;
}

std::wstring Cue::GetDescription() const {
	return _description;
}

void Cue::SetName(const std::wstring& n) {
	_name = n;
}

bool Cue::HasAssignments() const {
	return _assignments && !_assignments->IsEmpty();
}

bool Cue::HasExpression() const {
	return _condition->HasChild();
}

RGBColor Cue::GetColor() {
	return _color;
}

void Cue::SetColor(const RGBColor& c) {
	_color = c;
}

void Cue::Move(Time t, int h) {
	_t = t;
}

void Cue::SetPlaybackStateToAction(ref<Instance> controller, Action ca) {
	switch(ca) {
		case ActionStart:
			controller->SetPlaybackState(PlaybackPlay);
			break;

		case ActionStop:
			controller->SetPlaybackState(PlaybackStop);
			break;

		case ActionPause:
			controller->SetPlaybackState(PlaybackPause);
			break;
	}
}

/** When isCapacity is true, we were waiting for a capacity. Otherwise, we were waiting for a condition
to be satisfied. If that's the case, we may need to wait again for the capacity... */
void Cue::DoReleaseAction(ref<Instance> c, bool isCapacity) {
	if(!isCapacity) {
		// Start acquiring capacities first if we have to
		if(!DoAcquireCapacity(c)) {
			// we're waiting *AGAIN*
			return;
		}
	}

	///if(c->GetPlaybackState()==PlaybackWaiting) {
		DoLeaveAction(c);

		Action ca = (_action==ActionNone) ? ActionStart : _action; // if there is no action, the action is 'start'
		SetPlaybackStateToAction(c,ca);
	///}
}

namespace tj {
	namespace show {
		class WaitingCue: public Waiting {
			public:
				enum WaitingFor {
					Capacity = 1,
					Expression,
				};

				WaitingCue(ref<Cue> cue, ref<Instance> c, WaitingFor w): _cue(cue), _instance(c), _for(w) {
				}

				virtual ~WaitingCue() {
				}

				virtual std::wstring GetName() {
					ref<Cue> cue = _cue;
					if(cue) {
						return cue->GetName();
					}
					return L"[Unknown cue]";
				}

				virtual void Acquired(int n) {
					ref<Cue> cue = _cue;
					ref<Instance> ct = _instance;

					if(cue && ct) {
						cue->DoReleaseAction(ct, _for==Capacity);
					}
					// else forget about it, cue or controller got deleted; acquisition is freed automagically
				}

				weak<Cue> _cue;
				weak<Instance> _instance;
				WaitingFor _for;
		};
	}
}

/** This returns true if we do not have to acquire capacity or we could acquire directly. Otherwise,
this returns false and the calling procedure (either DoAction or DoLeaveAction) will have to return. **/
bool Cue::DoAcquireCapacity(ref<Instance> controller) {
	if(IsUsingCapacity()) {
		ref<Capacity> cap = Application::Instance()->GetModel()->GetCapacities()->GetCapacityByID(_takeCapacityID);
		if(cap) {
			bool allowWaiting = true;
			if(_conditionType==ExpressionTypePause) {
				allowWaiting = false;
			}
			ref<Acquisition> acq = controller->GetAcquisitionFor(cap);
			if(!acq->Ensure(_capacityCount, allowWaiting ? GC::Hold(new WaitingCue(this, controller, WaitingCue::Capacity)) : null)) {
				controller->SetPlaybackState(allowWaiting ? PlaybackWaiting : PlaybackPause);
				return false;
			}
		}
	}
	return true;
}

/** As simple as it may seem, Cue action processing is pretty complicated, because we need to take care of conditions,
lock semantics, stuff like that. First thing we do is lock the cue, so the important things cannot change while we
are working. Cue::DoAction is usually called from a separate 'CueThread' (per controller), whilst changes to properties
are mostly made from the main (UI) thread.

First, we check if we have a condition and if we do, whether its 'true' or not. If it is not true, we tell the variables
manager to notify us when the condition is satisfied. We give the Variables class a WaitingCue instance to do so. If
the condition is satisfied, WaitingCue will call 'DoReleaseAction'. If the condition was true the first time we tried,
DoAction will continue.

In either case (DoAction/DoReleaseAction), we then call DoAcquireCapacity. This method checks if there is any capacity 
that we need to acquire, and if there is, it tries to. If it cannot acquire directly, it hands the capacity manager a 
'Waiting' class (WaitingCue) that gets notified when the capacity has been acquired after all (in the same way as we do
for conditions). WaitingCue then calls DoReleaseAction (sometimes again if we have been waiting on a condition before; but this
doesn't matter, since the 'bool' parameter to DoReleaseAction tells us if we need to acquire capacities or not).

When all is done, DoReleaseAction and DoAction call DoLeaveAction, which does stuff like executing scripts, making jumps,
triggering other stuff, etc. 

All objects that help with waiting, such as Acquisition and WaitingCue, are automatically removed by the Controller when the timeline
stops for whatever reason (it was triggered or stopped by hand). 

DoAction returns true if the CueThread can continue processing cues linearly (e.g. no jumps or state changes happened) or false if
it cannot. 
**/
bool Cue::DoAction(Application* app, ref<Instance> controller) {
	ThreadLock lock(&(controller->_lock));
	ref<Variables> vars = controller->GetVariables();

	if(vars && _condition->IsComplete(vars)) {
		// This automagically cancels previous waiting operations on conditions
		bool allowWaiting = _conditionType==ExpressionTypeWait;

		if(allowWaiting) {
			controller->SetWaitingFor(GC::Hold(new WaitingCue(this, controller, WaitingCue::Expression)));
		}
		else {
			controller->SetWaitingFor(null);
		}
			
		if(!vars->Evaluate(_condition, controller->GetWaitingFor())) {
			if(_conditionType!=ExpressionTypeSkip) {
				controller->SetPlaybackState(allowWaiting ? PlaybackWaiting : PlaybackPause);
				return false;
			}
			return true; // cue skipped
		}
	}

	if(!DoAcquireCapacity(controller)) {
		return false; // we're waiting...
	}

	// DoLeaveAction executes the final part of cue processing. Triggers are also handled
	// there. It could be that the cue is triggering a cue in the same timeline as this cue.
	// If that's the case, we should return false here, since a jump has occurred and the CueThread
	// should continue processing the other cues.
	bool cancelLinearProcessing = DoLeaveAction(controller);
	
	// If we have a playback action, set our controller to it
	SetPlaybackStateToAction(controller, _action);
	return !cancelLinearProcessing && (_action!=Cue::ActionStop && _action!=Cue::ActionPause);
}

bool Cue::IsUsingCapacity() const {
	return _takeCapacityID.length()>0 && _capacityCount > 0;
}

bool Cue::IsReleasingCapacity() const {
	return _releaseCapacityID.length()>0 && _capacityCount > 0;
}

int Cue::GetCapacityCount() const {
	return _capacityCount;
}

ref<Capacity> Cue::GetCapacityUsed() {
	if(IsUsingCapacity()) {
		return Application::Instance()->GetModel()->GetCapacities()->GetCapacityByID(_takeCapacityID);
	}
	return 0;
}

ref<Capacity> Cue::GetCapacityReleased() {
	if(IsReleasingCapacity()) {
		return Application::Instance()->GetModel()->GetCapacities()->GetCapacityByID(_releaseCapacityID);
	}
	return 0;
}

bool Cue::DoLeaveAction(ref<Instance> controller) {
	ref<Model> model = Application::Instance()->GetModel();

	// Release capacities that we have acquired, but do not have to keep, since the timeline is not playing,
	// and not going to because of this cue
	if(IsUsingCapacity() && controller->GetPlaybackState()==PlaybackStop && _action!=ActionPause && _action!=ActionStart) {
		ref<Capacity> cap = model->GetCapacities()->GetCapacityByID(_takeCapacityID);
		if(cap) {
			ref<Acquisition> acq = controller->GetAcquisitionFor(cap);
			if(acq) {
				acq->Release(_capacityCount);
			}
			else {
				Log::Write(L"TJShow/Cue", L"Trying to release a lock that this controller never acquired (cue: "+GetName()+L" in "+controller->GetTimeline()->GetName()+L")");
			}
		}
	}

	// Release capacities we have to release
	if(_releaseCapacityID.length()>0 && _capacityCount>0) {
		ref<Capacity> cap = model->GetCapacities()->GetCapacityByID(_releaseCapacityID);
		if(cap) {
			ref<Acquisition> acq = controller->GetAcquisitionFor(cap);
			if(acq) {
				acq->Release(_capacityCount);
			}
			else {
				Log::Write(L"TJShow/Cue", L"Trying to release a lock that this controller never acquired (cue: "+GetName()+L" in "+controller->GetTimeline()->GetName()+L")");
			}
		}
	}

	// Set variables (this might trigger other stuff)
	ref<Variables> vars = controller->GetVariables();
	if(vars) {
		controller->GetVariables()->Assign(_assignments, vars);
	}

	// trigger other cue if there is one (IsLinkedToDistantCue also checks if we're not accidentaly intending to trigger ourselves)
	if(IsLinkedToDistantCue()) {
		ref<Instance> inst;

		if(_distantTimelineID == controller->GetTimeline()->GetID()) {
			/* This trigger is a self-trigger. Self-triggers always execute in the current instance,
			so call the trigger on the controller that was given as parameter to this method. */
			inst = controller;
		}
		else {
			inst = Application::Instance()->GetInstances()->GetInstanceByTimelineID(_distantTimelineID);
		}

		if(inst) {
			strong<CueList> cl = inst->GetCueList();
			ref<Cue> cue = cl->GetCueByID(_distantCueID);
			if(cue) {
				inst->Trigger(cue);
			}

			// The cue we triggered is in the same cue list as this cue is, so the
			// CueThread cannot continue linearly.
			if(ref<CueList>(cl)==ref<CueList>(_cueList)) {
				return false;
			}
		}
	}

	return true;
}

bool Cue::IsLinkedToDistantCue() const {
	// TODO: Chances are that another cue with the same ID exists, but it just shouldn't happen... check on load?
	return _distantCueID.size()>0 && _distantCueID!=_id;
}

void Cue::Save(TiXmlElement* cue) {
	SaveAttributeSmall(cue,"name", _name);
	SaveAttributeSmall(cue, "time", _t);
	SaveAttributeSmall(cue, "id", _id);

	if(IsReleasingCapacity()) {
		SaveAttributeSmall(cue, "release", _releaseCapacityID);
	}

	if(IsUsingCapacity()) {
		SaveAttributeSmall(cue, "use", _takeCapacityID);
	}

	SaveAttributeSmall(cue, "capacity", _capacityCount);
	SaveAttributeSmall(cue, "private", _private);
	SaveAttributeSmall(cue, "condition-type", ExpressionTypeToString(_conditionType));

	if(_description.length()>0) {
		SaveAttribute(cue, "description", _description);
	}

	if(IsLinkedToDistantCue()) {
		SaveAttributeSmall(cue, "distant-timeline", _distantTimelineID);
		SaveAttributeSmall(cue, "distant-cue", _distantCueID);
	}

	TiXmlElement condition("condition");
	_condition->Save(&condition);
	cue->InsertEndChild(condition);

	if(!_assignments->IsEmpty()) {
		TiXmlElement actions("actions");
		_assignments->Save(&actions);
		cue->InsertEndChild(actions);
	}

	TiXmlElement color("color");
	_color.Save(&color);
	cue->InsertEndChild(color);

	// action
	const wchar_t* astr = L"none";
	switch(_action) {
		case ActionStart:
			astr = L"start";
			break;
		case ActionStop:
			astr = L"stop";
			break;
		case ActionPause:
			astr = L"pause";
			break;
		case ActionNone:
		default:
			break;
	};

	SaveAttributeSmall(cue, "action", std::wstring(astr));
}

void Cue::Load(TiXmlElement* you) {
	_name = LoadAttributeSmall(you, "name", std::wstring(L""));
	_t = LoadAttributeSmall(you, "time", Time(0));
	_id = LoadAttributeSmall<std::wstring>(you, "id", _id);
	_distantTimelineID = LoadAttributeSmall<std::wstring>(you, "distant-timeline", _distantTimelineID);
	_distantCueID = LoadAttributeSmall<std::wstring>(you, "distant-cue", _distantTimelineID);
	_description = LoadAttribute<std::wstring>(you, "description", _description);
	_capacityCount = LoadAttributeSmall<int>(you, "capacity", _capacityCount);
	_takeCapacityID = LoadAttributeSmall<CapacityIdentifier>(you, "use", _takeCapacityID);
	_releaseCapacityID = LoadAttributeSmall<CapacityIdentifier>(you, "release", _releaseCapacityID);
	_private = LoadAttributeSmall(you, "private", _private);
	_conditionType = ExpressionTypeFromString(LoadAttributeSmall<std::wstring>(you, "condition-type", ExpressionTypeToString(_conditionType)));

	std::wstring ws = LoadAttributeSmall(you, "action", std::wstring(L"none"));
	if(ws==L"start") {
		_action = ActionStart;
	}	
	else if(ws==L"stop") {
		_action = ActionStop;
	}
	else if(ws==L"pause") {
		_action = ActionPause;
	}
	else {
		_action = ActionNone;
	}

	TiXmlElement* color = you->FirstChildElement("color");
	if(color!=0) _color.Load(color);

	TiXmlElement* condition = you->FirstChildElement("condition");
	if(condition!=0) _condition->Load(condition);

	TiXmlElement* actions = you->FirstChildElement("actions");
	if(actions!=0) _assignments->Load(actions);
}

ref<PropertySet> Cue::GetProperties() {
	ref<PropertySet> prs = GC::Hold(new PropertySet());
	ref<Instance> controller;
	ref<CueList> cuelist = _cueList;
	if(cuelist) {
		controller = cuelist->_controller;
	}

	prs->Add(GC::Hold(new GenericProperty<std::wstring>(TL(cue_name), this, &_name, _name)));
	prs->Add(GC::Hold(new TextProperty(TL(cue_description), this, &_description, 50)));
	prs->Add(GC::Hold(new GenericProperty<Time>(TL(cue_time), this, &_t, _t)));
	prs->Add(GC::Hold(new ColorProperty(TL(cue_color), this, &_color, 0)));

	// Behaviours
	prs->Add(GC::Hold(new PropertySeparator(TL(cue_actions))));
	if(controller) {
		prs->Add(GC::Hold(new ExpressionProperty(TL(expression), _condition, controller->GetVariables())));
	}

	ref<GenericListProperty<ExpressionType> > ct = GC::Hold(new GenericListProperty<ExpressionType>(TL(cue_expression_type), this, &_conditionType, _conditionType));
	ct->AddOption(TL(cue_expression_type_wait), ExpressionTypeWait);
	ct->AddOption(TL(cue_expression_type_pause), ExpressionTypePause);
	ct->AddOption(TL(cue_expression_type_skip), ExpressionTypeSkip);
	prs->Add(ct);

	prs->Add(GC::Hold(new CapacityProperty(TL(cue_take_capacity), &_takeCapacityID)));

	// Distant linking
	ref<CueList> list = _cueList;
	if(list) {
		prs->Add(GC::Hold(new view::DistantCueProperty(TL(cue_distant_property), list, &_distantTimelineID, &_distantCueID)));
	}

	if(controller) {
		prs->Add(GC::Hold(new AssignmentsProperty(TL(assignments), _assignments, controller->GetVariables())));
	}

	GenericListProperty<Action>* gp = new GenericListProperty<Action>(TL(cue_action), this, &_action, ActionNone);
	gp->AddOption(TL(cue_action_none), ActionNone);
	gp->AddOption(TL(cue_action_start), ActionStart);
	gp->AddOption(TL(cue_action_stop), ActionStop);
	gp->AddOption(TL(cue_action_pause), ActionPause);
	prs->Add(GC::Hold(gp));
	prs->Add(GC::Hold(new CapacityProperty(TL(cue_release_capacity), &_releaseCapacityID)));

	prs->Add(GC::Hold(new PropertySeparator(TL(cue_advanced), true)));
	prs->Add(GC::Hold(new GenericProperty<int>(TL(cue_capacity_size), this, &_capacityCount, _capacityCount)));
	prs->Add(GC::Hold(new GenericProperty<bool>(TL(cue_private), this, &_private, _private)));
	return prs;
}

Cue::ExpressionType Cue::ExpressionTypeFromString(const std::wstring& ct) {
	if(ct==L"wait") {
		return ExpressionTypeWait;
	}
	else if(ct==L"skip") {
		return ExpressionTypeSkip;
	}
	else if(ct==L"pause") {
		return ExpressionTypePause;
	}

	return ExpressionTypeWait;
}

std::wstring Cue::ExpressionTypeToString(Cue::ExpressionType ct) {
	switch(ct) {
		case ExpressionTypeSkip:
			return L"skip";

		case ExpressionTypePause:
			return L"pause";

		default:
		case ExpressionTypeWait:
			return L"wait";
	}
}

ref<Expression> Cue::GetCondition() {
	return _condition;
}

void Cue::DoBindInputDialog(ref<Wnd> w) {
	// Find rules table
	ref<Model> model = Application::Instance()->GetModel();
	if(!model) {
		return;
	}
	ref<input::Rules> rules = model->GetInputRules();
	if(!rules) {
		return;
	}

	// find out my tlid and cid, create our endpoint ID and then call model's input rules dialog
	ref<CueList> list = _cueList;
	if(list) {
		ref<Instance> controller = list->_controller;
		if(controller) {
			ref<Timeline> tl = controller->GetTimeline();
			if(tl) {
				EndpointID eid = CueEndpointCategory::GetEndpointID(tl->GetID(), GetID());
				ref<input::Rule> rule = rules->DoBindDialog(w, CueEndpointCategory::KCueEndpointCategoryID, eid);
				if(rule) {
					rules->AddRule(rule);
				}
			}
		}
	}
}

/* CueEndpoint */
class CueEndpoint: public Endpoint {
	public:
		CueEndpoint(const std::wstring& tlid, const std::wstring& cid): _tlid(tlid), _cid(cid) {
		}

		virtual ~CueEndpoint() {
		}

		virtual void Set(const Any& f) {
			if((bool)f) {
				ref<Instance> ctrl = Application::Instance()->GetInstances()->GetInstanceByTimelineID(_tlid);
				if(ctrl) {
					ref<Cue> cue = ctrl->GetCueList()->GetCueByID(_cid);
					if(cue) {
						ctrl->Trigger(cue);
					}
				}
			}
		}

		virtual std::wstring GetName() const {
			std::wstring name = TL(cue);

			ref<Instance> ctrl = Application::Instance()->GetInstances()->GetInstanceByTimelineID(_tlid);
			if(ctrl) {
				ref<Cue> cue = ctrl->GetCueList()->GetCueByID(_cid);
				if(cue) {
					name = name + L" '" + cue->GetName() + L"' ";
				}
			}
			return name;
		}

	private:
		const std::wstring _cid;
		const std::wstring _tlid;
};

/* CueEndpointCategory */
const EndpointCategoryID CueEndpointCategory::KCueEndpointCategoryID = L"cue";

CueEndpointCategory::CueEndpointCategory(): EndpointCategory(KCueEndpointCategoryID) {
}

CueEndpointCategory::~CueEndpointCategory() {
}

ref<Endpoint> CueEndpointCategory::GetEndpointById(const EndpointID& id) {
	std::wstring::size_type sep = id.find_first_of(L":");
	std::wstring tlid = id.substr(0, sep);
	std::wstring cid = id.substr(sep+1);
	return GC::Hold(new CueEndpoint(tlid, cid));
}

EndpointID CueEndpointCategory::GetEndpointID(const std::wstring& timelineID, const std::wstring& cueID) {
	return timelineID + L":" + cueID;
}