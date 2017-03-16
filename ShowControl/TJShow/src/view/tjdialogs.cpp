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
#include "../../include/internal/tjnetwork.h"
#include "../../include/internal/view/tjdialogs.h"
#include <commctrl.h>
#include <limits.h>
#include <windowsx.h>

using namespace tj::show::view;

void Dialogs::AddTrackFromFile(ref<Wnd> owner, strong<Instance> controller) {
	std::wstring fn = Dialog::AskForOpenFile(owner, TL(add_track_from_file), L"TJShow track (*.ttx)\0*.ttx\0\0", L"ttx");
	if(fn.length()>0) {
		TiXmlDocument doc;
		if(doc.LoadFile(Mbs(fn))) {
			TiXmlElement* track = doc.FirstChildElement("track");

			const char* hashString = track->Attribute("plugin");
			if(hashString==0) return;
			unsigned int hash = StringTo<unsigned int>(std::string(hashString), 0);

			ref<PluginWrapper> pw = PluginManager::Instance()->GetPluginByHash((PluginHash)hash);
			if(pw) {
				ref<TrackWrapper> tw = pw->CreateTrack(controller->GetPlayback(), Application::Instance()->GetNetwork(), controller);
				tw->Load(track);
				tw->Clone(); // for resetting ID's (note that this is done *after* the call to Load, since that could load id's from the file)
				
				controller->GetTimeline()->AddTrack(tw);
				Application::Instance()->GetView()->OnAddTrack(tw);
			}
		}
	}
}

void Dialogs::AddTrack(ref<Wnd> owner, strong<Instance> tl, bool multi) {
	// Data object for holding entered data
	class AddTrackDialogData: public Inspectable {
		public:
			AddTrackDialogData(bool multi): _count(1), _multi(multi) {}
			virtual ~AddTrackDialogData() {}
			
			virtual ref<PropertySet> GetProperties() {
				ref<PropertySet> ps = GC::Hold(new PropertySet());

				if(_multi) {
					ps->Add(GC::Hold(new TextProperty(TL(track_names), this, &_name, 200)));
				}
				else {
					ps->Add(GC::Hold(new GenericProperty<std::wstring>(TL(track_name), this, &_name, _name)));
				}
				
				// Enumerate track types and add them as choices to the list property
				ref< GenericListProperty<PluginHash> > lp = GC::Hold(new GenericListProperty<PluginHash>(TL(track_type), this, &_type, 0));
				std::map<PluginHash, ref<PluginWrapper> >* plugins = PluginManager::Instance()->GetPluginsByHash();
				std::map<PluginHash, ref<PluginWrapper> >::iterator it = plugins->begin();

				std::map< std::wstring, std::vector< ref<PluginWrapper> > > byCategory;

				while(it!=plugins->end()) {
					ref<PluginWrapper> pluginWrapper = (*it).second;
					if(pluginWrapper->IsOutputPlugin()) {
						std::wstring cat = pluginWrapper->GetPlugin()->GetFriendlyCategory();
						std::vector< ref<PluginWrapper> >& list = byCategory[cat];
						list.push_back(pluginWrapper);
					}
					++it;
				}

				lp->AddOption(L"", 0);
				std::map< std::wstring, std::vector< ref<PluginWrapper> > >::iterator mit = byCategory.begin();
				while(mit!=byCategory.end()) {
					if(mit->first.length()>0) {
						lp->AddSeparator(mit->first);
					}
					std::vector< ref<PluginWrapper> >::iterator vit = mit->second.begin();
					while(vit!=mit->second.end()) {
						ref<PluginWrapper> pw = *vit;
						if(pw) {
							lp->AddOption(pw->GetFriendlyName(), pw->GetHash());
						}
						++vit;
					}
					++mit;
				}

				ps->Add(lp);

				if(!_multi) {
					ps->Add(GC::Hold(new PropertySeparator(TL(advanced), !_multi)));
					ps->Add(GC::Hold(new GenericProperty<int>(TL(track_count), this, &_count, _count)));
				}
				return ps;
			}

			std::wstring _name;
			PluginHash _type;
			int _count;
			bool _multi;
	};

	// Create data object instance
	ref<AddTrackDialogData> atdd = GC::Hold(new AddTrackDialogData(multi));

	// Create and run dialog
	ref<PropertyDialogWnd> dw = GC::Hold(new PropertyDialogWnd(TL(track_add_dialog), TL(track_add_dialog_question)));
	dw->GetPropertyGrid()->Inspect(atdd);
	
	// Set size, multi dialog should be larger
	if(multi) {
		dw->SetSize(400, 400);
	}
	else {
		dw->SetSize(400, 200);
	}

	if(dw->DoModal(owner)) {
		// Record each track we add in an UndoBlock, so it can be undone
		class AddTrackChange: public Change {
			public:
				AddTrackChange(ref<Timeline> tl, ref<TrackWrapper> tw): Change(TL(change_add_track)), _tl(tl), _tw(tw) {}
				virtual ~AddTrackChange() {}
				virtual bool CanUndo() { return true; }
				virtual bool CanRedo() { return true; }

				virtual void Redo() {
					strong<Application> app = Application::Instance();
					_tl->AddTrack(_tw);
					ref<View> view = app->GetView();	
					if(view) {
						view->OnAddTrack(_tw);
					}
					Application::Instance()->Update();
				}

				virtual void Undo() {
					strong<Application> app = Application::Instance();
					ref<View> view = app->GetView();	
					if(view) {
						view->OnRemoveTrack(_tw);
					}
					_tl->RemoveTrack(_tw);
					Application::Instance()->Update();
					
				}

			protected:
				ref<TrackWrapper> _tw;
				ref<Timeline> _tl;
		};

		if(atdd->_name.length()<1) {
			Alert::Show(TL(error), TL(no_name_given), Alert::TypeError);
			return;
		}

		UndoBlock ub;

		if(multi) {
			// Split name
			std::vector<std::wstring> names = Explode<std::wstring>(atdd->_name, L"\r\n");
			std::vector<std::wstring>::const_iterator it = names.begin();
			while(it!=names.end()) {
				const std::wstring& name = *it;
				ref<TrackWrapper> tw = Application::Instance()->CreateTrack(atdd->_type, tl);
				if(!tw) {
					Alert::Show(TL(error), TL(couldnt_create_track), Alert::TypeError);
					return;
				}
				UndoBlock::AddChange(GC::Hold(new AddTrackChange(tl->GetTimeline(), tw)));
				tw->SetInstanceName(name);
				++it;
			}
		}
		else {
			if(atdd->_count<0) {
				Alert::Show(TL(error), TL(track_count_cannot_be_negative), Alert::TypeError);
			}

			for(int a=0;a<atdd->_count;a++) {
				std::wstring trackName = atdd->_name;
				if(atdd->_count>1) {
					trackName += L" ("+Stringify(a+1)+L")"; 
				}

				ref<TrackWrapper> tw = Application::Instance()->CreateTrack(atdd->_type, tl);
				if(!tw) {
					Alert::Show(TL(error), TL(couldnt_create_track), Alert::TypeError);
					return;
				}
				UndoBlock::AddChange(GC::Hold(new AddTrackChange(tl->GetTimeline(), tw)));
				tw->SetInstanceName(trackName);
			}
		}
	}
}
