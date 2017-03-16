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
#ifndef _TJTRACKSPOTEDITOR_H
#define _TJTRACKSPOTEDITOR_H


class DMXPositionEditorWnd: public Wnd, public Listener<ButtonWnd::NotificationClicked> {
	public:
		DMXPositionEditorWnd(ref<Wnd> parent, ref<DMXPositionTrack> track);
		virtual ~DMXPositionEditorWnd();
		virtual void Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme);
		virtual LRESULT Message(UINT msg, WPARAM wp, LPARAM lp);
		virtual void Layout();
		virtual void Notify(ref<Object> source, const ButtonWnd::NotificationClicked& nf);
		void SetRange(Range<Time> t);
		void Step(Time s);

	protected:
		virtual void OnCreated();
		Range<Time> GetRange();

		ref<DMXPositionTrack> _track;
		static const int ToolbarHeight;
		ref<EditWnd> _from;
		ref<EditWnd> _to;
		ref<ButtonWnd> _delete;
		ref<ButtonWnd> _leftStep;
		ref<ButtonWnd> _leftBlock;
		ref<ButtonWnd> _rightStep;
		ref<ButtonWnd> _rightBlock;
		HBRUSH _editBackground;
		bool _drawing;
		tj::shared::graphics::Color _editBackgroundColor;
		std::list< std::pair<int,int> > _points;
		MouseCapture _capture;
		
		enum ShowTimesMode {
			ShowTimesNone=0,
			ShowTimesMS,
			ShowTimesSequential,
			ShowTimesFormatted,
		};

		ShowTimesMode _timeMode;
};

#endif