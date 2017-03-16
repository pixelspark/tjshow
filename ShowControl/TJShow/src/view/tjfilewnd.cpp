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
#include "../../include/internal/tjnetwork.h"

#include "../../include/internal/view/tjfilewnd.h"
#include "../../include/internal/view/tjscripteditorwnd.h"
using namespace tj::shared::graphics;
using namespace tj::show;
using namespace tj::show::view;

namespace tj {
	namespace show {
		namespace view {
			class FileListWnd: public ListWnd {
				public:
					FileListWnd(ref<Model> model);
					virtual ~FileListWnd();

					// ListWnd
					virtual int GetItemCount();
					virtual void PaintItem(int id, tj::shared::graphics::Graphics& g, Area& row, const ColumnInfo& ci);
					virtual void OnClickItem(int id, int col, Pixels x, Pixels y);
					virtual void OnRightClickItem(int id, int col);
					virtual void OnDoubleClickItem(int id, int col);

				protected:
					ref<Model> _model;
					Icon _scriptFileIcon, _otherFileIcon;

					enum {
						KColPath=1,
						KColSize,
					};
			};

			class FileToolbarWnd: public ToolbarWnd {
				public:
					FileToolbarWnd(FileWnd* parent) {
						_fw = parent;
						Add(GC::Hold(new ToolbarItem(KCAddNew, L"icons/add.png", TL(resource_add_new), false)));
						Add(GC::Hold(new ToolbarItem(KCAdd, L"icons/add_file.png", TL(resource_add_file), false)));
						Add(GC::Hold(new ToolbarItem(KCRemove, L"icons/remove.png", TL(resource_remove_file), false)));
						Add(GC::Hold(new ToolbarItem(KCSearch, Icons::GetIconPath(Icons::IconSearch), TL(resource_search), false)), true);
					}

					virtual ~FileToolbarWnd() {
					}

					virtual void OnCommand(ref<ToolbarItem> ti) {
						OnCommand(ti->GetCommand());
					}

					virtual void OnCommand(int c) {
						if(c==KCAdd) {
							DoAdd();
						}
						else if(c==KCAddNew) {
							DoAddNew();
						}
						else if(c==KCSearch) {
							DoSearch();
						}
						else if(c==KCRemove) {
							ref<Resources> resc = _fw->_model->GetResources();
							if(resc) {
								int index = _fw->_list->GetSelectedRow();
								if(index>=0) {
									ResourceIdentifier res = resc->GetResourceAt(index);
									_fw->_list->SetSelectedRow(-1);
									resc->Remove(res);
								}
							}

							GetParent()->Update();
						}
					}

				protected:
					void DoAddNew() {
						if(!_fw->_model->IsSaved()) {
							Throw(TL(resource_model_not_saved), ExceptionTypeError);
						}

						ContextMenu cm;
						enum {KTScript=1,};
						cm.AddItem(TL(resource_script), KTScript);

						int r = cm.DoContextMenu(this);

						ref<Resources> resc = _fw->_model->GetResources();

						if(r==KTScript) {
							std::wstring file = Dialog::AskForSaveFile(_fw, TL(resource_select), L"Files (*.*)\0*.*\0\0", L"tjs");
							std::wstring showfile = _fw->_model->GetFileName();
							if(showfile.length()>0) {
								wchar_t* path = new wchar_t[MAX_PATH+2];
								if(PathRelativePathTo(path,showfile.c_str(), FILE_ATTRIBUTE_NORMAL, file.c_str(), FILE_ATTRIBUTE_NORMAL)==TRUE) {
									file = path;
								}
							}

							if(file.length()>0) {
								ref<ResourceProvider> showResources = Application::Instance()->GetModel()->GetResourceManager();
								ResourceIdentifier relative = showResources->GetRelative(file);
								resc->Add(relative);
								_fw->OnSize(_fw->GetClientArea()); // update scrollbar stuff
								_fw->_list->Repaint();
							
							
								// Open a script editor for this one
								ref<View> view = Application::Instance()->GetView();
								if(view) {
									view->OpenScriptEditor(file, Application::Instance()->GetScriptContext());
								}
							}
						}
					}

					void DoAdd() {
						if(!_fw->_model->IsSaved()) {
							Throw(TL(resource_model_not_saved), ExceptionTypeError);
						}

						ref<Resources> resc = _fw->_model->GetResources();
						std::wstring file = Dialog::AskForOpenFile(_fw, TL(resource_select), L"Files (*.*)\0*.*\0\0", L"");

						if(file.length()>0) {
							ref<ResourceProvider> rp = Application::Instance()->GetModel()->GetResourceManager();
							ResourceIdentifier rid = rp->GetRelative(file);
							resc->Add(rid);
							_fw->OnSize(_fw->GetClientArea()); // update scrollbar stuff
							_fw->_list->Repaint();
						}
					}

					void DoSearch() {
						ref<FileSearchWnd> fsw = GC::Hold(new FileSearchWnd(Application::Instance()->GetNetwork()));
						Application::Instance()->GetView()->AddUtilityWindow(fsw, TL(resource_search_title), L"resource-search-wnd", L"icons/tabs/files.png");
					}

					FileWnd* _fw;
					enum {
						KCAdd=1,
						KCAddNew,
						KCRemove,
						KCSearch,
					};
			};
		}
	}
}

FileWnd::FileWnd(ref<Model> model): ChildWnd(false) {
	_model = model;
	_list = GC::Hold(new FileListWnd(model));
	_tools = GC::Hold(new FileToolbarWnd(this));
	Add(_list);
	Add(_tools);

	Layout();
	SetDropTarget(true);
	_list->SetEmptyText(TL(resource_list_empty));
}

FileWnd::~FileWnd() {
}

void FileWnd::OnDropFiles(const std::vector<std::wstring>& files) {
	ref<Resources> res = _model->GetResources();
	strong<ResourceManager> rmg = Application::Instance()->GetModel()->GetResourceManager();

	std::vector<std::wstring>::const_iterator it = files.begin();
	while(it!=files.end()) {
		ResourceIdentifier& rid = rmg->GetRelative(*it);
		res->Add(rid);
		++it;
	}

	Update();
}

void FileWnd::OnSize(const tj::shared::Area &ns) {
	Layout();
}

void FileWnd::Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme) {
}

void FileWnd::OnSettingsChanged() {
	_list->SetSettings(GetSettings()->GetNamespace(L"list"));
}

void FileWnd::Layout() {
	Area area = GetClientArea();
	_tools->Fill(LayoutTop, area);
	_list->Fill(LayoutFill, area);
}

/** FileListWnd **/
FileListWnd::FileListWnd(ref<Model> model): _scriptFileIcon(L"icons/toolbar/editor.png"), _otherFileIcon(L"icons/file.png") {
	_model = model;
	AddColumn(TL(resource_path), KColPath, 0.8f);
	AddColumn(TL(resource_size), KColSize, 0.2f);
}

FileListWnd::~FileListWnd() {
}

int FileListWnd::GetItemCount() {
	ref<Resources> res = _model->GetResources();
	if(res) {
		return res->GetResourceCount();
	}
	return 0;
}

void FileListWnd::PaintItem(int id, tj::shared::graphics::Graphics& g, Area& row, const ColumnInfo& ci) {
	strong<Theme> theme = ThemeManager::GetTheme();
	ref<Resources> res = _model->GetResources();
	if(res) {
		ResourceIdentifier rid = res->GetResourceAt(id);

		if(rid!=L"") {
			StringFormat sf;
			sf.SetLineAlignment(StringAlignmentCenter);
			sf.SetAlignment(StringAlignmentNear);
			sf.SetTrimming(StringTrimmingEllipsisPath);
			SolidBrush textBr(theme->GetColor(Theme::ColorText));

			Area pathRect(row);
			pathRect.SetWidth(int(GetColumnWidth(KColPath)*row.GetWidth()));
			pathRect.SetX(int(GetColumnX(KColPath)*row.GetWidth()));
			pathRect.Narrow(24,4,0,4);

			// draw file icon
			g.DrawImage(_otherFileIcon, RectF(GetColumnX(KColPath)*row.GetWidth()+4.0f, float(row.GetTop())+2.0f, 16.0f, 16.0f));
			g.DrawString(rid.c_str(), (int)rid.length(), theme->GetGUIFont(), pathRect, &sf, &textBr);

			// Draw size
			std::wstring localPath;
			if(_model->GetResourceManager()->GetPathToLocalResource(rid, localPath)) {
				Bytes fileSize = File::GetFileSize(localPath);
				DrawCellText(g, &sf, &textBr, theme->GetGUIFont(), KColSize, row, Util::GetSizeString(fileSize));
			}
		}
	}
}

void FileListWnd::OnClickItem(int id, int col, Pixels x, Pixels y) {
	SetSelectedRow(id);
}

void FileListWnd::OnRightClickItem(int id, int col) {
	ref<Resources> resc = _model->GetResources();

	if(resc->GetResourceCount()<=id) {
		return;
	}

	enum {
		KRemove=1,
		KOpen,
		KOpenFolder,
		KRun,
		KOpenBrowser,
	};
	
	ContextMenu m;
	m.AddItem(TL(resource_open), KOpen, true);
	m.AddItem(TL(resource_open_in_browser), KOpenBrowser);
	m.AddItem(TL(resource_run), KRun);
	m.AddItem(TL(resource_open_folder), KOpenFolder);
	m.AddItem(TL(resource_remove), KRemove);
	
	Area rc = GetClientArea();
	int c = m.DoContextMenu(this, Pixels(GetColumnX(col)*rc.GetWidth()), GetRowArea(id).GetBottom());

	if(c==KRemove) {
		resc->Remove(resc->GetResourceAt(id));
		OnSize(GetClientArea()); // update scrollbars
		Repaint();
	}
	else if(c==KOpen) {
		ResourceIdentifier res = resc->GetResourceAt(id);
		ref<Resource> resource = Application::Instance()->GetModel()->GetResourceManager()->GetResource(res);
		if(resource && resource.IsCastableTo<LocalFileResource>()) {
			ref<LocalFileResource>(resource)->Open();
		}
	}
	else if(c==KOpenBrowser) {
		// TODO implement
	}
	else if(c==KOpenFolder) {
		ResourceIdentifier rid = resc->GetResourceAt(id);
		ref<Resource> res = Application::Instance()->GetModel()->GetResourceManager()->GetResource(rid);

		if(res && res.IsCastableTo<LocalFileResource>()) {
			ref<LocalFileResource>(res)->OpenFolder();
		}
	}
	else if(c==KRun) {
		// TODO implement
	}
}

void FileListWnd::OnDoubleClickItem(int id, int col) {
	ref<Resources> resc = _model->GetResources();

	if(resc->GetResourceCount()<=id) {
		return;
	}
}

namespace tj {
	namespace show {
		namespace view {
			class FileSearchListWnd: public ListWnd, public Listener<network::FindResourceTransaction::ResourceFoundNotification> {
				public:
					FileSearchListWnd(strong<Network> net);
					virtual ~FileSearchListWnd();
					virtual int GetItemCount();
					virtual void PaintItem(int id, graphics::Graphics& g, tj::shared::Area& area, const ColumnInfo& ci);
					virtual void SetTransaction(ref<network::FindResourceTransaction> frs, const std::wstring& rid);
					virtual void Notify(ref<Object> src, const network::FindResourceTransaction::ResourceFoundNotification& rfn);
					virtual void OnClickItem(int id, int col, Pixels x, Pixels y);

				protected:
					enum {
						KColLocation = 1,
					};

					ref<network::FindResourceTransaction> _frs;
					ref<Network> _net;
					std::wstring _rid;
			};

			class FileSearchToolbarWnd: public SearchToolbarWnd {
				public:
					FileSearchToolbarWnd(ref<FileSearchListWnd> flsw, ref<Network> net);
					virtual ~FileSearchToolbarWnd();
					virtual void OnCommand(ref<ToolbarItem> ti);
					virtual void OnSearchChange(const std::wstring& q);
					virtual void OnTimer(unsigned int id);

				protected:
					ref<FileSearchListWnd> _flist;
					ref<Network> _net;
					ref<ThrobberToolbarItem> _throbber;
			};
		}
	}
}

/** FileSearchToolbarWnd **/
FileSearchToolbarWnd::FileSearchToolbarWnd(ref<FileSearchListWnd> flsw, ref<Network> net): _flist(flsw), _net(net) {
	_throbber = GC::Hold(new ThrobberToolbarItem());
	Add(_throbber, true);
	Area sb = GetSearchBoxArea();
	SetSearchBoxSize(200, sb.GetHeight());
}

FileSearchToolbarWnd::~FileSearchToolbarWnd() {
}

void FileSearchToolbarWnd::OnCommand(ref<ToolbarItem> ti) {
}

void FileSearchToolbarWnd::OnTimer(unsigned int id) {
	if(_throbber && _throbber->Progress.IsAnimating()) {
		Repaint();
	}
	else {
		StopTimer(1);
	}
}

void FileSearchToolbarWnd::OnSearchChange(const std::wstring &q) {
	if(q.length()<1) {
		_throbber->Progress.Stop();
		_flist->SetTransaction(null, L"");
	}
	else {
		ref<network::FindResourceTransaction> frs = _net->SendFindResource(q);
		_throbber->Progress.Start(frs->GetTimeOut(), false, Animation::EaseQuadratic);
		StartTimer(Time(25), 1);
		_flist->SetTransaction(frs, q);
	}
	_flist->Repaint();
}

/** FileSearchListWnd **/
FileSearchListWnd::FileSearchListWnd(strong<Network> net): _net(net) {
	AddColumn(TL(resource_location), KColLocation, 1.0f-0.619f, true);
	SetEmptyText(TL(resource_search_hint));
}

FileSearchListWnd::~FileSearchListWnd() {
}

int FileSearchListWnd::GetItemCount() {
	return _frs ? int(_frs->_foundOn.size()) : 0;
}

void FileSearchListWnd::PaintItem(int id, graphics::Graphics& g, tj::shared::Area& area, const ColumnInfo& ci) {
	StringFormat sf;
	sf.SetAlignment(StringAlignmentNear);
	sf.SetTrimming(StringTrimmingEllipsisCharacter);

	strong<Theme> theme = ThemeManager::GetTheme();

	if(_frs) {
		InstanceID clid = _frs->_foundOn.at(id);
		ref<Client> client = _net->GetClientByInstanceID(clid);
		if(client) {
			SolidBrush tbr(theme->GetColor(Theme::ColorText));
			std::wstring str = client->GetHostName() + L" (" + StringifyHex(clid)+L")";
			DrawCellText(g, &sf, &tbr, theme->GetGUIFont(), KColLocation, area, str);
		}
		else if(clid==_net->GetInstanceID()) {
			// Local file
			ref<ResourceProvider> showResources = Application::Instance()->GetModel()->GetResourceManager();
			std::wstring resolved;
			if(showResources->GetPathToLocalResource(_rid,resolved)) {
				SolidBrush tbr(theme->GetColor(Theme::ColorText));
				DrawCellText(g, &sf, &tbr, theme->GetGUIFontBold(), KColLocation, area, resolved);
			}
		}
	}
}

void FileSearchListWnd::Notify(ref<Object> src, const network::FindResourceTransaction::ResourceFoundNotification& rfn) {
	Update();
	Repaint();
}

void FileSearchListWnd::OnClickItem(int id, int col, Pixels x, Pixels y) {
	SetSelectedRow(id);
}

void FileSearchListWnd::SetTransaction(ref<network::FindResourceTransaction> frs, const std::wstring& rid) {
	_frs = frs;
	_rid = rid;
	if(_frs) {
		_frs->EventResourceFound.AddListener(this);
	}
	Update();
}

/** FileSearchWnd **/
FileSearchWnd::FileSearchWnd(strong<Network> net): ChildWnd(false) {
	_list = GC::Hold(new FileSearchListWnd(net));
	_toolbar = GC::Hold(new FileSearchToolbarWnd(_list, net));
	Add(_toolbar);
	Add(_list);
}

FileSearchWnd::~FileSearchWnd() {
}

void FileSearchWnd::Layout() {
	Area rc = GetClientArea();
	if(_toolbar) {
		_toolbar->Fill(LayoutTop, rc);
	}

	if(_list) {
		_list->Fill(LayoutFill, rc);
	}
}

void FileSearchWnd::Paint(graphics::Graphics& g, strong<Theme> theme) {
}

void FileSearchWnd::OnSize(const Area& ns) {
	Layout();
}

void FileSearchWnd::OnSettingsChanged() {
	ref<Settings> st = GetSettings();
	_list->SetSettings(st->GetNamespace(L"list"));
	_toolbar->SetSettings(st->GetNamespace(L"toolbar"));
}