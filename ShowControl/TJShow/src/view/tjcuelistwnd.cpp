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
#include "../../include/internal/tjcontroller.h"
#include "../../include/internal/view/tjcuelistwnd.h"
#include "../../include/internal/view/tjtimelinewnd.h"
#include <windowsx.h>
#include <algorithm>
using namespace tj::shared::graphics;
using namespace tj::show::view;

/** CueAddChange **/
class CueAddChange: public Change {
	public:
		CueAddChange(ref<CueList> list, strong<Cue> cue);
		virtual ~CueAddChange();
		virtual void Redo();
		virtual void Undo();
		virtual bool CanUndo();
		virtual bool CanRedo();

	protected:
		weak<CueList> _cueList;
		strong<Cue> _cue;
};

class CueRemoveChange: public CueAddChange {
	public:
		CueRemoveChange(ref<CueList> list, strong<Cue> cue);
		virtual ~CueRemoveChange();
		virtual void Redo();
		virtual void Undo();
};

/** CueRemoveChange **/
CueRemoveChange::CueRemoveChange(ref<CueList> list, strong<Cue> cue): CueAddChange(list,cue) {
}

CueRemoveChange::~CueRemoveChange() {
}

void CueRemoveChange::Undo() {
	CueAddChange::Redo();
}

void CueRemoveChange::Redo() {
	CueAddChange::Undo();
}

/** CueAddChange **/
CueAddChange::CueAddChange(ref<CueList> cl, strong<Cue> cue): Change(TL(change_add_cue)+cue->GetTime().Format()), _cueList(cl), _cue(cue) {
}

CueAddChange::~CueAddChange() {
}

void CueAddChange::Redo() {
	ref<CueList> cl = _cueList;
	if(cl) {
		cl->AddCue(_cue);
	}
}

void CueAddChange::Undo() {
	ref<CueList> cl = _cueList;
	if(cl) {
		cl->RemoveCue(_cue);
	}
}

bool CueAddChange::CanRedo() {
	return ref<CueList>(_cueList);
}

bool CueAddChange::CanUndo() {
	return CanRedo();
}

/** CueWnd **/
CueWnd::CueWnd(strong<Instance> inst): ChildWnd(false), _controller(inst) {
	_list = GC::Hold(new CueListWnd(inst));
	_toolbar = GC::Hold(new CueListToolbarWnd(_list.GetPointer()));
	_next = GC::Hold(new ButtonWnd(L"icons/go-next.png", TL(cue_next_unknown)));
	_next->SetDisabled(true);
	Add(_list);
	Add(_toolbar);
	Add(_next);
	Update();
}

CueWnd::~CueWnd() {
}

void CueWnd::Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme) {
}

void CueWnd::OnCreated() {
	_next->EventClicked.AddListener(ref< Listener<ButtonWnd::NotificationClicked> >(this));
}

void CueWnd::Notify(ref<Object> source, const ButtonWnd::NotificationClicked& ni) {
	if(source==ref<Object>(_next)) {
		_controller->Fire();
		Update();
	}
}

void CueWnd::OnSize(const Area& ns) {
	Layout();
	Repaint();
}

void CueWnd::Update() {
	PlaybackState currentState = _controller->GetPlaybackState();
	Time current = _controller->GetTime();
	ref<Cue> next = _controller->GetCueList()->GetNextCue(current);
	if(next) {
		std::wstring name = next->GetName();
		if(name.length()>0) {
			_next->SetText(name.c_str());
		}
		else {
			_next->SetText(next->GetTime().Format().c_str());
		}
		_next->SetDisabled(false);
	}
	else {
		_next->SetDisabled(true);
		_next->SetText(TL(cue_next_unknown));
	}
}

void CueWnd::Layout() {
	Area rc = GetClientArea();
	if(_list) {
		_toolbar->Fill(LayoutTop, rc);
		rc.Widen(0,0,0,1); // fixup for extra button border
		_next->Fill(LayoutBottom, rc);
		rc.Narrow(0,0,0,0);
		_list->Fill(LayoutFill, rc);
	}
}

CueListWnd::CueListWnd(strong<Instance> c): 
	_actionPlayIcon(L"icons/play.png"),
	_actionPauseIcon(L"icons/pause.png"),
	_actionStopIcon(L"icons/stop.png"),
	_triggerIcon(L"icons/link.png"),
	_guardIcon(L"icons/guarded.png"),
	_conditionIcon(L"icons/condition.png"),
	_assignmentIcon(L"icons/assign.png"),
	_controller(c),
	_cues(c->GetCueList()) {

	SetText(TL(cues));
	SetWantMouseLeave(true);
	SetVerticallyScrollable(true);
	_in = false;
	_showNameless = true;
	_onlyPlayCues = false;
	_format = TimeFormatFriendly;
}

CueListWnd::~CueListWnd() {
}

void CueListWnd::Update() {
	Layout();
	Repaint();
}

void CueListWnd::Layout() {
	UpdateScrollBars();
}

int CueListWnd::GetItemHeightInPixels() const {
	strong<Theme> theme = ThemeManager::GetTheme();
	return theme->GetMeasureInPixels(Theme::MeasureListItemHeight);
}

void CueListWnd::UpdateScrollBars() {
	Area rc = GetClientArea();
	// calculate number of cues with a name
	int count = 0;
	std::vector< ref<Cue> >::iterator it = _cues->GetCuesBegin();
	while(it!=_cues->GetCuesEnd()) {
		ref<Cue> cue = *it;
		if(IsCueVisible(cue)) {
			++count;
		}
		++it;
	}

	unsigned int h = (count+1) * (GetItemHeightInPixels());
	SetVerticalScrollInfo(Range<int>(0, h), rc.GetHeight());
}

void CueListWnd::Paint(Graphics& g, strong<Theme> theme) {
	Area rect = GetClientArea();
	int offset = -int(GetVerticalPos());
	
	StringFormat sf;
	sf.SetTrimming(StringTrimmingEllipsisPath);
	sf.SetFormatFlags(StringFormatFlagsLineLimit);
	sf.SetLineAlignment(StringAlignmentCenter);

	g.Clear(theme->GetColor(Theme::ColorBackground));
	theme->DrawInsetRectangleLight(g, rect);

	if(_cues->GetCuesBegin()==_cues->GetCuesEnd()) {
		// no cues
		std::wstring text(TL(no_cues));
		SolidBrush descBrush(theme->GetColor(Theme::ColorHint));
		StringFormat sf;
		sf.SetAlignment(StringAlignmentCenter);

		// The 'empty text' is placed 0.5 times the header height from the top of the list window
		// this is more or less like what ListWnd and TimelineWnd do.
		strong<Theme> theme = ThemeManager::GetTheme();
		Pixels headHeight = theme->GetMeasureInPixels(Theme::MeasureListHeaderHeight);
		rect.Narrow(0, Pixels(headHeight*0.5f), 0, 0);
		g.DrawString(text.c_str(), (INT)text.length(), theme->GetGUIFont(), rect, &sf, &descBrush);
	}
	else {
		SolidBrush textBrush(theme->GetColor(Theme::ColorText));
		SolidBrush textBrushDisabled(theme->GetColor(Theme::ColorActiveEnd));
		std::vector< ref<Cue> >::iterator it = _cues->GetCuesBegin();
		int h = offset + rect.GetTop();
		unsigned int n = 1;
		const static Pixels KIconsWidth = 5*16;

		bool iconColVisible = (rect.GetWidth() > KIconsWidth + KTimeColumnWidth + KIconsWidth);
		bool timeColVisible = (rect.GetWidth() > KTimeColumnWidth + KIconsWidth);

		while(it!=_cues->GetCuesEnd()) {
			ref<Cue> cue = *it;
			Area row(rect.GetLeft(), h, rect.GetWidth(), GetItemHeightInPixels());
			Area textArea = row;
			textArea.Narrow(0,0,timeColVisible?KTimeColumnWidth:0,0);
			textArea.Narrow(0,0,iconColVisible?KIconsWidth:0,0);
			Area iconArea(textArea.GetRight(), row.GetTop(), iconColVisible?KIconsWidth:0, row.GetHeight());
			Area timeArea(iconArea.GetRight(), row.GetTop(), timeColVisible?KTimeColumnWidth:0, row.GetHeight());

			if(!IsCueVisible(cue)) {
				++it;
				continue;
			}

			if(h<-19 || h>(rect.GetHeight()+19)) {
				h += GetItemHeightInPixels();
				++it;
				continue;
			}

			if(cue==_over && _in) {
				LinearGradientBrush lbr(PointF(0.0f, float(h)), PointF(0.0f, float(h+GetItemHeightInPixels())), theme->GetColor(Theme::ColorActiveStart), theme->GetColor(Theme::ColorActiveEnd));
				g.FillRectangle(&lbr, row);

				LinearGradientBrush gbr(PointF(0.0f, float(h)), PointF(0.0f, float(h+GetItemHeightInPixels()/2)), theme->GetColor(Theme::ColorGlassStart), theme->GetColor(Theme::ColorGlassEnd));
				row.SetHeight(row.GetHeight()/2);
				g.FillRectangle(&lbr, row);
			}

			SolidBrush cueColor(cue->GetColor());
			g.FillRectangle(&cueColor, Rect(6, h+6, 8, 8));
			textArea.Narrow(15,0,0,0);
			if(cue->GetName().length()>0) {
				g.DrawString(cue->GetName().c_str(), (INT)cue->GetName().length(), theme->GetGUIFont(), textArea, &sf, &textBrush);
			}
			else {
				std::wstring s = TL(cue_nameless);
				g.DrawString(s.c_str(), (INT)s.length(), theme->GetGUIFont(), textArea, &sf, &textBrushDisabled);
			}

			// draw icons
			if(iconColVisible) {
				Area linkIconArea = iconArea;
				linkIconArea.SetWidth(16);
				linkIconArea.SetHeight(16);
				linkIconArea.SetX(iconArea.GetRight()-16);


				switch(cue->GetAction()) {
					case Cue::ActionStart:
						g.DrawImage(_actionPlayIcon, linkIconArea);
						iconArea.Narrow(0,0,16,0);
						break;
					case Cue::ActionStop:
						g.DrawImage(_actionStopIcon, linkIconArea);
						iconArea.Narrow(0,0,16,0);
						break;
					case Cue::ActionPause:
						g.DrawImage(_actionPauseIcon, linkIconArea);
						iconArea.Narrow(0,0,16,0);
						break;
				}

				if(cue->IsLinkedToDistantCue()) {
					Area linkIconArea = iconArea;
					linkIconArea.SetWidth(16);
					linkIconArea.SetHeight(16);
					linkIconArea.SetX(iconArea.GetRight()-16);
					g.DrawImage(_triggerIcon, linkIconArea);
					iconArea.Narrow(0,0,16,0);
				}

				if(cue->IsUsingCapacity() || cue->IsReleasingCapacity()) {
					Area linkIconArea = iconArea;
					linkIconArea.SetWidth(16);
					linkIconArea.SetHeight(16);
					linkIconArea.SetX(iconArea.GetRight()-16);
					g.DrawImage(_guardIcon, linkIconArea);
					iconArea.Narrow(0,0,16,0);
				}

				if(cue->HasExpression()) {
					Area linkIconArea = iconArea;
					linkIconArea.SetWidth(16);
					linkIconArea.SetHeight(16);
					linkIconArea.SetX(iconArea.GetRight()-16);
					_conditionIcon.Paint(g, linkIconArea, false);
					iconArea.Narrow(0,0,16,0);
				}

				if(cue->HasAssignments()) {
					Area linkIconArea = iconArea;
					linkIconArea.SetWidth(16);
					linkIconArea.SetHeight(16);
					linkIconArea.SetX(iconArea.GetRight()-16);
					_assignmentIcon.Paint(g, linkIconArea, false);
					iconArea.Narrow(0,0,16,0);
				}

				if(rect.GetRight()-KTimeColumnWidth > KTimeColumnWidth) {
					std::wstring time;
					switch(_format) {
						case TimeFormatMS:
							time = Stringify(cue->GetTime()) + L"ms";
							break;

						default:
						case TimeFormatFriendly:
							time = cue->GetTime().Format();
							break;

						case TimeFormatSequential:
							time = Stringify(n);
							break;
					}

					g.DrawString(time.c_str(), (INT)time.length(), theme->GetGUIFont(), timeArea, &sf, &textBrush);
				}
			}
			
			h += GetItemHeightInPixels();
			++it;
			n++;
		}
	}
}

void CueListWnd::OnMouse(MouseEvent ev, Pixels x, Pixels y) {
	if(ev==MouseEventMove) {
		_over = GetCueAt(y);
		_in = true;
		Repaint();
	}
	else if(ev==MouseEventLeave) {
		_in = false;
		Repaint();
	}
	else if(ev==MouseEventLDown) {
		ref<Cue> x = GetCueAt(y);
		if(x) {
			_controller->Trigger(x,false);
		}
		Repaint();
	}
	else if(ev==MouseEventRDown) {
		OnContextMenu(x,y);
	}
	ChildWnd::OnMouse(ev,x,y);
}

void CueListWnd::OnContextMenu(Pixels x, Pixels y) {
	ref<Cue> cue = GetCueAt(y);
	DoCueMenu(cue,x,y); // cue can be null
	Repaint();
}

void CueListWnd::OnSize(const Area& ns) {
	Layout();
	Repaint();
}

ref<Cue> CueListWnd::GetCueAt(Pixels uh) {
	uh += GetVerticalPos();
	int h = 0;
	std::vector< ref<Cue> >::iterator it = _cues->GetCuesBegin();
	while(it!=_cues->GetCuesEnd()) {
		ref<Cue> cue = *it;

		if(!IsCueVisible(cue)) {
			++it;
			continue;
		}
		
		h += 19;
		if(uh<h) {
			return cue;
		}
		++it;
	}

	return 0;
}

bool CueListWnd::IsCueVisible(ref<Cue> cue) {
	if(!_showNameless && cue->GetName().length()<=0) {
		return false;
	}

	if(_onlyPlayCues && cue->GetAction()!=Cue::ActionStart) {
		return false;
	}

	if(_filter.length()>0) {
		std::wstring cueName = cue->GetName();
		std::transform(cueName.begin(), cueName.end(), cueName.begin(), tolower);
		if(cueName.find(_filter, 0)==std::string::npos) {
			return false;
		}
	}

	return true;
}

ref<CueListWnd> CueWnd::GetCueListWindow() {
	return _list;
}

void CueListWnd::DoCueMenu(ref<Cue> cue, Pixels x, Pixels y) {
	DoCueMenu(this, cue, _cues, x, y);
}

void CueListWnd::DoCueMenu(ref<Wnd> wnd, ref<Cue> cue, ref<CueList> list, Pixels x, Pixels y) {
	ContextMenu menu;
	if(cue) {
		menu.AddItem(TL(properties), 1, true);
		menu.AddSeparator();

		menu.AddItem(GC::Hold(new MenuItem(TL(delete_cue), 2, false, MenuItem::NotChecked, L"icons/recycle.png")));
		menu.AddItem(GC::Hold(new MenuItem(TL(cue_bind_to_input), 3, false, MenuItem::NotChecked, L"icons/input.png")));
		menu.AddSeparator();
		menu.AddItem(GC::Hold(new MenuItem(TL(copy), 4, false, MenuItem::NotChecked, Icons::GetIconPath(Icons::IconCopy), L"Ctrl-C")));
		menu.AddItem(GC::Hold(new MenuItem(TL(cut), 5, false, MenuItem::NotChecked, Icons::GetIconPath(Icons::IconCut), L"Ctrl-X")));
	}

	bool canPaste = Clipboard::IsObjectAvailable() && wnd.IsCastableTo<CueListWnd>();
	menu.AddItem(GC::Hold(new MenuItem(TL(paste), canPaste ? 6 : -1, false, MenuItem::NotChecked, Icons::GetIconPath(Icons::IconPaste), L"Ctrl-V")));

	int cmd = menu.DoContextMenu(wnd, x, y);

	if(cmd==2) {
		UndoBlock ub;
		UndoBlock::AddAndDoChange(GC::Hold(new CueRemoveChange(list, cue)));
		Application::Instance()->Update();
	}
	else if(cmd==1) {
		ref<Path> path = GC::Hold(new Path());
		path->Add(Application::Instance()->GetModel()->CreateModelCrumb());
		path->Add(cue->CreateCrumb());
		Application::Instance()->GetView()->Inspect(cue,path);
	}
	else if(cmd==3) {
		cue->DoBindInputDialog(wnd);
	}
	else if(cmd==4 || cmd==5) {
		Clipboard::SetClipboardObject(cue);
		if(cmd==5) {
			UndoBlock ub;
			UndoBlock::AddAndDoChange(GC::Hold(new CueRemoveChange(list, cue)));
			Application::Instance()->Update();
		}
	}
	else if(cmd==6) {
		UndoBlock ub;
		ref<CueListWnd>(wnd)->OnPaste();
	}

	wnd->Layout();
	wnd->Repaint();
}

void CueListWnd::OnPaste() {
	ref<Cue> cue = GC::Hold(new Cue(_cues));
	if(Clipboard::GetClipboardObject(cue) && cue->GetTime()>=Time(0)) {
		cue->Clone();
		UndoBlock::AddAndDoChange(GC::Hold(new CueAddChange(_cues, cue)));
	}
}

void CueListWnd::OnCopy() {
}

void CueListWnd::OnCut() {
}

bool CueListWnd::GetShowNameless() const {
	return _showNameless;
}

void CueListWnd::SetShowNameless(bool t) {
	_showNameless = t;

	ref<Settings> st = GetSettings();
	if(st) {
		st->SetValue(L"show-nameless", t?L"yes":L"no");
	}
	Layout();
	Update();
}

TimeFormat CueListWnd::GetTimeFormat() const {
	return _format;
}

void CueListWnd::SetTimeFormat(TimeFormat f) {
	_format = f;

	ref<Settings> st = GetSettings();
	if(st) {
		st->SetValue(L"time-format", Stringify((int)_format));
	}
	Update();
}

void CueListWnd::SetShowPlayCuesOnly(bool t) {
	_onlyPlayCues = t;

	ref<Settings> st = GetSettings();
	if(st) {
		st->SetValue(L"only-play-cues", t?L"yes":L"no");
	}
	Layout();
	Update();
}

void CueListWnd::SetFilter(const std::wstring& filter) {
	_filter = filter;
	std::transform(_filter.begin(), _filter.end(), _filter.begin(), tolower);
	Layout();
	Update();
}

bool CueListWnd::GetShowPlayCuesOnly() const {
	return _onlyPlayCues;
}

/* CueListToolbarWnd */
CueListToolbarWnd::CueListToolbarWnd(CueListWnd* cw): SearchToolbarWnd() {
	_cw = cw;
	Add(GC::Hold(new ToolbarItem(KAddCue, L"icons/add.png", TL(cue_add))));
	Add(GC::Hold(new ToolbarItem(KViewSettings, L"icons/viewsettings.png", TL(cue_view_settings))));
	SetSearchBoxRightMargin(5);
}

CueListToolbarWnd::~CueListToolbarWnd() {
}

void CueListToolbarWnd::OnSearchChange(const std::wstring& q) {
	_cw->SetFilter(q);
}

void CueListToolbarWnd::OnCommand(ref<ToolbarItem> ti) {
	OnCommand(ti->GetCommand());
}


void CueListWnd::OnSettingsChanged() {
	ref<Settings> st = GetSettings();
	_format = (TimeFormat)StringTo<int>(st->GetValue(L"time-format", Stringify((int)_format)), (int)_format);
	_showNameless = (st->GetValue(L"show-nameless", L"yes")!=L"no");
	_onlyPlayCues = (st->GetValue(L"play-cues-only", L"yes")!=L"no");
	Update();
}

void CueListToolbarWnd::OnCommand(int c) {
	UndoBlock ub;

	switch(c) {
		case KAddCue: {
			ref<Instance> controller = _cw->_controller;
			
			ref<Cue> cue = GC::Hold(new Cue(L"", Cue::DefaultColor, controller->GetTime(), controller->GetCueList()));
			UndoBlock::AddAndDoChange(GC::Hold(new CueAddChange(controller->GetCueList(), cue)));

			if(controller.IsCastableTo<Controller>()) {
				ref<Controller>(controller)->UpdateUI();
			}

			_cw->Layout(); // Updates the scroll bars
			_cw->Repaint();
			break;
		}

		case KViewSettings: {
			ContextMenu cm;
			enum {MCShowNameless=1,MCFormatTime, MCMSTime, MCSequentialTime, MCOnlyPlayCues};

			cm.AddItem(TL(cuelist_only_show_play_cues), MCOnlyPlayCues, false, _cw->GetShowPlayCuesOnly());
			cm.AddItem(TL(cuelist_show_nameless), MCShowNameless, false, _cw->GetShowNameless());
			cm.AddSeparator();
			cm.AddItem(TL(time_format_friendly), MCFormatTime, false, (_cw->GetTimeFormat()==TimeFormatFriendly)?MenuItem::RadioChecked:MenuItem::NotChecked);
			cm.AddItem(TL(time_format_ms), MCMSTime, false, (_cw->GetTimeFormat()==TimeFormatMS)?MenuItem::RadioChecked:MenuItem::NotChecked);
			cm.AddItem(TL(time_format_sequential), MCSequentialTime, false, (_cw->GetTimeFormat()==TimeFormatSequential)?MenuItem::RadioChecked:MenuItem::NotChecked);

			strong<Theme> theme = ThemeManager::GetTheme();
			switch(cm.DoContextMenu(this, GetButtonX(KViewSettings), theme->GetMeasureInPixels(Theme::MeasureToolbarHeight))) {
				case MCShowNameless:
					_cw->SetShowNameless(!_cw->GetShowNameless());
					break;

				case MCOnlyPlayCues:
					_cw->SetShowPlayCuesOnly(!_cw->GetShowPlayCuesOnly());
					break;

				case MCMSTime:
					_cw->SetTimeFormat(TimeFormatMS);
					break;

				case MCFormatTime:
					_cw->SetTimeFormat(TimeFormatFriendly);
					break;

				case MCSequentialTime:
					_cw->SetTimeFormat(TimeFormatSequential);
					break;
			}
			break;
		}
	}
}

