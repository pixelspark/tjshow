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
#include "../../include/tjdmx.h"
#include "../../resource.h"
#include <set>
#include <algorithm>
#include <windowsx.h>
#include <commdlg.h>
using namespace tj::shared::graphics;

#define ISVKKEYDOWN(vk_code) ((GetAsyncKeyState(vk_code) & 0x8000))
BOOL WINAPI FixtureSettingsProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp);

class DMXViewerWnd: public ChildWnd, public Serializable, public virtual ChangeDelegate {
	const static int KLeftBarWidth = 32;
	const static int KUpdateTime = 50; // Repaint every 50ms

	public:
		DMXViewerWnd(ref<DMXViewerTrack> track): ChildWnd(L"DMX Viewer"), _offX(0), _offY(0), _backgroundImage(0), _modID(0), _isDragging(false),
		_bulbImage(L"icons/viewer/bulb.png"),
		_textImage(L"icons/viewer/text.png"),
		_textHoverImage(L"icons/viewer/text_hover.png"),
		_bulbHoverImage(L"icons/viewer/bulb_hover.png") {
			assert(track);
			_track = track;
			/* TODO put time somewhere in a constant */
			SetTimer(GetWindow(), 1337, KUpdateTime, NULL);
		}

		virtual ~DMXViewerWnd() {
			delete _backgroundImage;
		}

		virtual void OnChangeOccurred(ref<Change> change, bool wasUndo) {
			Repaint();
		}

		virtual void OnKey(Key k, wchar_t code, bool down, bool isAccelerator) {
			UndoBlock ub;
			if(!down) {
				if(k==KeyDelete) {
					RemoveSelectedFixtures();
				}
				else if(k==KeyCharacter && code==L'D' && IsKeyDown(KeyControl)) {
					DuplicateSelectedFixtures();
				}
				else if(k==KeyCharacter && code==L'A' && IsKeyDown(KeyControl)) {
					ref<DMXViewerTrack> track = _track;
					if(track) {
						_selected = track->GetFixtures();
						Repaint();
					}
				}
				else {
					std::set< ref<DMXViewerFixture> >::iterator it = _selected.begin();
					while(it!=_selected.end()) {
						ref<DMXViewerFixture> fix = *it;

						if(fix) {
							if(k==KeyUp) {
								Undoable<DMXViewerFixture, float>(fix, fix->_y, this) = max(0.0f, fix->_y - 0.01f);
							}
							else if(k==KeyDown) {
								Undoable<DMXViewerFixture, float>(fix, fix->_y, this) = min(1.0f, fix->_y + 0.01f);
							}
							else if(k==KeyLeft) {
								Undoable<DMXViewerFixture, float>(fix, fix->_x, this) = max(0.0f, fix->_x - 0.01f);
							}
							else if(k==KeyRight) {
								Undoable<DMXViewerFixture, float>(fix, fix->_x, this) = min(1.0f, fix->_x + 0.01f);
							}
							else if(k==KeyPageUp) {
								Undoable<DMXViewerFixture, float>(fix, fix->_rotate, this) += 5.0f;
							}
							else if(k==KeyPageDown) {
								Undoable<DMXViewerFixture, float>(fix, fix->_rotate, this) -= 5.0f;
							}
						}

						++it;
					}

					Repaint();
				}
			}
		}

		virtual void OnTimer(unsigned int id) {
			if(IsShown()) {
				Update();
			}
		}

		// TODO: remove this alltogether
		virtual LRESULT Message(UINT msg, WPARAM wp, LPARAM lp) {
			if(msg==WM_MOUSEWHEEL) {
				int delta = GET_WHEEL_DELTA_WPARAM(wp);

				std::set< ref<DMXViewerFixture> >::iterator it = _selected.begin();
				while(it!=_selected.end()) {
					ref<DMXViewerFixture> fix = *it;

					if(delta>0) {
						Undoable<DMXViewerFixture, float>(fix, fix->_size, this) += 5.0f;
					}
					else {
						Undoable<DMXViewerFixture, float>(fix, fix->_size, this) -= 5.0f;
						if(fix->_size<0.0f) {
							Undoable<DMXViewerFixture, float>(fix, fix->_size, this) = 1.0f;
						}
					}

					++it;
				}
				Repaint();
			}
			return ChildWnd::Message(msg, wp, lp);
		}

		/** Copies the current selection to clipboard **/
		virtual void Save(TiXmlElement* you) {
			std::set< ref<DMXViewerFixture> >::iterator it = _selected.begin();
			while(it!=_selected.end()) {
				ref<DMXViewerFixture> fix = *it;
				if(fix) {
					TiXmlElement fixture("fixture");
					fix->Save(&fixture);
					you->InsertEndChild(fixture);
				}
				++it;
			}
		}

		virtual void Load(TiXmlElement* me) {
			ref<DMXViewerTrack> track = _track;
			_selected.clear();
			TiXmlElement* fixture = me->FirstChildElement("fixture");
			while(fixture!=0) {
				ref<DMXViewerFixture> fix = track->CreateFixture();
				fix->Load(fixture);
				_selected.insert(fix);

				fixture = fixture->NextSiblingElement("fixture");
			}
			Repaint();
		}

		virtual void OnCopy() {
			Clipboard::SetClipboardObject(this);
		}

		virtual void OnPaste() {
			Clipboard::GetClipboardObject(this);
		}

		virtual void OnCut() {
			OnCopy();
			RemoveSelectedFixtures();
		}

		virtual void OnMouse(MouseEvent ev, Pixels x, Pixels y) {
			Area rc = GetClientArea();

			if(ev==MouseEventLDown) {
				Focus();
				_isDragging = true;

				if(x < KLeftBarWidth) {
					int i = int(y / 32.0f);
					if(i==0) {
						ref<DMXViewerTrack> track = _track;
						if(track) {
							ref<DMXViewerFixture> fix = track->CreateFixture();
							fix->_x = float(x-KLeftBarWidth-int(fix->_size/2.0f)) / float(rc.GetWidth()-KLeftBarWidth);
							fix->_y = float(y-int(fix->_size/2.0f)) / float(rc.GetHeight());
							_dragging = fix;
							_focused = fix;
							_selected.clear();
							_selected.insert(fix);
						}
						Repaint();
					}
					else if(i==1) {
						strong<Theme> theme = ThemeManager::GetTheme();
						tj::shared::graphics::Color tc = theme->GetColor(Theme::ColorText);

						Area rc = GetClientArea();
						ref<DMXViewerTrack> track = _track;
						if(track) {
							ref<DMXViewerFixture> fix = track->CreateFixture();
							fix->_type = DMXViewerFixture::FixtureTypeText;
							fix->_text = TL(fixture_type_text);
							fix->_color = RGBColor(tc.GetR(), tc.GetG(), tc.GetB());
							fix->_x = float(x-int(fix->_size/2.0f)-KLeftBarWidth) / float(rc.GetWidth()-KLeftBarWidth);
							fix->_y = float(y-int(fix->_size/2.0f)) / float(rc.GetHeight());
							_dragging = fix;
							_focused = fix;
							
							_selected.clear();
							_selected.insert(_dragging);
						}
						Repaint();
					}
				}
				else {
					ref<DMXViewerTrack> track = _track;
					if(track) {
						ref<DMXViewerFixture> fix = track->GetFixtureAt(x-KLeftBarWidth, y, float(rc.GetWidth()-KLeftBarWidth), float(rc.GetHeight()));
						if(fix) {
							if(IsKeyDown(KeyShift)) {
								_selected.insert(fix);
							}
							else {
								_selected.clear();
								_selected.insert(fix);
							}
						}
						else {
							_selected.clear();
						}

						// offsets
						if(fix) {
							Graphics g(GetWindow());
							AreaF bounds = fix->GetArea(g);
							bounds.SetX(bounds.GetX()*(rc.GetWidth()-KLeftBarWidth));
							bounds.SetY(bounds.GetY()*(rc.GetHeight()));
							_offX = x - KLeftBarWidth - int(bounds.GetLeft());
							_offY = y - int(bounds.GetTop());
						}
						else {
							_offX = _offY = 0;
						}

						_dragging = fix;
						_focused = fix;
					}

					if(_properties && _dragging) {
						_properties->SetProperties(_dragging);
						Focus();
					}
				}

				Repaint();
			}
			else if(ev==MouseEventMove) {
				if(_dragging && _isDragging) {
					int mx = x - _offX;
					int my = y - _offY;
					float dx = float(mx-KLeftBarWidth) / float(rc.GetWidth()-KLeftBarWidth) - _dragging->_x;
					float dy = float(my) / float(rc.GetHeight()) - _dragging->_y;

					std::set< ref<DMXViewerFixture> >::iterator it = _selected.begin();
					while(it!=_selected.end()) {
						ref<DMXViewerFixture> fix = *it;
						if(IsKeyDown(KeyControl)) {
							fix->_rotate = (atan2(dy,dx)*360.0f)/(2.0f*3.14159f);
						}
						else if(!IsKeyDown(KeyShift)) {
							fix->_x += dx;
							fix->_y += dy;
						}
						++it;
					}
					Repaint();
				}
			}
			else if(ev==MouseEventLUp) {
				ref<DMXViewerTrack> track = _track;
				if(track) {
					_isDragging = false;
					std::set< ref<DMXViewerFixture> >::iterator it = _selected.begin();
					while(it!=_selected.end()) {
						ref<DMXViewerFixture> fix = *it;
						if(fix->_x < 0.0f) {
							track->RemoveFixture(fix);
							if(_dragging==fix) _dragging = 0;
						}

						++it;
					}

					if(_properties && !_dragging) {
						_properties->SetProperties(null);
					}
				}

				Repaint();
			}
			else if(ev==MouseEventRDown) {
				OnContextMenu(x, y);
			}
		}

		void OnContextMenu(Pixels x, Pixels y) {
			Area rc = GetClientArea();
			rc.Narrow(KLeftBarWidth,0,0,0);
			Area wrc = GetWindowArea();
			wrc.Narrow(KLeftBarWidth,0,0,0);

			ref<DMXViewerTrack> track = _track;
			if(track) {
				ref<DMXViewerFixture> fix = track->GetFixtureAt(x,y , float(rc.GetWidth()), float(rc.GetHeight()));

				ContextMenu menu;
				enum {cmdNew=1, cmdRemove, cmdProps, cmdColor, cmdSave, cmdDuplicate};
				
				menu.AddItem(TL(dmx_viewer_save_image), cmdSave);
				menu.AddSeparator();

				if(fix) {
					menu.AddItem(TL(fixture_remove), cmdRemove, false);
					menu.AddItem(TL(fixture_duplicate), cmdDuplicate, false);
					menu.AddSeparator();					
				}

				menu.AddItem(TL(fixture_add), cmdNew, true);

				switch(menu.DoContextMenu(this,x,y)) {
					case cmdNew:
						fix = track->CreateFixture();
						Undoable<DMXViewerFixture, float>(fix, fix->_x, this) = float(x-7-rc.GetLeft()) / float(rc.GetWidth()-KLeftBarWidth);
						Undoable<DMXViewerFixture, float>(fix, fix->_y, this) = float(y-7-rc.GetTop()) / float(rc.GetHeight());
						Repaint();
						break;

					case cmdDuplicate:
						DuplicateSelectedFixtures();
						break;

					case cmdRemove:
						RemoveSelectedFixtures();
						break;

					case cmdSave:
						SaveAsImage();
						break;
				}
			}
		}


		void RemoveSelectedFixtures() {
			ref<DMXViewerTrack> track = _track;
			if(track) {
				std::set< ref<DMXViewerFixture> >::iterator it = _selected.begin();
				while(it!=_selected.end()) {
					ref<DMXViewerFixture> fix = *it;
					if(_dragging==fix) _dragging = 0;
					track->RemoveFixture(fix);
					++it;				
				}

				if(_properties && !_dragging) {
					_properties->SetProperties(null);
				}
			}

			Repaint();
		}

		void DuplicateSelectedFixtures() {
			ref<DMXViewerTrack> track = _track;
			if(track) {
				std::set< ref<DMXViewerFixture> > newSelection;

				std::set< ref<DMXViewerFixture> >::iterator it = _selected.begin();
				while(it!=_selected.end()) {
					ref<DMXViewerFixture> fix = *it;
					ref<DMXViewerFixture> nf = track->CreateFixture(fix);
					nf->_x += 0.05f;
					nf->_y += 0.05f;
					newSelection.insert(nf);
					++it;				
				}
				
				_properties->SetProperties(null);
				_selected = newSelection;
				Repaint();
			}
		}

		void SaveAsImage() {
			strong<Theme> theme = ThemeManager::GetTheme();
			std::wstring path = AskWhereToSaveFile();
			if(path.length()>4) {
				std::wstring ext = path.substr(path.length()-3, 3);
				std::transform(ext.begin(), ext.end(), ext.begin(), tolower);
				if(ext==L"jpg") ext = L"jpeg";
				std::wstring mime = std::wstring(L"image/")+ext;

				Area rc = GetClientArea();
				Bitmap* bmp = new Bitmap(rc.GetWidth(), rc.GetHeight());
				tj::shared::graphics::Graphics g(bmp);
				Paint(g, false, theme);
				bmp->Save(path.c_str(), mime.c_str());
				delete bmp;
			}
		}

		std::wstring AskWhereToSaveFile() {
			return Dialog::AskForSaveFile(this, TL(save_file_dialog_title), L"PNG (*.png)\0*.png\0JPEG (*.jpg)\0*.jpg\0GIF (*.gif)\0*.gif\0\0", L"png");
		}

		void UpdateBackgroundImage(tj::shared::graphics::Graphics* g) {
			ref<DMXViewerTrack> track = _track;
			if(track && track->_background != _backgroundImageCached) {
				_backgroundImageCached = track->_background;
				delete _backgroundImage;
				_backgroundImage = 0;
				_backgroundImage = (Bitmap*)tj::shared::graphics::Bitmap::FromFile(track->GetBackgroundImagePath().c_str(), TRUE);
			}
		}

		virtual void Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme) {
			Paint(g, true, theme);
		}

		virtual void Paint(tj::shared::graphics::Graphics& g, bool cache, strong<Theme> theme) {
			g.SetHighQuality(true);

			ref<DMXViewerTrack> track = _track;
			if(track) {
				// get background image
				int lbw = cache?KLeftBarWidth:0;
				UpdateBackgroundImage(&g);

				Area rc = GetClientArea();
				float cW = float(rc.GetWidth()-lbw);
				float cH = float(rc.GetHeight());

				SolidBrush back(track->_backgroundColor);
				if(!_backgroundImage || _backgroundImage->GetHeight()==0 || _backgroundImage->GetWidth()==0) {
					g.FillRectangle(&back, rc);
				}
				else {
					g.DrawImage(_backgroundImage, Rect(rc.GetLeft()+lbw, rc.GetTop(), rc.GetWidth()-lbw, rc.GetHeight() ));
				}

				// draw toolbar
				if(cache) {
					POINT cursor;
					GetCursorPos(&cursor);
					ScreenToClient(GetWindow(), &cursor);

					g.FillRectangle(&back, RectF(float(rc.GetLeft()), float(rc.GetTop()), float(lbw), float(rc.GetHeight())));
					LinearGradientBrush toolbarBrush(PointF(0.0f, 0.0f), PointF(float(lbw), 0.0f), Theme::ChangeAlpha(theme->GetColor(Theme::ColorActiveStart),70), Theme::ChangeAlpha(theme->GetColor(Theme::ColorActiveEnd),70));
					g.FillRectangle(&toolbarBrush, RectF(float(rc.GetLeft()), float(rc.GetTop()), float(lbw-2), float(rc.GetHeight())));
					
					bool hoverBulb = (cursor.x < 26 &&  cursor.y > 0 && cursor.y < 32);
					g.DrawImage(hoverBulb?_bulbHoverImage:_bulbImage, RectF(2.0f, 2.0f, 24.0f, 24.0f));

					bool hoverText = (cursor.x < 26 &&  cursor.y > 26 && cursor.y < 64);
					g.DrawImage(hoverText?_textHoverImage:_textImage, RectF(2.0f, 32.0f+2.0f, 24.0f, 24.0f));
				}

				std::vector< ref<DMXViewerFixture> >::iterator it = track->_fixtures.begin();
				while(it != track->_fixtures.end()) {
					ref<DMXViewerFixture> fixture = *it;
					int value = track->_plugin->_dmx->GetChannelResult(fixture->_channel);

					// draw guides when the user wants them
					bool currentSelected = std::find(_selected.begin(), _selected.end(), fixture)!=_selected.end();
					
					// pen for all guides
					Pen guide(Theme::ChangeAlpha(theme->GetColor(Theme::ColorActiveStart), 127), 1.0f);
					// TODO: implement in Graphics
					// guide.SetDashStyle(DashStyleDot);

					// get bounding rectangle
					AreaF bound = fixture->GetArea(g);
					bound.SetX(bound.GetX()*cW);
					bound.SetY(bound.GetY()*cH);
					bound.Translate((float)lbw, 0.0f);
					bound.Widen(5,5,5,5);

					// calculate center
					PointF center(bound.GetLeft()+bound.GetWidth()/2.0f, bound.GetTop()+bound.GetHeight()/2);

					if(currentSelected && _selected.size()<2 && cache && IsKeyDown(KeyMouseLeft)) {
						//g.DrawLine(&guide, 0.0f, fixture->_y*cH + (0.5f*fixture->_size), cW+lbw, fixture->_y*cH + (0.5f*fixture->_size));
						//g.DrawLine(&guide, fixture->_x*cW+lbw+(fixture->_size*0.5f), 0.0f,  fixture->_x*cW+lbw+(fixture->_size*0.5f), cH);
						g.DrawLine(&guide, float(KLeftBarWidth), center._y, cW+float(KLeftBarWidth), center._y);
						g.DrawLine(&guide, center._x, 0.0f, center._x, cH);
					}

					RGBColor col = fixture->_color;
					SolidBrush valueBrush(Color(value, int(col._r*255.0f), int(col._g*255.0f), int(col._b*255.0f)));

					Pen pen(theme->GetColor(Theme::ColorText), (currentSelected && cache)?2.0f:1.0f);

					GraphicsContainer gc = g.BeginContainer();
					g.SetHighQuality(true);
					g.RotateAtTransform(PointF(fixture->_x*cW+lbw+(fixture->_size*0.5f), fixture->_y*cH+(fixture->_size*0.5f)), fixture->_rotate);

					// draw bounding rectangle
					if(HasFocus() && _focused == fixture && cache) {
						theme->DrawFocusRectangle(g, Area((Pixels)bound.GetLeft(), (Pixels)bound.GetTop(), (Pixels)bound.GetWidth(), (Pixels)bound.GetHeight()));
					}

					float x = fixture->_x*cW+lbw;
					float y = fixture->_y*cH;

					if(fixture->_type==DMXViewerFixture::FixtureTypeDefault) {
						g.FillEllipse(&valueBrush, RectF(fixture->_x*cW+lbw, fixture->_y*cH, fixture->_size, fixture->_size));
						g.DrawEllipse(&pen, RectF(fixture->_x*cW+lbw, fixture->_y*cH, fixture->_size, fixture->_size));
					}
					else if(fixture->_type==DMXViewerFixture::FixtureTypeSquare) {
						g.FillRectangle(&valueBrush, RectF(fixture->_x*cW+lbw, fixture->_y*cH, fixture->_size, fixture->_size));
						g.DrawRectangle(&pen, RectF(fixture->_x*cW+lbw, fixture->_y*cH, fixture->_size, fixture->_size));
					}
					else if(fixture->_type==DMXViewerFixture::FixtureTypePar) {
						g.FillRectangle(&valueBrush, RectF(fixture->_x*cW+lbw, fixture->_y*cH, fixture->_size, fixture->_size));
						g.DrawRectangle(&pen, RectF(fixture->_x*cW+lbw, fixture->_y*cH, fixture->_size, fixture->_size));
						g.DrawLine(&pen, fixture->_x*cW+lbw+fixture->_size, fixture->_y*cH+fixture->_size,fixture->_x*cW+lbw+(2.0f*fixture->_size),fixture->_y*cH+fixture->_size);
						g.DrawLine(&pen, fixture->_x*cW+lbw+fixture->_size, fixture->_y*cH,fixture->_x*cW+lbw+(2.0f*fixture->_size),fixture->_y*cH);
					}
					else if(fixture->_type==DMXViewerFixture::FixtureTypeCIEFresnel) {
						PointF points[7];
						float s = fixture->_size;
						float gw = 0.25; // width of the 'grill' 
						points[0] = PointF(x,y);
						points[1] = PointF(x+1.5f*s,		y);
						points[2] = PointF(x+(1.5f-gw)*s,	y+0.25f*s);
						points[3] = PointF(x+1.5f*s,		y+0.5f*s);
						points[4] = PointF(x+(1.5f-gw)*s,	y+0.75f*s);
						points[5] = PointF(x+1.5f*s,		y+1.0f*s);
						points[6] = PointF(x+0.0f*s,		y+1.0f*s);
						g.FillPolygon(&valueBrush, points, 7);
						g.DrawPolygon(&pen, points, 7);
					}
					else if(fixture->_type==DMXViewerFixture::FixtureTypeCIEHorizon) {
						PointF points[4];
						float s = fixture->_size;
						float sw = 0.25f;
						points[0] = PointF(x,y+sw*s);
						points[1] = PointF(x,y+(2.0f-sw)*s);
						points[2] = PointF(x+s, y+2.0f*s);
						points[3] = PointF(x+s, y);
						g.FillPolygon(&valueBrush, points, 4);
						g.DrawPolygon(&pen, points, 4);
					}
					if(fixture->_type==DMXViewerFixture::FixtureTypeCIEProfile) {
						g.FillEllipse(&valueBrush, RectF(fixture->_x*cW+lbw, fixture->_y*cH, fixture->_size, fixture->_size));
						g.FillRectangle(&valueBrush, RectF(x+fixture->_size, y+fixture->_size*(0.33f), fixture->_size*0.25f, fixture->_size*0.33f));

						g.DrawEllipse(&pen, RectF(fixture->_x*cW+lbw, fixture->_y*cH, fixture->_size, fixture->_size));
						g.DrawRectangle(&pen, RectF(x+fixture->_size*0.75f, y+fixture->_size*(0.33f), fixture->_size*0.5f, fixture->_size*0.33f));
					}
					else if(fixture->_type==DMXViewerFixture::FixtureTypePar64) {
						g.FillRectangle(&valueBrush, RectF(fixture->_x*cW+lbw, fixture->_y*cH, fixture->_size*2, fixture->_size));
						g.DrawRectangle(&pen, RectF(fixture->_x*cW+lbw, fixture->_y*cH, fixture->_size*2, fixture->_size));
						g.DrawLine(&pen, fixture->_x*cW+lbw+2*fixture->_size, fixture->_y*cH+fixture->_size,fixture->_x*cW+lbw+(3.0f*fixture->_size),fixture->_y*cH+fixture->_size);
						g.DrawLine(&pen, fixture->_x*cW+lbw+2*fixture->_size, fixture->_y*cH,fixture->_x*cW+lbw+(3.0f*fixture->_size),fixture->_y*cH);
					}
					else if(fixture->_type==DMXViewerFixture::FixtureTypeCIEParShort) {
						RectF rect(x, y, fixture->_size*1.0f, fixture->_size*1.0f);
						g.FillPie(&valueBrush, rect, 90.0f, 180.0f);
						g.DrawPie(&pen, rect, 90.0f, 180.0f);
						float ls = 0.5f*fixture->_size;
						// draw lines
						g.DrawLine(&pen, x+ls,  y, x+ls*2.0f, y);
						g.DrawLine(&pen, x+ls, y+fixture->_size, x+ls*2.0f, y+fixture->_size);
					}
					else if(fixture->_type==DMXViewerFixture::FixtureTypeCIEPC) {
						// fill it
						g.FillRectangle(&valueBrush, RectF(x,y,fixture->_size*1.5f, fixture->_size));
						g.FillPie(&valueBrush, RectF(x+fixture->_size, y, fixture->_size, fixture->_size), 270.0f, 180.0f);

						g.DrawRectangle(&pen, RectF(x, y, fixture->_size*1.5f, fixture->_size));
						g.DrawArc(&pen, RectF(x+fixture->_size, y, fixture->_size, fixture->_size), 270.0f, 180.0f);
					}
					else if(fixture->_type==DMXViewerFixture::FixtureTypeCIEPar64) {
						PointF points[3];
						points[0] = PointF(x, y+fixture->_size*0.5f);
						points[1] = PointF(x+fixture->_size*1.5f, y);
						points[2] = PointF(x+fixture->_size*1.5f, y+fixture->_size);
						
						g.FillPolygon(&valueBrush, points, 3);
						g.FillPie(&valueBrush, RectF(x+fixture->_size*1.0f, y, fixture->_size*1.0f, fixture->_size*1.0f), 270.0f, 180.0f);

						// border
						g.DrawLine(&pen, points[0], points[1]);
						g.DrawLine(&pen,points[0], points[2]);
						g.DrawArc(&pen, RectF(x+fixture->_size*1.0f, y, fixture->_size*1.0f, fixture->_size*1.0f), 270.0f, 180.0f);					
					}
					else if(fixture->_type==DMXViewerFixture::FixtureTypePC) {
						float size = pow(2.0f, -1.0f)* fixture->_size;
						g.FillRectangle(&valueBrush, RectF(fixture->_x*cW+lbw, fixture->_y*cH, fixture->_size, fixture->_size));
						g.DrawRectangle(&pen, RectF(fixture->_x*cW+lbw, fixture->_y*cH, fixture->_size, fixture->_size));
						g.DrawLine(&pen, fixture->_x*cW+lbw+fixture->_size, fixture->_y*cH+fixture->_size,fixture->_x*cW+lbw+fixture->_size+size,fixture->_y*cH+fixture->_size+size);
						g.DrawLine(&pen, fixture->_x*cW+lbw+fixture->_size, fixture->_y*cH,fixture->_x*cW+lbw+fixture->_size+size,fixture->_y*cH-size);
					}
					else if(fixture->_type==DMXViewerFixture::FixtureTypePCTop) {
						float size = pow(2.0f, -1.0f)* fixture->_size * 0.5f;
						g.FillRectangle(&valueBrush, RectF(fixture->_x*cW+lbw, fixture->_y*cH, fixture->_size, fixture->_size));
						g.DrawRectangle(&pen, RectF(fixture->_x*cW+lbw, fixture->_y*cH, fixture->_size, fixture->_size));
						g.DrawLine(&pen, fixture->_x*cW+lbw+fixture->_size, fixture->_y*cH+fixture->_size,fixture->_x*cW+lbw+fixture->_size+size,fixture->_y*cH+fixture->_size+size);
						g.DrawLine(&pen, fixture->_x*cW+lbw+fixture->_size, fixture->_y*cH,fixture->_x*cW+lbw+fixture->_size+size,fixture->_y*cH-size);
						
						g.DrawLine(&pen, fixture->_x*cW+lbw, fixture->_y*cH+fixture->_size,fixture->_x*cW+lbw-size,fixture->_y*cH+fixture->_size+size);
						g.DrawLine(&pen, fixture->_x*cW+lbw, fixture->_y*cH,fixture->_x*cW+lbw-size,fixture->_y*cH-size);
					}
					else if(fixture->_type==DMXViewerFixture::FixtureTypeText) {
						strong<Theme> theme = ThemeManager::GetTheme();
						std::wstring uiFontName = theme->GetGUIFontName();
						Font myfont(uiFontName.c_str(), fixture->_size, FontStyleRegular);
						SolidBrush tbr(fixture->_color);
						StringFormat sf;
						sf.SetLineAlignment(StringAlignmentNear);
						sf.SetAlignment(StringAlignmentNear);
						g.DrawString(fixture->_text.c_str(), (int)fixture->_text.length(), &myfont, PointF(fixture->_x*cW+lbw,fixture->_y*cH), &sf, &tbr);
					}

					if(fixture->_text.length()>0 && fixture->_type!=DMXViewerFixture::FixtureTypeText) {
						Font* fnt = ThemeManager::GetTheme()->GetGUIFontSmall();
						SolidBrush tbr(ThemeManager::GetTheme()->GetColor(Theme::ColorText));
						StringFormat sf;
						g.DrawString(fixture->_text.c_str(), (int)fixture->_text.length(), fnt, RectF(fixture->_x*cW+lbw, fixture->_y*cH+fixture->_size, 2*fixture->_size, 32.0f), &sf, &tbr);
					}

					g.EndContainer(gc);
					++it;
				}
			}
			// draw selection square
		}

		virtual void Update() {
			ref<DMXViewerTrack> track = _track;
			if(track) {
				int mid = track->_plugin->_dmx->GetController()->GetModificationID();
				if(_modID!=mid) {
					_modID = mid;
					Repaint();
				}
			}
		}

		virtual void SetPropertyGrid(ref<PropertyGridProxy> pg) {
			_properties = pg;
		}

	protected:
		weak<DMXViewerTrack> _track;
		ref<DMXViewerFixture> _dragging;
		ref<DMXViewerFixture> _focused;
		std::set< ref<DMXViewerFixture> > _selected;
		std::wstring _backgroundImageCached;
		tj::shared::graphics::Bitmap* _backgroundImage;
		Icon _bulbImage;
		Icon _textImage;
		Icon _textHoverImage;
		Icon _bulbHoverImage;

		ref<PropertyGridProxy> _properties;
		bool _isDragging;
		int _offX, _offY; // offset for dragging
		volatile unsigned int _modID;
};

class DMXViewerToolbarWnd: public ToolbarWnd {
	public:
		DMXViewerToolbarWnd() {
		}

		virtual ~DMXViewerToolbarWnd() {
		}

		virtual void OnCommand(ref<ToolbarItem> ti) {
		}
};

class DMXViewerMainWnd: public ChildWnd {
	public:
		DMXViewerMainWnd(ref<DMXViewerTrack> track): ChildWnd(false) {
			_toolbar = GC::Hold(new DMXViewerToolbarWnd());
			_viewer = GC::Hold(new DMXViewerWnd(track));
			Add(_toolbar);
			Add(_viewer);
			Layout();
		}

		virtual ~DMXViewerMainWnd() {
		}

		virtual void Layout() {
			Area rect = GetClientArea();
			_toolbar->Fill(LayoutTop, rect);
			_viewer->Fill(LayoutFill, rect);
		}

		ref<DMXViewerWnd> GetViewerWindow() {
			return _viewer;
		}

		virtual void OnSize(const Area& ns) {
			Layout();
		}

		virtual void Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme) {
		}

	protected:
		ref<ToolbarWnd> _toolbar;
		ref<DMXViewerWnd> _viewer;
};

class DMXViewerControl: public LiveControl {
	public:
		DMXViewerControl(ref<DMXViewerTrack> track) {
			_wnd = GC::Hold(new DMXViewerMainWnd(track));
		}

		virtual ~DMXViewerControl() {
		}

		virtual void Tick() {
			if(_wnd) {
				_wnd->Repaint();
			}
		}

		virtual ref<Wnd> GetWindow() {
			return _wnd;
		}
		
		virtual std::wstring GetGroupName() {
			return L"DMX Viewer";
		}

		virtual bool IsSeparateTab() {
			return true;
		}

		virtual void SetPropertyGrid(ref<PropertyGridProxy> pg) {
			ref<DMXViewerMainWnd> wnd = _wnd;
			if(wnd) {
				wnd->GetViewerWindow()->SetPropertyGrid(pg);
			}
		}

		virtual void Update() {
			ref<DMXViewerMainWnd> wnd = _wnd;
			if(wnd) {
				wnd->Update();
			}
		}

	protected:
		ref<DMXViewerMainWnd> _wnd;
};

ref<LiveControl> DMXViewerTrack::CreateControl(ref<Stream> str) {
	return GC::Hold(new DMXViewerControl(ref<DMXViewerTrack>(this)));
}
