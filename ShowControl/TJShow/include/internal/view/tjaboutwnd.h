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
#ifndef _TJABOUTWND_H
#define _TJABOUTWND_H

namespace tj {
	namespace show {
		namespace view {
			class AboutWnd: public ChildWnd {
				public:
					AboutWnd();
					virtual ~AboutWnd();
					virtual void Paint(tj::shared::graphics::Graphics& g, strong<Theme> theme);
					virtual void Layout();
					virtual void OnSize(const Area& ns);
					virtual void OnTimer(unsigned int id);

				protected:
					struct Particle {
						float _x;
						float _y;
						float _size;
						float _dir;
						float _speed;
						RGBColor _color;
					};

					Icon _logo;
					static const int KParticleCount;
					static const int KTimerID;
					std::vector<Particle> _particles;
			};
		}
	}
}

#endif