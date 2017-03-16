#include "../../include/internal/tjshow.h"
#include "../../include/internal/tjcontroller.h"
#include "../../include/internal/view/tjtimelinewnd.h"
#include "../../include/internal/view/tjsplittimelinewnd.h"
#include "../../include/internal/view/tjtimelinetoolbarwnd.h"
#include "../../include/internal/view/tjcuelistwnd.h"
#include "../../include/internal/tjsubtimeline.h"
#include "../../include/internal/view/tjdialogs.h"

#include <iomanip>
#include <algorithm>
#include <windowsx.h>
#include <math.h>

using namespace tj::shared::graphics;
using namespace tj::show::view;

TimelineWnd::TimelineWnd(strong<Instance> inst):
	_controller(inst),
	_arrowIcon(L"icons/arrow.png"),
	_runIcon(L"icons/run.png"),
	_norunIcon(L"icons/norun.png"),
	_bothrunIcon(L"icons/runboth.png"),
	_collapseIcon(L"icons/shared/collapse.png"),
	_expandIcon(L"icons/shared/expand.png"),
	_lockedIcon(L"icons/locked.png"),
	_isDraggingSplitter(false),
	_timeWidth(KDefaultTimeWidth),
	_nameColumnWidth(150),
	_isInterpolating(false),
	_showAddresses(false),
	_selStart(-1),
	_selEnd(-1),
	_selDrag(SelectionDragNone),
	_selMode(false),
	_zoomSelMode(false),
	_isLocked(false),
	_timeline(inst->GetTimeline()),
	_cues(inst->GetCueList()) {

	SetText(TL(timeline));
	_toolbar = GC::Hold(new TimelineToolbarWnd(this));
	Add(_toolbar);

	_timeWidth = _timeline->GetTimeLengthMS();
	SetHorizontallyScrollable(true);
	SetHorizontalScrollInfo(Range<int>(0,_timeline->GetTimeLengthMS().ToInt()), _timeWidth.ToInt());
	SetVerticallyScrollable(true);

	ref<ResourceManager> rm = ResourceManager::Instance();
	/* Tooltip */
	_tt = GC::Hold(new TooltipWnd(GetWindow()));
	SetVerticallyScrollable(true);
	SetHorizontallyScrollable(true);
	Layout();
} 

TimelineWnd::~TimelineWnd() {
}

void TimelineWnd::SetCurrentTrack(ref<TrackWrapper> tw) {
	/** We reset the focused item here to make OnCopy do it's job better: when a track is
	selected, but not an item, OnCopy puts the whole track on the clipboard. Else, OnCopy will
	put the currently focused item on it **/
	_currentTrack = tw;
	_focusedItem = null;
	Repaint();
}

void TimelineWnd::RemoveTrack(ref<TrackWrapper> tw, bool askConfirmation) {
	class RemoveTrackChange: public Change {
		public:
			RemoveTrackChange(ref<TrackWrapper> tw, ref<Timeline> tl): Change(TL(change_remove_track)), _tw(tw), _tl(tl) {
			}
			
			virtual ~RemoveTrackChange() {
			}

			virtual void Redo() {
				strong<Application> app = Application::Instance();
				ref<View> view = app->GetView();	
				if(view) {
					view->OnRemoveTrack(_tw);
				}
				_tl->RemoveTrack(_tw);
				Application::Instance()->Update();
			}

			virtual void Undo() {
				strong<Application> app = Application::Instance();
				_tl->AddTrack(_tw);
				ref<View> view = app->GetView();	
				if(view) {
					view->OnAddTrack(_tw);
				}
				Application::Instance()->Update();
			}

			virtual bool CanUndo() { return true; }
			virtual bool CanRedo() { return true; }

		protected:
			ref<TrackWrapper> _tw;
			ref<Timeline> _tl;
	};

	if(!askConfirmation || Alert::ShowYesNo(TL(application_name), TL(confirm_remove_track), Alert::TypeQuestion)) {
		UndoBlock ub;
		UndoBlock::AddAndDoChange(GC::Hold(new RemoveTrackChange(tw, _timeline)));

		if(ref<TrackWrapper>(_currentTrack)==tw) {
			SetCurrentTrack(null);
		}
		
		Update();
	}
}

bool TimelineWnd::IsInSelectionMode() const {
	return _selMode;
}

void TimelineWnd::SetSelectionMode(bool s) {
	_selMode = s;
	_draggedItem = null;
	_draggedItemTrack = null;

	if(!_selMode) {
		_selStart = Time(-1);
		_selEnd = Time(-1);
	}
}

bool TimelineWnd::GetShowAddresses() const {
	return _showAddresses;
}

void TimelineWnd::SetShowAddresses(bool s) {
	_showAddresses = s;
	Update();
}

int TimelineWnd::GetZoom() {
	return int(_timeWidth);
}

bool TimelineWnd::MatchesFilter(ref<TrackWrapper> tr) {
	if(!tr) return false;
	if(_filterText.length()<=0) return true;

	std::wstring trackName = tr->GetInstanceName() + L" " + tr->GetTrack()->GetTypeName();
	std::transform(trackName.begin(), trackName.end(), trackName.begin(), tolower);
	return trackName.find(_filterText, 0)!=std::string::npos;
}

void TimelineWnd::Update() {
	_toolbar->Update();
	UpdateTrackingPosition();
	Repaint();
}

void TimelineWnd::UpdateThreaded() {
	_toolbar->UpdateThreaded();
	Repaint();
}

std::wstring TimelineWnd::GetTabTitle() const {
	std::wstring tn = _timeline->GetName();
	if(tn.length()<1) {
		return TL(timeline);
	}
	return tn;
}

ref<Icon> TimelineWnd::GetTabIcon() const {
	return PlaybackStateIcons::GetTabIcon(_controller->GetPlaybackState());
}

void TimelineWnd::SetLock(bool l) {
	_isLocked = l;
	if(l) {
		StartTimer(Time(20), KFollowTimerID);
	}
	else {
		StopTimer(KFollowTimerID);
	}
	Update();
}

void TimelineWnd::OnTimer(unsigned int id) {
	if(id==KFollowTimerID) {
		UpdateTrackingPosition();
	}
}

void TimelineWnd::Layout() {
	Time length = _timeline->GetTimeLengthMS();

	Area rc = GetClientArea();
	_toolbar->Fill(LayoutTop,rc);
	int cX = -20+2;

	SetHorizontalScrollInfo(Range<int>(0,length), _timeWidth);
	UpdateTrackingPosition();

	unsigned int ch = 0;
	ref<Iterator<ref<TrackWrapper> > > it = _timeline->GetTracks();
	while(it->IsValid()) {
		ref<TrackWrapper> track = it->Get();
		if(!MatchesFilter(track)) {it->Next(); continue; }
		unsigned int trackHeight = track->GetHeight();

		ch += trackHeight +2 ;
		it->Next();
	}

	if(ch<(unsigned int)rc.GetHeight()) {
		SetVerticalPos(0);
	}

	SetVerticalScrollInfo(Range<int>(0, ch+100), rc.GetHeight());
}

void TimelineWnd::UpdateTrackingPosition() {
	if(_isLocked && _controller->GetPlaybackState()!=PlaybackStop && !IsKeyDown(KeyMouseLeft)) {
		SetHorizontalPos(min(max(1,int(_controller->GetTime())-int(_timeWidth.ToInt()/4)), _controller->GetTimeline()->GetTimeLengthMS().ToInt()-_timeWidth.ToInt()));
	}
}

void TimelineWnd::UpdateCursor(Pixels x, Pixels y) {
	if(_selMode && y > 2*KHeaderHeight && x > int(_nameColumnWidth)) {
		Mouse::Instance()->SetCursorType(CursorBeam);
	}
	else if(y > KHeaderHeight+KCueBarHeight && y < KHeaderHeight*2) {
		//SetCursor(LoadCursor(0, IDC_SIZEWE));
	}
	else if(x>=KZoomX && x<=KZoomX+KZoomW*2 && y < KHeaderHeight) {
		Mouse::Instance()->SetCursorType(CursorSizeNorthSouth);
	}
	else {
		strong<Theme> theme = ThemeManager::GetTheme();

		if(_draggedItem) {
			Mouse::Instance()->SetCursorType(CursorHandGrab);
		}
		else if(y>(2*KHeaderHeight)&&x>int(_nameColumnWidth+2)) {
			Mouse::Instance()->SetCursorType(CursorHand);
		}
		else if(x>(int(_nameColumnWidth)-2) && x<(int(_nameColumnWidth)+2) && y > 2*KHeaderHeight) {
			Mouse::Instance()->SetCursorType(CursorSizeEastWest);
		}
		else {
			Mouse::Instance()->SetCursorType(CursorDefault);
		}
	}
}

int TimelineWnd::GetTrackHeight(ref<TrackWrapper> tr) {
	ref< Iterator< ref<TrackWrapper> > > it = _timeline->GetTracks();
	RECT rc;
	GetClientRect(GetWindow(),&rc);
	Time t = _controller->GetTime();
	unsigned int cH = 2*KHeaderHeight;

	unsigned int vpos = GetVerticalPos();
	int realy = -int(vpos);

	if(tr) {
		while(it->IsValid()) {
			ref<TrackWrapper> track = it->Get();

			if(!track) {
				it->Next();
				continue;
			}

			if(!MatchesFilter(track)) {it->Next(); continue; };			

			unsigned int tH = track->GetHeight();

			// vertical scroll bar
			if(realy < 0) {
				realy += int(tH);
				it->Next();
				continue;
			}

			cH += tH;
			if(track==tr) break;
			it->Next();
		}
	}
	return cH;
}

ref<TrackWrapper> TimelineWnd::GetTrackAt(int x, int y) {
	unsigned int vpos = GetVerticalPos();
	int realy = -int(vpos);

	ref<Iterator< ref<TrackWrapper> > > it = _timeline->GetTracks();
	int cH = 2*KHeaderHeight;

	ref<TrackWrapper> currentTrack = 0;
	while(it->IsValid()) {
		ref<TrackWrapper> track = it->Get();
		if(!track || !MatchesFilter(track)) {it->Next(); continue; }
		unsigned int tH = track->GetHeight();

		// vertical scroll bar
		if(realy < 0) {
			realy += int(tH);
			it->Next();
			continue;
		}
		
		cH += int(tH);

		if(y<cH) {
			currentTrack = track;
			break;
		}
		it->Next();
	}

	return currentTrack;
}

void TimelineWnd::SetFilter(const std::wstring& q) {
	// search filter has changed
	_filterText = q;
	_copiedSelection.clear();
	SetVerticalPos(0);
	std::transform(_filterText.begin(), _filterText.end(), _filterText.begin(), tolower);
	Update();
}

LRESULT TimelineWnd::Message(UINT msg, WPARAM wp, LPARAM lp) {
	if(msg==WM_MOUSEWHEEL) {
		int delta = GET_WHEEL_DELTA_WPARAM(wp);

		if(IsKeyDown(KeyControl)) {
			if(delta<0) {
				Time old = _timeWidth;
				_timeWidth-=1000;
				if(_timeWidth<=Time(0)) {
					_timeWidth = old;
				}
			}
			else if(delta>0) {
				_timeWidth = min(_timeline->GetTimeLengthMS(),_timeWidth+Time(1000));
			}

			Time endTime = _timeWidth + Time(GetHorizontalPos());
			if(endTime>_timeline->GetTimeLengthMS()) {
				SetHorizontalPos(_timeline->GetTimeLengthMS()-_timeWidth);
			}
		}
		else {
			Time old = GetHorizontalPos();
			Time granularity = _timeWidth/Time(20);
			Time n = 0;
			if(delta>0) {
				 n = Time(GetHorizontalPos())-granularity;
				if(old<n || n<Time(0)) {
					n = Time(0);
				}
			}
			else {
				n = Time(GetHorizontalPos())+granularity;
				Time tl = _timeline->GetTimeLengthMS()-_timeWidth;
				if(n>tl) {
					n = tl;
				}
			}		
			SetHorizontalPos(n);
		}

		Update();
		return 0;
	}
	else if(msg==WM_GETMINMAXINFO) {
		MINMAXINFO* mm = (MINMAXINFO*)lp;
		mm->ptMinTrackSize.x = KMinSizeX;
		mm->ptMinTrackSize.y = KMinSizeY;
		return 0;
	}
	else {
		return ChildWnd::Message(msg,wp,lp);
	}
}

void TimelineWnd::Interpolate(Time old, Time newTime, Time left, Time right) {
	_timeline->Interpolate(old,newTime,left,right);
}

void TimelineWnd::OnMouse(MouseEvent ev, Pixels x, Pixels y) {
	Area rc = GetClientArea();
	UndoBlock ub;

	if(ev==MouseEventMove) {
		if(_selMode && (IsKeyDown(KeyMouseLeft)||IsKeyDown(KeyMouseRight)) && _selDrag != SelectionDragNone) {
			float msPerPixel = float(_timeWidth)/float(rc.GetWidth()-_nameColumnWidth);
			long clickedTime = long(GetHorizontalPos())+long((x-_nameColumnWidth) * msPerPixel);

			if(_selDrag == SelectionDragEnd) {
				_selEnd = Time(clickedTime);	
				if(_selStart>=_selEnd) {
					_selStart = _selEnd;
				}
			}
			else if(_selDrag==SelectionDragStart) {
				_selStart = Time(clickedTime);
			}

			// Show a tooltip with the time at the location where the mouse currently is
			if(Application::Instance()->GetSettings()->GetFlag(L"view.tooltips")) {
				std::wstring text  = Time(clickedTime).Format();
				if(text.length()>0) {
					_tt->SetTooltip(text);
				
					strong<Theme> theme = ThemeManager::GetTheme();
					POINT p;
					p.x = int(x * theme->GetDPIScaleFactor());
					p.y = int(y * theme->GetDPIScaleFactor());
					ClientToScreen(GetWindow(), &p);
					_tt->SetTrackEnabled(true);
					_tt->SetTrackPosition(p.x+15, p.y+15);
				}
			}

			Update();
			UpdateCursor(x,y);
		}
		else {
			if(_isDraggingSplitter) {
				_nameColumnWidth = x;
				if(_nameColumnWidth<KColumnWidthMin) _nameColumnWidth = KColumnWidthMin;
				if(_nameColumnWidth>KColumnWidthMax) _nameColumnWidth = KColumnWidthMax;
				Mouse::Instance()->SetCursorType(CursorSizeEastWest);
				Update();
			}
			else if(_draggedItem) {				
				if(x>_nameColumnWidth) {
					float msPerPixel = float(_timeWidth)/float(rc.GetWidth()-_nameColumnWidth);

					long clickedTime = long(GetHorizontalPos())+long((x-_nameColumnWidth) * msPerPixel);
					long totalTime = (long)int(_timeline->GetTimeLengthMS());
					if(clickedTime>=0 && clickedTime<=totalTime) {
						int cH = GetTrackHeight(_draggedItemTrack);

						/* Snap to cue */
						int offset = _draggedItem->GetSnapOffset();
						ref<Cue> next = _cues->GetNextCue(clickedTime+offset);
						ref<Cue> prev = _cues->GetPreviousCue(clickedTime+offset);
						ref<Cue> nearest = 0;
						if(next && !prev) {
							nearest = next;
						}
						else if(prev && !next) {
							nearest = prev;
						}
						else if(prev&&next) {
							if(abs(int(next->GetTime())-clickedTime) < abs(int(prev->GetTime())-clickedTime)) {
								nearest = next;
							}
							else {
								nearest = prev;
							}
						}

						// if we're dragging a cue, no sticking applies
						if(nearest && !_draggedItem.IsCastableTo<Cue>()) {
							strong<Theme> theme = ThemeManager::GetTheme();
							int limit = int(msPerPixel*theme->GetMeasureInPixels(Theme::MeasureMaximumSnappingDistance));
							if(abs(int(nearest->GetTime())-clickedTime+offset) < limit) {
								clickedTime = (long)(int)Time(nearest->GetTime()) + offset;
							}
						}

						if(_isInterpolating && _draggedItem.IsCastableTo<Cue>()) {
							ref<Cue> draggedCue = _draggedItem;
							Interpolate(draggedCue->GetTime(), clickedTime, _left, _right);
						}

						_draggedItem->Move(Time(clickedTime), cH-y);
						Repaint();
					}
				}
			}
			else if(IsKeyDown(KeyMouseLeft)) {
				// second header
				if(int(x)>_nameColumnWidth&&y<(2*KHeaderHeight)&&y>(KHeaderHeight+KCueBarHeight)) {
					unsigned int startTime = GetHorizontalPos();
					unsigned int endTime = _timeWidth + Time(startTime);

					float percent = float(x-_nameColumnWidth)/float(rc.GetWidth()-_nameColumnWidth);
					_controller->Jump(Time(unsigned int(percent*float((endTime-startTime))+float(startTime))));
					Update();
				}
			}
		}

		UpdateCursor(x,y);
	}
	else if(ev==MouseEventRDown) {
		float msPerPixel = float(_timeWidth)/float(rc.GetWidth()-_nameColumnWidth);
		Time clickedTime = Time(GetHorizontalPos())+Time(int(int(x-_nameColumnWidth) * msPerPixel));

		if(y<2*KHeaderHeight && y>KHeaderHeight+KCueBarHeight) {
			_selMode = true;
			_zoomSelMode = true;
			_selEnd = clickedTime;
			_selDrag = SelectionDragEnd;
			_selStart = clickedTime;
		}
		else if(y>(2*KHeaderHeight) && !_selMode) {
			// find out which track we've clicked
			ref<TrackWrapper> currentTrack = GetTrackAt(x,y);
		
			if(currentTrack) {
				int cH = GetTrackHeight(currentTrack);
				if(x<_nameColumnWidth) {
					DoTrackMenu(currentTrack,x,y);
				}
				else if(!currentTrack->IsLocked()) {
					_draggedItem = currentTrack->GetTrack()->GetItemAt(clickedTime, cH-y, true, currentTrack->GetHeight(), 1.0f/msPerPixel);
					_focusedItem = _draggedItem;
					_draggedItemTrack = currentTrack;
					UpdateCursor(x,y);
					Update();
				}
			}
			else {
				DoGeneralContextMenu(x,y);
			}
		}
		else if(y>KHeaderHeight && y<(KHeaderHeight+KCueBarHeight)) {
			// cue bar
			ref<Cue> cue = GetCueAt(x,y);
			DoCueMenu(cue,x,y);
			UpdateCursor(x,y);
			Update();
		}
		else if(_selMode) {
			// selection context neu
			DoSelectionContextMenu(x,y);
		}
	}
	else if(ev==MouseEventLDown||ev==MouseEventLDouble) {
		if(ev==MouseEventLDown) {
			Focus();
			SetWantMouseLeave(true);

			if(x>(_nameColumnWidth-2) && x<(_nameColumnWidth+2) && y > 2*KHeaderHeight) {
				_isDraggingSplitter = true;
				UpdateCursor(x,y);
				Update();
				return;
			}
			// selection mode
			else if(_selMode && x>_nameColumnWidth && y > 2*KHeaderHeight) {
				float pixelsPerMs = float(rc.GetWidth()-_nameColumnWidth)/int(_timeWidth);
				Time startTime = Time(GetHorizontalPos());

				if(_selStart>=Time(0)) {
					int xStart = int((int(_selStart-startTime) * pixelsPerMs)) + _nameColumnWidth;
					if(abs(int(x-xStart))<5) {
						_selDrag = SelectionDragStart;
						return;
					}
				}
				
				if(_selEnd>=Time(0)) {
					int xEnd  = int((int(_selEnd-startTime) * pixelsPerMs)) + _nameColumnWidth;
					if(abs(int(x-xEnd))<5) {
						_selDrag = SelectionDragEnd;
						return;
					}
				}

				_selStart = Time(int(float(x-_nameColumnWidth)/pixelsPerMs)) + startTime;
				_selDrag = SelectionDragEnd;
			}
			// first header
			else if(y>KHeaderHeight) {
				if(y<(KHeaderHeight+KCueBarHeight)) {
					// cue bar
					if(x>_nameColumnWidth) {
						ref<Cue> cue = GetCueAt(x,y);
						if(cue) {
							ref<Path> path = _timeline->CreatePath();
							path->Add(cue->CreateCrumb());
							Application::Instance()->GetView()->Inspect(cue, path);

							if(IsKeyDown(KeyControl) || IsKeyDown(KeyShift)) {
								_draggedItem = cue;
								_draggedItemTrack = 0;
								float pixelsPerMs = float(rc.GetWidth()-_nameColumnWidth)/int(_timeWidth);
								Time startTime = Time(GetHorizontalPos());
								x = int(int(cue->GetTime()-startTime)*pixelsPerMs + _nameColumnWidth);

								if(IsKeyDown(KeyShift)) {
									// Find interpolation boundaries
									ref<Cue> prev = _cues->GetPreviousCue(cue->GetTime());
									if(prev) {
										_left = prev->GetTime();
									}
									else {
										_left = 0;
									}
									
									ref<Cue> next = _cues->GetNextCue(cue->GetTime());
									if(next) {
										_right = next->GetTime();
									}
									else {
										_right = _timeline->GetTimeLengthMS();
									}

									_isInterpolating = true;
								}
							}
							else {
								_tt->SetTooltip(TL(cue_press_ctrl_to_move));
								_tt->SetTrackEnabled(true);

								strong<Theme> theme = ThemeManager::GetTheme();
								POINT p;
								p.x = int(x * theme->GetDPIScaleFactor());
								p.y = int(y * theme->GetDPIScaleFactor());
								ClientToScreen(GetWindow(), &p);
								_tt->SetTrackPosition(p.x+15, p.y+15);
							}
						}
						_activeCue = cue;
						_focusedItem = cue;
						UpdateCursor(x,y);
						Update();
						return;
					}
				}
				else if(y < (2*KHeaderHeight) && x < _nameColumnWidth) {
					// The 'empty area' left of the cue bar; for consistency with ListWnd, use it to resize the name column
					_isDraggingSplitter = true;
					UpdateCursor(x,y);
					Update();
					return;
				}
			}
		}

		if(y>2*KHeaderHeight) {
			ref<TrackWrapper> currentTrack = GetTrackAt(x,y);
			int cH = GetTrackHeight(currentTrack);

			if(currentTrack) {
				if(x<(16+KDefaultMarginLeft)) {
					// run mode button
					// TODO: change runmodes here just like we did in older versions? Note that we can only do this
					// when not playing back
				}
				// Expand/Collapse button or arrow button for sub timelines
				else if(x>(_nameColumnWidth-KDefaultMarginLeft-16) && x<_nameColumnWidth) {
					ref<Track> track = currentTrack->GetTrack();

					// for sub timelines, reveal the timeline window associated with the sub timeline on click
					if(currentTrack) {
						ref<SubTimeline> sub = currentTrack->GetSubTimeline();
						if(sub) {
							 ref<Instance> inst = sub->GetInstance();
							 if(inst && inst.IsCastableTo<Controller>()) {
								 ref<Wnd> twnd = ref<Controller>(inst)->GetSplitTimelineWindow();
								 if(twnd) {
									Application::Instance()->GetView()->RevealTimeline(twnd);
								}
							 }
						}
						// for other tracks, expand or collapse
						else if(track && track->IsExpandable()) {
							track->SetExpanded(!track->IsExpanded());
							// Update scrollbar
							OnSize(GetClientArea());
						}
					}
				}
				else {
					SetCurrentTrack(currentTrack);

					float msPerPixel = float(_timeWidth)/float(rc.GetWidth()-_nameColumnWidth);
					Time clickedTime = Time(GetHorizontalPos())+Time(int(float(x-_nameColumnWidth) * msPerPixel));

					if(x>_nameColumnWidth && !_selMode) {
						if(ev==MouseEventLDouble && !currentTrack->IsLocked()) {
							currentTrack->GetTrack()->OnDoubleClick(clickedTime, cH-y);
						}
						else {
							if(!currentTrack->IsLocked() || IsKeyDown(KeyControl)) { 
								_draggedItem = currentTrack->GetTrack()->GetItemAt(clickedTime, cH-y, false, currentTrack->GetHeight(), 1.0f/msPerPixel);
								_draggedItemTrack = currentTrack;
								_focusedItem = _draggedItem;
								if(_draggedItem) {
									ref<Path> path = _timeline->CreatePath();
									path->Add(GC::Hold(new BasicCrumb(currentTrack->GetInstanceName(), L"icons/track.png", currentTrack)));
									path->Add(TL(item), L"icons/item.png", _draggedItem);
									Application::Instance()->GetView()->Inspect(_draggedItem, path);
									Focus();
									Update();
									UpdateCursor(x,y);
									Application::Instance()->Update();

									// set context stuff
									if(Application::Instance()->GetSettings()->GetFlag(L"view.tooltips")) {
										std::wstring text  = _draggedItem->GetTooltipText() + TL(timeline_remove_hint);
										if(text.length()>0) {
											_tt->SetTooltip(text);
										
											// some DPI magic
											strong<Theme> theme = ThemeManager::GetTheme();
											POINT p;
											p.x = int(x * theme->GetDPIScaleFactor());
											p.y = int(y * theme->GetDPIScaleFactor());
											ClientToScreen(GetWindow(), &p);
											_tt->SetTrackEnabled(true);
											_tt->SetTrackPosition(p.x+15, p.y+15);
										}
									}

									return;
								}
							}
						}
					}
					else {
						ref<TrackWrapper> currentTrack = _currentTrack;
						if(currentTrack) {
							ref<Path> p = _timeline->CreatePath();
							p->Add(GC::Hold(new BasicCrumb(currentTrack->GetInstanceName(), L"icons/track.png", currentTrack)));
							Application::Instance()->GetView()->Inspect(currentTrack, p);
							Focus();
						}
					}
				}
			}
			else {
				_draggedItem = 0;
				_draggedItemTrack = 0;
			}
		}

		UpdateCursor(x,y);
		Update();
	}
	else if(ev==MouseEventMDown) {
		SetSelectionMode(!IsInSelectionMode());
		Update();
	}
	else if(ev==MouseEventLUp||ev==MouseEventRUp||ev==MouseEventLeave) {
		_tt->SetTrackEnabled(false);
		_selDrag = SelectionDragNone;
		_isInterpolating = false;

		if(_selMode && _zoomSelMode) {
			_selMode = false;
			_zoomSelMode = false;

			// Don't zoom if the interval is too small
			Time nw = _selEnd - _selStart;
			if(nw>=Time(25)) {
				SetHorizontalPos(_selStart);
				_timeWidth = nw;
			}
			Repaint();
		}

		_isDraggingSplitter = false;
		if(_draggedItem) {
			_draggedItem->MoveEnded();
			_draggedItem = 0;
			_draggedItemTrack = 0;
			Application::Instance()->Update();
		}
		_capture.StopCapturing();
		UpdateCursor(x,y);
	}
}

void TimelineWnd::OnKey(Key key, wchar_t t, bool down, bool accelerator) {
	UndoBlock ub;
	if(!accelerator) {
		if(down) {
			if(key==KeyLeft) {
				if(_focusedItem && !IsKeyDown(KeyControl)) {
					_focusedItem->MoveRelative(Item::Left);
				}
				else {
					// TODO: focus on previous item in track?
				}
			}
			if(key==KeyRight) {
				if(_focusedItem && !IsKeyDown(KeyControl)) {
					_focusedItem->MoveRelative(Item::Right);
				}
				else {
					// TODO: focus on next item in track?
				}
			}
			if(key==KeyUp) {
				if(_focusedItem && !IsKeyDown(KeyControl)) {
					_focusedItem->MoveRelative(Item::Up);
				}
				else {
					// TODO: Move to track above?
					
				}
			}
			if(key==KeyDown) {
				if(_focusedItem && !IsKeyDown(KeyControl)) {
					_focusedItem->MoveRelative(Item::Down);
				}
				else {
					// TODO: Move to track below?
				}
			}
			else if(key==KeyPageUp) {
				int horz = (int)GetHorizontalPos();
				horz = max(0, horz-(int)(float(_timeWidth)/100.0f));
				SetHorizontalPos((unsigned int)horz);
				Update();
			}
			else if(key==KeyPageDown) {
				int horz = (int)GetHorizontalPos();
				horz = min((int)(_timeline->GetTimeLengthMS())-int(_timeWidth), horz+int(float(_timeWidth)/100.0f));
				SetHorizontalPos((unsigned int)horz);
				Update();
			}
			else if(key==KeyDelete) {
				if(_selMode) {
					DoDeleteSelection();
				}
				else if(_focusedItem) {
					_focusedItem->Remove();
					_focusedItem = 0;
				}
				else if(!_focusedItem) {
					ref<TrackWrapper> tw = _currentTrack;
					if(tw) {
						RemoveTrack(tw, true);
					}
				}
			}
			else if(key==KeyInsert) {
				ref<Cue> cue = GC::Hold(new Cue(L"", Cue::DefaultColor, _controller->GetTime(), _controller->GetCueList()));
				_cues->AddCue(cue);
				Update();
			}
			else if(key==KeyReturn) {
				if(IsKeyDown(KeyShift)) {
					_controller->SetPlaybackState(PlaybackPause);
				}
				else {
					if(_controller->GetPlaybackState()==PlaybackPlay) {
						_controller->SetPlaybackState(PlaybackStop);
					}
					else {
						_controller->SetPlaybackState(PlaybackPlay);
					}
				}
			}
			else if(key==KeySpace) {
				_controller->Fire();
			}
		}
	}

	ChildWnd::OnKey(key,t,down, accelerator);
	Repaint();
}

void TimelineWnd::GetAccelerators(std::vector<Accelerator>& accels) {
	std::wstring keyDeleteDesc;
	if(_selMode) {
		keyDeleteDesc = TL(key_delete_selection);
	}
	else {
		if(_focusedItem) {
			keyDeleteDesc = TL(key_delete_item);
		}
		else if(ref<TrackWrapper>(_currentTrack)) {
			keyDeleteDesc = TL(key_delete_track);
		}
	}

	if(keyDeleteDesc.length()>0) {
		accels.push_back(Accelerator(KeyDelete, TL(key_delete), keyDeleteDesc));
	}

	accels.push_back(Accelerator(KeyInsert, TL(key_insert), TL(cue_add)));
	accels.push_back(Accelerator(KeySpace, TL(key_space), TL(timeline_next_play_cue)));
	accels.push_back(Accelerator(KeyReturn, TL(key_return), TL(timeline_play)));
	accels.push_back(Accelerator(KeyReturn, TL(key_return), TL(timeline_pause), false, KeyShift));
}

void TimelineWnd::OnSize(const Area& newSize) {
	Layout();
	Update();
}

void TimelineWnd::OnFocus(bool focus) {
	Repaint();
}

void TimelineWnd::DoDeleteSelection() {
	ref< Iterator< ref<TrackWrapper> > > it = _timeline->GetTracks();
	while(it->IsValid()) {
		ref<TrackWrapper> tw = it->Get();
		if(MatchesFilter(tw)) {
			ref<Track> track = tw->GetTrack();
			if(track) {
				ref<TrackRange> range = track->GetRange(_selStart, _selEnd);
				if(range) {
					range->RemoveItems();
				}
			}
		}
		it->Next();
	}

	Update();
}



void TimelineWnd::InsertFromControls() {
	Time t = _controller->GetTime();
	ref< Iterator< ref<TrackWrapper> > > it = _timeline->GetTracks();
	while(it->IsValid()) {
		ref<TrackWrapper> tw = it->Get();
		if(MatchesFilter(tw)) {
			ref<LiveControl> control = tw->GetLiveControl();
			if(control) {
				tw->GetTrack()->InsertFromControl(t, control, true);
			}
		}
		it->Next();
	}
}

void TimelineWnd::InsertFromCurrent() {
	Time t = _controller->GetTime();
	ref<TrackWrapper> currentTrack = _currentTrack;
	if(currentTrack) {
		currentTrack->GetTrack()->InsertFromControl(t, currentTrack->GetLiveControl(), true);
	}
}

void TimelineWnd::Paint(Graphics& g, strong<Theme> theme) {
	Area rc = GetClientArea();

	// Define brushes and fonts
	SolidBrush textBrush(theme->GetColor(Theme::ColorText));
	Pen linePen(theme->GetColor(Theme::ColorLine), 1.0);
	SolidBrush lineBrush(theme->GetColor(Theme::ColorLine));
	Font* boldFont = theme->GetGUIFontBold();
	Font* normalFont = theme->GetGUIFont();
	
	// fill background
	g.Clear(theme->GetColor(Theme::ColorBackground));

	// current time line
	Time startTime = GetHorizontalPos();
	Time endTime = Time(min(int(_timeWidth + startTime), int(_timeline->GetTimeLengthMS())));
	
	float pixelsPerMs = float(rc.GetWidth()-_nameColumnWidth)/int(_timeWidth);
	Time t = _controller->GetTime();

	// draw tracks or a help text when there are no tracks
	if(_timeline->GetTrackCount()<1) {
		std::wstring msg = TL(timeline_no_tracks);
		SolidBrush descTextBrush(theme->GetColor(Theme::ColorHint));
		StringFormat sf;
		sf.SetAlignment(StringAlignmentCenter);
		Area lr = GetClientArea();
		lr.Narrow(KDefaultMarginLeft+_nameColumnWidth,Pixels(KHeaderHeight*2.5f), 0, 0);
		g.DrawString(msg.c_str(), (int)msg.length(), theme->GetGUIFont(), lr, &sf, &descTextBrush);
	}
	else {
		int marginLeft = KDefaultMarginLeft;
		if(_showAddresses) {
			marginLeft += 22;
		}

		// When in 'minimal information mode', only the track names and group colors are shown
		bool minimalInformationMode = false;
		if((_nameColumnWidth-marginLeft) < 64) {
			minimalInformationMode = true;
			marginLeft = 8;
		}

		// Get focused item
		ref<Item> focus = _focusedItem;

		// Iterate over all tracks and draw them
		ref< Iterator< ref<TrackWrapper> > > it = _timeline->GetTracks();
		unsigned int cY = 2*KHeaderHeight;
		unsigned int vpos = GetVerticalPos();
		int realy = -int(vpos);

		while(it->IsValid()) {
			ref<TrackWrapper> track = it->Get();
			if(!track) {
				it->Next();
				continue; 
			}

			if(!MatchesFilter(track)) {it->Next(); continue; }
			unsigned int tH = track->GetHeight();

			// vertical scroll bar
			if(realy < 0) {
				realy += int(tH);
				it->Next();
				continue;
			}

			if(realy > (rc.GetHeight())) {
				break; // that was the last track we're going to paint
			}

			// If we are in selection mode, the current track has no special role, so don't paint the background
			if(_currentTrack==track && !_selMode) {
				SolidBrush activeBrush(theme->GetColor(Theme::ColorActiveTrack));
				g.FillRectangle(&activeBrush, RectF(0.0f, float(cY)+1.0f, float(rc.GetWidth()), float(tH)));
			}			

			// Draw the name in the left column
			std::wstring iname = track->GetInstanceName();
			std::wstring tname = track->GetTypeName();

			// runmode icon
			if(!minimalInformationMode) {
				Area runIconArea(marginLeft, cY+2, 14, 17);
				RunMode runMode = track->GetRunMode();
				switch(runMode) {
					case RunModeMaster:
						_runIcon.Paint(g, runIconArea);
						break;

					case RunModeClient:
						_norunIcon.Paint(g, runIconArea);
						break;

					case RunModeBoth:
						_bothrunIcon.Paint(g, runIconArea);
						break;
				}
			}

			// TODO cY+3 veranderen in het 'midden' relatief aan de standaard track hoogte
			RectF lr(float(marginLeft+(minimalInformationMode ? 2 :16)), float(cY+3), float(_nameColumnWidth-marginLeft-2-(minimalInformationMode ? 0 : (5+16))), 16.0f); // TODO regelhoogte const static int maken
			RectF bounding;
			g.SetClip(lr);

			// Draw track name and type
			StringFormat sf;
			sf.SetTrimming(StringTrimmingEllipsisCharacter);
			g.DrawString(iname.c_str(), (unsigned int)iname.length(), boldFont,lr, &sf, &textBrush);
			g.MeasureString(iname.c_str(), iname.length(), boldFont, lr, &sf, &bounding);
			lr.SetX((float)max(0,bounding.GetWidth()+bounding.GetLeft() - 3));
			lr.SetWidth((float)(_nameColumnWidth - lr.GetLeft() - 10 - 16));
			g.DrawString(tname.c_str(), (unsigned int)tname.length(), normalFont, lr, &sf, &textBrush);

			g.ResetClip();

			if(_showAddresses) {
				// Show the group number; the background color is of course the group color				
				GroupID gid = track->GetGroup();
				ref<Group> group = Application::Instance()->GetModel()->GetGroups()->GetGroupById(gid);
				if(group) {
					Area lr(KDefaultMarginLeft, cY+4, 16, 12);
					Area groupRect = Area(0, cY+2, 3, tH-4);
					SolidBrush groupBrush(group->GetColor());
					g.FillRectangle(&groupBrush, groupRect);

					if(!minimalInformationMode) {
						SolidBrush addressBrush(theme->GetColor(Theme::ColorActiveEnd));
						StringFormat sf;
						sf.SetAlignment(StringAlignmentNear);
						sf.SetLineAlignment(StringAlignmentNear);
						std::wstring address = Stringify(gid);
						g.DrawString(address.c_str(), (int)address.length(), theme->GetGUIFontSmall(), lr, &sf, &addressBrush);
					}
				}
			}

			// paint expand/collapse buttons; subtimelines have an arrow button on that location
			// that, when clicked, reveals the timeline window associated with the sub timeline.
			if(!minimalInformationMode) {
				if(track->GetTrack()->IsExpandable()) {
					g.DrawImage(track->GetTrack()->IsExpanded()?_collapseIcon:_expandIcon, RectF(float(_nameColumnWidth-KDefaultMarginLeft-16), float(cY+2), 16.0f, 16.0f));
				}
				else if(track->IsSubTimeline()) {
					g.DrawImage(_arrowIcon, RectF(float(_nameColumnWidth-KDefaultMarginLeft-16), float(cY+2), 16.0f, 16.0f));
				}
			}
				
			g.SetClip(Rect(_nameColumnWidth+1,cY, rc.GetWidth()-_nameColumnWidth, tH+1));
			track->GetTrack()->Paint(&g, t, pixelsPerMs, _nameColumnWidth, cY+2, tH-4, startTime, endTime, focus);
			g.ResetClip();

			// If track is locked, draw disabled overlay
			if(track->IsLocked()) {
				Area track(_nameColumnWidth, cY+2, rc.GetWidth()-_nameColumnWidth, tH);
				SolidBrush disabled(theme->GetColor(Theme::ColorDisabledOverlay));
				g.FillRectangle(&disabled, track);
				_lockedIcon.Paint(g, Area(_nameColumnWidth+1, cY+1, 16, 16));
			}

			cY += tH;
			realy += tH;
			g.DrawLine(&linePen, 0, cY, rc.GetWidth(), cY);
			it->Next();
		}
	}

	// Search edit box
	StringFormat sf;
	sf.SetAlignment(StringAlignmentFar);
	const wchar_t* searchStr = TL(search);
	g.DrawString(searchStr, (INT)wcslen(searchStr), boldFont, RectF(float(rc.GetWidth()-265), 3.0f, 55.0f, float(KHeaderHeight)), &sf, &textBrush);
	g.DrawLine(&linePen, PointF(float(rc.GetWidth()-210+55), 0.0f), PointF( float(rc.GetWidth()-210+55), float(KHeaderHeight)));

	theme->DrawToolbarBackground(g, (float)rc.GetLeft(), (float)KHeaderHeight, float(rc.GetWidth()), (float)KHeaderHeight);
	g.DrawLine(&linePen, PointF(0.0f, float(KHeaderHeight)), PointF(float(rc.GetWidth()), float(KHeaderHeight)));
	g.DrawLine(&linePen, PointF(0.0f, float(2*KHeaderHeight)), PointF(float(rc.GetWidth()), float(2*KHeaderHeight)));

	// name column splitter
	g.DrawLine(&linePen, PointF(float(_nameColumnWidth), float(KHeaderHeight)), PointF(float(_nameColumnWidth),float(rc.GetHeight())));

	// Shadow
	Area shadowArea(rc.GetLeft()+_nameColumnWidth, rc.GetTop()+2*KHeaderHeight, rc.GetWidth(), rc.GetHeight());
	theme->DrawInsetRectangleLight(g, shadowArea);

	// current time position line
	if(t>startTime&&t<=endTime) {	
		float barLeft = pixelsPerMs * float(t-startTime) + _nameColumnWidth;
		Pen timePen(theme->GetColor(Theme::ColorCurrentPosition), 1.0f);
		g.DrawLine(&timePen, PointF(barLeft, float(KHeaderHeight)), PointF(barLeft, float(rc.GetHeight())));
	}
	
	// Time markers
	if(_timeWidth==Time(0)) _timeWidth = 100;
	float precision = float(int(log10(double(_timeWidth))-1));
	int maxMarkers = 20;
	float length = pow(10.0f,precision);
	if((int(_timeWidth)/length)>=maxMarkers) {
		length *= 2;
	}

	Time firstSecond = Time(startTime + Time(int(length - (unsigned int)fmod(float(startTime), length))));

	for(Time s=firstSecond;s<=endTime;s+=Time(int(length))) {
		g.DrawLine(&linePen, PointF(int(s-startTime)*pixelsPerMs+_nameColumnWidth, (2*KHeaderHeight-8)), PointF(int(s-startTime)*pixelsPerMs+_nameColumnWidth, (2*KHeaderHeight)));
		std::wstring second = Stringify<float>(s.ToInt()/1000.0f);
		g.DrawString(second.c_str(), (unsigned int)second.length(), normalFont, PointF(int(s-startTime)*pixelsPerMs+_nameColumnWidth+1, (2*KHeaderHeight-14+1)), &lineBrush);
	}

	// Paint cues
	g.SetHighQuality(true);
	std::vector< ref<Cue> >::iterator cit = _cues->GetCuesBegin();
	while(cit!=_cues->GetCuesEnd()) {
		ref<Cue> cue = *cit;
		Time cueTime = cue->GetTime();
		if(cueTime>=startTime&&cueTime<=(_timeWidth+startTime)) {
			int x = int((int(cueTime - startTime) * pixelsPerMs) + _nameColumnWidth);
			
			// get color
			Color cueColor(cue->GetColor());
			Color cueAlphaColor = Theme::ChangeAlpha(cueColor, _activeCue==cue ? 255 : 140);
			SolidBrush br(cueAlphaColor);
			Pen brp(cueAlphaColor, 1.0f);
			
			// draw
			g.FillRectangle(&br, RectF(float(x), float(KHeaderHeight+1), 8.0f, (float)KCueBarHeight));
			g.DrawLine(&brp, PointF(float(x), float(KHeaderHeight+2)), PointF(float(x), float(2*KHeaderHeight-1)));
		
			// draw 'icons'
			SolidBrush iconColor(theme->GetColor(Theme::ColorDisabledOverlay));
			switch(cue->GetAction()) {
				case Cue::ActionStop:
					g.FillRectangle(&iconColor, RectF(float(x+1), float(KHeaderHeight+2), 6.0f, (float)KCueBarHeight-2.0f));
					break;

				case Cue::ActionPause:
					g.FillRectangle(&iconColor, RectF(float(x+1), float(KHeaderHeight+2), 2.0f, (float)KCueBarHeight-2.0f));
					g.FillRectangle(&iconColor, RectF(float(x+5), float(KHeaderHeight+2), 2.0f, (float)KCueBarHeight-2.0f));
					break;

				case Cue::ActionStart:
					PointF polys[3];
					polys[0] = PointF((float)x+1, float(KHeaderHeight+2));
					polys[2] = PointF((float)x+1, float(KHeaderHeight+KCueBarHeight+1));
					polys[1] = PointF((float)x+6, float(KHeaderHeight+2+((KCueBarHeight-2)/2)));
					g.FillPolygon(&iconColor, polys, 3);
					break;
			}
		} 
		++cit;
	}

	// When interpolating, show interpolation range
	if(_isInterpolating) {
		int xa = max(int((int(_left - startTime) * pixelsPerMs) + _nameColumnWidth), _nameColumnWidth);
		int xb = int((int(_right - startTime) * pixelsPerMs) + _nameColumnWidth);

		LinearGradientBrush selbr(PointF(float(xa), 0.0f), PointF(float(xb), 0.0f), theme->GetColor(Theme::ColorHighlightStart), theme->GetColor(Theme::ColorHighlightEnd));
		g.FillRectangle(&selbr, RectF(float(xa), float(KHeaderHeight)*1.5f, float(xb-xa), float(KHeaderHeight/2)));
	}
 
	// draw selection
	if(_selMode && _selStart>=Time(0) && _selEnd >= Time(0) && _selEnd > startTime) {
		g.SetClip(Rect(_nameColumnWidth, KHeaderHeight, rc.GetRight(), rc.GetBottom()));
		int xa = int((int(_selStart - startTime) * pixelsPerMs) + _nameColumnWidth);
		int xb = int(int(_selEnd-_selStart) * pixelsPerMs);
		int h = rc.GetHeight();
		if(_zoomSelMode) {
			h = KHeaderHeight/2;
		}

		LinearGradientBrush selbr(PointF(0.0f, float(KHeaderHeight)), PointF(0.0f, float(rc.GetHeight()) ), theme->GetColor(Theme::ColorTimeSelectionStart), theme->GetColor(Theme::ColorTimeSelectionEnd));
		g.FillRectangle(&selbr, RectF(float(xa), float(KHeaderHeight), float(xb), float(h)));
	}
}

void TimelineWnd::OnSetTimeLength(Time t) {
	SetHorizontalScrollInfo(Range<int>(0,(_timeline->GetTimeLengthMS())), _timeWidth);
	Update();
}

void TimelineWnd::DoSelectionContextMenu(int x, int y) {
	if(_selStart >=Time(0) && _selEnd >= Time(0) && _selEnd > _selStart) {
		ContextMenu menu;
		enum {cmdDeleteItems=2, cmdDeleteCues, cmdCopyItems, cmdPasteItems, cmdZoomSelection};
		menu.AddItem(GC::Hold(new MenuItem(TL(selection_remove_items), cmdDeleteItems, false, MenuItem::NotChecked, L"icons/shared/recycle.png", L"Del")));
		menu.AddItem(TL(selection_remove_cues), cmdDeleteCues);
		menu.AddItem(GC::Hold(new MenuItem(TL(selection_copy_items), cmdCopyItems, false, MenuItem::NotChecked, L"icons/shared/recycle.png", L"Ctrl-C")));
		menu.AddItem(TL(selection_zoom), cmdZoomSelection);
		if(_copiedSelection.size() > 0) {
			menu.AddItem(GC::Hold(new MenuItem(TL(selection_paste_items), cmdPasteItems, false, MenuItem::NotChecked, L"icons/shared/recycle.png", L"Ctrl-V")));
		}

		switch(menu.DoContextMenu(this,x,y)) {
			case cmdDeleteItems:
				DoDeleteSelection();
				break;

			case cmdDeleteCues:
				_cues->RemoveCuesBetween(_selStart, _selEnd);
				break;

			case cmdCopyItems:
				DoCopySelection();
				break;

			case cmdPasteItems:
				DoPasteSelection();

			case cmdZoomSelection:
				SetHorizontalPos(_selStart);
				_timeWidth = _selEnd - _selStart;
				if(_timeWidth<=Time(10)) {
					_timeWidth = Time(10);
				}
				break;
		}

		UpdateCursor(x,y);
		Update();
	}
}

ref<Timeline> TimelineWnd::GetTimeline() {
	return _timeline;
}

void TimelineWnd::SetTimeline(ref<Timeline> md) {
	assert(md);
	_timeline = md;
	_timeWidth = _timeline->GetTimeLengthMS();
	Update();
}

void TimelineWnd::Jump(Time t, bool x) {
	if(x) {
		SetHorizontalPos(max(0,int(t)-(int(_timeWidth)/2)));
		Update();
	}
}

bool TimelineWnd::IsLocked() {
	return _isLocked;
}

ref<Cue> TimelineWnd::GetCueAt(int x, int y) {
	Area rc = GetClientArea();

	Time startTime = GetHorizontalPos();
	float pixelsPerMs = float(rc.GetWidth()-_nameColumnWidth)/int(_timeWidth);
	Time clickedTime = Time(int((x - _nameColumnWidth) / pixelsPerMs)) + startTime;

	std::vector< ref<Cue> >::iterator it = _cues->GetCuesBegin();
	while(it!=_cues->GetCuesEnd()) {
		ref<Cue> cue = *it;
		Time cueTime = cue->GetTime();
		if(cueTime>=startTime && cueTime<=(startTime+_timeWidth)) {
			if(Near<Time>(cueTime, clickedTime, Time(int(10/pixelsPerMs)))) {
				return cue;
			}
		}
		++it;
	}

	return 0;
}

void TimelineWnd::DoCueMenu(ref<Cue> cue, Pixels x, Pixels y) {
	CueListWnd::DoCueMenu(this, cue, _cues, x, y);
}

void TimelineWnd::DoGeneralContextMenu(Pixels x, Pixels y) {
	enum { KCPaste = 1, KCPasteSelection =2 };

	ContextMenu cm;
	if(_selMode) {
		cm.AddItem(GC::Hold(new MenuItem(TL(paste_selection), KCPasteSelection, false, MenuItem::NotChecked, Icons::GetIconPath(Icons::IconPaste), L"Ctrl-V")));
	}
	else {
		bool canPaste = Clipboard::IsObjectAvailable();
		cm.AddItem(GC::Hold(new MenuItem(TL(paste), canPaste ? KCPaste : -1, false, MenuItem::NotChecked, Icons::GetIconPath(Icons::IconPaste), L"Ctrl-V")));
	}

	int cmd = cm.DoContextMenu(this,x,y);
	if(cmd==KCPaste) {
		DoPasteTrack();
	}
	else if(cmd==KCPasteSelection) {
		DoPasteSelection();
	}
}

void PopulateTimelineMenu(strong<SubMenuItem> menu, strong<Timeline> parent, int& idx, std::map<int, ref<Timeline> >& tls, int level, strong<Timeline> self) {
	if(parent!=self) {
		tls[idx] = parent;
		std::wstring name = parent->GetName();
		if(name.length()<1) {
			name = TL(timeline);
		}
		menu->AddItem(GC::Hold(new MenuItem(name, idx, false, MenuItem::NotChecked)));
		++idx;
	}

	ref< Iterator< ref< TrackWrapper > > > it = parent->GetTracks();
	while(it->IsValid()) {
		ref<TrackWrapper> tw = it->Get();
		ref<SubTimeline> sub = tw->GetSubTimeline();
		if(sub) {
			ref<Timeline> tl = sub->GetTimeline();
			if(tl) {
				PopulateTimelineMenu(menu, sub->GetTimeline(), idx, tls, level + 1, self);
			}
		}
		it->Next();
	}
}

void TimelineWnd::DoTrackMenu(ref<TrackWrapper> cur, Pixels x, Pixels y) {
	ContextMenu menu;
	enum { renameTrack=1, deleteTrack, readdrTrack, lockTrack, duplicateTrack, insertFromControl, copyTrack, pasteTrack, cutTrack, rmNone, rmClient, rmMaster, rmBoth, saveTrack, /* moveToTimeline has to be last */ moveToTimeline };
	RunMode rm = cur->GetRunMode();

	menu.AddItem(GC::Hold(new MenuItem(TL(delete_track), deleteTrack, false, MenuItem::NotChecked, L"icons/recycle.png")));
	menu.AddItem(TL(duplicate_track), duplicateTrack);
	menu.AddItem(TL(lock_track), lockTrack, false, cur->IsLocked());
	menu.AddItem(TL(insert_from_control), insertFromControl);
	menu.AddItem(TL(save_track), saveTrack);

	// Copy/paste items
	menu.AddSeparator();
	menu.AddItem(GC::Hold(new MenuItem(TL(copy), copyTrack, false, MenuItem::NotChecked, Icons::GetIconPath(Icons::IconCopy), L"Ctrl-C")));
	menu.AddItem(GC::Hold(new MenuItem(TL(cut), cutTrack, false, MenuItem::NotChecked, Icons::GetIconPath(Icons::IconCut), L"Ctrl-X")));
	bool canPaste = Clipboard::IsObjectAvailable();
	menu.AddItem(GC::Hold(new MenuItem(TL(paste), canPaste ? pasteTrack : -1, false, MenuItem::NotChecked, Icons::GetIconPath(Icons::IconPaste), L"Ctrl-V")));

	// Run mode selection
	Flags<RunMode> supportedModes = cur->GetSupportedRunModes();
	bool playing = _controller->GetPlaybackState() != PlaybackStop;
	menu.AddSeparator(TL(runmode));
	menu.AddItem(TL(runmode_none), playing?-1:rmNone, false, (rm==RunModeDont)?MenuItem::RadioChecked:MenuItem::NotChecked);
	menu.AddItem(TL(runmode_client), playing?-1:(supportedModes.IsSet(RunModeClient)?rmClient:-1), false, (rm==RunModeClient)?MenuItem::RadioChecked:MenuItem::NotChecked);
	menu.AddItem(TL(runmode_master), playing?-1:(supportedModes.IsSet(RunModeMaster)?rmMaster:-1), false, (rm==RunModeMaster)?MenuItem::RadioChecked:MenuItem::NotChecked);
	menu.AddItem(TL(runmode_both), playing?-1:((supportedModes.IsSet(RunModeClient) && supportedModes.IsSet(RunModeMaster))?rmBoth:-1), false, (rm==RunModeBoth)?MenuItem::RadioChecked:MenuItem::NotChecked);

	strong<SubMenuItem> moveMenu = GC::Hold(new SubMenuItem(TL(move_track_to), false, MenuItem::NotChecked, null));
	
	int idx = moveToTimeline;
	std::map<int, ref<Timeline> > tls;
	PopulateTimelineMenu(moveMenu, Application::Instance()->GetModel()->GetTimeline(), idx, tls, 0, _timeline);
	if(tls.size()>0) {
		menu.AddItem(moveMenu);
	}

	int cmd = menu.DoContextMenu(this,x,y);

	if(cmd==deleteTrack) {
		RemoveTrack(cur);
	}
	else if(cmd==lockTrack) {
		cur->SetLocked(!cur->IsLocked());
	}
	else if(cmd==copyTrack) {
		Clipboard::SetClipboardObject(cur);
	}
	else if(cmd==cutTrack) {
		Clipboard::SetClipboardObject(cur);
		RemoveTrack(cur,false);
	}
	else if(cmd==pasteTrack) {
		DoPasteTrack();
	}
	else if(cmd==saveTrack) {
		std::wstring fn = Dialog::AskForSaveFile(this, TL(save_track), L"TJShow spoor template (*.ttx)\0*.ttx\0\0", L"ttx");
		if(fn.length()>0) {
			TiXmlDocument document;
			TiXmlElement root("track");
			cur->Save(&root);
			document.InsertEndChild(root);
			document.SaveFile(Mbs(fn).c_str());
		}
	}
	else if(cmd==duplicateTrack) {
		ref<TrackWrapper> duplicate = cur->Duplicate(_controller);
		if(duplicate) {
			_controller->GetTimeline()->AddTrack(duplicate);
			Application::Instance()->GetView()->OnAddTrack(duplicate);
		}
	}
	else if(cmd==insertFromControl) {
		ref<LiveControl> control = cur->GetLiveControl();
		if(control) {
			cur->GetTrack()->InsertFromControl(_controller->GetTime(), control, true);
		}
	}
	else if(cmd==rmNone) {
		cur->SetRunMode(RunModeDont);
	}
	else if(cmd==rmMaster) {
		cur->SetRunMode(RunModeMaster);
	}
	else if(cmd==rmClient) {
		cur->SetRunMode(RunModeClient);
	}
	else if(cmd==rmBoth) {
		cur->SetRunMode(RunModeBoth);
	}
	else if(cmd==copyTrack) {
		
	}
	else if(cmd>=moveToTimeline) {
		std::map<int, ref<Timeline> >::iterator tt = tls.find(cmd);
		if(tt!=tls.end()) {
			ref<Timeline> tl = tt->second;
			_timeline->RemoveTrack(cur);
			Application::Instance()->GetView()->OnRemoveTrack(cur);

			tl->AddTrack(cur);
			Application::Instance()->GetView()->OnAddTrack(cur);
		}
	}

	Update();
}

void TimelineWnd::SetCueList(ref<CueList> list) {
	_cues = list;
	Update();
}

ref<CueList> TimelineWnd::GetCueList() {
	return _cues;
}

void TimelineWnd::DoCopySelection() {
	Log::Write(L"TJShow/TimelineWnd", L"DoCopySelection");

	_copiedSelection.clear();

	ref< Iterator< ref<TrackWrapper> > > it = _timeline->GetTracks();
	while(it->IsValid()) {
		ref<TrackWrapper> tw = it->Get();
		if(MatchesFilter(tw)) {
			_copiedSelection.push_back(tw->GetTrack()->GetRange(_selStart, _selEnd));
		}
		it->Next();
	}

	Update();
}

void TimelineWnd::DoPasteSelection() {
	ref< Iterator< ref<TrackWrapper> > > it = _timeline->GetTracks();
	std::vector< ref<TrackRange> >::iterator itc = _copiedSelection.begin();

	while(it->IsValid()) {
		ref<TrackWrapper> tw = it->Get();
		if(MatchesFilter(tw)) {
			if(itc!=_copiedSelection.end()) {
				ref<TrackRange> range = *itc;
				if(range) {
 					range->Paste(tw->GetTrack(), _controller->GetTime());
				}
				itc++;
			}
		}
		it->Next();
	}

	Update();
}

void TimelineWnd::OnCopy() {
	if(_selMode) {
		DoCopySelection();
	}
	else {
		ref<TrackWrapper> current = _currentTrack;
		if(current) {
			if(_focusedItem) {
				if(_focusedItem.IsCastableTo<Serializable>()) {
					strong<TaggedObject> to = GC::Hold(new TaggedObject(_focusedItem));
					to->SetTag(L"is-item", true);
					Clipboard::SetClipboardObject(ref<Serializable>(to));
				}
				else {
					// Cannot copy, item cannot be serialized
					// TODO: MessageBeep?
				}
			}
			else {
				Clipboard::SetClipboardObject(current);
			}
			_copiedSelection.clear();
		}
	}
}

void TimelineWnd::OnCut() {
	if(_selMode) {
		DoCopySelection();
		DoDeleteSelection();
	}
	else {
		ref<TrackWrapper> current = _currentTrack;
		if(current) {
			OnCopy();
			if(_focusedItem) {
				_focusedItem->Remove();
			}
			else {
				_timeline->RemoveTrack(current);
			}
		}
	}
}

void TimelineWnd::OnPaste() {
	if(_selMode) {
		DoPasteSelection();
	}
	else {
		ref<GenericObject> go = GC::Hold(new GenericObject());
		ref<TaggedObject> to = GC::Hold(new TaggedObject(go));
		Clipboard::GetClipboardObject(to);

		if(to->HasTag(L"is-item")) {
			ref<TrackWrapper> current = _currentTrack;
			if(current) {
				if(!current->GetTrack()->PasteItemAt(_controller->GetTime(), &(go->_element))) {
					// Cannot paste this item on this track
					// TODO: do something like MessageBeep?
				}
			}
		}
		else {
			DoPasteTrack();
		}
	}
}

void TimelineWnd::DoPasteTrack() {
	TiXmlDocument doc;
	if(Clipboard::GetClipboardObject(doc)) {
		// Find out what kind of track it is and create one!
		TiXmlElement* root = doc.FirstChildElement("object");
		if(root!=0) {
			PluginHash ph = LoadAttributeSmall<PluginHash>(root, "plugin", -1);
			ref<PluginManager> pm = PluginManager::Instance();
			if(pm) {
				ref<PluginWrapper> plugin = pm->GetPluginByHash(ph);
				if(plugin) {
					ref<Application> app = Application::InstanceReference();
					ref<TrackWrapper> track = plugin->CreateTrack(_controller->GetPlayback(), app->GetNetwork(), _controller);
					if(track) {
						track->Load(root);
						
						// Takes care of setting all tracks (and possibly contained tracks in sub timelines) to new channels
						// The Clone() *must* happen after AddTrack, because otherwise the channel number doesn't increment when multiple
						// tracks are created at this time (for example, with a sub timeline).
						track->Clone(); 

						_timeline->AddTrack(track);
						app->GetView()->OnAddTrack(track);
						Repaint();
					}
				}
			}
		}
	}
}

ref<Icon> PlaybackStateIcons::_playingIcon;
ref<Icon> PlaybackStateIcons::_pauseIcon;
ref<Icon> PlaybackStateIcons::_waitingIcon;

ref<Icon> PlaybackStateIcons::GetTabIcon(PlaybackState p) {
	if(!_playingIcon) {
		_playingIcon = GC::Hold(new Icon(L"icons/tabs/timeline.png"));
		_pauseIcon = GC::Hold(new Icon(L"icons/tabs/pause.png"));
		_waitingIcon = GC::Hold(new Icon(L"icons/tabs/waiting.png"));
	}

	switch(p) {
		case PlaybackPlay:
			return _playingIcon;

		case PlaybackPause:
			return _pauseIcon;

		case PlaybackWaiting:
			return _waitingIcon;

		default:
			return null;
	}
}