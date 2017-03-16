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
#ifndef _TJDIALOGS_H
#define _TJDIALOGS_H

namespace tj {
	namespace show {
		class Timeline;
		class Application;

		namespace view {
			class Dialogs {
				public:
					static void AddTrack(ref<Wnd> owner, strong<Instance> tl, bool multi = false);
					static void AddTrackFromFile(ref<Wnd> owner, strong<Instance> tl);
			};
		}
	}
}

#endif