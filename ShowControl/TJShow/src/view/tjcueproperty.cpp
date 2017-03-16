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
#include "../../include/internal/view/tjcueproperty.h"

using namespace tj::shared;
using namespace tj::shared::graphics;
using namespace tj::show;
using namespace tj::show::view;

/** DistantCueProperty is a property that 'links' to a cue in another timeline by ID. It stores timeline ID 
and cue ID in wstrings. **/
namespace tj {
	namespace show {
		namespace view {
			class DistantCuePropertyWnd: public ChildWnd {
				public:
					DistantCuePropertyWnd(std::wstring* tlid, std::wstring* cid): ChildWnd(L""), _linkIcon(L"icons/link.png") {
						assert(tlid!=0 && cid!=0);
						SetWantMouseLeave(true);
						_tlid = tlid;
						_cid = cid;
					}

					virtual ~DistantCuePropertyWnd() {
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
						std::wstring info;
						UpdateCache();
						ref<Timeline> timeline = _cachedTimeline;
						ref<Cue> cue = _cachedCue;
						if(timeline) {
							info = timeline->GetName();

							if(cue) {
								info += L": " + cue->GetName();

								tj::shared::graphics::SolidBrush cueColor(cue->GetColor());
								Area ccb(rc.GetRight()-rc.GetHeight(), rc.GetTop(), rc.GetHeight(), rc.GetHeight());
								ccb.Narrow(1,2,1,1);
								g.FillRectangle(&cueColor, ccb);
							}
						}

						SolidBrush tbr(theme->GetColor(Theme::ColorActiveStart));
						g.DrawString(info.c_str(), (int)info.length(), theme->GetGUIFont(), PointF(20.0f, 2.0f), &tbr);
					}

					void UpdateCache() {
						// update timeline
						ref<Timeline> cached = _cachedTimeline;
						if(!cached || cached->GetID() != *_tlid) {
							_cachedTimeline = Application::Instance()->GetModel()->GetTimelineByID(*_tlid);
							_cachedCue = ref<Cue>(null);
						}

						// fetch cue
						ref<Timeline> ct = _cachedTimeline;
						if(ct) {
							_cachedCue = ct->GetCueList()->GetCueByID(*_cid);
						}
						else {
							_cachedCue = ref<Cue>(null);
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
							strong<Instance> rootInstance = Application::Instance()->GetInstances()->GetRootInstance();
							std::map<int, std::pair< ref<Instance>, ref<Cue> > > commands;

							// Add 'None' option
							std::wstring none = TL(cue_distant_none);
							cm.AddItem(TL(cue_distant_none), 0, false, *_cid==L"");						
							int index = 1;
							PopulateCueMenu(cm.GetMenu(), rootInstance, &commands, index);

							Area rc = GetClientArea();
							int cmd = cm.DoContextMenu(this, rc.GetLeft(), rc.GetBottom());

							if(cmd==0) {
								*_tlid = L"";
								*_cid = L"";
							}
							else if(cmd>0 && commands.find(cmd)!=commands.end()) {
								std::pair< ref<Instance>, ref<Cue> > data = commands[cmd];
								*_tlid = data.first->GetTimeline()->GetID();
								*_cid = data.second->GetID();
							}
							Repaint();
						}
						else if(ev==MouseEventMove||ev==MouseEventLeave) {
							Repaint();
						}
					}

					int PopulateCueMenu(strong<Menu> parent, strong<Instance> controller, std::map<int, std::pair< ref<Instance>, ref<Cue> > >* commands, int& index) {
						int numItems = 0;
						parent->AddSeparator(controller->GetTimeline()->GetName());
						
						// Enumerate tracks for this controller and find all sub timelines
						ref<Iterator<ref<TrackWrapper> > > it = controller->GetTimeline()->GetTracks();
						while(it->IsValid()) {
							ref<TrackWrapper> tw = it->Get();
							ref<SubTimeline> sub = tw->GetSubTimeline();

							if(sub) {
								strong<Menu> subMenu = GC::Hold(new SubMenuItem(tw->GetInstanceName(), false, MenuItem::NotChecked, null));
								if(PopulateCueMenu(subMenu, sub->GetInstance(), commands, index)>0) {
									parent->AddItem(subMenu);
									++numItems;
								}
							}
							it->Next();
						}
						
						// Enumerate own cues
						ref<CueList> list = controller->GetCueList();
						std::vector< ref<Cue> >::iterator itc = list->GetCuesBegin();
						while(itc!=list->GetCuesEnd()) {
							ref<Cue> cue = *itc;
							
							if(cue->IsPrivate()) {
								++itc; 
								continue; 
							}

							std::wstring name = cue->GetName();
							if(name.length()>0) {
								strong<MenuItem> mi = GC::Hold(new MenuItem(name, index, false, (*_cid==cue->GetID()) ? MenuItem::Checked : MenuItem::NotChecked));
								parent->AddItem(mi);
								commands->operator[](index) = std::pair< ref<Instance>, ref<Cue> >(controller, cue);
								++index;
								++numItems;
							}
							++itc;
						}

						return numItems;
					}

					Icon _linkIcon;
					std::wstring* _tlid;
					std::wstring* _cid;
					weak<Timeline> _cachedTimeline;
					weak<Cue> _cachedCue;
			};
		}
	}
}

/** DistantCueProperty **/
DistantCueProperty::DistantCueProperty(const std::wstring& name, ref<CueList> list, std::wstring* tlid, std::wstring* cid): Property(name) {
	assert(cid!=0);

	_list = list;
	_tlid = tlid;
	_cid = cid;
}

DistantCueProperty::~DistantCueProperty() {
}

ref<Wnd> DistantCueProperty::GetWindow() {
	if(!_wnd) {
		_wnd = GC::Hold(new DistantCuePropertyWnd(_tlid, _cid));
	}
	return _wnd;
}

// Called when a repaint is about to begin and the value in the window needs to be updated
void DistantCueProperty::Update() {
	if(_wnd) {
		_wnd->Update();
	}
}

Pixels DistantCueProperty::GetHeight() {
	return 17;
}