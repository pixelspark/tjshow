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
 
 #ifndef _TJROOTWND_H
#define _TJROOTWND_H

namespace tj {
	namespace shared {
		class EXPORTED WindowManager: public virtual Object {
			public:
				virtual ~WindowManager();

				// Floating panes
				virtual ref<FloatingPane> AddFloatingPane(ref<Pane> pane) = 0;
				virtual void RemoveFloatingPane(ref<Pane> pn) = 0;

				// Tab windows (pane holders)
				virtual void AddTabWindow(ref<TabWnd> tw) = 0;
				virtual void RemoveTabWindow(ref<TabWnd> tw) = 0;
				virtual void RemoveTabWindow(TabWnd* tw) = 0;

				// Orphan panes
				virtual bool IsOrphanPane(ref<Wnd> wnd) = 0;
				virtual void AddOrphanPane(ref<Pane> pane) = 0;
				virtual std::vector< ref<Pane> >* GetOrphanPanes() = 0;
				virtual void RemoveOrphanPane(ref<Pane> pane) = 0;

				// Other
				virtual void RevealWindow(ref<Wnd> wnd, ref<TabWnd> addTo = ref<TabWnd>()) = 0;
				virtual ref<TabWnd> FindTabWindowAt(int x, int y) = 0;
				virtual ref<TabWnd> GetTabWindowById(const String& id) = 0;
				virtual void SetDragTarget(ref<TabWnd> tw) = 0;
				virtual ref<TabWnd> GetDragTarget() = 0;
				virtual void RemoveWindow(ref<Wnd> w) = 0;
				virtual void RenameWindow(ref<Wnd> w, String name) = 0;
				virtual void AddPane(ref<Pane> p, bool select = false) = 0; // add by preferred placement

		};

		class EXPORTED RootWnd: public TopWnd, public WindowManager {
			friend class NotificationWnd; 

			public:
				RootWnd(const String& title, bool useDoubleBuffering = true);
				virtual ~RootWnd();
				virtual void Update();

				// WindowManager implementation
				virtual ref<FloatingPane> AddFloatingPane(ref<Pane> pane);
				virtual void RemoveFloatingPane(ref<Pane> pn);

				virtual void AddTabWindow(ref<TabWnd> tw);
				virtual void RemoveTabWindow(ref<TabWnd> tw);
				virtual void RemoveTabWindow(TabWnd* tw);

				virtual void RevealWindow(ref<Wnd> wnd, ref<TabWnd> addTo = ref<TabWnd>());
				virtual ref<TabWnd> FindTabWindowAt(int x, int y);
				virtual ref<TabWnd> GetTabWindowById(const String& id);
				virtual void SetDragTarget(ref<TabWnd> tw);
				virtual ref<TabWnd> GetDragTarget();
				virtual bool IsOrphanPane(ref<Wnd> wnd);

				virtual void AddOrphanPane(ref<Pane> pane);
				virtual std::vector< ref<Pane> >* GetOrphanPanes();
				virtual void RemoveOrphanPane(ref<Pane> pane);
				virtual void RemoveWindow(ref<Wnd> w);
				virtual void RenameWindow(ref<Wnd> w, String name);
				virtual void AddPane(ref<Pane> p, bool select = false); // add by preferred placement

				void FullRepaint(); // use after switching theme
				virtual void Layout();
				virtual Area GetClientArea() const;
				virtual void Paint(graphics::Graphics& g, strong<Theme> theme);
				virtual void PaintStatusBar(graphics::Graphics& g, strong<Theme> theme, const Area& statusBarArea);
				virtual void OnSize(const Area& ns);

				virtual void SetShowStatusBar(bool s);
				virtual bool IsStatusBarShown() const;

			protected:
				#ifdef TJ_OS_WIN
					virtual LRESULT Message(UINT msg, WPARAM wp, LPARAM lp);
				#endif
				bool _showStatusBar;
				const static Pixels KStatusBarHeight;
				Icon _grabberIcon;
				std::vector< ref<FloatingPane> > _floatingPanes;
				std::vector < ref<TabWnd> > _tabWindows;
				std::vector< ref<Pane> > _orphans;
				ref<TabWnd> _dragTarget;
		};
	}
}

#endif
