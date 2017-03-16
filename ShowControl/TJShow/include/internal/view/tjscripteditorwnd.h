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
#ifndef _TJSCRIPTEDITORWND_H
#define _TJSCRIPTEDITORWND_H

namespace tj {
	namespace show {
		namespace view {
			class ScriptEditorWnd: public ChildWnd {
				public:
					ScriptEditorWnd(ref<ScriptContext> script);
					virtual ~ScriptEditorWnd();
					virtual void Layout();
					virtual LRESULT Message(UINT msg, WPARAM wp, LPARAM lp);
					virtual void Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme);
					virtual void SetText(const std::wstring& text);
					virtual std::wstring GetTabTitle() const;
					virtual void SetFilePath(std::wstring path);
					virtual std::wstring GetFilePath() const;
					virtual HWND GetScintillaWindow();
					virtual ref<ScriptContext> GetScriptContext();
					virtual std::wstring GetFileName() const;
					virtual void Open(std::wstring path);
					
				protected:
					virtual void OnSize(const Area& newSize);
					void UpdateCodeStyle();

					HWND _scintilla;
					ref<ScriptContext> _ctx;
					ref<ToolbarWnd> _toolbar;
					std::wstring _filepath;
					std::wstring _filename;
			};
		}
	}
}

#endif