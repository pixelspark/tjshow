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
#ifndef _TJCUETRACK_H
#define _TJCUETRACK_H

namespace tj {
	namespace show {
		template<class TCue> class CueTrackItem;

		template<class TCue> class CueTrack: public Track {
			friend class CueTrackItem<TCue>;

			public:
				CueTrack(ref<Playback> pb = null): _pb(pb) {
				}
				
				virtual ~CueTrack() {
				}
				
				virtual std::wstring GetEmptyHintText() const = 0;

				virtual std::string GetElementName() const {
					return "cue";
				}

				virtual bool PasteItemAt(const Time& at, TiXmlElement* you) {
					std::string elementName = GetElementName();
					TiXmlElement* cueElement = you->FirstChildElement(elementName);
					if(cueElement!=0) {
						ref<TCue> cue = GC::Hold(new TCue(at));
						cue->Load(cueElement);
						cue->SetTime(at); // Load might have loaded the time from the copied cue
						_cues.push_back(cue);
						return true;
					}
					return false;
				}

				virtual void Save(TiXmlElement* parent) {
					std::vector< ref<TCue> >::iterator it = _cues.begin();
					while(it!=_cues.end()) {
						(*it)->Save(parent);
						++it;
					}
				}

				virtual void Load(TiXmlElement* you) {
					_cues.clear();
					std::string elementName = GetElementName();

					TiXmlElement* cue = you->FirstChildElement(elementName.c_str());
					while(cue!=0) {
						ref<TCue> newCue = GC::Hold(new TCue());
						newCue->Load(cue);
						_cues.push_back(newCue);
						cue = cue->NextSiblingElement(elementName.c_str());
					}
				}

				virtual void Paint(tj::shared::graphics::Graphics* g, Time position, float pixelsPerMs, unsigned int x, unsigned int y, unsigned int h, Time start, Time end, ref<Item> focus) {
					strong<Theme> theme = ThemeManager::GetTheme();
					tj::shared::graphics::SolidBrush descBrush = theme->GetColor(Theme::ColorDescriptionText);
					
					ref<TCue> focusedCue = 0;
					if(focus!=0 && focus.IsCastableTo<CueTrackItem<TCue> >()) {
						focusedCue = ref<CueTrackItem<TCue> >(focus)->_cue;
					}

					if(_cues.size()==0) {
						tj::shared::graphics::StringFormat sf;
						sf.SetAlignment(tj::shared::graphics::StringAlignmentCenter);

						std::wstring str = GetEmptyHintText();
						g->DrawString(str.c_str(), (int)str.length(), theme->GetGUIFontSmall(), tj::shared::graphics::PointF((float)(((int(end-start)*pixelsPerMs)/2)+x), (float)((h/4)+y)), &sf, &descBrush);
					}
					else {
						std::vector< ref<TCue> >::iterator it = _cues.begin();
						while(it!=_cues.end()) {
							ref<TCue> cue = *it;
							Time cueTime = cue->GetTime();
							if(!(cueTime>=start && cueTime <=end)) {
								++it;
								continue;
							}
							
							float pixelLeft = (pixelsPerMs * int(cueTime-start)) - (KCueWidth/2.0f) + float(x) + 1.0f;
							
							cue->Paint(g,theme, pixelLeft, y, h, this, focusedCue==cue);
							++it;
						}
					}
				}

				virtual ref<Item> GetItemAt(Time pos,unsigned int h, bool right, int th, float pixelsPerMs) {
					return _GetItemAt(pos,h,right,th);
				}

				virtual ref<TCue> GetCueBefore(Time pos) {
					ref<TCue> nearest = 0;
					std::vector< ref<TCue> >::iterator it = _cues.begin();
					while(it!=_cues.end()) {
						ref<TCue> cue = *it;

						if(cue->GetTime()<pos) {	
							if(!nearest) {
								nearest = cue; 
							}
							else if(abs(int(cue->GetTime()-pos)) < abs(int(nearest->GetTime()-pos))) {
								nearest = cue;
							}
						}
						++it;
					}

					return nearest;
				}

				virtual ref<TCue> GetCueAfter(Time pos) {
					ref<TCue> nearest = 0;
					std::vector< ref<TCue> >::iterator it = _cues.begin();
					while(it!=_cues.end()) {
						ref<TCue> cue = *it;

						if(cue->GetTime()>pos) {	
							if(!nearest) {
								nearest = cue; 
							}
							else if(abs(int(cue->GetTime()-pos)) < abs(int(nearest->GetTime()-pos))) {
								nearest = cue;
							}
						}
						++it;
					}

					return nearest;
				}

				virtual void RemoveItemAt(Time t) {
					std::vector< ref<TCue> >::iterator it = _cues.begin();
					while(it!=_cues.end()) {
						ref<TCue> cue = *it;

						if(cue->GetTime()==t) {	
							_cues.erase(it);
							return;
						}
						++it;
					}
				}

				virtual void RemoveAllCues() {
					_cues.clear();
				}

				virtual void GetCuesBetween(Time start, Time end, std::vector< ref<TCue> >& matches) {			
					std::vector< ref<TCue> >::iterator it = _cues.begin();
					while(it!=_cues.end()) {
						ref<TCue> cue = *it;
						Time ctime = cue->GetTime();
						if(ctime>=start && ctime <end) {
							matches.push_back(cue);
						}
						
						++it;
					}
				}

				virtual ref<TrackRange> GetRange(Time start, Time end);

				const static Pixels KCueWidth = 5;
				std::vector< ref<TCue> > _cues;			

			protected:
				ref<Playback> _pb;
				ref<Item> _GetItemAt(Time pos,unsigned int h, bool right, int th);
		};
		
		template<class TCue> class CueTrackItem: public Item, public Serializable, public virtual tj::shared::Inspectable {
			friend class CueTrack<TCue>;

			public:
				CueTrackItem(strong<TCue> cue, strong< CueTrack<TCue> > track, Time clicked): _cue(cue), _track(track) {
					_clicked = cue->GetTime() - clicked;
				}

				virtual ~CueTrackItem() {
				}

				virtual int GetSnapOffset() {
					return -(_clicked.ToInt());
				}

				virtual void Move(Time t, int h) {
					if(h<-25) {
						_track->RemoveItemAt(_cue->GetTime());
						return;
					}
					_cue->Move(t+_clicked,h);
				}

				virtual void Remove() {
					_track->RemoveItemAt(_cue->GetTime());
				}

				virtual void Save(TiXmlElement* you) {
					if(ref<TCue>(_cue).IsCastableTo<Serializable>()) {
						ref<Serializable>(ref<TCue>(_cue))->Save(you);
					}	
				}

				virtual void Load(TiXmlElement* you) {
					if(ref<TCue>(_cue).IsCastableTo<Serializable>()) {
						ref<Serializable>(ref<TCue>(_cue))->Load(you);
					}
				}

				virtual void MoveRelative(Item::Direction d) {
					switch(d) {
						case Item::Left:
							_cue->SetTime(_cue->GetTime()-Time(1));
							break;

						case Item::Right:
							_cue->SetTime(_cue->GetTime()+Time(1));
							break;
					}
				}

				virtual ref<PropertySet> GetProperties() {
					return _cue->GetProperties(_track->_pb, _track);
				}

			protected:
				strong<TCue> _cue;
				strong< CueTrack<TCue> > _track;
				Time _clicked;
		};

		template<class TCue> class CueTrackRange: public TrackRange {
			public:
				CueTrackRange(ref< CueTrack<TCue> > tr, Time s, Time e): _track(tr), _start(s), _end(e) {
				}

				virtual ~CueTrackRange() {
				}

				virtual void RemoveItems() {
					std::vector< ref<TCue> > goodCues;

					std::vector< ref<TCue> >::iterator it = _track->_cues.begin();
					while(it!=_track->_cues.end()) {
						ref<TCue> bc = *it;
						if(bc) {
							Time t = bc->GetTime();
							if(t < _start || t>_end) {
								goodCues.push_back(bc);
							}
						}
						++it;
					}

					_track->_cues = goodCues; // TODO: Not really efficient?
				}

				virtual void Paste(ref<Track> other, Time start) {
					if(other && other.IsCastableTo<CueTrack<TCue> >()) {
						ref<CueTrack<TCue> > cueTrack = other;
						if(cueTrack) {

							std::vector< ref<TCue> >::iterator it = _track->_cues.begin();
							std::vector< ref<TCue> > toInsert;
							while(it!=_track->_cues.end()) {
								ref<TCue> cue = *it;
								if(cue) {
									Time cueTime = cue->GetTime();
									if(cueTime >= _start && cueTime <= _end) {
										ref<TCue> cloned = cue->Clone();
										cloned->SetTime((cueTime-_start) + start);
										toInsert.push_back(cloned);
									}
								}
								++it;
							}

							std::vector<ref<TCue> >::iterator tit = toInsert.begin();
							while(tit!=toInsert.end()) {
								ref<TCue> cue = *tit;
								cueTrack->_cues.push_back(cue);
								++tit;
							}
						}
					}
				}

				virtual void InterpolateLeft(Time left, float ratio, Time right) {
					std::vector< ref<TCue> >::iterator it = _track->_cues.begin();
					while(it!=_track->_cues.end()) {
						ref<TCue> bc = *it;
						if(bc) {
							Time t = bc->GetTime();
							if(t > left && t < right) {
								Time nt = Time(int((t - left).ToInt()*ratio)) + left;
								bc->SetTime(nt);
							}
						}
						++it;
					}
				}

				virtual void InterpolateRight(Time left, float ratio, Time right) {
					std::vector< ref<TCue> >::iterator it = _track->_cues.begin();
					while(it!=_track->_cues.end()) {
						ref<TCue> bc = *it;
						if(bc) {
							Time t = bc->GetTime();
							if(t > left && t < right) {
								Time nt = right - Time(int((right - t).ToInt()*ratio));
								bc->SetTime(nt);
							}
						}
						++it;
					}
				}

			protected:
				Time _start, _end;
				ref< CueTrack<TCue> > _track;
		};

		template<class TCue> ref<Item> CueTrack<TCue>::_GetItemAt(Time pos,unsigned int h, bool right, int th) {
			if(right) {
				// create new cue
				ref<TCue> cue = GC::Hold(new TCue(pos));
				_cues.push_back(cue);
				return 0;
			}
			else {
				// get nearest cue
				ref<TCue> nearest = 0;
				std::vector< ref<TCue> >::iterator it = _cues.begin();
				while(it!=_cues.end()) {
					ref<TCue> cue = *it;
					if(!nearest) {
						nearest = cue; 
					}
					else if(abs(int(cue->GetTime()-pos)) < abs(int(nearest->GetTime()-pos))) {
						nearest = cue;
					}
					++it;
				}
				
				if(nearest) {
					return GC::Hold(new CueTrackItem<TCue>(nearest, ref< CueTrack<TCue> >(this), pos));
				}
				return 0;
			}
		}

		template<class TCue> ref<TrackRange> CueTrack<TCue>::GetRange(Time s, Time e) {
			return GC::Hold(new CueTrackRange<TCue>(this,s,e));
		}
	}
}

#endif