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
#include "../include/tjdataplugin.h"
#include <algorithm>
#include <fstream>

class DataLiveWnd;

class DataExportParameters: public Inspectable {
	public:
		DataExportParameters(strong<DataLiveWnd> dlw);
		virtual ref<PropertySet> GetProperties();

		ResourceIdentifier _file;
		std::wstring _fieldSeparator;
		std::wstring _lineSeparator;
		std::wstring _fieldBegin;
		std::wstring _fieldEnd;
		bool _includeColumnNames;
		ref<DataTrack> _track;
};

class DataLiveListWnd: public ListWnd {
	friend class DataLiveWnd;

	public:
		DataLiveListWnd(ref<DataLiveWnd> dlw);
		virtual ~DataLiveListWnd();
		virtual void SetQuery(ref<Query> q);
		virtual int GetItemCount();
		virtual void PaintItem(int id, graphics::Graphics& g, Area& row, const ColumnInfo& ci);
		virtual void OnClickItem(int id, int col, Pixels x, Pixels y);
		virtual void Clear();

	protected:
		weak<DataLiveWnd> _dlw;
		std::vector< ref<Tuple> > _data;
};

class DataLiveWnd: public ChildWnd {
	friend class DataLiveToolbarWnd;

	public:
		DataLiveWnd(ref<DataTrack> track);
		virtual ~DataLiveWnd();
		virtual void Paint(graphics::Graphics& g, strong<Theme> theme);
		virtual void Layout();
		virtual void OnSize(const Area& ns);
		virtual void OnCreated();
		virtual void SetQuery(const std::wstring& q);
		virtual void SetDatabase(const std::wstring& path);
		virtual ref<DataQueryHistory> GetHistory();
		virtual ref<DataTrack> GetTrack();
		virtual void DoExport(strong<DataExportParameters> dp);

	protected:	
		unsigned int _currentColumnCount;
		ref<Query> _query;
		ref<ToolbarWnd> _tools;
		ref<DataLiveListWnd> _list;
		weak<DataTrack> _track;
};

class DataLiveToolbarWnd: public SearchToolbarWnd {
	public:
		DataLiveToolbarWnd(ref<DataLiveWnd> dlw);
		virtual ~DataLiveToolbarWnd();
		virtual void OnCommand(ref<ToolbarItem> ti);
		virtual void OnSize(const Area& ns);
		virtual void OnSearchChange(const std::wstring& q);

	protected:
		virtual void DoExport();

		enum {
			KCQuery = 1,
			KCMenu = 2,
			KCHistory = 3,
		};

		weak<DataLiveWnd> _dlw;
		ref<ThrobberToolbarItem> _tti;
		std::wstring _query;
};

/** DataExportParameters **/
DataExportParameters::DataExportParameters(strong<DataLiveWnd> dlw): _fieldSeparator(L","), _lineSeparator(L"\\n"), _fieldBegin(L"\""), _fieldEnd(L"\""), _includeColumnNames(false) {
	_track = dlw->GetTrack();
}

ref<PropertySet> DataExportParameters::GetProperties() {
	ref<PropertySet> ps = GC::Hold(new PropertySet());
	if(_track) {
		strong<ResourceProvider> rmg = _track->GetPlayback()->GetResources();
		ps->Add(GC::Hold(new FileProperty(TL(data_export_file), this, &_file, rmg)));
		ps->Add(GC::Hold(new PropertySeparator(TL(data_export_format))));
		ps->Add(GC::Hold(new GenericProperty<std::wstring>(TL(data_export_field_separator), this, &_fieldSeparator, _fieldSeparator)));
		ps->Add(GC::Hold(new GenericProperty<std::wstring>(TL(data_export_line_separator), this, &_lineSeparator, _lineSeparator)));
		ps->Add(GC::Hold(new GenericProperty<std::wstring>(TL(data_export_field_begin), this, &_fieldBegin, _fieldBegin)));
		ps->Add(GC::Hold(new GenericProperty<std::wstring>(TL(data_export_field_end), this, &_fieldEnd, _fieldEnd)));
		ps->Add(GC::Hold(new GenericProperty<bool>(TL(data_export_column_names), this, &_includeColumnNames, _includeColumnNames)));
	}

	return ps;
}

/** DataLiveToolbarWnd **/
DataLiveToolbarWnd::DataLiveToolbarWnd(ref<DataLiveWnd> dlw): _dlw(dlw) {
	SetSearchBoxHint(TL(data_query_hint));
	_tti = GC::Hold(new ThrobberToolbarItem());
	Add(GC::Hold(new ToolbarItem(KCQuery, L"icons/db/execute.png", TL(data_query_execute), true)), true);
	Add(GC::Hold(new ToolbarItem(KCMenu, L"icons/db/database.png", TL(data_options), false)), false);
	Add(GC::Hold(new ToolbarItem(KCHistory, L"icons/db/history.png", TL(data_query_history), false)), false);
	Add(_tti, true);
}

DataLiveToolbarWnd::~DataLiveToolbarWnd() {
}

void DataLiveToolbarWnd::OnSearchChange(const std::wstring& q) {
	_query = q;
}

void DataLiveToolbarWnd::DoExport() {
	ref<DataLiveWnd> dlw = _dlw;
	if(dlw) {
		ref<DataExportParameters> data = GC::Hold(new DataExportParameters(dlw));
		ref<PropertyDialogWnd> dw = GC::Hold(new PropertyDialogWnd(TL(data_export), TL(data_export_question)));
		dw->GetPropertyGrid()->Inspect(data);

		if(dw->DoModal(this)) {
			dlw->DoExport(data);
		}
	}
}

void DataLiveToolbarWnd::OnCommand(ref<ToolbarItem> ti) {
	if(ti->GetCommand()==KCQuery) {
		ref<DataLiveWnd> dlw = _dlw;
		if(dlw) {
			dlw->SetQuery(_query);
		}
	}
	else if(ti->GetCommand()==KCMenu) {
		ContextMenu cm;

		ref<DataLiveWnd> dlw = _dlw;
		if(dlw) {
			enum { KCOpen = 1, KCCreate = 2, KCSaveAs = 3, KCExport = 4 };
			cm.AddItem(GC::Hold(new MenuItem(TL(data_open_database), KCOpen, false, MenuItem::NotChecked, L"icons/db/load.png")));
			cm.AddItem(GC::Hold(new MenuItem(TL(data_create_database), KCCreate, false, MenuItem::NotChecked, L"icons/db/create.png")));
			cm.AddSeparator();
			cm.AddItem(GC::Hold(new MenuItem(TL(data_export_database), dlw->_query ? KCExport : -1, false, MenuItem::NotChecked, L"icons/db/export.png")));
			///cm.AddItem(GC::Hold(new MenuItem(TL(data_save_database), KCSaveAs, false, MenuItem::NotChecked, L"icons/db/save.png")));

			Area rc = GetClientArea();
		
			int r = cm.DoContextMenu(this, GetButtonX(KCMenu), rc.GetBottom());
			if(r==KCOpen || r==KCCreate) {
				std::wstring path = Dialog::AskForOpenFile(this, TL(data_choose_database), L"*.db (Database)\0*.db\0\0", L"db");
				if(path.length()>0) {
					dlw->SetDatabase(path);
				}
			}
			else if(r==KCExport) {
				DoExport();
			}
		}
	}
	else if(ti->GetCommand()==KCHistory) {
		ref<DataLiveWnd> dlw = _dlw;
		if(dlw) {
			ref<DataQueryHistory> dh = dlw->GetHistory();
			if(dh) {
				std::deque<std::wstring> commands = dh->_history;
				ContextMenu cm;

				std::deque<std::wstring>::const_iterator it = commands.begin();
				int n = 0;
				while(it!=commands.end()) {
					const std::wstring& q = *it;
					if(q.length()>0) {
						cm.AddItem(q, n, false, false);
					}
					++n;
					++it;
				}

				Area rc = GetClientArea();
				int r = cm.DoContextMenu(this, GetButtonX(KCHistory), rc.GetBottom());
				if(r>=0 && r<int(commands.size())) {
					std::wstring query = commands.at(r);
					SetSearchBoxText(query);
				}
			}
		}
	}
}

void DataLiveToolbarWnd::OnSize(const Area& ns) {
	Area sh = GetSearchBoxArea();
	SetSearchBoxSize(ns.GetWidth()-ns.GetHeight()*5, sh.GetHeight());
	SearchToolbarWnd::OnSize(ns);
}

/** DataLiveWnd **/
DataLiveWnd::DataLiveWnd(ref<DataTrack> track): ChildWnd(false), _track(track), _currentColumnCount(0) {
}

DataLiveWnd::~DataLiveWnd() {
}

ref<DataTrack> DataLiveWnd::GetTrack() {
	return _track;
}

void EscapeExportParameter(std::wstring& p) {
	ReplaceAll<std::wstring>(p, L"\\r", L"\r");
	ReplaceAll<std::wstring>(p, L"\\n", L"\n");
	ReplaceAll<std::wstring>(p, L"\\t", L"\t");
}

void DataLiveWnd::DoExport(strong<DataExportParameters> dp) {
	std::vector< ref<Tuple> >& data = _list->_data;
	
	// Replace escaped characters in parameters
	EscapeExportParameter(dp->_fieldBegin);
	EscapeExportParameter(dp->_fieldEnd);
	EscapeExportParameter(dp->_fieldSeparator);
	EscapeExportParameter(dp->_lineSeparator);

	ref<DataTrack> dt = GetTrack();
	if(dt) {
		ref<ResourceProvider> rp = dt->GetPlayback()->GetResources();
		std::wstring path;
		if(rp->GetPathToLocalResource(dp->_file, path)) {
			std::wofstream fileOut(path.c_str());

			// Export column names
			unsigned int colCount = _currentColumnCount;
			Log::Write(L"TJData/Export", L"colCount="+Stringify(colCount));
			for(unsigned int c=0;c<colCount;c++) {
				bool isLast = (c==(colCount-1));
				fileOut << dp->_fieldBegin << _query->GetColumnName(c) << dp->_fieldEnd;
				if(!isLast) {
					fileOut << dp->_fieldSeparator;	
				}
			}
			fileOut << dp->_lineSeparator;

			// Export data
			std::vector< ref<Tuple> >::const_iterator it = data.begin();
			while(it!=data.end()) {
				ref<Tuple> row = *it;
				if(row) {
					for(unsigned int a=0;a<colCount;a++) {
						bool isLast = (a==(colCount-1));
						fileOut << dp->_fieldBegin << row->Get(a).ToString() << dp->_fieldEnd;
						if(!isLast) {
							fileOut << dp->_fieldSeparator;	
						}
					}
					fileOut << dp->_lineSeparator;
				}
				++it;
			}

			fileOut.flush();
			fileOut.close();
		}
		else {
			Throw(L"Could not export to file, file not found or access denied!", ExceptionTypeError);
		}
	}
}

void DataLiveWnd::OnCreated() {
	_tools = GC::Hold(new DataLiveToolbarWnd(this));
	Add(_tools);

	_list = GC::Hold(new DataLiveListWnd(this));
	Add(_list);
	Layout();
}

void DataLiveWnd::Paint(graphics::Graphics& g, strong<Theme> theme) {
}

void DataLiveWnd::SetQuery(const std::wstring& qs) {
	ref<DataTrack> dt = _track;
	if(dt) {
		ref<Database> db = dt->GetDatabase();
		if(db) {
			ref<Query> q = db->CreateQuery(qs);
			if(q) {
				dt->GetHistory()->AddQuery(qs);
				q->Execute();
				_query = q;
				_currentColumnCount = q->GetColumnCount();
				_list->SetQuery(q);
			}
		}
	}
}

ref<DataQueryHistory> DataLiveWnd::GetHistory() {
	ref<DataTrack> dt = _track;
	if(dt) {
		return dt->GetHistory();
	}
	return null;
}

void DataLiveWnd::SetDatabase(const std::wstring& path) {
	ref<DataTrack> dt = _track;
	if(dt) {
		dt->SetDatabase(path);
		_list->Clear();
	}
}

void DataLiveWnd::Layout() {
	Area rc = GetClientArea();

	if(_tools) {
		_tools->Fill(LayoutTop, rc);
	}

	if(_list) {
		_list->Fill(LayoutFill, rc);
	}
}

void DataLiveWnd::OnSize(const Area& ns) {
	Layout();
}

/** DataLiveControl **/
DataLiveControl::DataLiveControl(ref<DataTrack> track, ref<Stream> str): _track(track) {
}

DataLiveControl::~DataLiveControl() {
}

ref<tj::shared::Wnd> DataLiveControl::GetWindow() {
	if(!_wnd) {
		return GC::Hold(new DataLiveWnd(_track));	
	}

	return _wnd;
}

void DataLiveControl::Update() {
}

std::wstring DataLiveControl::GetGroupName() {
	return TL(data_live_group);
}

bool DataLiveControl::IsSeparateTab() {
	return true;
}

int DataLiveControl::GetWidth() {
	return -1;
}

void DataLiveControl::SetPropertyGrid(ref<PropertyGridProxy> pg) {
}

/** DataLiveListWnd **/
DataLiveListWnd::DataLiveListWnd(ref<DataLiveWnd> dlw): _dlw(dlw) {
}

DataLiveListWnd::~DataLiveListWnd() {
}

void DataLiveListWnd::Clear() {
	_data.clear();
	ListWnd::_cols.clear();
	ListWnd::OnSize(GetClientArea());
}

void DataLiveListWnd::SetQuery(ref<Query> q) {
	_data.clear();
	ListWnd::_cols.clear();

	if(q) {
		unsigned int colCount = q->GetColumnCount();
			
		if(colCount>0) {
			float cw = 1.0f / colCount;
			for(unsigned int a=0;a<colCount;a++) {
				AddColumn(q->GetColumnName(a), int(a), cw);
			}

			while(q->HasRow()) {
				ref<Tuple> current = GC::Hold(new Tuple(colCount));
				for(unsigned int a=0;a<colCount;a++) {
					current->Set(a,q->GetAny(a));
				}
				_data.push_back(current);
				q->Next();
			}
		}
	}
	ListWnd::OnSize(GetClientArea());
}

int DataLiveListWnd::GetItemCount() {
	return (int)_data.size();
}

void DataLiveListWnd::PaintItem(int id, graphics::Graphics& g, Area& row, const ColumnInfo& ci) {
	ref<Tuple> tpl = _data.at(id);
	if(tpl) {
		StringFormat sf;
		sf.SetAlignment(StringAlignmentNear);
		sf.SetTrimming(StringTrimmingEllipsisCharacter);

		strong<Theme> theme = ThemeManager::GetTheme();
		SolidBrush br(theme->GetColor(Theme::ColorText));
		Font* fnt = theme->GetGUIFont();

		for(unsigned int a=0;a<tpl->GetLength();a++) {
			DrawCellText(g, &sf, &br, fnt, int(a), row, tpl->Get(a).ToString());
		}
	}
}

void DataLiveListWnd::OnClickItem(int id, int col, Pixels x, Pixels y) {
	SetSelectedRow(id);
}