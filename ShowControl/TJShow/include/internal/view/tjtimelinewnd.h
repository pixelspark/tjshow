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
#ifndef _TJTIMELINEWND_H
#define _TJTIMELINEWND_H

namespace tj {
	namespace show {
		class tj::shared::graphics::Bitmap;
		class tj::shared::graphics::Graphics;

		namespace view {
			class TimelineToolbarWnd;

			class PlaybackStateIcons {
				public:
					static ref<Icon> GetTabIcon(PlaybackState p);

				private:
					static ref<Icon> _playingIcon;
					static ref<Icon> _pauseIcon;
					static ref<Icon> _waitingIcon;
			};

			class TimelineWnd: public ChildWnd {
				friend class TimelineToolbarWnd;

				public:
					TimelineWnd(strong<Instance> inst);
					virtual ~TimelineWnd();
					
					void SetTimeline(ref<Timeline> md);
					ref<Timeline> GetTimeline();
					void SetCueList(ref<CueList> cues);
					ref<CueList> GetCueList();

					bool IsInSelectionMode() const;
					void SetSelectionMode(bool s);
					bool GetShowAddresses() const;
					void SetShowAddresses(bool s);
					void SetCurrentTrack(ref<TrackWrapper> tw);

					virtual LRESULT Message(UINT msg, WPARAM wp, LPARAM lp);
					virtual void Update();
					virtual void UpdateThreaded();
					virtual void Layout();
					void Jump(Time t, bool moveView=true);
					void OnSetTimeLength(Time t);
					void SetLock(bool l);
					bool IsLocked();
					
					int GetTrackHeight(ref<TrackWrapper> tr);
					ref<TrackWrapper> GetTrackAt(int x, int y);
					int GetZoom();
					virtual void Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme);
					void DoSelectionContextMenu(int x, int y);
					void RemoveTrack(ref<TrackWrapper> tw, bool askConfirmation = true);
					void InsertFromControls();
					void InsertFromCurrent();
					virtual std::wstring GetTabTitle() const;
					virtual ref<Icon> GetTabIcon() const;
					virtual void GetAccelerators(std::vector<Accelerator>& accels);
					
					virtual void UpdateTrackingPosition(); // if we're locked, update horizontal position
					virtual void SetFilter(const std::wstring& q);
					virtual void OnKey(Key k, wchar_t t, bool down, bool accelerator); // needs to be public so SplitTimelineWnd can forward events to us

				protected:
					void Interpolate(Time old, Time newTime, Time left, Time right);
					void UpdateCursor(Pixels x, Pixels y);
					ref<Cue> GetCueAt(Pixels x, Pixels y);
					bool MatchesFilter(ref<TrackWrapper> tr);
					
					virtual void OnSize(const Area& r);
					virtual void OnMouse(MouseEvent ev, Pixels x, Pixels y);
					virtual void OnFocus(bool focus);
					virtual void OnCopy();
					virtual void OnPaste();
					virtual void OnCut();
					virtual void OnTimer(unsigned int id);

					// TODO: FIXME: Selection copy/paste only works inside a timeline; doesn't use the Windows clipboard
					void DoCopySelection();
					void DoPasteSelection();
					void DoDeleteSelection();
					void DoPasteTrack();

					void DoGeneralContextMenu(Pixels x, Pixels y);
					void DoCueMenu(ref<Cue> cue, Pixels x, Pixels y);
					void DoTrackMenu(ref<TrackWrapper> track, Pixels x, Pixels y);
					
					strong<Instance> _controller;
					strong<Timeline> _timeline;
					strong<CueList> _cues;

					std::vector< ref<TrackRange> > _copiedSelection;
					ref<Cue> _activeCue;
					ref<TimelineToolbarWnd> _toolbar;
					ref<Item> _draggedItem;
					ref<Item> _focusedItem;
					ref<TrackWrapper> _draggedItemTrack;
					ref<TooltipWnd> _tt;
					weak<TrackWrapper> _currentTrack;

					Time _timeWidth; // in ms
					int _nameColumnWidth;
					bool _isDraggingSplitter;
					bool _isLocked;
					std::wstring _filterText;	
					Time _selStart, _selEnd;
					
					enum SelectionDrag {SelectionDragStart, SelectionDragEnd, SelectionDragNone};
					SelectionDrag _selDrag;
					bool _selMode;
					bool _zoomSelMode;
					bool _showAddresses;
					
					MouseCapture _capture;

					// Interpolation
					bool _isInterpolating;
					Time _left, _right;

					Icon _arrowIcon, _runIcon, _norunIcon, _bothrunIcon, _expandIcon, _collapseIcon, _lockedIcon;

					enum {
						KHeaderHeight = 23, 
						KDefaultTimeWidth = 10000,
						KDefaultMarginLeft = 5,
						KZoomX = 150,
						KZoomY = 2,
						KZoomW = 16,
						KZoomH = 16,
						KColumnWidthMin = 20,
						KColumnWidthMax = 400,
						KDefaultTrackHeight = 19,
						KPlayButtonX = 4,
						KPlayButtonY = 2,
						KPlayButtonSize = 16,
						KMinSizeX = 430,
						KMinSizeY = 200,
						KCueBarHeight = 7,
						KFollowTimerID = 421,
					};
			};
		}
	}
}

#endif