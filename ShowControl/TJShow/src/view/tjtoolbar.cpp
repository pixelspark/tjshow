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
#include "../../include/internal/tjcontroller.h"
#include "../../include/internal/view/tjeventlogwnd.h"
#include "../../include/internal/view/tjtoolbar.h"
#include "../../include/internal/view/tjtimelinewnd.h"
#include "../../include/internal/view/tjsplittimelinewnd.h"
#include "../../include/internal/view/tjtimelinetreewnd.h"
#include <windowsx.h>

using namespace tj::shared::graphics;
using namespace tj::show;
using namespace tj::show::view;
using namespace tj::shared;

ApplicationToolbarWnd::ApplicationToolbarWnd(): _primaryIcon(L"icons/primary.png"), _promotingIcon(L"icons/promoting.png") {	
	ref<ToolbarItem> newItem = GC::Hold(new ToolbarItem(ID_NEW, L"icons/toolbar/new.png", TL(toolbar_new) ));
	Add(newItem);

	ref<ToolbarItem> openItem = GC::Hold(new ToolbarItem(ID_OPEN, L"icons/toolbar/open.png", TL(toolbar_open) ));
	Add(openItem);

	ref<ToolbarItem> saveItem = GC::Hold(new ToolbarItem(ID_SAVE, L"icons/toolbar/save.png", TL(toolbar_save) ));
	Add(saveItem);

	ref<ToolbarItem> saveasItem = GC::Hold(new ToolbarItem(ID_SAVEAS, L"icons/toolbar/saveas.png", TL(toolbar_save_as) ));
	Add(saveasItem);

	ref<ToolbarItem> modelPropertiesItem = GC::Hold(new ToolbarItem(ID_MODEL_PROPERTIES, L"icons/toolbar/properties.png", TL(toolbar_doc_properties)));
	modelPropertiesItem->SetSeparator(true);
	Add(modelPropertiesItem);

	_undoItem = GC::Hold(new ToolbarItem(ID_UNDO, L"icons/toolbar/undo.png", TL(undo), false));
	_undoItem->SetEnabled(false);
	Add(_undoItem);

	_redoItem = GC::Hold(new ToolbarItem(ID_REDO, L"icons/toolbar/redo.png", TL(redo), true));
	_redoItem->SetEnabled(false);
	Add(_redoItem);

	ref<ToolbarItem> settingsItem = GC::Hold(new ToolbarItem(ID_SETTINGS, L"icons/toolbar/settings.png", TL(toolbar_settings)));
	Add(settingsItem);

	ref<ToolbarItem> varsItem = GC::Hold(new ToolbarItem(ID_VARIABLES, L"icons/variables.png", TL(variables)));
	Add(varsItem);

	ref<ToolbarItem> capItem = GC::Hold(new ToolbarItem(ID_CAPACITIES, L"icons/capacity.png", TL(capacities)));
	Add(capItem);
	
	ref<ToolbarItem> prefsItem = GC::Hold(new ToolbarItem(ID_PREFERENCES, L"icons/toolbar/preferences.png", TL(toolbar_preferences)));
	Add(prefsItem);

	ref<ToolbarItem> themeItem = GC::Hold(new ToolbarItem(ID_THEME, L"icons/theme.png", TL(toolbar_theme), true));
	Add(themeItem);

	ref<ToolbarItem> globalStopItem = GC::Hold(new ToolbarItem(ID_GLOBAL_STOP, L"icons/stop.png", TL(toolbar_global_stop) ));
	Add(globalStopItem);

	ref<ToolbarItem> globalPauseItem = GC::Hold(new ToolbarItem(ID_GLOBAL_PAUSE, L"icons/pause.png", TL(toolbar_global_pause) ));
	Add(globalPauseItem);

	ref<ToolbarItem> globalResumeItem = GC::Hold(new ToolbarItem(ID_GLOBAL_RESUME, L"icons/resume.png", TL(toolbar_global_resume) ));
	Add(globalResumeItem);

	_globalPlayItem = GC::Hold(new ToolbarItem(0, L"icons/play.png", TL(toolbar_global_play)));
	Add(_globalPlayItem);

	_promoteItem = GC::Hold(new ToolbarItem(0, L"icons/primary.png", TL(toolbar_promote), false));
	Add(_promoteItem, true);

	_messagesItem = GC::Hold(new ToolbarItem(0, L"icons/toolbar/notify.png", TL(event_log), false));
	_messagesItem->SetSeparator(true);
	Add(_messagesItem, true);

	Add(GC::Hold(new LogoToolbarItem(L"icons/toolbar/logo.png", 43, ID_ABOUT)), true);
	
	SetTimer(GetWindow(), 1337, 10000, NULL);
	SetWantMouseLeave(true);
	UnsetStyle(WS_CLIPCHILDREN|WS_CLIPSIBLINGS);
}

ApplicationToolbarWnd::~ApplicationToolbarWnd() {
	KillTimer(GetWindow(), 1337);
}

void ApplicationToolbarWnd::OnCreated() {
	ToolbarWnd::OnCreated();

	// hook into Network class to update toolbar when promoted/demoted
	ref<Network> net = Application::Instance()->GetNetwork();
	if(net) {
		net->EventPromoted.AddListener(this);
		net->EventDemoted.AddListener(this);
	}

	strong<UndoStack> us = UndoStack::Instance();
	us->EventUndoChange.AddListener(ref<ApplicationToolbarWnd>(this));
}

void ApplicationToolbarWnd::Notify(ref<Object> source, const Network::Notification& not) {
	Update();
}

void ApplicationToolbarWnd::Notify(ref<Object> source, const UndoStack::UndoNotification& notification) {
	ref<UndoStack> us = source;
	if(us) {
		_redoItem->SetEnabled(us->GetRedoStepsAvailableCount() > 0);
		_undoItem->SetEnabled(us->GetUndoStepsAvailableCount() > 0);
		Repaint();
	}
}

void ApplicationToolbarWnd::OnTimer(unsigned int id) {
	Update();
	ToolbarWnd::OnTimer(id);
}

void ApplicationToolbarWnd::Update() {
	ref<EventLogWnd> ew = Application::Instance()->GetView()->GetEventLogger();
	_messagesItem->SetActive(ew && ew->HasUnreadMessages());
	_promoteItem->SetActive(Application::Instance()->GetNetwork()->IsPrimaryMaster());
	Repaint();
}

void ApplicationToolbarWnd::DoPromoteMenu() {
	ref<Network> net = Application::Instance()->GetNetwork();
	
	if(net->GetRole()==RoleMaster) {
		bool ispm = net->IsPrimaryMaster();
		ContextMenu cm;
		enum { KCPromote = 1, KCDemote = 2, KCForcePromote = 3, KCCancelPromote };

		cm.AddSeparator(ispm ? TL(is_primary_master) : TL(is_secondary_master));
		if(ispm) {
			cm.AddItem(GC::Hold(new MenuItem(TL(demote), KCDemote, false, MenuItem::NotChecked, L"icons/demoting.png")));
		}	
		else {
			if(net->IsPromotionInProgress()) {
				cm.AddItem(TL(promotion_in_progress), -1, false, false);
				cm.AddItem(GC::Hold(new MenuItem(TL(promote_cancel), KCCancelPromote, false, MenuItem::NotChecked, L"icons/secondary.png")));
			}
			else {
				cm.AddItem(GC::Hold(new MenuItem(TL(promote), KCPromote, false, MenuItem::NotChecked, L"icons/promoting.png")));
			}
			cm.AddItem(GC::Hold(new MenuItem(TL(promote_force), KCForcePromote, false, MenuItem::NotChecked, L"icons/promotion-force.png")));
		}

		Area brc = _promoteItem->GetClientArea();
		int r = cm.DoContextMenu(this, brc.GetLeft(),brc.GetBottom());
		if(r==KCPromote) {
			net->Promote();
		}
		else if(r==KCDemote) {
			net->Demote(false);
		}
		else if(r==KCForcePromote) {
			net->ForcePromotion();
		}
		else if(r==KCCancelPromote) {
			net->CancelPromotion();
		}
	}
}

void ApplicationToolbarWnd::OnCommand(ref<ToolbarItem> ti) {
	if(ti==_promoteItem) {
		DoPromoteMenu();
	}
	else if(ti==_messagesItem) {
		Application::Instance()->GetView()->Command(ID_EVENT_LOG);
	}
	else if(ti==_globalPlayItem) {
		OnCommand(ID_GLOBAL_PLAY);
	}
	else {
		OnCommand(ti->GetCommand());
	}
}

void ApplicationToolbarWnd::OnCommand(int c) {
	ref<View> view = Application::Instance()->GetView();

	if(c==ID_THEME) {
		ContextMenu cm;
		std::vector< ref<Theme> > themes;
		ThemeManager::ListThemes(themes);

		std::vector< ref<Theme> >::iterator it = themes.begin();
		int i = 1;
		ref<Theme> currentTheme = ThemeManager::GetTheme();

		while(it!=themes.end()) {
			ref<Theme> theme = *it;
			if(theme) {
				cm.AddItem(theme->GetName(), i, false, (currentTheme==theme)?MenuItem::RadioChecked:MenuItem::NotChecked);
				++i;
			}
			++it;
		}

		Area rc = GetClientArea();
		int r = cm.DoContextMenu(this, GetButtonX(ID_THEME), rc.GetBottom());
		if(r>0 && (r-1)<int(themes.size())) {
			ref<Theme> select = themes.at(r-1);
			if(select) {
				ThemeManager::SelectTheme(strong<Theme>(select));
			}
		}

		Application::Instance()->OnThemeChange();
	}
	else if(c==ID_UNDO) {
		UndoStack::Instance()->Undo(1);
		Application::Instance()->Update();
		view->GetRootWindow()->FullRepaint();
	}
	else if(c==ID_REDO) {
		UndoStack::Instance()->Redo(1);
		Application::Instance()->Update();
		view->GetRootWindow()->FullRepaint();
	}
	else {
		view->Command(c);
	}
}