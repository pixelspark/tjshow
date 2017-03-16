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
using namespace tj::jingle;
using namespace tj::shared;
using namespace tj::shared::graphics;

class JingleAboutWnd: public ChildWnd {
	public:
		JingleAboutWnd(): ChildWnd(L"") {
			for(int a=0;a<KTotalValues;a++) {
				float r = pow(float(a)/float(KTotalValues), 2.0f);
				_values[a] = r * float(rand())/float(RAND_MAX) + (1.0f-r)*sin(r*4.0f*2.0f*3.14159f);
			}
			StartTimer(Time(2000), 1);
		}

		virtual ~JingleAboutWnd() {
		}

		virtual void OnTimer(unsigned int id) {
			Repaint();
		}

		virtual void Paint(graphics::Graphics& g, strong<Theme> theme) {
			Area rc = GetClientArea();
			SolidBrush back(theme->GetColor(Theme::ColorBackground));
			g.FillRectangle(&back, rc);

			PointF points[KTotalValues];
			float x = float(rc.GetLeft());
			float i = float(rc.GetWidth()) / float(KTotalValues);
			for(int a=0;a<KTotalValues;a++) {
				points[a] = PointF(x, _values[a]*float(rc.GetHeight()/2));
				x += i;
			}

			for(int a=-5;a<5;a++) {
				GraphicsContainer gg = g.BeginContainer();
				float t = float(rand())/float(RAND_MAX);
				Pen penCurve(Theme::ChangeAlpha(theme->GetColor(Theme::ColorHighlightEnd), 75.0f*t), 5.0f*t);
				g.RotateTransform(a*10.0f);
				g.TranslateTransform(0.0f, float(rc.GetHeight()/2));
				g.DrawCurve(&penCurve, &(points[0]), KTotalValues);
				g.EndContainer(gg);
			}

			std::wstring about = L"TJingle (C) Tommy van der Vorst, 2006-2017\r\n\r\nBased on technology from TJShow, an interactive show-controller. See www.tjshow.com for more information. Uses the BASS (www.un4seen.com) library for audio playback. Thanks to Joost Wijgers for his feedback and testing various parts of this software.\r\n\r\nThis software may be freely redistributed. It may not however be charged for or modified in any way without written permission from the author(s). Please regularly check www.tjshow.com for updates. This software is provided 'as is', and the author(s) do not accept any liability in case of failures and/or any other damage caused by this program.";
			StringFormat sf;
			sf.SetAlignment(StringAlignmentCenter);
			sf.SetLineAlignment(StringAlignmentCenter);
			SolidBrush tbr(theme->GetColor(Theme::ColorText));

			RectF bounds;
			g.MeasureString(about.c_str(), (int)about.length(), theme->GetGUIFont(), rc, &sf, &bounds);
			SolidBrush disabled(theme->GetColor(Theme::ColorDisabledOverlay));
			g.FillRectangle(&disabled, bounds);

			g.DrawString(about.c_str(),(int)about.length(), theme->GetGUIFont(), rc, &sf, &tbr);
		}

		const static int KTotalValues = 100;
		float _values[KTotalValues];
};


JingleView::JingleView(): RootWnd(TL(jingle_application_name)) {
	SetStyle(WS_OVERLAPPEDWINDOW);
	SetQuitOnClose(true);
	RegisterHotKey(GetWindow(), 1, MOD_WIN, L'J');
}

void JingleView::Create() {
	_tab = GC::Hold(new TabWnd(this));
	Add(_tab);

	_toolbar = GC::Hold(new JingleToolbarWnd());
	Add(_toolbar);
	strong<Theme> theme = ThemeManager::GetTheme();
	_toolbar->SetBackgroundColor(theme->GetColor(Theme::ColorHighlightStart));

	_master = GC::Hold(new SliderWnd(TL(jingle_master)));
	Add(_master);
	_master->SetValue(1.0f, false);
	_master->SetDisplayValue(1.0f, false);
	_master->EventChanged.AddListener(ref< Listener<SliderWnd::NotificationChanged> >(this));
	
	SetMenu(GetWindow(), LoadMenu(GetModuleHandle(NULL),MAKEINTRESOURCE(IDR_JINGLEMENU)));
	Language::Translate(GetWindow());
	Layout();

	for(int a=1;a<=12;a++) {
		std::wstring name = std::wstring(L"F")+Stringify(a);
		ref<JingleCollection> collection = GC::Hold(new JingleCollection());
		ref<JinglePane> pane = GC::Hold(new JinglePane(_tab->GetWindow(), collection));
		_tab->AddPane(GC::Hold(new Pane(name, pane, false, false, 0)));
		_panes[a] = pane;
	}
}

JingleView::~JingleView() {
	UnregisterHotKey(GetWindow(), 1);
}

void JingleView::ShowAboutInformation() {
	if(_tab) {
		_tab->AddPane(GC::Hold(new Pane(TL(jingle_about), GC::Hold(new JingleAboutWnd()), false, false,0)), true);
	}
}

void JingleView::OnThemeChanged() {
	std::map<int, ref<JinglePane> >::iterator it = _panes.begin();
	while(it!=_panes.end()) {
		ref<JinglePane> pane = it->second;
		if(pane) {
			pane->OnThemeChanged();
		}
		++it;
	}

	Layout();
	Update();
	FullRepaint();
}

void JingleView::GetMinimumSize(Pixels& w, Pixels& h) {
	w = 640;
	h = 480;
}

void JingleView::Inspect(ref<Inspectable> ins) {
	ref<PropertyDialogWnd> dw = GC::Hold(new PropertyDialogWnd(TL(jingle_application_name), TL(jingle_properties_change)));
	dw->GetPropertyGrid()->Inspect(ins);
	dw->SetSize(300,250);
	dw->DoModal(this);
	FullRepaint();
}

void JingleView::Save(TiXmlElement* parent) {
	std::map<int, ref<JinglePane> >::iterator it = _panes.begin();
	while(it!=_panes.end()) {
		std::pair<int,ref<JinglePane> > data = *it;
		ref<JinglePane> jpane = data.second;
		TiXmlElement pane("pane");
		ref<JingleCollection> col = jpane->_collection;
		SaveAttributeSmall<int>(&pane, "index", data.first);
		SaveAttributeSmall<std::wstring>(&pane, "name", col->_name);
		col->Save(&pane);
		parent->InsertEndChild(pane);
		++it;
	}
}

void JingleView::Load(TiXmlElement* you) {
	if(you==0) return;

	TiXmlElement* pane = you->FirstChildElement("pane");
	while(pane!=0) { 
		int index = LoadAttributeSmall(pane, "index", 0);
		if(_panes.find(index)!=_panes.end()) {
			ref<JinglePane> paner = _panes[index];
			ref<JingleCollection> col = paner->_collection;
			std::wstring name = LoadAttributeSmall<std::wstring>(pane, "name", L"");
			col->_name = name;
			col->Load(pane);
		}
		pane = pane->NextSiblingElement("pane");
	}
}

void JingleView::Notify(ref<Object> source, const SliderWnd::NotificationChanged& evt) {
	float f = _master->GetValue();
	BASS_SetVolume(int(100*f));
	_master->SetDisplayValue(f, false);
}

void JingleView::StopAll() {
	std::map<int, ref<JinglePane> >::iterator it = _panes.begin();
	while(it!=_panes.end()) {
		std::pair<int,ref<JinglePane> > data = *it;
		ref<JinglePane> jpane = data.second;
		jpane->_collection->StopAll();
		++it;
	}
}

void JingleView::PaintStatusBar(graphics::Graphics& g, strong<Theme> theme, const Area& statusBarArea) {
	RootWnd::PaintStatusBar(g, theme, statusBarArea);
}

void JingleView::FindJinglesByName(const std::wstring& q, std::deque< ref<Jingle> >& results) {
	std::map<int, ref<JinglePane> >::iterator it = _panes.begin();
	while(it!=_panes.end()) {
		std::pair<int,ref<JinglePane> > data = *it;
		ref<JinglePane> jpane = data.second;
		ref<JingleCollection> jc = jpane->GetJingleCollection();
		if(jc) {
			jc->FindJinglesByName(q,results);
		}
		++it;
	}
}

void JingleView::StartSearch() {
	_toolbar->StartSearch();
}

LRESULT JingleView::Message(UINT msg, WPARAM wp, LPARAM lp) {
	if(msg==WM_SIZE) {
		Layout();
	}
	else if(msg==WM_COMMAND||msg==WM_SYSCOMMAND) {
		JingleApplication::Instance()->Command(LOWORD(wp));
	}
	else if(msg==WM_HOTKEY) {
		ShowWindow(GetWindow(), SW_SHOWNORMAL);
		BringToFront();
		ref<Wnd> w = _tab->GetCurrentPane();
		if(w) w->Focus();
	}
	else if(msg==WM_KEYDOWN||msg==WM_SYSKEYDOWN) {
		if(LOWORD(wp)>=VK_F1 && LOWORD(wp)<=VK_F12) {
			int idx = LOWORD(wp) - VK_F1 + 1;
			_tab->SelectPane(ref<Wnd>(_panes[idx]));
			_toolbar->CancelSearch();
			return 0;
		}
		else if(LOWORD(wp)==L'\b') {
			StopAll();
		}
		else if(LOWORD(wp)==VK_ESCAPE) {
			_toolbar->StartSearch();
		}
		Repaint();
	}

	return RootWnd::Message(msg,wp,lp);
}

void JingleView::Paint(tj::shared::graphics::Graphics& g, ref<Theme> theme) {
	RootWnd::Paint(g,theme);
}

void JingleView::Layout() {
	Area rc = GetClientArea();
	_toolbar->Fill(LayoutTop, rc);
	_master->Fill(LayoutLeft, rc);
	_tab->Fill(LayoutFill, rc);
}

void JingleView::Update() {
}