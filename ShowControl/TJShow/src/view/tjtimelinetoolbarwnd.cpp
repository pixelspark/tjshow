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
#include "../../include/internal/view/tjtimelinetoolbarwnd.h"
#include "../../include/internal/view/tjtimelinewnd.h"
#include "../../include/internal/view/tjdialogs.h"
#include "../../include/internal/tjsubtimeline.h"
#include "../../include/internal/view/tjtricks.h"

#include <windowsx.h>

using namespace tj::shared::graphics;
using namespace tj::show::view;

TimelineToolbarWnd::TimelineToolbarWnd(TimelineWnd* tw) {
	_timeline = tw;

	// track add/remove/move buttons
	Add(GC::Hold(new ToolbarItem(CommandAdd, L"icons/add.png", TL(toolbar_add_track))));
	Add(GC::Hold(new ToolbarItem(CommandRemove, L"icons/remove.png", TL(toolbar_remove_track))));
	Add(GC::Hold(new ToolbarItem(CommandUp, L"icons/up.png", TL(toolbar_move_track_up))));
	ref<ToolbarItem> down = GC::Hold(new ToolbarItem(CommandDown, L"icons/down.png", TL(toolbar_move_track_down)));
	down->SetSeparator(true);
	Add(down);

	// view flag buttons
	Add(GC::Hold(new ToolbarItem(CommandViewSettings, L"icons/viewsettings.png", TL(toolbar_view_settings))));

	// zoom
	_zoomItem = GC::Hold(new ToolbarItem(CommandZoom, L"icons/zoom.png", TL(toolbar_zoom)));
	_zoomItem->SetSeparator(true);
	Add(_zoomItem);

	// insert buttons
	Add(GC::Hold(new ToolbarItem(CommandInsertAll, L"icons/record.png", TL(toolbar_insert))));
	Add(GC::Hold(new ToolbarItem(CommandInsertOne, L"icons/record_one.png", TL(toolbar_insert_one))));
	Add(GC::Hold(new ToolbarItem(CommandRunModes, L"icons/toolbar/run.png", TL(toolbar_set_runmodes))));
	Add(GC::Hold(new ToolbarItem(CommandProperties, L"icons/toolbar/properties.png", TL(toolbar_timeline_properties))));

	// icons
	_playItem = GC::Hold(new ToolbarItem(CommandPlay, L"icons/play.png", TL(timeline_play), false));
	_stopItem = GC::Hold(new ToolbarItem(CommandStop, L"icons/stop.png", TL(timeline_stop), false));
	_pauseItem = GC::Hold(new ToolbarItem(CommandPause, L"icons/pause.png", TL(timeline_pause), false));
	_restartItem = GC::Hold(new ToolbarItem(CommandRestart, L"icons/restart.png", TL(timeline_restart), false));
	_timeItem = GC::Hold(new TimelineTimeToolbarItem(CommandPlayStop));

	
	Add(_timeItem, true);
	Add(_restartItem, true);
	Add(_stopItem, true);
	Add(_pauseItem, true);
	Add(_playItem,true);

	_stopItem->SetEnabled(false);
	_pauseItem->SetEnabled(false);
	_isDraggingZoom = false;

	// Set tip stuff
	SetTip(Tricks::LoadTricksFromFile(L"resources/help/timeline-wnd.xml"));
}

TimelineToolbarWnd::~TimelineToolbarWnd() {
}

void TimelineToolbarWnd::OnSearchChange(const std::wstring& q) {
	_timeline->SetFilter(q);
}

void TimelineToolbarWnd::OnCommand(ref<ToolbarItem> ti) {
	OnCommand(ti->GetCommand());
}

void TimelineToolbarWnd::OnCommand(int c) {
	switch(c) {
		case CommandAdd: {
			ContextMenu cm;
			enum {KCAddTrack = 1, KCAddMulti = 4, KCAddFromFile = 2, KCAddTimeline = 3};
			cm.AddItem(TL(toolbar_add_track), KCAddTrack, true, false);
			cm.AddItem(TL(toolbar_add_multi), KCAddMulti, false, false);
			cm.AddItem(TL(toolbar_add_track_from_file), KCAddFromFile, false, false);

			Area rc = GetClientArea();
			int r = cm.DoContextMenu(this,0, rc.GetHeight()-1);
			if(r==KCAddTrack) {
				Dialogs::AddTrack(_timeline, _timeline->_controller,false);
			}
			else if(r==KCAddMulti) {
				Dialogs::AddTrack(_timeline, _timeline->_controller,true);
			}
			else if(r==KCAddFromFile) {
				Dialogs::AddTrackFromFile(_timeline, _timeline->_controller);
			}

			break;
		}
		case CommandRemove: {
			ref<TrackWrapper> currentTrack = _timeline->_currentTrack;
			if(currentTrack) {
				_timeline->RemoveTrack(currentTrack);
			}
			else {
				Alert::Show(TL(application_name), TL(no_track_selected), Alert::TypeError);
			}
			break;
		}
			
		case CommandUp: {
			ref<TrackWrapper> currentTrack = _timeline->_currentTrack;
			if(currentTrack) {
				_timeline->_timeline->MoveUp(currentTrack);
			}
			break;
		}
		
		case CommandDown: {
			ref<TrackWrapper> currentTrack = _timeline->_currentTrack;
			if(currentTrack) {
				_timeline->_timeline->MoveDown(currentTrack);
			}
			break;
		}

		case CommandViewSettings: {
			ContextMenu cm;

			enum { KCMSelect = 1, KCMPointer, KCMLock, KCMAddresses };
			cm.AddItem(GC::Hold(new MenuItem(TL(timeline_pointer), KCMPointer, false, _timeline->IsInSelectionMode() ? MenuItem::NotChecked : MenuItem::RadioChecked)));
			cm.AddItem(GC::Hold(new MenuItem(TL(timeline_select), KCMSelect, false, _timeline->IsInSelectionMode() ? MenuItem::RadioChecked : MenuItem::NotChecked)));
			cm.AddSeparator();
			cm.AddItem(TL(timeline_follow), KCMLock, false, _timeline->IsLocked());
			cm.AddItem(TL(timeline_show_addresses), KCMAddresses, false, _timeline->GetShowAddresses());

			Area rc = GetClientArea();
			int r = cm.DoContextMenu(this, GetButtonX(CommandViewSettings), rc.GetHeight()-1);
			switch(r) {
				case KCMPointer:
					_timeline->SetSelectionMode(false);
					break;

				case KCMSelect:
					_timeline->SetSelectionMode(true);
					break;

				case KCMLock:
					_timeline->SetLock(!_timeline->IsLocked());
					break;

				case KCMAddresses:
					_timeline->SetShowAddresses(!_timeline->GetShowAddresses());
					break;
			}
			break;						  
		}

		case CommandPlayStop: {
			PlaybackState cp = _timeline->_controller->GetPlaybackState();
			PlaybackState np;
			switch(cp) {
				case PlaybackPlay:
					np = PlaybackStop;
					break;

				default:
					np = PlaybackPlay;
			}
			_timeline->_controller->SetPlaybackState(np);
			break;
		}

		case CommandStop:
			_timeline->_controller->SetPlaybackState(PlaybackStop);
			break;

		case CommandPlay:
			_timeline->_controller->SetPlaybackState(PlaybackPlay);
			break;

		case CommandPause: {
			PlaybackState cp = _timeline->_controller->GetPlaybackState();
			if(cp==PlaybackPlay) {
				_timeline->_controller->SetPlaybackState(PlaybackPause);
			}
			break;
		}

		case CommandRestart:
			_timeline->_controller->Jump(Time(0));
			break;

		case CommandInsertAll:
			_timeline->InsertFromControls();
			break;

		case CommandInsertOne:
			_timeline->InsertFromCurrent();
			break;

		case CommandRunModes: {
			strong<Theme> theme = ThemeManager::GetTheme();
			Area rc = GetClientArea();
			ContextMenu cm;

			if(_timeline->_controller->GetPlaybackState() != PlaybackStop) {
				cm.AddItem(TL(cannot_change_run_mode_while_playing), -1, false, false);
				cm.DoContextMenu(this, GetButtonX(CommandRunModes),rc.GetBottom());
			}
			else {			
				cm.AddSeparator(TL(visible_tracks));
				cm.AddItem(TL(runmode_none), int(RunModeDont)+1);
				cm.AddItem(TL(runmode_master), int(RunModeMaster)+1);
				cm.AddItem(TL(runmode_client), int(RunModeClient)+1);
				cm.AddItem(TL(runmode_both), int(RunModeBoth)+1);
				
				int m = cm.DoContextMenu(this, GetButtonX(CommandRunModes),rc.GetBottom());
				if(m>0) {
					RunMode c = (RunMode)(m-1);

					ref< Iterator< ref<TrackWrapper> > > it = _timeline->GetTimeline()->GetTracks();
					while(it->IsValid()) {
						ref<TrackWrapper> track = it->Get();
						if(!_timeline->MatchesFilter(track)) {it->Next(); continue; }
						track->SetRunMode(c);
						it->Next();
					}
				}
				break;
			}
		}

		case CommandProperties: {
			ref<Path> p = _timeline->GetTimeline()->CreatePath();
			Application::Instance()->GetView()->Inspect(_timeline->GetTimeline(),p);
			break;
		}
	}

	_timeline->Update();
}

void TimelineToolbarWnd::Update() {
	UpdateThreaded();
	Layout();
}

void TimelineToolbarWnd::UpdateThreaded() {
	// current time
	if(_timeItem) {
		std::wstring formattedTime = _timeline->_controller->GetTime().Format();
		_timeItem->SetTime(formattedTime);
	}

	if(_pauseItem && _playItem && _stopItem) {
		PlaybackState state = _timeline->_controller->GetPlaybackState();
		if(state==PlaybackStop) {
			_pauseItem->SetEnabled(false);
			_stopItem->SetEnabled(false);
			_playItem->SetEnabled(true);
		}
		else if(state==PlaybackPlay) {
			_pauseItem->SetEnabled(true);
			_stopItem->SetEnabled(true);
			_playItem->SetEnabled(false);
		}
		else if(state==PlaybackPause) {
			_pauseItem->SetEnabled(false);
			_stopItem->SetEnabled(true);
			_playItem->SetEnabled(true);
		}
	}
	Repaint();
}

LRESULT TimelineToolbarWnd::Message(UINT msg, WPARAM wp, LPARAM lp) {
	return SearchToolbarWnd::Message(msg, wp, lp);
}

void TimelineToolbarWnd::OnMouse(MouseEvent ev, Pixels x, Pixels y) {
	if(ev==MouseEventLDown) {
		Area rc = GetClientArea();
		Area frc = GetFreeArea();
		strong<Theme> theme = ThemeManager::GetTheme();
		Pixels buttonSize = theme->GetMeasureInPixels(Theme::MeasureToolbarHeight);

		int idx = x/buttonSize;
		if(idx>=0 && idx < (int)_items.size()) {
			ref<ToolbarItem> item = _items.at(idx);
			if(item->GetCommand()==CommandZoom) {
				// zoom button
				_capture.StartCapturing(Mouse::Instance(), ref<Wnd>(this));
				Mouse::Instance()->SetCursorType(CursorSizeNorthSouth);
				_isDraggingZoom = true;
				_zoomX = x;
				_zoomY = y;
				Update();
				return;
			}
		}
	}
	else if(ev==MouseEventMove && _isDraggingZoom) {
		if(_isDraggingZoom) {
			int yDiff = int(_zoomY)-y;
			
			Time old = _timeline->_timeWidth;
			_timeline->_timeWidth += yDiff*250;
			if(_timeline->_timeWidth>_timeline->_timeline->GetTimeLengthMS()) {
				_timeline->_timeWidth = _timeline->_timeline->GetTimeLengthMS();
			}

			if(_timeline->_timeWidth<Time(10)) {
				_timeline->_timeWidth = Time(10);
			}
			
			Time currentTime = _timeline->_controller->GetTime();
			Time diff = Time(int(float(currentTime.ToInt()-int(_timeline->GetHorizontalPos()))/float(old)*float(_timeline->_timeWidth)));
			int val = max(0, Time(currentTime-diff).ToInt());
			_timeline->SetHorizontalPos(val);

			Time endTime = _timeline->_timeWidth + Time(_timeline->GetHorizontalPos());
			if(endTime>_timeline->_timeline->GetTimeLengthMS()) {
				_timeline->SetHorizontalPos(_timeline->_timeline->GetTimeLengthMS()-_timeline->_timeWidth);
			}
			
			_zoomX = x;
			_zoomY = y;
			_timeline->Update(); // will call Update() in toolbar
			return;
		}
	}
	else if(ev==MouseEventLUp) {
		_isDraggingZoom = false;
		_timeline->Layout(); // causes scroll info update
		_capture.StopCapturing();
		Mouse::Instance()->SetCursorType(CursorDefault);
	}

	SearchToolbarWnd::OnMouse(ev,x,y);
}

TimelineTimeToolbarItem::TimelineTimeToolbarItem(int command): ToolbarItem(command,0,TL(timeline_time), true) {
	strong<Theme> theme = ThemeManager::GetTheme();
	SetPreferredSize(87, theme->GetMeasureInPixels(Theme::MeasureToolbarHeight));
}

TimelineTimeToolbarItem::~TimelineTimeToolbarItem() {
}

void TimelineTimeToolbarItem::Paint(graphics::Graphics& g, strong<Theme> theme, bool over, bool down, float alpha) {
	Area rc = GetClientArea();
	DrawToolbarButton(g, rc, theme, over, down, IsSeparator(), IsEnabled(), alpha);

	rc.Narrow(0,0,3,0);
	SolidBrush textBrush(theme->GetColor(Theme::ColorText));
	StringFormat fm;
	fm.SetAlignment(StringAlignmentFar);
	fm.SetLineAlignment(StringAlignmentCenter);
	g.DrawString(_time.c_str(), (unsigned int)_time.length(), theme->GetGUIFontBold(), rc, &fm, &textBrush);

}

void TimelineTimeToolbarItem::SetTime(const std::wstring& time) {
	_time = time;
}