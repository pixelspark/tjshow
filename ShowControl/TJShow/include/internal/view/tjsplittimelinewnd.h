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
#ifndef _TJSPLITTIMELINEWND_H
#define _TJSPLITTIMELINEWND_H

namespace tj {
	namespace show {
		namespace view {
			class CueWnd;
			class OutputWnd;
			class TimelineWnd;
			class CueListWnd;
			class VariableWnd;

			class SplitTimelineWnd: public SplitterWnd {
				public:
					SplitTimelineWnd(strong<Instance> ctrl);
					virtual ~SplitTimelineWnd();
					ref<TimelineWnd> GetTimelineWindow();
					ref<CueListWnd> GetCueListWindow();
					ref<OutputWnd> GetOutputWindow();
					virtual LRESULT Message(UINT msg, WPARAM wp, LPARAM lp);
					virtual std::wstring GetTabTitle() const;
					virtual ref<Icon> GetTabIcon() const;
					ref<CueWnd> GetCueWindow();
					
				protected:
					virtual void OnCreated();
					virtual void OnKey(Key k, wchar_t t, bool down, bool accelerator);

					weak<Instance> _instance;
					ref<TabWnd> _tabWnd;
					ref<CueWnd> _cueListWnd;
					ref<OutputWnd> _outputWnd;
					ref<VariableWnd> _variableWnd;
			};
		}
	}
}

#endif