/* This file is part of TJShow. TJShow is free software: you 
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
 
 #ifndef _TJGENERICPROPERTY_H
#define _TJGENERICPROPERTY_H

namespace tj {
	namespace shared {
		template<typename T> class PropertyChange: public Change {
			public:
				PropertyChange(ref<Inspectable> is, const std::wstring& name, T* value, const T& oldValue, const T& newValue): Change(L""), _holder(is), _value(value), _oldValue(oldValue), _newValue(newValue) {
					std::wostringstream wos;
					wos << name << L": '" << oldValue << L"' => '" << newValue << L"'";
					this->_description = wos.str();
				}

				virtual ~PropertyChange() {
				}
				
				virtual void Undo() {
					ref<Inspectable> holder = _holder;
					if(holder) {
						*_value = _oldValue;
						holder->OnPropertyChanged(reinterpret_cast<void*>(_value));
					}
				}
				
				virtual void Redo() {
					ref<Inspectable> holder = _holder;
					if(holder) {
						*_value = _newValue;
						holder->OnPropertyChanged(reinterpret_cast<void*>(_value));
					}
				}

				virtual bool CanUndo() {
					return ref<Inspectable>(_holder) && _value!=0L;
				}

				virtual bool CanRedo() {
					return CanUndo();
				}

			private:
				weak<Inspectable> _holder;
				T* _value;
				const T _oldValue;
				const T _newValue;
		};

		template<typename T> class GenericProperty: public Property, public Listener<EditWnd::EditingNotification> {
			public:
				GenericProperty(const String& name, ref<Inspectable> holder, T* value, const T& defaultValue): Property(name), _holder(holder), _multiLine(false), _value(value), _defaultValue(defaultValue) {
					if(value==0) Throw(L"Property value pointer cannot be null", ExceptionTypeWarning);
				}

				virtual ~GenericProperty() {
					_value = 0;
				}

				virtual void Notify(ref<Object> source, const EditWnd::EditingNotification& ev) {
					if(_wnd && _value!=0) {
						ref<Inspectable> is = _holder;
						if(is) {
							EditWnd::EditingType type = ev.GetType();
							if(type==EditWnd::EditingTextChanged) {
								T value = StringTo<T>(_wnd->GetText(), _defaultValue);
								*_value = value;
								is->OnPropertyChanged(reinterpret_cast<void*>(_value));
							}
							else if(type==EditWnd::EditingStarted) {
								_oldValue = *_value;
							}
							else if(type==EditWnd::EditingEnded) {
								if(_oldValue!=*_value) {
									UndoBlock::AddChange(GC::Hold(new PropertyChange<T>(is, GetName(), _value, _oldValue, *_value)));
								}
							}
						}
					}
				}

				virtual ref<Wnd> GetWindow() {
					if(!_wnd) {
						ref<EditWnd> ew = GC::Hold(new EditWnd(_multiLine));
						ew->EventEditing.AddListener(this);
						ew->SetBorder(true);
						ew->SetCue(Stringify(_defaultValue));
						_wnd = ew;
						Update();
					}
					return _wnd;
				}

				virtual void Update() {
					if(_wnd) {
						ref<Inspectable> is = _holder;
						if(is) {
							_wnd->SetText(Stringify(*_value));
							_wnd->UnsetStyle(WS_DISABLED);
						}
						else {
							_wnd->SetStyle(WS_DISABLED);
						}
						_wnd->Repaint();
					}
				}

			protected:
				virtual void SetMultiline(bool t) {
					_multiLine = t;
				}

				weak<Inspectable> _holder;
				T* _value;
				T _defaultValue;
				T _oldValue;
				ref<Wnd> _wnd;
				bool _multiLine;
		};

		// GenericProperty<bool>
		template<> ref<Wnd> EXPORTED GenericProperty<bool>::GetWindow();
		template<> void EXPORTED GenericProperty<bool>::Update();
		
		// GenericProperty<Time>
		// The default for Stringify(Time) is to output a number of milliseconds, we want the formatted string
		// since StringTo<Time> can perfectly handle that.
		template<> void EXPORTED GenericProperty<Time>::Update();
	}
}

#endif