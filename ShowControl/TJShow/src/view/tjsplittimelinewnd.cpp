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
#include "../../include/internal/tjshow.h"
#include "../../include/internal/tjcontroller.h"
#include "../../include/internal/view/tjsplittimelinewnd.h"
#include "../../include/internal/view/tjtimelinewnd.h"
#include "../../include/internal/view/tjoutputwnd.h"
#include "../../include/internal/view/tjcuelistwnd.h"
#include "../../include/internal/view/tjvariablewnd.h"
using namespace tj::show;
using namespace tj::show::view;
using namespace tj::shared::graphics;

SplitTimelineWnd::SplitTimelineWnd(strong<Instance> ctrl): SplitterWnd(OrientationVertical), _instance(ref<Instance>(ctrl)) {	
	_tabWnd = GC::Hold(new TabWnd(dynamic_cast<RootWnd*>(GetRootWindow())));
	_tabWnd->SetChildStyle(true);
	_tabWnd->SetDetachAttachAllowed(false);

	_outputWnd = GC::Hold(new OutputWnd(ctrl));
	_cueListWnd = GC::Hold(new CueWnd(ctrl));
	_variableWnd = GC::Hold(new VariableWnd(ctrl->GetLocalVariables()));

	_tabWnd->AddPane(GC::Hold(new Pane(TL(cues), _cueListWnd, false, false, 0, Placement(), L"icons/tabs/cues.png")));
	_tabWnd->AddPane(GC::Hold(new Pane(TL(outputs), _outputWnd, false, false, 0, Placement(), L"icons/tabs/outputs.png")));
	_tabWnd->AddPane(GC::Hold(new Pane(TL(local_variables), _variableWnd, false, false, 0, Placement(), L"icons/tabs/variables.png")));
	
	SetRatio(0.75f);
	SetFirst(GC::Hold(new TimelineWnd(ctrl)));
	SetSecond(_tabWnd);
}

void SplitTimelineWnd::OnCreated() {
	ref<Instance> instance = _instance;
	if(instance && instance.IsCastableTo<Controller>()) {
		ref<Controller> controller = instance;
		if(controller) {
			controller->SetSplitTimelineWindow(this);
		}
	}
}

SplitTimelineWnd::~SplitTimelineWnd() {
}

ref<TimelineWnd> SplitTimelineWnd::GetTimelineWindow() {
	return ref<TimelineWnd>(GetFirst());
}

ref<CueListWnd> SplitTimelineWnd::GetCueListWindow() {
	return _cueListWnd->GetCueListWindow();
}

ref<CueWnd> SplitTimelineWnd::GetCueWindow() {
	return _cueListWnd;
}

ref<OutputWnd> SplitTimelineWnd::GetOutputWindow() {
	return _outputWnd;
}

std::wstring SplitTimelineWnd::GetTabTitle() const {
	return GetFirst()->GetTabTitle();
}

ref<Icon> SplitTimelineWnd::GetTabIcon() const {
	return GetFirst()->GetTabIcon();
}

void SplitTimelineWnd::OnKey(Key k, wchar_t t, bool down, bool accelerator) {
	ref<Wnd> first = GetFirst();
	if(first) {
		ref<TimelineWnd>(first)->OnKey(k,t,down, accelerator);
	}
}

LRESULT SplitTimelineWnd::Message(UINT msg, WPARAM wp, LPARAM lp) {
	if(msg==WM_PARENTNOTIFY) {
		SplitterWnd::Update();
	}

	return SplitterWnd::Message(msg, wp, lp);
}