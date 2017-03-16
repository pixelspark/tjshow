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
#ifndef _TJMULTIFADERTRACK_H
#define _TJMULTIFADERTRACK_H

using namespace tj::shared;
using namespace tj::shared::graphics;

namespace tj {
	namespace show {
		template<typename T> inline Time FaderSnap(Time position, ref< Fader<T> > fader) {
			Time nearest = fader->GetNearest(position);
			if(abs(int(nearest-position)) < 50) {
				return nearest;
			}
			return position;
		}

		class MultifaderTrackRange;

		class MultifaderTrack: public Track {
			public:
				MultifaderTrack() {
					_faderHeight = KDefaultFaderHeight;
					_expanded = false;
				}

				virtual ~MultifaderTrack() {
				}

				class NamedFader {
					public:
						NamedFader(ref< Fader<float> >  f, std::wstring name, int id) {
							_name = name;
							_fader = f;
							_id = id;
						}

						virtual ~NamedFader() {
						}

						ref< Fader<float> > _fader;
						std::wstring _name;
						int _id;
				};

				virtual void Save(TiXmlElement* parent) {
					// Save enabled/disabled faders separately
					SaveFaders(parent, _faders.begin(), _faders.end(), true);
					SaveFaders(parent, _disabledFaders.begin(), _disabledFaders.end(), false);
				}

				virtual void Load(TiXmlElement* you) {
					TiXmlElement* faderElement = you->FirstChildElement("fader");
					while(faderElement!=0) {
						int id = LoadAttributeSmall<int>(faderElement, "id", 0);
						std::wstring name = LoadAttributeSmall<std::wstring>(faderElement,"name", L"");
						bool enabled = LoadAttributeSmall<bool>(faderElement, "enabled", true);

						ref<NamedFader> fader;

						// Try to find the fader in the enabled faders list
						std::vector< ref<NamedFader> >::iterator it = _faders.begin();
						while(it!=_faders.end()) {
							ref<NamedFader> fb = *it;
							if(fb->_id==id) {
								fader = fb;
								break;
							}
							++it;
						}

						// If not found, try to find it in the disabled faders list
						if(!fader) {
							std::vector< ref<NamedFader> >::iterator it = _disabledFaders.begin();
							while(it!=_disabledFaders.end()) {
								ref<NamedFader> fb = *it;
								if(fb->_id==id) {
									fader = fb;
									break;
								}
								++it;
							}
						}

						if(!fader) {
							ref< Fader<float> > fadeable = GC::Hold(new Fader<float>(1.0f, 0.0f, 1.0f));
							fader = GC::Hold(new NamedFader(fadeable, name, id));
							if(enabled) {
								_faders.push_back(fader);
							}
							else {
								_disabledFaders.push_back(fader);
							}
						}

						TiXmlElement* subFader = faderElement->FirstChildElement("fader");
						if(subFader!=0) {
							fader->_fader->Load(subFader);
						}
						faderElement = faderElement->NextSiblingElement("fader");
					}
				}

				virtual void AddFader(int id, std::wstring name, float defaultValue=1.0f, bool disabled = false) {
					ref<Fader<float> > fader = GC::Hold(new Fader<float>(defaultValue, 0.0f, 1.0f));
					ref<NamedFader> nf = GC::Hold(new NamedFader(fader, name,id));

					if(disabled) {
						_disabledFaders.push_back(nf);
					}
					else {
						_faders.push_back(nf);
					}
				}

				virtual bool IsFaderEnabled(int id) {
					std::vector< ref<NamedFader> >::iterator it = _faders.begin();
					while(it!=_faders.end()) {
						if(*it && (*it)->_id==id) {
							return true;
						}
						++it;
					}

					return false;
				}

				virtual bool IsExpandable() {
					return true;
				}

				virtual void SetExpanded(bool t) {
					_expanded = t;
				}

				virtual bool IsExpanded() {
					return _expanded;
				}

				virtual void Paint(tj::shared::graphics::Graphics* g, Time position, float pixelsPerMs, unsigned int x, unsigned int y, unsigned int h, Time start, Time end, ref<Item> focus) {
					int cy = y+_faderHeight;
					strong<Theme> theme = ThemeManager::GetTheme();
					Pen pen(theme->GetColor(Theme::ColorFader), 1.5f);
					SolidBrush textBrush(theme->GetColor(Theme::ColorActiveStart));
					StringFormat sf;
					sf.SetAlignment(StringAlignmentNear);
					sf.SetLineAlignment(StringAlignmentCenter);

					Color background = theme->GetColor(Theme::ColorTimeBackground);
					Color aBackground = Theme::ChangeAlpha(background, 0);
					LinearGradientBrush gradientBrush(PointF(float(x-2), float(y)), PointF(float(x+55), float(y)), theme->GetColor(Theme::ColorTimeBackground), aBackground);
					
					if(_expanded) {
						std::vector< ref<NamedFader> >::iterator it = _faders.begin();
						while(it!=_faders.end()) {
							ref<NamedFader> fader = *it;
							FaderPainter<float> painter(fader->_fader);
							int w = int(float(end.ToInt()-start.ToInt())*pixelsPerMs);
							painter.Paint(g, start, end-start, _faderHeight-1, w, x, cy, &pen, Time(0), true,focus);

							RectF lr(float(x)+2.0f, float(cy), 50.0f, float(_faderHeight));
							g->FillRectangle(&gradientBrush, lr);
							g->DrawString(fader->_name.c_str(), (int)fader->_name.length(), theme->GetGUIFontSmall(), lr, &sf, &textBrush);

							++it;
							cy += _faderHeight;
						}
					}
				}

				virtual ref<Item> GetItemAt(Time position,unsigned int h, bool rightClick, int th, float pixelsPerMs) { 
					if(!_expanded) return 0;

					int hi = (int(_faders.size())*_faderHeight) - h;

					if(_faders.size()<=0 || hi < 0) {
						return 0;
					}

					int index = hi/_faderHeight;
					if(index>int(_faders.size())) {
						return 0;
					}

					ref<NamedFader> fader = _faders.at(index);
					if(fader) {
						Time pos = FaderSnap<float>(position, fader->_fader);
						int nindex = int(_faders.size()) - index;
						if(rightClick) {
							// Right click only adds a new point at the current value
							fader->_fader->AddPoint(pos, fader->_fader->GetValueAt(pos));
							return 0;
						}
						else {
							// Get the nearest point. If it is too far away, create a new one
							Time nearestPoint = fader->_fader->GetNearest(pos);
							if(IsStickingAllowed() && (abs(nearestPoint.ToInt()-pos.ToInt())*pixelsPerMs)>KFaderStickLimit) {
								return GC::Hold(new FaderItem<float>(fader->_fader, pos,_faderHeight, -nindex*_faderHeight+_faderHeight));	
							}
							else {
								return GC::Hold(new FaderItem<float>(fader->_fader, nearestPoint,_faderHeight, -nindex*_faderHeight+_faderHeight));	
							}
						}
					}

					return 0;
				}

				virtual Pixels GetHeight() {
					return _expanded?(int(_faders.size()+1)*_faderHeight):_faderHeight;
				}

				int SetFaderHeight(int fh) {
					_faderHeight = fh;
				}

				int GetFaderHeight() const {
					return _faderHeight;
				}
				
				virtual void RemoveItemsBetween(Time s, Time e) {
					std::vector< ref<NamedFader> >::iterator it = _faders.begin();
					while(it!=_faders.end()) {
						ref<NamedFader> fader = *it;
						fader->_fader->RemoveItemsBetween(s,e);
						++it;
					}
				}

				virtual Time GetNextEvent(Time t) {
					Time evt(-1);
					std::vector< ref<NamedFader> >::iterator it = _faders.begin();
					while(it!=_faders.end()) {
						ref<NamedFader> fader = *it;
						Time myEvent = fader->_fader->GetNextEvent(t);
						if(myEvent!=Time(-1) && (myEvent<evt || evt==Time(-1))) {
							evt = myEvent;
						}
						++it;
					}

					return evt;
				}

				virtual void InterpolateLeft(Time left, float r, Time right) {
					std::vector< ref<NamedFader> >::iterator it = _faders.begin();
					while(it!=_faders.end()) {
						ref<NamedFader> fader = *it;
						fader->_fader->InterpolateLeft(left,r,right);
						++it;
					}
				}

				virtual void InterpolateRight(Time left, float r, Time right) {
					std::vector< ref<NamedFader> >::iterator it = _faders.begin();
					while(it!=_faders.end()) {
						ref<NamedFader> fader = *it;
						fader->_fader->InterpolateRight(left,r,right);
						++it;
					}
				}

				ref< Fader<float> > GetFaderById(int id) {
					std::vector< ref<NamedFader> >::iterator it = _faders.begin();
					while(it!=_faders.end()) {
						ref<NamedFader> fader = *it;
						if(fader->_id==id) return fader->_fader;
						++it;
					}

					it = _disabledFaders.begin();
					while(it!=_disabledFaders.end()) {
						ref<NamedFader> fader = *it;
						if(fader->_id==id) return fader->_fader;
						++it;
					}

					return 0;
				}

				virtual void SetFaderEnabled(int id, bool t) {
					if(!t) {
						// Find the fader in _faders and move it to _disabledFaders
						std::vector< ref<NamedFader> >::iterator it = _faders.begin();
						while(it!=_faders.end()) {
							if(*it && (*it)->_id==id) {
								ref<NamedFader> nf = *it;
								it = _faders.erase(it);
								_disabledFaders.push_back(nf);
								return;
							}
							++it;
						}
					}
					else {
						// Find the fader in _disabledFaders and move it to _faders
						std::vector< ref<NamedFader> >::iterator it = _disabledFaders.begin();
						while(it!=_disabledFaders.end()) {
							if(*it && (*it)->_id==id) {
								ref<NamedFader> nf = *it;
								it = _disabledFaders.erase(it);
								_faders.push_back(nf);
								return;
							}
							++it;
						}
					}
				}

				int GetFaderCount() const {
					return int(_faders.size());
				}

				/** Sticking means that when you left-click a fader, a new point can be created
				if there is no existing point near the place you've clicked. If there is a point
				near, then that point might be moved to where you clicked. **/
				static bool IsStickingAllowed() {
					ref<Settings> st = ThemeManager::GetLayoutSettings();
					if(st) {
						return st->GetFlag(L"layout.faders.sticky", true);
					}
					return true;
				}

				// Defines how far the user should click (in pixels) from an existing point
				// to create a new one. If the user is closer than KFaderStickLimit pixels,
				// the nearest point will be used.
				const static Pixels KFaderStickLimit = 5;
				const static int KDefaultFaderHeight = 24;

			private:
				class ChooseFaderProperty: public LinkProperty {
					public:
						ChooseFaderProperty(const std::wstring& name, ref<Inspectable> holder, const std::wstring& text, ref<MultifaderTrack> mf): LinkProperty(name, text, L"icons/fader.png"), _mf(mf), _holder(holder) {
						}

						virtual ~ChooseFaderProperty() {
						}

						virtual void OnClicked() {
							ref<MultifaderTrack> mf = _mf;
							if(mf) {
								mf->DoEnableDisableDialog();
							}
						}

					private:
						ref<Inspectable> _holder;
						weak<MultifaderTrack> _mf;
				};

			protected:				
				ref<Property> CreateChooseFadersProperty(const std::wstring& name, ref<Inspectable> holder, const std::wstring& hint) {
					return GC::Hold(new ChooseFaderProperty(name, holder, hint, this));
				}

				class EnableDisableDialogData: public Inspectable {
					public:	
						EnableDisableDialogData(MultifaderTrack& track): _track(track) {
							size_t nf = _track._faders.size();
							size_t nd = _track._disabledFaders.size();

							_faders = new bool[nf];
							_disabledFaders = new bool[nd];

							for(size_t a=0;a<nf;a++) {
								_faders[a] = true;
							}
							for(size_t b=0;b<nd;b++) {
								_disabledFaders[b] = false;
							}
						}

						virtual ~EnableDisableDialogData() {
						}

						virtual ref<PropertySet> GetProperties() {
							ref<PropertySet> ps = GC::Hold(new PropertySet());

							// Add properties for enabled faders
							std::vector< ref<NamedFader> >::iterator it = _track._faders.begin();
							int a = 0;
							while(it!=_track._faders.end()) {
								ref<NamedFader> fader = *it;
								if(fader) {
									ps->Add(GC::Hold(new GenericProperty<bool>(fader->_name, this, &(_faders[a]), true)));
								}
								++it;
								++a;
							}

							// Add properties for disabled faders
							it = _track._disabledFaders.begin();
							a = 0;
							while(it!=_track._disabledFaders.end()) {
								ref<NamedFader> fader = *it;
								if(fader) {
									ps->Add(GC::Hold(new GenericProperty<bool>(fader->_name, this, &(_disabledFaders[a]), false)));
								}
								++it;
								++a;
							}

							return ps;
						}

						virtual void Update() {
							std::vector< ref<NamedFader> > disable;

							// Check for enabled faders that now want to be disabled
							std::vector< ref<NamedFader> >::iterator it = _track._faders.begin();
							int a = 0;
							while(it!=_track._faders.end()) {
								ref<NamedFader> fader = *it;
								if(fader) {
									bool pref = _faders[a];
									if(!pref) {
										// Can't add the fader to disabledFaders right here, since we need to process
										// those too; add it to the temporary 'disable' list
										it = _track._faders.erase(it);
										disable.push_back(fader);
										++a;
										continue;
									}
								}
								++it;
								++a;
							}

							// Add properties for disabled faders
							it = _track._disabledFaders.begin();
							a = 0;
							while(it!=_track._disabledFaders.end()) {
								ref<NamedFader> fader = *it;
								if(fader) {
									bool pref = _disabledFaders[a];
									if(pref) {
										it = _track._disabledFaders.erase(it);
										_track._faders.push_back(fader);
										++a;
										continue;
									}
									
								}
								++it;
								++a;
							}

							// Process the disabled temporary list
							it = disable.begin();
							while(it!=disable.end()) {
								_track._disabledFaders.push_back(*it);
								++it;
							}
						}

					protected:
						MultifaderTrack& _track;
						bool* _faders;
						bool* _disabledFaders;
				};

				void DoEnableDisableDialog() {
					ref<PropertyDialogWnd> dw = GC::Hold(new PropertyDialogWnd(TL(multifader_enable_dialog_title), TL(multifader_enable_dialog_question)));
					ref<PropertyGridWnd> pg = dw->GetPropertyGrid();
					ref<EnableDisableDialogData> eddd = GC::Hold(new EnableDisableDialogData(*this));
					pg->Inspect(eddd);
					dw->SetSize(300, 400);
					if(dw->DoModal(0)) {
						eddd->Update();
					}
				}

				void SaveFaders(TiXmlElement* parent, std::vector< ref<NamedFader> >::iterator it, std::vector< ref<NamedFader> >::iterator end, bool enabled) {
					while(it!=end) {
						ref<NamedFader> fader = *it;
						TiXmlElement faderElement("fader");
						SaveAttributeSmall(&faderElement, "name", fader->_name);
						SaveAttributeSmall(&faderElement, "id", fader->_id);
						SaveAttributeSmall(&faderElement, "enabled", enabled);
						fader->_fader->Save(&faderElement);
						parent->InsertEndChild(faderElement);
						++it;
					}
				}

				std::vector< ref<NamedFader> > _faders;
				std::vector< ref<NamedFader> > _disabledFaders;
				int _faderHeight;
				bool _expanded;
		};

		class MultifaderTrackRange: public TrackRange {
			public:
				MultifaderTrackRange(ref<MultifaderTrack> tr, Time s, Time e): _track(tr), _start(s), _end(e) {
					assert(tr);
				}

				virtual ~MultifaderTrackRange() {
				}

				virtual void RemoveItems() {
					_track->RemoveItemsBetween(_start,_end);
				}

				virtual void Paste(ref<Track> other, Time start) {
				}

				virtual void InterpolateLeft(Time left, float ratio, Time right) {
					_track->InterpolateLeft(left,ratio,right);
				}

				virtual void InterpolateRight(Time left, float ratio, Time right) {
					_track->InterpolateRight(left,ratio,right);
				}

			protected:
				Time _start, _end;
				ref<MultifaderTrack> _track;

		};
	}
}

#endif