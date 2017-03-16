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
#include "../include/tjingle.h"
#include "../resource.h"
#include <iomanip>
using namespace tj::shared;
using namespace tj::jingle;
using namespace tj::shared::graphics;

namespace tj {
	namespace jingle {
		class JingleSearchListWnd: public ListWnd {
			public:
				JingleSearchListWnd();
				virtual ~JingleSearchListWnd();
				virtual void SetSearchString(const std::wstring& q);
				virtual int GetItemCount();
				virtual void PaintItem(int idx, graphics::Graphics& g, tj::shared::Area& rc,const tj::shared::ColumnInfo& ci);
				virtual void OnClickItem(int id, int col, Pixels x, Pixels y);
				virtual void OnDoubleClickItem(int id, int col);
				virtual void OnTimer(unsigned int id);
				virtual void OnKey(Key k, wchar_t t, bool down, bool isAccel);

			protected:
				std::deque< ref<Jingle> > _results;
		};

		class JingleSearchPopupWnd: public PopupWnd {
			public:
				JingleSearchPopupWnd(ref<Wnd> parent);
				virtual ~JingleSearchPopupWnd();
				virtual void OnCreated();
				virtual void Paint(graphics::Graphics& g, strong<Theme> theme);
				virtual void SetSearchString(const std::wstring& q);
				virtual void Layout();
				virtual void OnSize(const Area& ns);
				virtual void FocusOnResults();

			protected:
				ref<JingleSearchListWnd> _searchList;
		};
	}
}

/** JingleSearchListWnd **/
JingleSearchListWnd::JingleSearchListWnd() {
	AddColumn(TL(jingle_search_results), 1, 1.0f, true);
	StartTimer(Time(50), 1);
}

JingleSearchListWnd::~JingleSearchListWnd() {
}

void JingleSearchListWnd::OnKey(Key k, wchar_t t, bool down, bool isAccel) {
	if(k==KeyReturn) {
		int r = GetSelectedRow();
		if(r>=0 && r<int(_results.size())) {
			ref<Jingle> ji = _results.at(r);
			if(ji) {
				ji->Play();
			}
		}
	}
	else if(k==KeyEscape) {
		ref<PopupWnd> parent = GetParent();
		if(parent) {
			parent->Show(false);
		}
		JingleApplication::Instance()->GetView()->Focus();
	}
	else {
		ListWnd::OnKey(k, t, down, isAccel);
	}
}

void JingleSearchListWnd::OnTimer(unsigned int id) {
	if(id==1 && _results.size()>0 && IsShown()) {
		Repaint();
	}
}

void JingleSearchListWnd::SetSearchString(const std::wstring& q) {
	_results.clear();
	strong<JingleApplication> ja = JingleApplication::Instance();
	std::wstring lowQ = q;
	Util::StringToLower(lowQ);
	ja->FindJinglesByName(lowQ,_results);
	OnSize(GetClientArea());
	Update();
	Repaint();
}

void JingleSearchListWnd::OnClickItem(int id, int col, Pixels x, Pixels y) {
	SetSelectedRow(id);
}

void JingleSearchListWnd::OnDoubleClickItem(int id, int col) {
	if(id>=0 && id < int(_results.size())) {
		ref<Jingle> ji = _results.at(id);
		if(ji) {
			ji->Play();
		}
	}
}

int JingleSearchListWnd::GetItemCount() {
	return (int)_results.size();
}

void JingleSearchListWnd::PaintItem(int idx, graphics::Graphics& g, tj::shared::Area& rc,const tj::shared::ColumnInfo& ci) {
	ref<Jingle> ji = _results.at(idx);
	if(ji) {
		strong<Theme> theme = ThemeManager::GetTheme();
		
		// Draw progress, if playing
		if(ji->IsPlaying()) {
			Area prc(rc.GetLeft(), rc.GetTop(), rc.GetWidth(), rc.GetHeight());
			LinearGradientBrush pback(PointF(0.0f, float(rc.GetTop())), PointF(0.0f, float(rc.GetBottom())), theme->GetColor(Theme::ColorProgressBackgroundStart),theme->GetColor(Theme::ColorProgressBackgroundStart));
			g.FillRectangle(&pback, prc);

			float pos = ji->GetPosition();
			bool warning = pos>0.65f;
			Color startColor = warning?Color(99,0,0):theme->GetColor(Theme::ColorProgress);
			Color endColor = warning?Color(255,0,0):theme->GetColor(Theme::ColorProgress);

			LinearGradientBrush ptop(PointF(float(rc.GetLeft()), float(rc.GetTop())), PointF(float(rc.GetRight()), float(rc.GetTop())), Theme::ChangeAlpha(startColor, 127), endColor);
			g.FillRectangle(&ptop, RectF(float(rc.GetLeft()+2), float(rc.GetTop()), float(rc.GetWidth()-4)*ji->GetPosition(), float(rc.GetHeight()-2)));

			// glassify
			prc.Narrow(0,0,0,11);
			LinearGradientBrush pglass(PointF(float(prc.GetLeft()), float(prc.GetTop())), PointF(float(prc.GetLeft()), float(prc.GetBottom())),  theme->GetColor(Theme::ColorGlassStart), theme->GetColor(Theme::ColorGlassEnd));
			g.FillRectangle(&pglass, prc);
		}

		// Draw jingle name
		StringFormat sf;
		sf.SetAlignment(StringAlignmentNear);
		Font* font = theme->GetGUIFont();
		SolidBrush tbr(theme->GetColor(Theme::ColorText));
		DrawCellText(g, &sf, &tbr, font, 1, rc, ji->GetName());
	}
}

/** JingleSearchPopupWnd **/
JingleSearchPopupWnd::JingleSearchPopupWnd(ref<Wnd> parent): PopupWnd(parent,false) {
}

JingleSearchPopupWnd::~JingleSearchPopupWnd() {
}

void JingleSearchPopupWnd::OnCreated() {
	PopupWnd::OnCreated();
	_searchList = GC::Hold(new JingleSearchListWnd());
	Add(_searchList);
	Layout();
}

void JingleSearchPopupWnd::OnSize(const Area& ns) {
	PopupWnd::OnSize(ns);
	Layout();
}

void JingleSearchPopupWnd::FocusOnResults() {
	_searchList->SetSelectedRow(0);
	_searchList->Focus();
}

void JingleSearchPopupWnd::Layout() {
	Area rc = GetClientArea();
	rc.Narrow(1,1,1,1);
	_searchList->Fill(LayoutFill, rc);
}

void JingleSearchPopupWnd::SetSearchString(const std::wstring& q) {
	_searchList->SetSearchString(q);
}

void JingleSearchPopupWnd::Paint(graphics::Graphics& g, strong<Theme> theme) {
	SolidBrush backBrush(theme->GetColor(Theme::ColorActiveStart));
	Area rc = GetClientArea();
	g.FillRectangle(&backBrush, rc);
}

/** JingleToolbarWnd **/
JingleToolbarWnd::JingleToolbarWnd(): _logo(L"icons/toolbar/logo.png") {
	Add(GC::Hold(new ToolbarItem(JID_NEW, L"icons/toolbar/new.png", TL(jingle_new))));
	Add(GC::Hold(new ToolbarItem(JID_OPEN, L"icons/toolbar/open.png", TL(jingle_open))));
	Add(GC::Hold(new ToolbarItem(JID_SAVE, L"icons/toolbar/save.png", TL(jingle_save), true)));
	Add(GC::Hold(new ToolbarItem(JID_THEME, L"icons/theme.png", TL(toolbar_theme), false)));
	Add(GC::Hold(new ToolbarItem(JID_SETTINGS, L"icons/toolbar/settings.png", TL(jingle_settings), false)));
	Add(GC::Hold(new ToolbarItem(JID_STOP_ALL, L"icons/stop.png", TL(jingle_stop_all_global), false)), true);
	SetTimer(GetWindow(), 0, 1000, NULL);
}

JingleToolbarWnd::~JingleToolbarWnd() {
}

void JingleToolbarWnd::OnSearchChange(const tj::shared::String& q) {
	if(!_searchWnd) {
		_searchWnd = GC::Hold(new JingleSearchPopupWnd(null));
	}

	_searchWnd->SetSearchString(q);

	if(q.length()>0) {
		Area rc = GetClientArea();
		Pixels w = 200;
		_searchWnd->SetSize(w, w*2);
		_searchWnd->PopupAt(rc.GetRight()-w, rc.GetBottom(), this);
	}
	else {
		_searchWnd->Show(false);
	}
}

void JingleToolbarWnd::CancelSearch() {
	if(_searchWnd) {
		_searchWnd->Show(false);
	}
}

void JingleToolbarWnd::StartSearch() {
	SearchToolbarWnd::SetSearchBoxText(L"");
	SearchToolbarWnd::FocusSearchBox();
}

void JingleToolbarWnd::OnCommand(int c) {
	if(c==JID_THEME) {
		ContextMenu cm;
		std::vector< ref<Theme> > themes;
		ThemeManager::ListThemes(themes);

		std::vector< ref<Theme> >::iterator it = themes.begin();
		int i = 1;
		ref<Theme> currentTheme = ThemeManager::GetTheme();

		while(it!=themes.end()) {
			ref<Theme> theme = *it;
			if(theme) {
				cm.AddItem(theme->GetName(), i, false, currentTheme==theme);
				++i;
			}
			++it;
		}

		Area rc = GetClientArea();
		int r = cm.DoContextMenu(this, GetButtonX(JID_THEME), rc.GetBottom());
		if(r>0 && (r-1)<int(themes.size())) {
			ref<Theme> select = themes.at(r-1);
			if(select) {
				ThemeManager::SelectTheme(strong<Theme>(select));
			}
		}

		JingleApplication::Instance()->OnThemeChanged();
	}
	else {
		JingleApplication::Instance()->Command(c);
	}
}

void JingleToolbarWnd::OnCommand(ref<ToolbarItem> ti) {
	OnCommand(ti->GetCommand());
}

void JingleToolbarWnd::OnSize(const Area& ns) {
	ToolbarWnd::OnSize(ns);
	Repaint();
}

void JingleToolbarWnd::OnSearchCommit() {
	if(_searchWnd) {
		_searchWnd->FocusOnResults();
	}
}

void JingleToolbarWnd::Paint(tj::shared::graphics::Graphics& g, ref<Theme> theme) {
	ToolbarWnd::Paint(g, theme);
	Area rc = GetClientArea();

	// draw logo
	g.DrawImage(_logo, RectF(float(rc.GetRight()-41), 0.0f, 41.0f, float(theme->GetMeasureInPixels(Theme::MeasureToolbarHeight))));
	_logo.Paint(g, Area(rc.GetRight()-41, 0, 41, theme->GetMeasureInPixels(Theme::MeasureToolbarHeight)));

	// draw time
	SYSTEMTIME time;
	GetLocalTime(&time);
	std::wostringstream wos;

	wos << std::setfill(L'0') << std::setw(2) << std::setprecision(2) << time.wHour << L":";
	wos << std::setfill(L'0') << std::setw(2) << std::setprecision(2) << time.wMinute;
	std::wstring tijd = wos.str();
	StringFormat sf;

	Color textColor = theme->GetColor(Theme::ColorText);
	Color disabledColor = theme->GetColor(Theme::ColorDisabledOverlay);
	SolidBrush tbr(Color(disabledColor.GetA(), textColor.GetR(), textColor.GetG(), textColor.GetB()));
	

	g.DrawString(tijd.c_str(), (int)tijd.length(), theme->GetGUIFontBold(), RectF(float(rc.GetWidth())-80.0f, 5.0f, 55.0f, theme->GetMeasureInPixels(Theme::MeasureToolbarHeight)-10.0f), &sf, &tbr); 
}