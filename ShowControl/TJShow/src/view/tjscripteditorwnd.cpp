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
#include "../../include/internal/tjshow.h"
#include "../../include/internal/view/tjscripteditorwnd.h"

#include <fstream>
#include "../../../Libraries/scintilla.h"
#include "../../../Libraries/scilexer.h"

using namespace tj::show::view;
using namespace tj::shared::graphics;

namespace tj {
	namespace show {
		static bool _ScintillaLoaded = false;
		static const char KScriptKeywords[] = 
			"break if else function return delegate for void true false null var new";

		class ScriptToolbarWnd: public ToolbarWnd {
			enum Commands {
				KCommandRun=1,
				KCommandUndo,
				KCommandRedo,
				KCommandNew,
				KCommandOpen,
				KCommandSave,
				KCommandSaveAs,
			};

			public:
				ScriptToolbarWnd(ScriptEditorWnd* wnd) {
					_editor = wnd;
					Add(GC::Hold(new ToolbarItem(KCommandNew, L"icons/toolbar/new.png", TL(editor_new))));
					Add(GC::Hold(new ToolbarItem(KCommandOpen, L"icons/toolbar/open.png", TL(editor_open))));
					Add(GC::Hold(new ToolbarItem(KCommandSave, L"icons/toolbar/save.png", TL(editor_save))));
					Add(GC::Hold(new ToolbarItem(KCommandSaveAs, L"icons/toolbar/saveas.png", TL(editor_save_as), true)));

					Add(GC::Hold(new ToolbarItem(KCommandUndo, L"icons/editor/undo.png", TL(editor_undo))));
					Add(GC::Hold(new ToolbarItem(KCommandRedo, L"icons/editor/redo.png", TL(editor_redo), true)));
					Add(GC::Hold(new ToolbarItem(KCommandRun, L"icons/editor/run.png", TL(editor_run))));	
				}

				virtual ~ScriptToolbarWnd() {
				}

				virtual void OnCommand(ref<ToolbarItem> ti) {
					OnCommand(ti->GetCommand());
				}

				virtual void OnCommand(int c) {
					if(c==KCommandRun) {
						int len = (int)SendMessage(_editor->GetScintillaWindow(), SCI_GETLENGTH, 0,0);
						char* buffer = new char[len+2];
						SendMessage(_editor->GetScintillaWindow(), SCI_GETTEXT, len+1, (LPARAM)buffer);
						std::string source = buffer;
						delete[] buffer;

						ref<CompiledScript> script = _editor->GetScriptContext()->Compile(Wcs(source));
						if(script) {
							if(_thread) {
								_thread->WaitForCompletion();
							}
							ref<ScriptThread> st = _editor->GetScriptContext()->CreateExecutionThread(script);
							st->SetCleanupAfterRun(true);
							_thread = st;
							_thread->Start();
						}						
					}
					else if(c==KCommandUndo) {
						SendMessage(_editor->GetScintillaWindow(), SCI_UNDO, 0, 0);
					}
					else if(c==KCommandRedo) {
						SendMessage(_editor->GetScintillaWindow(), SCI_REDO, 0, 0);
					}
					else if(c==KCommandNew) {
						_editor->SetText(L"");
						_editor->SetFilePath(L"");
					}
					else if(c==KCommandOpen) {
						std::wstring fn = Dialog::AskForOpenFile(_editor, TL(editor_open_file), L"TJShow Script (*.tss)\0*.tss\0\0", L"tss");
						_editor->Open(fn);
					}
					else if(c==KCommandSave||c==KCommandSaveAs) {
						if(_editor->GetFilePath().length()<1 || c==KCommandSaveAs) {
							// decide about a filename first
							_editor->SetFilePath(Dialog::AskForSaveFile(_editor, TL(editor_save_file), L"TJShow Script (*.tss)\0*.tss\0\0", L"tss"));
						}

						DeleteFile(_editor->GetFilePath().c_str());
						int len = (int)SendMessage(_editor->GetScintillaWindow(), SCI_GETLENGTH, 0, 0);
						char* buffer = new char[len+3];
						SendMessage(_editor->GetScintillaWindow(), SCI_GETTEXT, len+1, (LPARAM)buffer);						
						HANDLE file = CreateFile(_editor->GetFilePath().c_str(), GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);
						DWORD written = 0;
						WriteFile(file, buffer, len, &written, 0);
						CloseHandle(file);
						delete[] buffer;
					}
					Repaint();	
				}

				virtual void Paint(tj::shared::graphics::Graphics& g, ref<Theme> theme) {
					ToolbarWnd::Paint(g, theme);
					Area r = GetFreeArea();
					tj::shared::graphics::StringFormat sf;
					sf.SetAlignment(tj::shared::graphics::StringAlignmentFar);
					tj::shared::graphics::SolidBrush tb(theme->GetColor(Theme::ColorActiveEnd));
					std::wstring fn = _editor->GetFileName();
					g.DrawString(fn.c_str(), int(fn.length()), theme->GetGUIFont(), r, &sf, &tb);
				}

			protected:
				ScriptEditorWnd* _editor;
				ref<Thread> _thread;
		};
	}
}

void SetAStyle(HWND scintilla, int style, COLORREF fore, COLORREF back, int size=0, const char *face=0) {
	SendMessage(scintilla, SCI_STYLESETFORE, style, fore);
	SendMessage(scintilla, SCI_STYLESETBACK, style, back);

	if (size >= 1) {
		SendMessage(scintilla, SCI_STYLESETSIZE, style, size);
	}
	if (face) {
		SendMessage(scintilla, SCI_STYLESETFONT, style, reinterpret_cast<LPARAM>(face));

	}
}

ScriptEditorWnd::ScriptEditorWnd(ref<ScriptContext> script): ChildWnd(false) {
	if(!_ScintillaLoaded) {
		HMODULE hmod = LoadLibrary(L"scintilla.dll");
		if(hmod!=NULL) {
			_ScintillaLoaded = true;
			Log::Write(L"TJShow/ScriptEditorWnd",L"Scintilla DLL loaded!");
		}
		else {
			Throw(L"Scintilla DLL could not be loaded; editor functionality not available", ExceptionTypeError);
		}
	}

	_ctx = script;
	_scintilla = CreateWindowEx(0,L"Scintilla",L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPCHILDREN,10,10,500,400,GetWindow(),0L, GetModuleHandle(NULL),NULL);
	// init color coding stuff
	UpdateCodeStyle();
	
	_toolbar = GC::Hold(new ScriptToolbarWnd(this));
	Add(_toolbar);
	Layout();
}

void ScriptEditorWnd::Open(std::wstring fn) {
	HANDLE file = CreateFile(fn.c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	int size = GetFileSize(file,0);
	CloseHandle(file);

	SetFilePath(fn);
	char* buffer = new char[size+2];
	memset(buffer, 0, sizeof(char)*(size+2));

	std::string filename = Mbs(fn);
	std::ifstream str(filename.c_str());						
	str.read(buffer, size+1);
	str.close();
	SendMessage(GetScintillaWindow(), SCI_SETTEXT, 0L, (LPARAM)buffer);
	delete[] buffer;	
}

void ScriptEditorWnd::UpdateCodeStyle() {
	strong<Theme> theme = ThemeManager::GetTheme();

	Color backColor = theme->GetColor(Theme::ColorBackground);
	Color textColor = theme->GetColor(Theme::ColorText);
	COLORREF back = RGB(backColor.GetRed(), backColor.GetGreen(), backColor.GetBlue());
	COLORREF text = RGB(textColor.GetRed(), textColor.GetGreen(), textColor.GetBlue());

	SendMessage(_scintilla, SCI_SETLEXER, SCLEX_CPP, SCLEX_CPP);
	SendMessage(_scintilla, SCI_SETSTYLEBITS, 7, 7);
	SendMessage(_scintilla, SCI_SETKEYWORDS, 0, (LPARAM)KScriptKeywords);
	
	// colors
	SetAStyle(_scintilla, STYLE_DEFAULT, text, back, 8, "Verdana");
	SendMessage(_scintilla, SCI_STYLECLEARALL,0,0);	// Copies global style to all others
	SetAStyle(_scintilla, SCE_C_STRING, RGB(190,0,0), back, 0, 0);
	SetAStyle(_scintilla, SCE_C_CHARACTER, RGB(190,0,0), back, 0, 0);
	SetAStyle(_scintilla, SCE_C_NUMBER, RGB(190,0,0), back, 0, 0);
	SetAStyle(_scintilla, SCE_C_COMMENT, RGB(0,100,0), back, 0, 0);
	SetAStyle(_scintilla, SCE_C_COMMENTLINE, RGB(0,100,0), back, 0, 0);
	SetAStyle(_scintilla, SCE_C_WORD, RGB(0,0,190), back, 0,0);
}

std::wstring ScriptEditorWnd::GetTabTitle() const {
	return _filename;
}

HWND ScriptEditorWnd::GetScintillaWindow() {
	return _scintilla;
}

std::wstring ScriptEditorWnd::GetFilePath() const {
	return _filepath;
}

std::wstring ScriptEditorWnd::GetFileName() const {
	return _filename;
}

ref<ScriptContext> ScriptEditorWnd::GetScriptContext() {
	return _ctx;
}

void ScriptEditorWnd::SetFilePath(std::wstring x) {
	_filepath = x;
	wchar_t* path = _wcsdup(x.c_str());
	_filename = std::wstring(PathFindFileName(path));
	delete[] path;

	// this changes our tab title so update parent
	Wnd* parent = GetParent();
	if(parent!=0) parent->Update();
}

ScriptEditorWnd::~ScriptEditorWnd() {
}

void ScriptEditorWnd::Layout() {
	Area rect = GetClientArea();
	_toolbar->Fill(LayoutTop, rect);
	
	// Manually do the DPI stuff
	strong<Theme> theme = ThemeManager::GetTheme();
	rect.Multiply(theme->GetDPIScaleFactor());
	SetWindowPos(_scintilla, 0L, rect.GetLeft(), rect.GetTop(), rect.GetWidth(), rect.GetHeight(), SWP_NOZORDER);
}

LRESULT ScriptEditorWnd::Message(UINT msg, WPARAM wp, LPARAM lp) {
	if(msg==WM_ERASEBKGND) {
		UpdateCodeStyle();
	}
	return ChildWnd::Message(msg,wp,lp);
}

void ScriptEditorWnd::SetText(const std::wstring& txt) {
	std::string mbs = Mbs(txt);
	SendMessage(_scintilla, SCI_SETTEXT, 0L, (LPARAM)mbs.c_str());
}

void ScriptEditorWnd::Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme) {
}

void ScriptEditorWnd::OnSize(const Area& newSize) {
	Layout();
}