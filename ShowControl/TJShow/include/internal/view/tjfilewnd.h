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
#ifndef _TJFILEWND_H
#define _TJFILEWND_H

namespace tj {
	namespace show {
		namespace view {
			class FileListWnd;

			class FileWnd: public ChildWnd {
				friend class FileToolbarWnd;

				public:
					FileWnd(ref<Model> model);
					virtual ~FileWnd();
					virtual void Layout();
					virtual void Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme);

				protected:
					virtual void OnSize(const Area& ns);
					virtual void OnDropFiles(const std::vector<std::wstring>& files);
					virtual void OnSettingsChanged();

					ref<FileListWnd> _list;
					ref<ToolbarWnd> _tools;
					ref<Model> _model;
			};

			class FileSearchListWnd;

			class FileSearchWnd: public ChildWnd {
				public:
					FileSearchWnd(strong<Network> net);
					virtual ~FileSearchWnd();
					virtual void Layout();
					virtual void Paint(graphics::Graphics& g, strong<Theme> theme);
					virtual void OnSize(const Area& ns);
					virtual void OnSettingsChanged();

				protected:
					ref<ToolbarWnd> _toolbar;
					ref<FileSearchListWnd> _list;
			};
		}
	}
}

#endif
