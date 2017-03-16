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
#ifndef _TJCAPACITYWND_H
#define _TJCAPACITYWND_H

namespace tj {
	namespace show {
		namespace view {
			class CapacityListWnd: public ListWnd {
				public:
					CapacityListWnd(ref<Capacities> caps);
					virtual ~CapacityListWnd();
					virtual int GetItemCount();
					virtual void PaintItem(int id, tj::shared::graphics::Graphics& g, Area& row, const ColumnInfo& ci);
					
				protected:
					virtual void OnClickItem(int id, int col, Pixels x, Pixels y);
					virtual void OnRightClickItem(int id, int col);
					virtual void OnContextMenu(Pixels x, Pixels y);
					virtual void OnCopy();
					virtual void OnPaste();
					virtual void OnCut();

					void Inspect(ref<Capacity> c);

					enum {
						KColName,
						KColInitial,
						KColValue,
						KColWaiting,
						KColDescription,
					};

					ref<Capacities> _caps;
			};

			class CapacitiesWnd: public ChildWnd {
				friend class CapacityToolbarWnd;

				public:
					CapacitiesWnd(ref<Capacities> caps);
					virtual ~CapacitiesWnd();
					virtual void Layout();
					virtual void OnSize(const Area& ns);
					virtual void Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme);
					virtual void Update();

				protected:
					ref<CapacityListWnd> _list;
					ref<ToolbarWnd> _tools;
					ref<Capacities> _caps;
			};
		}
	}
}

#endif