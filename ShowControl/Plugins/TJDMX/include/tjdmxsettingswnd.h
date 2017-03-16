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
#ifndef _TJDMXSETTINGSWND_H
#define _TJDMXSETTINGSWND_H

class DMXPatchListWnd: public EditableListWnd {
	public:
		DMXPatchListWnd(ref<DMXPlugin> plg);
		virtual ~DMXPatchListWnd();
		virtual void PaintItem(int id, tj::shared::graphics::Graphics& g, Area& r, const ColumnInfo& ci);
		virtual int GetItemCount();
		virtual ref<Property> GetPropertyForItem(int id, int col);

	protected:
		virtual void OnClickItem(int id, int col, Pixels x, Pixels y);
		virtual void OnDoubleClickItem(int id, int col);

		enum {
			KColTNPAddress = 1,
			KColDMXAddress,
			KColName,
			KColType,
			KColReset,
		};

		weak<DMXPlugin> _plugin;
		Icon _resetIcon;
};

class DMXPatchWnd: public ChildWnd {
	public:
		DMXPatchWnd(ref<DMXPlugin> plugin, ref<PropertyGridProxy> pg);
		virtual ~DMXPatchWnd();
		virtual void Layout();
		virtual void Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme);
		virtual void OnSize(const Area& ns);
		virtual void OnSettingsChanged();

	protected:
		ref<DMXPatchListWnd> _list;
		ref<ToolbarWnd> _tools;
};

class DMXDeviceListWnd: public ListWnd {
	public:
		DMXDeviceListWnd(ref<DMXPlugin> p, ref<PropertyGridProxy> pg);
		virtual ~DMXDeviceListWnd();
		virtual int GetItemCount();
		virtual void PaintItem(int id, tj::shared::graphics::Graphics& g, Area& row, const ColumnInfo& ci);
		virtual void OnClickItem(int id, int col, Pixels x, Pixels y);
		
	protected:
		ref<DMXDevice> GetDeviceByIndex(int id);

		ref<DMXPlugin> _plugin;
		ref<PropertyGridProxy> _pg;
		Icon _checkedIcon;
		Icon _uncheckedIcon;

		enum {
			KColName = 1,
			KColEnabled = 2,
			KColUniverses = 3,
			KColPort = 4,
			KColInfo = 5,
			KColSerial = 6,
		};
};

class DMXSettingsWnd: public ChildWnd {
	public:
		DMXSettingsWnd(ref<DMXPlugin> plugin, ref<PropertyGridProxy> pg);
		virtual ~DMXSettingsWnd();
		virtual void Layout();
		virtual void OnSize(const Area& ns);
		virtual void Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme);

	protected:
		virtual void OnSettingsChanged();

		ref<TabWnd> _tab;
		ref<DMXDeviceListWnd> _devices;
		ref<DMXPatchWnd> _pw;
		ref<PropertyGridProxy> _pgp;
		const static int KDeviceInfoHeight = 70;
		const static int KMarginLeft = 10;
		const static int KMarginTop = 10;
		const static int KCellWidth = 22;
		const static int KCellHeight = 12;
};

#endif