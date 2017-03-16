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
#ifndef _TJFADEABLEITEM_H
#define _TJFADEABLEITEM_H

#include <math.h>

namespace tj {
	namespace show {
		template<typename T> class FaderChange: public Change {
			public:
				FaderChange(ref< Fader<T> > fader): _fader(fader) {
				}

				virtual ~FaderChange() {
				}

				virtual bool CanUndo() {
					return ref< Fader<T> >(_fader);
				}

				virtual bool CanRedo() {
					return CanUndo();
				}

			protected:
				ref< Fader<T> > _fader;
		};

		template<typename T> class FaderValueChange: public FaderChange<T> {
			public:
				FaderValueChange(ref< Fader<T> > fader, const Time& selected, const T& oldValue, const T& newValue): FaderChange<T>(fader), _selected(selected), _oldValue(oldValue), _newValue(newValue) {
				}

				virtual ~FaderValueChange() {
				}

				virtual void Redo() {
					ref< Fader<T> > fader = _fader;
					if(fader) {
						fader->SetPoint(_selected, _newValue);
					}
				}

				virtual void Undo() {
					ref< Fader<T> > fader = _fader;
					if(fader) {
						fader->SetPoint(_selected, _oldValue);
					}
				}

			protected:
				const Time& _selected;
				const T _oldValue;
				const T _newValue;
		};

		template<typename T> class FaderPointRemoveChange: public FaderChange<T> {
			public:
				FaderPointRemoveChange(ref< Fader<T> > fader, const Time& at, const T& oldValue): FaderChange<T>(fader), _at(at), _value(oldValue) {
				}

				virtual ~FaderPointRemoveChange() {
				}

				virtual void Undo() {
					ref< Fader<T> > fader = _fader;
					if(fader) {
						fader->AddPoint(_at, _value);
					}
				}

				virtual void Redo() {
					ref< Fader<T> > fader = _fader;
					if(fader) {
						fader->RemovePoint(_at);
					}
				}

			protected:
				Time _at;
				const T _value;
		};

		template<typename T> class FaderItem: public Item {
			public:
				FaderItem(ref< Fader<T> > fb, Time selected, int height, int offset=0): _fader(fb), _selected(selected), _oldTime(selected), _height(height), _offset(offset) {
					_oldValue = fb->GetValueAt(selected);
				}

				virtual ~FaderItem() {
				}

				virtual std::wstring GetTooltipText() {
					return Stringify(_fader->GetValueAt(_selected))+std::wstring(L" @ ")+_selected.Format()+ L" ("+Stringify(int(_selected)) + L")";
				}

				virtual void MoveEnded() {
					if(_selected < Time(0)) {
						// Point was removed
						UndoBlock::AddChange(GC::Hold(new FaderPointRemoveChange<T>(_fader, _oldTime, _fader->GetDefaultValue())));
					}
					else if(_oldTime == _selected) {
						// Point only moved up or down
						T value = _fader->GetValueAt(_selected);
						UndoBlock::AddChange(GC::Hold(new FaderValueChange<T>(_fader, _selected, _oldValue, value)));
						_oldValue = value;
					}
					else {
						// Point moved to another time and value; add a change to remove the old one
						T value = _fader->GetValueAt(_selected);
						UndoBlock::AddChange(GC::Hold(new FaderPointRemoveChange<T>(_fader, _oldTime, _oldValue)));
						UndoBlock::AddChange(GC::Hold(new FaderValueChange<T>(_fader, _selected, _fader->GetDefaultValue(), value)));
					}
				}

				virtual void Move(Time t, int h) {
					if(_selected<Time(0)) return;

					h += _offset;
					if(h<-25) {
						_fader->RemovePoint(_selected);
						_selected = Time(-1);
						return;
					}
					else if(h>_height) {
						h = _height;
					}
					else if(h<1) {
						h = 0;
					}

					if(_fader->DoesPointExist(t) && t!=_selected) return;

					T value = T((float(h)/float(_height)) * fabs(float(_fader->GetMaximum()-_fader->GetMinimum()))) + _fader->GetMinimum();

					if(t!=_selected) {
						_fader->RemovePoint(_selected);
						_selected = t;
						_fader->AddPoint(t, value);
					}
					else {
						_fader->SetPoint(_selected, value);
					}
				}

				virtual void Remove() {
					if(_selected<Time(0)) return;
					UndoBlock::AddAndDoChange(GC::Hold(new FaderPointRemoveChange<T>(_fader, _selected, _fader->GetDefaultValue())));
					_selected = Time(-1);
				}

				virtual void MoveRelative(Item::Direction dir) {
					if(_selected<Time(0)) return;
					T increments = (_fader->GetMaximum() - _fader->GetMinimum()) / 10;
					
					switch(dir) {
						case Item::Up: {
							T newValue = Clamp(_fader->GetValueAt(_selected) + increments, _fader->GetMinimum(), _fader->GetMaximum());
							UndoBlock::AddAndDoChange(GC::Hold(new FaderValueChange<T>(_fader, _selected, _fader->GetValueAt(_selected), newValue)));
							break;
						}

						case Item::Down: {
							T newValue = Clamp(_fader->GetValueAt(_selected) - increments, _fader->GetMinimum(), _fader->GetMaximum());
							UndoBlock::AddAndDoChange(GC::Hold(new FaderValueChange<T>(_fader, _selected, _fader->GetValueAt(_selected), newValue)));
							break;
						}

						case Item::Left: {
							T value = _fader->GetValueAt(_selected);
							_fader->RemovePoint(_selected);
							_selected = _selected - Time(5);
							_fader->SetPoint(_selected, value);
							break;
						}

						case Item::Right: {
							T value = _fader->GetValueAt(_selected);
							_fader->RemovePoint(_selected);
							_selected = _selected + Time(5);
							_fader->SetPoint(_selected, value);
							break;
						}
					}
				}

				virtual Time GetTime() {
					return _selected;
				}

			protected:
				ref<Fader<T> > _fader;
				Time _selected;
				int _height;
				int _offset;
				T _oldValue;
				Time _oldTime;
		};
	}
}

#endif