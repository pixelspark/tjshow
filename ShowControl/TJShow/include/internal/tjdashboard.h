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
#ifndef _TJ_DASHBOARD_DEF_H
#define _TJ_DASHBOARD_DEF_H

namespace tj {
	namespace show {
		typedef std::wstring WidgetType;

		enum LayoutSizing {
			LayoutResizeNone = 0,
			LayoutResizeHorizontally = 1,
			LayoutResizeVertically = 2,
			LayoutResizeRetainAspectRatio = 4,
		};

		enum LayoutAnchor {
			LayoutAnchorNone = 0,
			LayoutAnchorTop = 1,
			LayoutAnchorLeft = 2,
			LayoutAnchorBottom = 4,
			LayoutAnchorRight = 8,
			LayoutAnchorTopLeft = LayoutAnchorTop | LayoutAnchorLeft,
			LayoutAnchorAll = LayoutAnchorTop | LayoutAnchorLeft | LayoutAnchorRight | LayoutAnchorBottom,
		};

		enum LabelPosition {
			LabelPositionBottom = 0,
			LabelPositionRight = 1,
			LabelPositionDefault = LabelPositionBottom,
		};

		class Widget: public Element, public virtual SupportsMouseInteraction, public virtual Serializable, public virtual Inspectable {
			friend class WidgetFactory;

			public:
				virtual ~Widget();
				virtual Area GetPreferredSize() const;
				virtual Flags<LayoutSizing>& GetLayoutSizing();
				virtual Flags<LayoutAnchor>& GetLayoutAnchor();
				virtual bool IsInDesignMode() const;
				virtual void SetDesignMode(bool dm);
				virtual ref<PropertySet> GetProperties();
				virtual void OnPropertyChanged(void* member);
				virtual const std::wstring& GetLabel() const;
				virtual LabelPosition GetLabelPosition() const;
				virtual void SetLabelPosition(LabelPosition lps);
				virtual bool CheckAndClearRepaintFlag();
				virtual void Move(Pixels x, Pixels y, Pixels w, Pixels h);

			protected:
				virtual void Repaint();
				virtual void SetPreferredSize(const Area& ps);
				virtual void SetLayoutSizing(const LayoutSizing& ls);
				virtual void SetLayoutAnchor(const LayoutAnchor& la);
				virtual void SaveWidget(TiXmlElement* el);
				virtual void LoadWidget(TiXmlElement* el);
				Widget();

			private:
				bool _wantsRepaint;
				bool _designMode;
				Pixels _pw, _ph;
				Flags<LayoutSizing> _layoutSizing;
				Flags<LayoutAnchor> _layoutAnchor;
				LabelPosition _labelPosition;
				std::wstring _label;
		};

		class WidgetFactory: public PrototypeBasedFactory<Widget> {
			public:
				virtual ~WidgetFactory();
				static strong<WidgetFactory> Instance();
				void SaveWidget(ref<Widget> wt, TiXmlElement* ti);
				ref<Widget> LoadWidget(TiXmlElement* el);
				ref<Widget> DoCreateWidgetMenu(ref<Wnd> wnd, Pixels x, Pixels y);

			private:
				static ref<WidgetFactory> _instance;
		};

		class Dashboard: public virtual Object, public Serializable {
			friend class tj::show::view::DashboardWnd; /* for _widgets */

			public:
				Dashboard();
				virtual void Clear();
				virtual void Clone();
				virtual ~Dashboard();
				virtual void Load(TiXmlElement* el);
				virtual void Save(TiXmlElement* el);
				virtual void RemoveAllWidgets();
				virtual void AddWidget(ref<Widget> wi);
				virtual ref<Widget> GetWidgetAt(Pixels x, Pixels y);
				virtual void RemoveWidget(ref<Widget> wi);

			protected:
				std::wstring _id;
				std::deque< ref<Widget> > _widgets;
		};
	}
}

#endif