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
#include "../../include/internal/view/tjpluginsettingswnd.h"

using namespace tj::show;
using namespace tj::show::view;
using namespace tj::shared::graphics;

PluginSettingsWnd::PluginSettingsWnd(ref<PluginManager> mgr) {
	assert(mgr);
	_mgr = mgr;
	AddColumn(TL(plugin_name), KColName);
	AddColumn(TL(plugin_author), KColAuthor);
	AddColumn(TL(plugin_version), KColVersion);
	AddColumn(TL(plugin_module), KColModule);
	AddColumn(TL(plugin_hash), KColID);
}

PluginSettingsWnd::~PluginSettingsWnd() {
}

int PluginSettingsWnd::GetItemCount() {
	return int(_mgr->_pluginsByHash.size());
}

void PluginSettingsWnd::PaintItem(int id, tj::shared::graphics::Graphics& g, Area& row, const ColumnInfo& ci) {
	std::map<PluginHash, ref<PluginWrapper> >::const_iterator it = _mgr->_pluginsByHash.begin();
	while((id--)!=0) {
		++it;
	}

	ref<PluginWrapper> pw = it->second;

	if(pw) {
		StringFormat sf;
		sf.SetAlignment(StringAlignmentNear);
		sf.SetLineAlignment(StringAlignmentCenter);
		sf.SetTrimming(StringTrimmingEllipsisPath);
		strong<Theme> theme = ThemeManager::GetTheme();
		SolidBrush text(theme->GetColor(Theme::ColorText));

		// name
		DrawCellText(g, &sf, &text, theme->GetGUIFontBold(), KColName, row, pw->GetFriendlyName());
		
		// module
		HMODULE module = pw->GetModule();
		wchar_t buffer[MAX_PATH+2];
		GetModuleFileName(module, buffer, MAX_PATH+1);
		std::wstring path = buffer;
		DrawCellText(g, &sf, &text, theme->GetGUIFont(), KColModule, row, path);
		
		// author
		std::wstring author = pw->GetPlugin()->GetAuthor();
		DrawCellText(g,&sf,&text,theme->GetGUIFont(), KColAuthor, row, author);
		
		// version
		std::wstring version = pw->GetPlugin()->GetVersion();
		DrawCellText(g, &sf, &text, theme->GetGUIFont(), KColVersion, row, version);
		
		// hash
		std::wstring hash = Stringify(pw->GetHash());
		DrawCellText(g, &sf, &text, theme->GetGUIFontBold(), KColID, row, hash);
	}
}

void PluginSettingsWnd::OnClickItem(int id, int col, Pixels x, Pixels y) {
	SetSelectedRow(id);
}

void PluginSettingsWnd::OnColumnSizeChanged() {
}