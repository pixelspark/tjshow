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
#ifndef _TJOUTPUTWND_H
#define _TJOUTPUTWND_H

namespace tj {
	namespace show {
		class Model;

		namespace view {
			class OutputWnd: public ChildWnd, public Listener<SliderWnd::NotificationChanged> {
				friend class OutputToolbarWnd;

				public:
					OutputWnd(strong<Instance> inst);
					virtual ~OutputWnd();
					virtual void Update();
					virtual void Layout();
					virtual void Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme);
					virtual void Notify(ref<Object> source, const SliderWnd::NotificationChanged& evt);
					
				protected:
					strong<Instance> _instance;
					ref<SliderWnd> _slider;
					ref<ToolbarWnd> _toolbar;

					virtual void OnCreated();
					virtual void OnSize(const Area& newSize);

					const static int KMargin = 2;
					const static int KMarginLeft = 5;
					const static int KMarginTop = 5;
					const static int KBlockSizeX = 16;
					const static int KBlockSizeY = 16;
					const static int KActiveTimeLimit = 250; // 0.25 seconds
					const static int KPlaybackSpeedRange = 4;
					const static int KSliderWidth = 32;
			};
		}
	}
}

#endif