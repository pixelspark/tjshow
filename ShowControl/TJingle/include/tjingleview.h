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
#ifndef _TJINGLEVIEW_H
#define _TJINGLEVIEW_H

namespace tj {
	namespace jingle {
		class Jingle;

		class JingleView: public tj::shared::RootWnd, public tj::shared::Listener<tj::shared::SliderWnd::NotificationChanged>, public tj::shared::Serializable {
			friend class JingleApplication;

			public:
				JingleView();
				virtual ~JingleView();
				virtual LRESULT Message(UINT msg, WPARAM wp, LPARAM lp);
				virtual void Create();
				virtual void Paint(tj::shared::graphics::Graphics& g, tj::shared::ref<tj::shared::Theme> theme);
				virtual void Layout();
				virtual void Update();
				virtual void Inspect(tj::shared::ref<tj::shared::Inspectable> ins);
				virtual void Notify(tj::shared::ref<tj::shared::Object> source, const tj::shared::SliderWnd::NotificationChanged& evt);
				virtual void Save(TiXmlElement* parent);
				virtual void Load(TiXmlElement* you);
				virtual void OnThemeChanged();
				virtual void ShowAboutInformation();
				virtual void PaintStatusBar(tj::shared::graphics::Graphics& g, tj::shared::strong<tj::shared::Theme> theme, const tj::shared::Area& statusBarArea);
				void StopAll();
				virtual void FindJinglesByName(const std::wstring& q, std::deque< tj::shared::ref<Jingle> >& results);
				virtual void StartSearch();

			protected:
				virtual void GetMinimumSize(tj::shared::Pixels& w, tj::shared::Pixels& h);

				tj::shared::ref<JingleToolbarWnd> _toolbar;
				tj::shared::ref<tj::shared::TabWnd> _tab;
				std::map<int, tj::shared::ref<JinglePane> > _panes;
				tj::shared::ref<tj::shared::SliderWnd> _master;
		};
	}
}

#endif