/* This file is part of TJShow. TJShow is free software: you 
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
 
 #ifndef _TJTOOLBARWND_H
#define _TJTOOLBARNWD_H

namespace tj {
	namespace shared {
		class EXPORTED ToolbarItem: public Element {
			public:
				ToolbarItem(int command=0, graphics::Bitmap* bmp=0, const String& text = L"", bool separator = false);
				ToolbarItem(int command, const ResourceIdentifier& icon, const String& text = L"", bool separator = false);
				~ToolbarItem();
				bool IsSeparator() const;
				void SetSeparator(bool s);
				virtual int GetCommand() const;
				virtual Icon& GetIcon();
				virtual String GetText() const;
				void SetEnabled(bool e);
				bool IsEnabled() const;
				void SetActive(bool e);
				bool IsActive() const;
				virtual Area GetPreferredSize() const;
				void SetPreferredSize(Pixels w, Pixels h);

				virtual void Paint(graphics::Graphics& g, strong<Theme> theme);
				virtual void Paint(graphics::Graphics& g, strong<Theme> theme, bool over, bool down, float backgroundAlpha = 1.0f);

				static void DrawToolbarButton(graphics::Graphics& g, Icon& icon, const Area& rc, ref<Theme> theme, bool over, bool down, bool separator=false, bool enabled = true, float alpha=1.0f);
				static void DrawToolbarButton(graphics::Graphics& g, const Area& rc, ref<Theme> theme, bool over, bool down, bool separator=false, bool enabled = true, float alpha = 1.0f);

			protected:
				int _command;
				Icon _icon;
				bool _separator;
				bool _enabled;
				bool _active;
				String _text;
				Pixels _preferredWidth, _preferredHeight;
		};

		class EXPORTED ToolbarWnd: public ChildWnd {
			public:
				ToolbarWnd();
				virtual ~ToolbarWnd();
				virtual void Paint(graphics::Graphics& g, strong<Theme> theme);
				virtual void Layout();
				virtual void Add(ref<ToolbarItem> item, bool alignRight = false);
				virtual void Remove(ref<ToolbarItem> item);
				virtual void OnCommand(ref<ToolbarItem> item) = 0;
				virtual void Fill(LayoutFlags f, Area& r, bool direct = true);
				virtual void SetBackground(bool t);
				virtual void SetBackgroundColor(graphics::Color c);
				virtual bool HasTip() const;
				virtual void SetTip(ref<Wnd> tipWindow);

			protected:
				virtual Pixels GetButtonX(int command);
				virtual Area GetFreeArea() const; // returns the area which child classes can freely use to paint on
				virtual void OnMouse(MouseEvent ev, Pixels x, Pixels y);
				virtual void OnSize(const Area& ns);
				virtual void OnTimer(unsigned int id);
				virtual bool CanShowHints();
				
				std::vector< ref<ToolbarItem> > _items;
				std::vector< ref<ToolbarItem> > _itemsRight;
				Area _freeArea;

				static const int KIconWidth = 16;
				static const int KIconHeight = 16;

				bool _in;
				ref<ToolbarItem> _over;
				bool _bk;
				graphics::Color _bkColor;
				ref<ToolbarItem> _tipItem;
				ref<Wnd> _tip;
				Animation _entryAnimation;
		};

		class EXPORTED StateToolbarItem: public ToolbarItem {
			public:
				StateToolbarItem(int command, const String& icon, const String& text = L"");
				virtual ~StateToolbarItem();
				virtual void SetState(bool on);
				bool IsOn() const;
				virtual void Paint(graphics::Graphics& g, ref<Theme> theme, bool over, bool down, float backgroundAlpha);

			protected:
				bool _on;
		};

		class EXPORTED SearchToolbarWnd: public ToolbarWnd, public Listener<EditWnd::EditingNotification> {
			public:
				SearchToolbarWnd();
				virtual ~SearchToolbarWnd();
				virtual void Layout();
				virtual void Paint(graphics::Graphics& g, strong<Theme> theme);
				virtual void Notify(ref<Object> src, const EditWnd::EditingNotification& data);

			protected:
				virtual void SetSearchBoxRightMargin(Pixels r);
				virtual void OnSearchChange(const String& q);
				virtual void OnSearchCommit();
				virtual void OnCreated();
				Area GetSearchBoxArea() const;
				bool IsSearchBoxVisible() const;
				virtual void SetSearchBoxSize(Pixels w, Pixels h);
				virtual void SetSearchBoxHint(const String& txt);
				virtual void SetSearchBoxText(const String& txt);
				virtual bool CanShowHints();
				virtual void FocusSearchBox();

			private:
				const static Pixels KDefaultBoxWidth = 100;
				const static Pixels KDefaultBoxHeight = 16;

				ref<EditWnd> _edit;
				Icon _searchIcon;
				Pixels _rightMargin, _searchWidth, _searchHeight;
		};

		class EXPORTED ThrobberToolbarItem: public ToolbarItem {
			public:
				ThrobberToolbarItem();
				virtual ~ThrobberToolbarItem();
				virtual void Paint(graphics::Graphics& g, ref<Theme> theme, bool over, bool down, float backgroundAlpha = 1.0f);

				Animation Progress;					
		};

		class EXPORTED TimeToolbarItem: public ToolbarItem {
			public:
				TimeToolbarItem();
				virtual ~TimeToolbarItem();
				virtual void Paint(graphics::Graphics& g, strong<Theme> theme, bool over, bool down, float alpha);
		};

		class EXPORTED LogoToolbarItem: public ToolbarItem {
			public:
				LogoToolbarItem(const String& iconRid, Pixels width, int cmd = 0);
				LogoToolbarItem(const Icon& icon, Pixels width, int cmd = 0);
				virtual ~LogoToolbarItem();

				virtual void Paint(graphics::Graphics& g, strong<Theme> theme, bool over, bool down, float alpha);

			protected:
				Icon _logo;
		};
	}
}

#endif
