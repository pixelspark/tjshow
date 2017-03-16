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
using namespace tj::jingle;
using namespace tj::shared;

namespace tj {
	namespace jingle {
		class QwertyKeyboardLayout: public JingleKeyboardLayout {
			public:
				virtual ~QwertyKeyboardLayout() {
				}

				virtual unsigned int GetRowCount() const {
					return 4;
				}

				virtual unsigned int GetColumnCount() const {
					return 10;
				}

				virtual wchar_t GetCharacterAt(unsigned int index) {
					if(index < (GetRowCount()*GetColumnCount())) {
						return KLayoutQwerty[index];
					}
					return 0;
				}

				virtual wchar_t GetCharacterAt(unsigned int row, unsigned int col) {
					return KLayoutQwerty[(row * GetColumnCount()) + col];
				}

			protected:
				static const wchar_t KLayoutQwerty[];
		};

		class QwertzKeyboardLayout: public JingleKeyboardLayout {
			public:
				virtual ~QwertzKeyboardLayout() {
				}

				virtual unsigned int GetRowCount() const {
					return 4;
				}

				virtual unsigned int GetColumnCount() const {
					return 10;
				}

				virtual wchar_t GetCharacterAt(unsigned int index) {
					if(index < (GetRowCount()*GetColumnCount())) {
						return KLayoutQwertz[index];
					}
					return 0;
				}

				virtual wchar_t GetCharacterAt(unsigned int row, unsigned int col) {
					return KLayoutQwertz[(row * GetColumnCount()) + col];
				}

			protected:
				static const wchar_t KLayoutQwertz[];
		};

		class AzertyKeyboardLayout: public JingleKeyboardLayout {
			public:
				virtual ~AzertyKeyboardLayout() {
				}

				virtual unsigned int GetRowCount() const {
					return 4;
				}

				virtual unsigned int GetColumnCount() const {
					return 10;
				}

				virtual wchar_t GetCharacterAt(unsigned int row, unsigned int col) {
					return KLayoutAzerty[(row * GetColumnCount()) + col];
				}

				virtual wchar_t GetCharacterAt(unsigned int index) {
					if(index < (GetRowCount()*GetColumnCount())) {
						return KLayoutAzerty[index];
					}
					return 0;
				}

			protected:
				static const wchar_t KLayoutAzerty[];
		};

		class DvorakKeyboardLayout: public JingleKeyboardLayout {
			public:
				virtual ~DvorakKeyboardLayout() {
				}

				virtual unsigned int GetRowCount() const {
					return 4;
				}

				virtual unsigned int GetColumnCount() const {
					return 10;
				}

				virtual wchar_t GetCharacterAt(unsigned int row, unsigned int col) {
					return KLayoutDvorak[(row * GetColumnCount()) + col];
				}

				virtual wchar_t GetCharacterAt(unsigned int index) {
					if(index < (GetRowCount()*GetColumnCount())) {
						return KLayoutDvorak[index];
					}
					return 0;
				}

			protected:
				static const wchar_t KLayoutDvorak[];
		};

		const wchar_t QwertyKeyboardLayout::KLayoutQwerty[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', 0, 'Z', 'X', 'C', 'V', 'B', 'N', 'M', 0, 0, 0, 0};
		const wchar_t QwertzKeyboardLayout::KLayoutQwertz[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 'Q', 'W', 'E', 'R', 'T', 'Z', 'U', 'I', 'O', 'P', 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', 0, 'Y', 'X', 'C', 'V', 'B', 'N', 'M', 0, 0, 0, 0};
		const wchar_t AzertyKeyboardLayout::KLayoutAzerty[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 'A', 'Z', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', 'Q', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', 'M', 'W', 'X', 'C', 'V', 'B', 'N', 0, 0, 0, 0, 0};
		
		const wchar_t DvorakKeyboardLayout::KLayoutDvorak[] = {
			'1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
			0, 'P', 'Y', 'F', 'G', 'C', 'R', 'L', 0, 0,
			'A', 'O', 'E', 'U', 'I', 'D', 'H', 'T', 'N', 'S',
			0, 'Q', 'J', 'K', 'X', 'B', 'M', 'W', 'V', 'Z',
		};
		
	}
}

JingleKeyboardLayout::~JingleKeyboardLayout() {
}

/** JingleKeyboardLayoutFactory **/
ref<JingleKeyboardLayoutFactory> JingleKeyboardLayoutFactory::_instance;

JingleKeyboardLayoutFactory::~JingleKeyboardLayoutFactory() {
}

strong<JingleKeyboardLayoutFactory> JingleKeyboardLayoutFactory::Instance() {
	if(!_instance) {
		_instance = GC::Hold(new JingleKeyboardLayoutFactory());
		
		// Friendly names of the prototypes are TL'ed elsewhere
		_instance->RegisterPrototype(L"qwerty",GC::Hold(new SubclassedPrototype<QwertyKeyboardLayout, JingleKeyboardLayout>(L"qwerty")));
		_instance->RegisterPrototype(L"qwertz",GC::Hold(new SubclassedPrototype<QwertzKeyboardLayout, JingleKeyboardLayout>(L"qwertz")));
		_instance->RegisterPrototype(L"azerty",GC::Hold(new SubclassedPrototype<AzertyKeyboardLayout, JingleKeyboardLayout>(L"azerty")));
		_instance->RegisterPrototype(L"dvorak",GC::Hold(new SubclassedPrototype<DvorakKeyboardLayout, JingleKeyboardLayout>(L"dvorak")));
	}
	return _instance;
}

