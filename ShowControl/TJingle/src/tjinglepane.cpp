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
#include <windowsx.h>
#include <shellapi.h>
#include <iomanip>
using namespace tj::jingle;
using namespace tj::shared;
using namespace tj::shared::graphics;

#define ISVKKEYDOWN(vk) ((GetAsyncKeyState(vk) & 0x8000) == 0x8000);

namespace tj {
	namespace jingle {
		class JinglePaneToolbarWnd: public ToolbarWnd {
			public:
				JinglePaneToolbarWnd(JinglePane* pane) {
					Add(GC::Hold(new ToolbarItem(CommandProperties, L"icons/toolbar/properties.png", TL(jingle_properties_description), false)));
					///Add(GC::Hold(new ToolbarItem(CommandCache, L"icons/browser/reload.png", TL(jingle_preload), true)));
					Add(GC::Hold(new ToolbarItem(CommandStopAll, L"icons/stop.png", TL(jingle_stop_all), false)), true);
					Add(GC::Hold(new ToolbarItem(CommandRandom, L"icons/wol.png", TL(jingle_play_random), false)));
					
					_pane = pane;
				}

				virtual ~JinglePaneToolbarWnd() {
				}

				virtual void OnCommand(ref<ToolbarItem> ti) {
					OnCommand(ti->GetCommand());
				}

				virtual void OnCommand(int c) {
					if(c==CommandProperties) {
						ref<JingleView> view = JingleApplication::Instance()->GetView();
						view->Inspect(_pane->_collection);
					}
					else if(c==CommandCache) {
						_pane->_collection->Cache();
					}
					else if(c==CommandRandom) {
						_pane->_collection->PlayRandom();
					}
					else if(c==CommandStopAll) {
						_pane->_collection->StopAll();
					}
					SetFocus(_pane->GetWindow());
				}

			protected:
				JinglePane* _pane;

				enum Commands {
					CommandProperties=1,
					CommandCache,
					CommandDirectory,
					CommandStopAll,
					CommandRandom,
				};
		};
	}
}

JinglePane::JinglePane(HWND parent, ref<JingleCollection> collection): ChildWnd(L""), _loadedIcon(L"icons/jingle/loaded.png") {
	_collection = collection;
	_toolbar = GC::Hold(new JinglePaneToolbarWnd(this));
	Add(_toolbar);
	OnThemeChanged();
	Layout();
	SetTimer(GetWindow(), 0, 100, 0);
	SetDropTarget(true);
}

JinglePane::~JinglePane() {
}

void JinglePane::OnThemeChanged() {
	strong<Theme> theme = ThemeManager::GetTheme();
	_toolbar->SetBackgroundColor(theme->GetColor(Theme::ColorHighlightStart));
}

void JinglePane::Layout() {
	Area rc = GetClientArea();
	_toolbar->Fill(LayoutTop,rc);
}

void JinglePane::Update() {
	Repaint();
}

ref<Jingle> JinglePane::GetJingleAt(Pixels x, Pixels y) {
	ref<JingleKeyboardLayout> layout = JingleApplication::Instance()->GetKeyboardLayout();
	Area rc = GetClientArea();
	//rc.Narrow(0, _toolbar->GetClientArea().GetHeight(), 0, 0);
	int row = int(y / (float(rc.GetHeight())/float(layout->GetRowCount())));
	int col = int(x / (float(rc.GetWidth())/float(layout->GetColumnCount())));

	return _collection->GetJingle(layout->GetCharacterAt(row,col));
}

void JinglePane::OnMouse(MouseEvent ev, Pixels x, Pixels y) {
	if(ev==MouseEventLDown || ev==MouseEventRUp || ev==MouseEventMDown) {
		ref<JingleKeyboardLayout> layout = JingleApplication::Instance()->GetKeyboardLayout();
		Area rc = GetClientArea();
		int row = int(y / (float(rc.GetHeight())/float(layout->GetRowCount())));
		int col = int(x / (float(rc.GetWidth())/float(layout->GetColumnCount())));

		ref<Jingle> jingle = GetJingleAt(x,y);
		if(jingle) {
			if(ev==MouseEventLDown) {
				SetFocus(GetWindow());
				
				if(IsKeyDown(KeyShift)) {
					jingle->Stop();
				}
				else if(IsKeyDown(KeyDown)) {
					jingle->FadeOut();
				}
				else if(IsKeyDown(KeyUp)) {
					JingleApplication::Instance()->OnBeforeJingleStart();
					jingle->FadeIn();
				}
				else {
					JingleApplication::Instance()->OnBeforeJingleStart();
					jingle->Play(); 
				}
			}
			else if(ev==MouseEventRUp) {
				SetFocus(GetWindow());
				Pixels bh = Pixels(float(rc.GetHeight())/float(layout->GetRowCount()));
				Pixels bw = Pixels(float(rc.GetWidth())/float(layout->GetColumnCount()));
				DoContextMenu(col*bw, (row+1)*bh, jingle);
				Repaint();
			}
			else if(ev==MouseEventMDown) {
				jingle->Stop();
				Repaint();
			}
		}
		Repaint();
	}
}

void JinglePane::DoContextMenu(Pixels x, Pixels y, ref<Jingle> jingle) {
	enum { KCLoadJingle = 1, KCPlayJingle, KCFadeOutJingle, KCFadeInJingle, KCStopJingle };

	ContextMenu cm;
	cm.AddItem(TL(jingle_open_jingle), KCLoadJingle,true,false);
	cm.AddSeparator();
	cm.AddItem(TL(jingle_play), jingle->IsEmpty() ? -1 : KCPlayJingle);
	cm.AddItem(TL(jingle_stop), !jingle->IsPlaying() ? -1 : KCStopJingle);
	cm.AddItem(TL(jingle_fade_in), jingle->IsEmpty() ? -1 : KCFadeInJingle);
	cm.AddItem(TL(jingle_fade_out), (jingle->IsEmpty() || !jingle->IsPlaying()) ? -1 : KCFadeOutJingle);

	// Temporarily disable animations, since otherwise jingle may start/stop too late
	bool ae = Animation::IsAnimationsEnabled();
	Animation::SetAnimationsEnabled(false);
	int r = cm.DoContextMenu(this,x,y);
	Animation::SetAnimationsEnabled(ae);

	if(r==KCLoadJingle) {
		std::wstring fn = Dialog::AskForOpenFile(this, TL(jingle_open), L"Audio (mp3,wav)\0*.*\0\0", L"mp3");
		jingle->SetFile(fn);
	}
	else if(r==KCPlayJingle) {
		JingleApplication::Instance()->OnBeforeJingleStart();
		jingle->Play();
	}
	else if(r==KCStopJingle) {
		jingle->Stop();
	}
	else if(r==KCFadeInJingle) {
		JingleApplication::Instance()->OnBeforeJingleStart();
		jingle->FadeIn();
	}
	else if(r==KCFadeOutJingle) {
		jingle->FadeOut();
	}

	Repaint();
}

void JinglePane::OnSize(const Area& ns) {
	Layout();
}

void JinglePane::OnTimer(unsigned int id) {
	Update();
}

void JinglePane::OnFocus(bool focus) {
	if(_toolbar) {
		_toolbar->SetBackground(focus);
	}
}

void JinglePane::GetAccelerators(std::vector<Accelerator>& alist) {
	Accelerator acc;
	acc._key = KeyControl;
	acc._isModifier = true;
	acc._keyName = TL(jingle_key_ctrl);
	acc._description = TL(jingle_key_ctrl_description);
	alist.push_back(acc);

	acc._key = KeyShift;
	acc._keyName = TL(jingle_key_shift);
	acc._description = TL(jingle_key_shift_description);
	alist.push_back(acc);

	acc._key = KeyUp;
	acc._keyName = TL(jingle_key_up);
	acc._description = TL(jingle_key_up_description);
	alist.push_back(acc);

	acc._key = KeyDown;
	acc._keyName = TL(jingle_key_down);
	acc._description = TL(jingle_key_down_description);
	alist.push_back(acc);

	acc._key = KeyNone;
	acc._isModifier = false;
	acc._keyName = TL(jingle_key_jingle);
	acc._description = L"";
	alist.push_back(acc);

	acc._key = KeyEscape;
	acc._isModifier = false;
	acc._keyName = TL(key_escape);
	acc._description = TL(jingle_search);
	alist.push_back(acc);

	acc._key = KeyBackspace;
	acc._isModifier = false;
	acc._keyName = TL(key_backspace);
	acc._description = TL(jingle_stop_all_global);
	alist.push_back(acc);
}

LRESULT JinglePane::Message(UINT msg, WPARAM wp, LPARAM lp) {
	if(msg==WM_KEYDOWN||msg==WM_SYSKEYDOWN) {
		Focus();
		if(LOWORD(wp)==VK_ESCAPE) {
			JingleApplication::Instance()->GetView()->StartSearch();
		}
		else if(LOWORD(wp)==L'\b') {
			if(IsKeyDown(KeyShift)) {
				_collection->StopAll();
			}
			else {
				JingleApplication::Instance()->GetView()->StopAll();
			}
		}
		else {
			if(wp>=VK_NUMPAD0 && wp <= VK_NUMPAD9) {
				wp = (WPARAM)((wp - VK_NUMPAD0) + L'0');
			}
			ref<Jingle> jingle = _collection->GetJingle(LOWORD(wp));
			if(jingle) {
				if(IsKeyDown(KeyShift)||IsKeyDown(KeyDown)) { // Shift+sample stops it, Down+sample fades it out
					if(IsKeyDown(KeyDown)) {
						jingle->FadeOut();
					}
					else {
						jingle->Stop();
					}
				}
				else {
					if(IsKeyDown(KeyPageUp)||IsKeyDown(KeyUp)) { // pg up
						JingleApplication::Instance()->OnBeforeJingleStart();
						jingle->FadeIn();
					}
					else {
						JingleApplication::Instance()->OnBeforeJingleStart();
						jingle->Play();
					}
				}
			}
		}
		Repaint();
	}

	if(msg==WM_KEYDOWN || msg==WM_KEYUP) {
		// Let the root window repaint its status bar
		Wnd* root = GetRootWindow();
		if(root!=0) {
			root->Repaint();
			// TODO replace with some RootWnd::RepaintStatusBar method
		}
	}
	return ChildWnd::Message(msg,wp,lp);
}

ref<JingleCollection> JinglePane::GetJingleCollection() {
	return _collection;
}

LRESULT JinglePane::PreMessage(UINT msg, WPARAM wp, LPARAM lp) {
	if(msg==WM_DROPFILES) {
		HDROP drop = (HDROP)wp;
		int n = DragQueryFile(drop, 0xFFFFFFFF, NULL, 0);

		if(n==1) {
			POINT pt;
			DragQueryPoint(drop, &pt);
			strong<Theme> theme = ThemeManager::GetTheme();
			Pixels dx = Pixels(pt.x / theme->GetDPIScaleFactor());
			Pixels dy = Pixels(pt.y / theme->GetDPIScaleFactor());

			ref<Jingle> jingle = GetJingleAt(dx,dy);

			if(jingle) {
				int size = DragQueryFile(drop, 0, NULL, 0);
				wchar_t* buffer = new wchar_t[size+2];
				DragQueryFile(drop, 0, buffer, size+1);
				jingle->SetFile(std::wstring(buffer));
				delete[] buffer;
			}
		}
		else {
			ref<JingleKeyboardLayout> layout = JingleApplication::Instance()->GetKeyboardLayout();

			for(unsigned int a=0; a< (unsigned int)n; a++) {
				int size = DragQueryFile(drop, a, NULL, 0);
				wchar_t* buffer = new wchar_t[size+2];
				DragQueryFile(drop, a, buffer, size+1);
				
				// Start filling this pane with all the jingles
				if(a<(layout->GetRowCount()*layout->GetColumnCount())) {
					ref<Jingle> jingle = _collection->GetJingle(layout->GetCharacterAt(a));
					if(jingle) {
						jingle->SetFile(std::wstring(buffer));
					}
				}
				delete[] buffer;
			}
		}
		DragFinish(drop);
		Repaint();
		return 0;
	}
	else {
		return ChildWnd::PreMessage(msg,wp,lp);
	}
}

void JinglePane::Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme) {
	ref<JingleKeyboardLayout> layout = JingleApplication::Instance()->GetKeyboardLayout();
	Area rc = GetClientArea();
	rc.Narrow(0, _toolbar->GetClientArea().GetHeight(), 0, 0);
	float bW = ceil(float(rc.GetWidth()+1)/float(layout->GetColumnCount()));
	float bH = ceil(float(rc.GetHeight()+2)/float(layout->GetRowCount()));

	Font* font = theme->GetGUIFontBold();
	Font* nameFont = theme->GetGUIFontSmall();
	SolidBrush tbr(theme->GetColor(Theme::ColorText));
	SolidBrush shadowTbr(theme->GetColor(Theme::ColorShadow));
	StringFormat sf;
	sf.SetAlignment(StringAlignmentNear);
	sf.SetLineAlignment(StringAlignmentNear);

	SolidBrush bbr(theme->GetColor(Theme::ColorBackground));
	g.FillRectangle(&bbr, rc);
	
	for(unsigned int r=0;r<layout->GetRowCount();r++) {
		for(unsigned int c=0;c<layout->GetColumnCount();c++) {
			wchar_t letter = layout->GetCharacterAt(r,c);
			ref<Jingle> jingle = _collection->GetJingle(letter);
			Area brc(Pixels(floor(bW*c))-1, rc.GetTop()+Pixels(floor(bH*r))-1, Pixels(ceil(bW)), Pixels(ceil(bH)));

			if(letter!=0) {
				Pen pn(theme->GetColor(Theme::ColorActiveStart), 1.0f);

				if(jingle) {
					if(jingle->IsPlaying()) {
						LinearGradientBrush lbr(PointF(0.0f, float(brc.GetTop())), PointF(0.0f, float(brc.GetBottom())), theme->GetColor(Theme::ColorHighlightStart), theme->GetColor(Theme::ColorHighlightEnd));
						g.FillRectangle(&lbr, brc);

						// draw glass over it to make it look sooo 2007
						LinearGradientBrush glass(PointF(0.0f, float(brc.GetTop())), PointF(0.0f, float(brc.GetTop()+brc.GetHeight()/2)), theme->GetColor(Theme::ColorGlassStart), theme->GetColor(Theme::ColorGlassEnd));
						Area glassRect = brc;
						glassRect.Narrow(0, 0, 0, glassRect.GetHeight()/2);
						g.FillRectangle(&glass, glassRect);
					}
						
					std::wstring file = jingle->GetName();
					g.DrawString(file.c_str(), (int)file.length(), nameFont, RectF(brc.GetLeft()+10.0f, brc.GetTop()+30.0f, brc.GetWidth()-20.0f, brc.GetHeight()-40.0f), &sf, &tbr);
			
					if(jingle->IsPlaying()) {
						// Progress thingie
						Area prc(brc.GetLeft(), brc.GetBottom()-22, brc.GetWidth(), 22);
						LinearGradientBrush pback(PointF(0.0f, float(brc.GetBottom()-22)), PointF(0.0f, float(brc.GetBottom())), theme->GetColor(Theme::ColorProgressBackgroundStart),theme->GetColor(Theme::ColorProgressBackgroundStart));
						g.FillRectangle(&pback, prc);

						float pos = jingle->GetPosition();
						bool warning = pos>0.65f;
						Color startColor = warning?Color(99,0,0):theme->GetColor(Theme::ColorProgress);
						Color endColor = warning?Color(255,0,0):theme->GetColor(Theme::ColorProgress);

						LinearGradientBrush ptop(PointF(float(brc.GetLeft()), float(brc.GetTop())), PointF(float(brc.GetRight()), float(brc.GetTop())), Theme::ChangeAlpha(startColor, 127), endColor);
						g.FillRectangle(&ptop, RectF(float(brc.GetLeft()+2), float(brc.GetBottom()-21), float(brc.GetWidth()-4)*jingle->GetPosition(), float(20)));

						// paint time remaining
						Area timeRect = prc;
						timeRect.Narrow(0,4,4,4);
						StringFormat sft;
						sft.SetAlignment(StringAlignmentFar);

						std::wostringstream wos;
						wos << std::fixed << std::setprecision(1) << float(jingle->GetRemainingTime().ToInt())/100.0f;
						std::wstring time = wos.str();
						SolidBrush timeBr(theme->GetColor(Theme::ColorBackground));
						g.DrawString(time.c_str(),(int)time.length(),nameFont, timeRect, &sft, &timeBr);

						// glassify
						prc.Narrow(0,0,0,11);
						LinearGradientBrush pglass(PointF(float(prc.GetLeft()), float(prc.GetTop())), PointF(float(prc.GetLeft()), float(prc.GetBottom())),  theme->GetColor(Theme::ColorGlassStart), theme->GetColor(Theme::ColorGlassEnd));
						g.FillRectangle(&pglass, prc);
					}
				}

				if(jingle->IsLoadedAsSample()) {
					_loadedIcon.Paint(g,Area(brc.GetRight()-16, brc.GetBottom()-16, 16, 16));
				}

				g.DrawRectangle(&pn, brc);
				std::wstring name = Stringify(letter);
				if(jingle->IsPlaying()) {
					g.DrawString(name.c_str(), (int)name.length(), font,PointF(brc.GetLeft()+11.0f, brc.GetTop()+11.0f), &sf, &shadowTbr);
				}
				g.DrawString(name.c_str(), (int)name.length(), font,PointF(brc.GetLeft()+10.0f, brc.GetTop()+10.0f), &sf, &tbr);
			}
		}
	}
}

std::wstring JinglePane::GetTabTitle() const {
	return _collection->GetName();
}