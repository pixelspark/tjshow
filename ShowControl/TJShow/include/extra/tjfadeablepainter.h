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
#ifndef _TJFADEABLEPAINTER_H
#define _TJFADEABLEPAINTER_H

namespace tj {
	namespace show {
		template<typename T> class FaderPainter {
			public:
				FaderPainter(ref< Fader<T> > fd): _fade(fd) {
				}

				~FaderPainter() {
				}

				void Paint(tj::shared::graphics::Graphics* g, Time start, Time length, int h, int w, int x, int y, tj::shared::graphics::Pen* pen, Time selected, bool allowIntegral=false, ref<Item> focus = 0) {
					assert(w!=0&&h!=0);
					strong<Theme> theme = ThemeManager::GetTheme();

					Time focused = -1;
					if(focus.IsCastableTo< FaderItem<T> >()) {
						focused = ref<FaderItem<T> >(focus)->GetTime();
					}

					GraphicsContainer gc = g->BeginContainer();
					g->SetClip(Rect(x,y,w,h));
					g->SetHighQuality(true);
					x -= 1;

					float pixelsPerMs = float(w) / float(length);
					float pixelsPerValue = (float(h) / float(_fade->GetMaximum()-_fade->GetMinimum()));
					float pixelOffset = 0.0f;
					if(_fade->GetMinimum()>=0) {
						pixelsPerValue = -pixelsPerValue;
						pixelOffset = (float)h;
					}

					std::map<Time, T>* map = _fade->GetPoints();
					float pixelHeight = float(_fade->GetDefaultValue())* pixelsPerValue + pixelOffset;

					std::map<Time, T>::const_iterator it = map->begin();
					float pixelPos = 0.0f;
					Color color = pen->GetColor();
					SolidBrush br(color);

					while(it!=map->end()) {
						const std::pair<Time, T>& data = *it;
						float newPixelPos = int(data.first-start)*pixelsPerMs;
						float newPixelHeight = abs(data.second) * pixelsPerValue + pixelOffset;
						g->DrawLine(pen, x+pixelPos, y+pixelHeight, x+newPixelPos, y+newPixelHeight);

						Area ellipseRC(Pixels(x+newPixelPos-KFaderPointSize), Pixels(y+newPixelHeight-KFaderPointSize), KFaderPointSize*2, KFaderPointSize*2);
						if(focused==data.first) {
							theme->DrawFocusEllipse(*g, ellipseRC);
						}
						g->FillEllipse(&br, ellipseRC);
						
						pixelPos = newPixelPos;
						pixelHeight = newPixelHeight;
						++it;
					}

					g->DrawLine(pen, x+pixelPos, y+pixelHeight, float(x+w), y+pixelHeight);


					if(allowIntegral && map->size()>0) {
						//SolidBrush integralBrush(Color(50, color.GetR(), color.GetG(), color.GetB()));
						LinearGradientBrush integralBrush(PointF(0.0f,float(y)), PointF(0.0f,float(h+y)), Theme::ChangeAlpha(color, 75), Theme::ChangeAlpha(color, 10));

						std::map<Time, T>::const_iterator begin = map->begin();
						std::map<Time, T>::const_iterator end = map->end();

						float minPixelHeight = float(y) + abs(_fade->GetMinimum()) * pixelsPerValue + pixelOffset;
						float defaultPixelHeight = float(y) + abs(_fade->GetDefaultValue())*pixelsPerValue + pixelOffset;
						float farX = float(x+w);
						PointF* points = new PointF[(int)map->size()+5];
						points[0] = PointF(float(x), minPixelHeight);
						points[1] = PointF(float(x), defaultPixelHeight);
						int n = 2;

						float newPixelHeight;
						while(begin!=end) {
							const std::pair< Time, T>& data = *begin;
							newPixelHeight = abs(data.second) * pixelsPerValue + pixelOffset + float(y);
							points[n] = PointF(float(x)+pixelsPerMs*float(data.first.ToInt()-start.ToInt()), newPixelHeight);
							n++;
							begin++;
						}

						points[n] = PointF(farX, newPixelHeight);
						points[n+1] = PointF(farX, minPixelHeight);
						g->FillPolygon(&integralBrush, points, n+2);
						delete[] points;
					}

					g->EndContainer(gc);
				}

			protected:
				ref< Fader<T> > _fade;
				const static tj::shared::Pixels KFaderPointSize = 2;
		};
	}
}

#endif