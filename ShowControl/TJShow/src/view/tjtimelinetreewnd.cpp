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
#include "../../include/internal/tjshow.h"
#include "../../include/internal/view/tjtimelinetreewnd.h"
#include "../../include/internal/tjsubtimeline.h"
#include "../../include/internal/view/tjtimelinewnd.h"
#include "../../include/internal/tjcontroller.h"
#include "../../include/internal/view/tjsplittimelinewnd.h"

using namespace tj::show;
using namespace tj::show::view;
using namespace tj::shared::graphics;

namespace tj {
	namespace show {
		namespace view {
			class TimelineTreeNode: public SimpleTreeNode {
				public:
					TimelineTreeNode(ref<Instance> st, bool isInstance, const std::wstring& iname = L"");
					virtual ~TimelineTreeNode();
					virtual void Generate(bool inInstance);
					virtual void Paint(graphics::Graphics& g, ref<Theme> theme, const Area& row, const TreeColumnInfo& ci);
					virtual void OnMouse(MouseEvent ev, Pixels x, Pixels y);

				protected:
					bool _isInstance;
					std::wstring _name;
					weak<Instance> _st;
					std::map< weak<Instance>, ref<TimelineTreeNode> > _childInstances;
			};

			class ShowTreeNode: public SimpleTreeNode {
				public:
					ShowTreeNode(strong<Model> m, ref<Instance> root);
					virtual ~ShowTreeNode();
					virtual void Generate();
					virtual void Paint(graphics::Graphics& g, ref<Theme> theme, const Area& row, const TreeColumnInfo& ci);
					virtual void OnMouse(MouseEvent ev, Pixels x, Pixels y);

				private:
					weak<Model> _model;
					ref<TimelineTreeNode> _root;
					Icon _packageIcon;
			};
		}
	}
}

/** TimelineTreeNode **/
TimelineTreeNode::TimelineTreeNode(ref<Instance> st, bool isInstance, const std::wstring& iname): SimpleTreeNode(L""), _st(st), _name(iname), _isInstance(isInstance) {
}

TimelineTreeNode::~TimelineTreeNode() {
}

void TimelineTreeNode::OnMouse(MouseEvent ev, Pixels x, Pixels y) {
	if(ev==MouseEventLDown || ev==MouseEventLDouble) {
		ref<Instance> controller = _st;
		if(controller) {
			ref<Timeline> timeline = controller->GetTimeline();
			if(timeline) {
				/*if(ev==MouseEventLDown) {
					Application::Instance()->GetView()->Inspect(timeline);
				}
				else */if(ev==MouseEventLDouble) {
					if(controller.IsCastableTo<Controller>()) {
						ref<Controller> ctrl = controller;
						ref<SplitTimelineWnd> stw = ctrl->GetSplitTimelineWindow();
						if(stw) {
							Application::Instance()->GetView()->RevealTimeline(stw);
						}
					}
				}
			}
		}
	}
	SimpleTreeNode::OnMouse(ev,x,y);
}

void TimelineTreeNode::Paint(graphics::Graphics& g, ref<Theme> theme, const Area& row, const TreeColumnInfo& ci) {
	ref<Instance> controller = _st;
	if(controller) {
		ref<Timeline> timeline = controller->GetTimeline();

		// Draw self
		SolidBrush text(theme->GetColor(Theme::ColorText));
		SolidBrush instanceText(theme->GetColor(Theme::ColorActiveStart));
		StringFormat sf;
		sf.SetFormatFlags(sf.GetFormatFlags()|StringFormatFlagsLineLimit);
		sf.SetLineAlignment(StringAlignmentCenter);
		
		if(ci.IsColumnVisible(TimelineTreeWnd::KColName)) {
			Area nameArea = ci.GetFieldArea(row, TimelineTreeWnd::KColName);
			ref<Icon> icon = PlaybackStateIcons::GetTabIcon(controller->GetPlaybackState());
			if(icon) {
				icon->Paint(g, Area(nameArea.GetLeft()+2, nameArea.GetTop()+2, 14, 14));
				nameArea.Narrow(15,0,0,0);
			}	
			
			std::wstring name;
			if(_isInstance) {
				name = _name + L" [" + timeline->GetName() + L"]";
			}
			else {
				name = timeline->GetName();
			}

			if(name.length()<1) {
				name = TL(timeline);
			}
			g.DrawString(name.c_str(), (int)name.length(), theme->GetGUIFont(), nameArea, &sf, _isInstance ? &instanceText : &text);
		}
		
		Time t = controller->GetTime();
	
		if(ci.IsColumnVisible(TimelineTreeWnd::KColTime)) {
			std::wstring time = t.Format();
			g.DrawString(time.c_str(), (int)time.length(), theme->GetGUIFont(), ci.GetFieldArea(row, TimelineTreeWnd::KColTime), &sf, &text);
		}

		if(ci.IsColumnVisible(TimelineTreeWnd::KColNextCue)) {
			ref<Cue> next = timeline->GetCueList()->GetNextCue(t);
			if(next) {
				Area cueArea = ci.GetFieldArea(row, TimelineTreeWnd::KColNextCue);
				Area color(cueArea);
				color.SetWidth(cueArea.GetHeight());
				color.Narrow(3,3,3,3);

				SolidBrush cueColor(next->GetColor());
				g.FillRectangle(&cueColor, color);
				cueArea.Narrow(color.GetWidth()+6,0,0,0);
				
				std::wstring cueName = next->GetName();
				g.DrawString(cueName.c_str(), (int)cueName.length(), theme->GetGUIFont(), cueArea, &sf, &text);

			}
		}

		if(ci.IsColumnVisible(TimelineTreeWnd::KColPreviousCue)) {
			ref<Cue> next = timeline->GetCueList()->GetPreviousCue(t);
			if(next) {
				Area cueArea = ci.GetFieldArea(row, TimelineTreeWnd::KColPreviousCue);
				Area color(cueArea);
				color.SetWidth(cueArea.GetHeight());
				color.Narrow(3,3,3,3);

				SolidBrush cueColor(next->GetColor());
				g.FillRectangle(&cueColor, color);
				cueArea.Narrow(color.GetWidth()+6,0,0,0);
				
				std::wstring cueName = next->GetName();
				g.DrawString(cueName.c_str(), (int)cueName.length(), theme->GetGUIFont(), cueArea, &sf, &text);
			}
		}
	}
}

/** ShowTreeNode **/
ShowTreeNode::ShowTreeNode(strong<Model> m, ref<Instance> root): SimpleTreeNode(L""), _model(ref<Model>(m)), _packageIcon(L"icons/package.png") {
	_root = GC::Hold(new TimelineTreeNode(root,  false, L""));
	Add(ref<TreeNode>(_root));
	SetAlwaysExpanded(true);
}

ShowTreeNode::~ShowTreeNode() {
}

void ShowTreeNode::Generate() {
	_root->Generate(false);
}

void ShowTreeNode::OnMouse(MouseEvent ev, Pixels x, Pixels y) {
	if(ev==MouseEventRDown) {
		ref<Model> model = _model;
		if(model) {
			ContextMenu cm;
			enum { KCProperties = 1 };
			cm.AddItem(TL(properties), KCProperties, true, false);

			int r = cm.DoContextMenu(ref<TreeWnd>(TreeNode::_parent), -1, -1);
			if(r==KCProperties) {
				ref<Path> p = GC::Hold(new Path());
				p->Add(model->CreateModelCrumb());
				Application::Instance()->GetView()->Inspect(model, p);
			}
		}
	}
	SimpleTreeNode::OnMouse(ev,x,y);
}

void ShowTreeNode::Paint(graphics::Graphics& g, ref<Theme> theme, const Area& row, const TreeColumnInfo& ci) {
	ref<Model> model = _model;
	if(model) {
		// Draw self
		SolidBrush text(theme->GetColor(Theme::ColorText));
		StringFormat sf;
		sf.SetFormatFlags(sf.GetFormatFlags()|StringFormatFlagsLineLimit);
		sf.SetLineAlignment(StringAlignmentCenter);
		
		if(ci.IsColumnVisible(TimelineTreeWnd::KColName)) {
			Area nameArea = ci.GetFieldArea(row, TimelineTreeWnd::KColName);
			_packageIcon.Paint(g, Area(nameArea.GetLeft()+2, nameArea.GetTop()+2, 14, 14));
			nameArea.Narrow(20,0,0,0);
			
			std::wstring name = model->GetTitle();

			if(name.length()<1) {
				name = TL(show);	
			}
			g.DrawString(name.c_str(), (int)name.length(), theme->GetGUIFontBold(), nameArea, &sf, &text);
		}
	}
}

/** TimelineTreeWnd **/
TimelineTreeWnd::TimelineTreeWnd(ref<Model> model, ref<Instance> root, bool compact): _root(root), _model(model) {
	SetShowHeader(!compact);
	AddColumn(TL(timeline_tree), KColName, compact ? 1.0f: 0.3f);
	if(!compact) {
		AddColumn(TL(timeline_time), KColTime, 0.2f, false);
		AddColumn(TL(timeline_next_cue), KColNextCue, 0.2f, !compact);
		AddColumn(TL(timeline_previous_cue), KColPreviousCue, 0.2f, !compact);
	}
	StartTimer(Time(1000), 1);
}

TimelineTreeWnd::~TimelineTreeWnd() {
}

void TimelineTreeWnd::OnCreated() {
	TreeWnd::OnCreated();
	Rebuild();
}

std::wstring TimelineTreeWnd::GetTabTitle() const {
	return TL(timeline_tree);
}

void TimelineTreeWnd::Rebuild() {
	ref<Instance> root = _root;
	ref<Model> model = _model;
	if(root && model) {
		strong<ShowTreeNode> rootNode = GC::Hold(new ShowTreeNode(model, root));
		rootNode->Generate();
		rootNode->SetExpanded(true,false);
		SetRoot(rootNode);
	}
}

void TimelineTreeWnd::OnTimer(unsigned int id) {
	ref<ShowTreeNode> root = GetRoot();
	if(root) {
		root->Generate();
	}
	Repaint();
}

void TimelineTreeNode::Generate(bool inInstance) {
	_children.clear();
	std::map< weak<Instance>, ref<TimelineTreeNode> > newChildren;

	ref<Instance> root = _st;
	if(root) {
		ref< Iterator< ref<TrackWrapper> > > it = root->GetTimeline()->GetTracks();
		while(it->IsValid()) {
			ref<TrackWrapper> tw = it->Get();
			if(tw) {
				/* Do not recurse into subtimelines if we're already in an instance; subtimelines are primary instances,
				not children of other instances. */
				ref<Instance> instance;
				ref<SubTimeline> st = tw->GetSubTimeline();
				if(!inInstance && st) {
					instance = st->GetInstance();
				}
				else {
					ref<Instancer> inst = tw->GetInstancer();
					if(inst) {
						// recurse into the instance, if it has been created (i.e. when root is playing)
						instance = root->GetChildInstance(inst);
					}
				}
				
				if(instance) {
					std::map<weak<Instance>, ref<TimelineTreeNode> >::iterator cit = _childInstances.find(weak<Instance>(instance));
					if(cit!=_childInstances.end()) {
						Add(ref<TreeNode>(cit->second));
						newChildren[instance] = cit->second;
						cit->second->Generate(inInstance || !st);
					}
					else {
						strong<TimelineTreeNode> node = GC::Hold(new TimelineTreeNode(instance, !st, tw->GetInstanceName()));
						node->Generate(inInstance || !st);
						Add(node);
						newChildren[instance] = node;
					}
				}
			}
			it->Next();
		}
	}

	_childInstances = newChildren;
}