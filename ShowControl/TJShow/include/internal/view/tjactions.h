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
#ifndef _TJACTIONS_H
#define _TJACTIONS_H

namespace tj {
	namespace show {
		namespace view {
			class OpenFileAction: public Action {
				public:
					OpenFileAction(Application* app, std::wstring path, ref<Model> model, bool import=false);
					virtual ~OpenFileAction();
					virtual void Execute();

				protected:
					Application* _app;
					std::wstring _file;
					ref<Model> _model;
					bool _import;
			};

			class SaveFileAction: public Action {
				public:
					enum Type {
						TypeSave = 1,
						TypeSaveAs,
						TypeSaveOnExit,
					};

					SaveFileAction(Application* app, ref<Model> model, Type type, const std::wstring& fn = L"");
					virtual ~SaveFileAction();
					virtual void Execute();

				protected:
					Application* _app;
					ref<Model> _model;
					Type _type;
					std::wstring _fn;
			};

			class InfoAction: public Action {
				public:
					InfoAction(ref<Application> app);
					virtual void Execute();

					ref<Application> _app;
			};

			class BundleResourcesAction: public Action {
				public:
					BundleResourcesAction(Application* app);
					virtual ~BundleResourcesAction();
					virtual void Execute();

				protected:
					Application* _app;
			};
		}
	}
}

#endif