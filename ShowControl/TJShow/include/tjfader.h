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
#ifndef _TJFADEABLE_H
#define _TJFADEABLE_H

namespace tj {
	namespace show {
		template<typename T> class FaderPainter;

		/** Fader<T> represents a fading value over time. You can add or remove points from it,
		and Player's can 'play' a fadeable value using FaderPlayer<T>. **/
		template<typename T> class Fader: public tj::shared::Serializable {
			friend class FaderPainter<T>;

			public:
				Fader(const T& defaultValue, const T& minimumValue, const T& maximumValue);
				virtual ~Fader();

				T GetDefaultValue();
				void SetDefaultValue(const T& x);
				T GetValueAt(const Time& t);
				void AddPoint(const Time& t, const T& value, bool fade=true);
				void SetPoint(const Time& t, const T& value);
				void RemovePoint(const Time& t);
				virtual void Save(TiXmlElement* parent);
				virtual void Load(TiXmlElement* you);
				Time GetNearest(const Time& t);
				inline T GetMinimum() { return _min; }
				inline T GetMaximum() { return _max; }
				void SetMaximum(const T& max) { _max = max; }
				void SetMinimum(const T& min) { _min = min; }
				bool DoesPointExist(const Time& t);
				Time GetNextEvent(const Time& t);
				Time GetNextPoint(const Time& t);
				std::map<Time, T>* GetPoints();
				void RemoveItemsBetween(const Time& start, const Time& end);
				void CopyTo(tj::shared::ref< Fader<T> > fd, const Time& start, const Time& end, const Time& to);
				tj::shared::Range<Time> GetRange();
				void InterpolateLeft(const Time& left, float r, const Time& right);
				void InterpolateRight(const Time& left, float r, const Time& right);

			protected:
				T _default;
				T _max;
				T _min;
				std::map<Time, T> _points;
		};

		template<typename T> class FaderProperty: public tj::shared::LinkProperty {
			public:
				FaderProperty(ref< Fader<T> > fader, const std::wstring& name, const std::wstring& text, const std::wstring& iconRid=L""): LinkProperty(name, text, iconRid), _fader(fader) {
				}

				virtual ~FaderProperty() {
				}

				virtual void OnClicked() {
					class FaderPropertyData: public tj::shared::Inspectable {
						public:
							std::wstring _data;
							
							virtual ref<PropertySet> GetProperties() {
								ref<PropertySet> ps = GC::Hold(new PropertySet());
								ref<TextProperty> tp = GC::Hold(new TextProperty(TL(fader_data_label), this, &_data, 300));
								tp->SetExpanded(true);
								ps->Add(tp);
								return ps;
							}
					};

					ref<FaderPropertyData> fpd = GC::Hold(new FaderPropertyData());

					if(_fader) {
						// Serialize fader data to text
						std::wostringstream wos;
						std::map<Time, T>* pts = _fader->GetPoints();
						std::map<Time, T>::const_iterator it = pts->begin();
						while(it!=pts->end()) {
							wos << it->first.ToInt() << L"=" << it->second << L"\r\n";
							++it;
						}

						fpd->_data = wos.str();

						ref<PropertyDialogWnd> pdw = GC::Hold(new PropertyDialogWnd(TL(fader_data), TL(fader_data_question)));
						pdw->GetPropertyGrid()->Inspect(fpd);
						pdw->SetSize(400,450);
						if(pdw->DoModal(GetWindow())) {
							pts->clear();
							std::wistringstream wis(fpd->_data);
							while(!wis.eof()) {
								int time = -1;
								T value;
								wchar_t separator;
								wis >> time >> separator >> value;
								if(time>=0) {
									_fader->AddPoint(Time(time), value);
								}
							}
						}
					}
				}

			protected:
				ref< Fader<T> > _fader;
		};

		template<typename T> tj::shared::Range<Time> Fader<T>::GetRange() {
			tj::shared::Range<Time> range(0,0);
			std::map<Time,T>::iterator b = _points.begin();
			if(b!=_points.end()) range.SetStart(b->first);

			std::map<Time,T>::reverse_iterator e = _points.rbegin();
			if(e!=_points.rend()) range.SetEnd(e->first);

			return range;
		}

		template<typename T> Fader<T>::Fader(const T& defaultValue, const T& minimumValue, const T& maximumValue): _default(defaultValue), _max(maximumValue), _min(minimumValue) {
		}

		template<typename T> Fader<T>::~Fader() {
		}

		template<typename T> T Fader<T>::GetDefaultValue() {
			return _default;
		}

		template<typename T> void Fader<T>::SetDefaultValue(const T& x) {
			_default = x;
		}

		template<typename T> void Fader<T>::CopyTo(tj::shared::ref< Fader<T> > other, const Time& start, const Time& end, const Time& t) {
			std::map<Time, T>::iterator low = _points.lower_bound(start);
			std::map<Time, T>::iterator hi = _points.upper_bound(end);

			if(low==_points.end()) return;

			if(hi==_points.end() && _points.size()>0) {
				hi = _points.end();
				hi--;
			}

			while(low!=hi && low!=_points.end() && hi!= _points.end()) {
				std::pair<Time, T> data = *low;
				other->AddPoint(data.first-start + t, data.second);
				low++;
			}
		}

		template<typename T> bool Fader<T>::DoesPointExist(const Time& t) {
			return _points.find(t) != _points.end();
		}

		template<typename T> void Fader<T>::RemovePoint(const Time& t) {
			std::map<Time, T>::iterator it = _points.find(t);
			if(it!=_points.end()) {
				_points.erase(it);
			}
		}

		template<typename T> T Fader<T>::GetValueAt(const Time& t) {
			std::map<Time,T>::iterator it = _points.begin();
			Time nearSmallerT = 0;
			T nearSmallerV = _default;
			Time nearBiggerT = 999999999;
			T nearBiggerV = _default;
			while(it!=_points.end()) {
				T value = it->second;
				Time time = it->first;

				if(time<=t && time>=nearSmallerT) {
					nearSmallerT = time;
					nearSmallerV = value;
				}
				else if(time>t && time<nearBiggerT) {
					nearBiggerT = time;
					nearBiggerV = value;
				}
				++it;
			}

			T dV = nearBiggerV-nearSmallerV;
			Time dT = nearBiggerT - nearSmallerT;
			if(dT==Time(0)) return nearSmallerV;
			float percent = float(t-nearSmallerT) / float(nearBiggerT-nearSmallerT);

			return T(float(nearSmallerV) + percent*float(dV));
		}


		template<typename T> void Fader<T>::AddPoint(const Time& t, const T& value, bool fade=true) {
			if(value>_max||value<_min) return;
			if(!fade) {
				_points.insert(std::pair<Time, T>(t-Time(1), GetValueAt(t)));
			}
			_points.insert(std::pair<Time,T>(t, value));
		}


		template<typename T> void Fader<T>::SetPoint(const Time& t, const T& value) {
			if(value>_max||value<_min) return;
			_points[t] = value;
		}

		template<typename T> std::map<Time, T>* Fader<T>::GetPoints() {
			return &_points;
		}

		template<typename T> void Fader<T>::RemoveItemsBetween(const Time& start, const Time& end) {
			std::map<Time, T>::iterator low = _points.lower_bound(start);
			std::map<Time, T>::iterator hi = _points.upper_bound(end);
			_points.erase(low, hi);
		}

		template<typename T> Time Fader<T>::GetNearest(const Time& t) {
			Time nearest = -1;

			std::map<Time, T>::iterator it = _points.begin();
			while(it!=_points.end()) {
				std::pair<Time, T> data = *it;
				if(nearest==Time(-1) || (abs(int(t-data.first)))<abs(int(t-nearest))) {
					nearest = data.first;
				}
				++it;
			}

			return nearest;
		}

		template<typename T> Time Fader<T>::GetNextEvent(const Time& t) {
			Time nearest = Time(-1);

			std::map<Time, T>::iterator it = _points.begin();
			while(it!=_points.end()) {
				std::pair<Time, T> data = *it;
				if(data.first>t) {
					if(nearest==Time(-1) || (abs(int(t-data.first)))<abs(int(t-nearest))) {
						nearest = data.first;
					}
				}
				++it;
			}

			if(nearest==Time(-1)) return nearest;

			T nearestValue = GetValueAt(nearest);
			if(nearestValue != GetValueAt(t)) {
				return t+Time(1);
			}

			return nearest;
		}

		template<typename T> Time Fader<T>::GetNextPoint(const Time& t) {
			Time nearest = Time(-1);

			std::map<Time, T>::iterator it = _points.begin();
			while(it!=_points.end()) {
				std::pair<Time, T> data = *it;
				if(data.first>t) {
					if(nearest==Time(-1) || (abs(int(t-data.first)))<abs(int(t-nearest))) {
						nearest = data.first;
					}
				}
				++it;
			}

			return nearest;
		}

		template<typename T> void Fader<T>::Save(TiXmlElement* parent) {
			TiXmlElement f("fader");
			f.SetAttribute("default", StringifyMbs(_default));
			std::map<Time, T>::iterator it = _points.begin();
			while(it!=_points.end()) {
				TiXmlElement point("point");
				point.SetAttribute("time", StringifyMbs(it->first));
				point.SetAttribute("value", StringifyMbs(it->second));
				f.InsertEndChild(point);
				++it;
			}
			parent->InsertEndChild(f);
		}

		template<typename T> void Fader<T>::Load(TiXmlElement* you) {
			StringTo<T>(you->Attribute("default"), _default);
			
			TiXmlElement* point = you->FirstChildElement("point");
			while(point!=0) {
				Time time = StringTo<Time>(point->Attribute("time"),-1);
				T value = StringTo<T>(point->Attribute("value"), _default);
				if(time>=Time(0)) {
					AddPoint(time,value);
				}
				point = point->NextSiblingElement("point");
			}
		}

		template<typename T> void Fader<T>::InterpolateLeft(const Time& left, float r, const Time& right) {
			std::map<Time, T>::iterator it = _points.lower_bound(left);
			std::map<Time, T>::iterator end = _points.upper_bound(right);

			std::map< Time, T> temp;

			while(it!=end) {
				std::pair<Time,T> data = *it;
				Time nt = Time(int((data.first-left).ToInt()*r)) + left;
				temp[nt] = data.second;
				++it;
			}

			_points.erase(_points.lower_bound(left),end);
			_points.insert(temp.begin(), temp.end());
		}

		template<typename T> void Fader<T>::InterpolateRight(const Time& left, float r, const Time& right) {
			std::map<Time, T>::iterator it = _points.lower_bound(left);
			std::map<Time, T>::iterator end = _points.upper_bound(right);

			std::map< Time, T> temp;

			while(it!=end) {
				std::pair<Time,T> data = *it;
				Time nt = right - Time((int)((right-data.first).ToInt()*r));
				temp[nt] = data.second;
				++it;
			}

			_points.erase(_points.lower_bound(left),end);
			_points.insert(temp.begin(), temp.end());
		}
	}
}

#endif