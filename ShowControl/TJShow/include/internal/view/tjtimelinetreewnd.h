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
#ifndef _TJTIMELINETREEWND_H
#define _TJTIMELINETREEWND_H

namespace tj {
	namespace show {
		namespace view {
			class TimelineTreeWnd: public TreeWnd {
				friend class TimelineTreeNode;
				friend class ShowTreeNode;

				public:
					TimelineTreeWnd(ref<Model> model, ref<Instance> root, bool compact = false);
					virtual ~TimelineTreeWnd();
					virtual void OnTimer(unsigned int id);
					virtual void Rebuild();
					virtual std::wstring GetTabTitle() const;

				protected:
					virtual void OnCreated();

					enum {
						KColName = 1,
						KColPreviousCue,
						KColNextCue,
						KColTime,
					};

					weak<Instance> _root;
					weak<Model> _model;
			};
		}
	}
}

#endif