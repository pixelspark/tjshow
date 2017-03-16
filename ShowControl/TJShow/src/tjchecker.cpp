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
#include "../include/internal/tjshow.h"
#include "../include/internal/tjchecker.h"
#include "../include/internal/tjsubtimeline.h"
#include "../include/internal/tjnetwork.h"

using namespace tj::show;
using namespace tj::show::checker;
using namespace tj::shared::graphics;

/** Problem **/
Problem::Problem(ExceptionType severity, ref<Inspectable> subject): _severity(severity), _subject(subject) {
}

Problem::~Problem() {
}

ExceptionType Problem::GetSeverity() const {
	return _severity;
}

ref<Inspectable> Problem::GetSubject() {
	return _subject;
}

/** ResourceNotFoundProblem **/
ResourceNotFoundProblem::ResourceNotFoundProblem(ref<Inspectable> subject, const std::wstring& rid): Problem(ExceptionTypeError, subject), _rid(rid) {
}

ResourceNotFoundProblem::~ResourceNotFoundProblem() {
}

std::wstring ResourceNotFoundProblem::GetTypeName() const {
	return TL(problem_resource_not_found);
}

std::wstring ResourceNotFoundProblem::GetProblemDescription() const {
	return _rid;
}

/** ChannelDoublyUsedProblem **/
ChannelDoublyUsedProblem::ChannelDoublyUsedProblem(Channel c): Problem(ExceptionTypeError, 0), _channel(c) {
}

ChannelDoublyUsedProblem::~ChannelDoublyUsedProblem() {
}

std::wstring ChannelDoublyUsedProblem::GetTypeName() const {
	return TL(problem_channel_doubly_used);
}

std::wstring ChannelDoublyUsedProblem::GetProblemDescription() const {
	return Stringify(_channel);
}

/** NoShowInfoProblem **/
NoShowInfoProblem::NoShowInfoProblem(ref<Model> model, const std::wstring& field): Problem(ExceptionTypeMessage, model), _field(field) {
}

NoShowInfoProblem::~NoShowInfoProblem() {
}

std::wstring NoShowInfoProblem::GetTypeName() const {
	return TL(problem_no_show_info);
}

std::wstring NoShowInfoProblem::GetProblemDescription() const {
	return _field;
}

/** ProblemListWnd **/
ProblemListWnd::ProblemListWnd(strong<ProblemList> pl): _problems(pl),
_notifyIcon(Icons::GetIconPath(Icons::IconExceptionNotify)),
_fatalIcon(Icons::GetIconPath(Icons::IconExceptionFatal)),
_errorIcon(Icons::GetIconPath(Icons::IconExceptionError)),
_messageIcon(Icons::GetIconPath(Icons::IconExceptionMessage))
{
	AddColumn(L"", KColIcon, 0.1f, true);
	AddColumn(TL(problem_type), KColType, 0.5f, true);
	AddColumn(TL(problem_description), KColDescription, 0.4f, true);
}

ProblemListWnd::~ProblemListWnd() {
}

int ProblemListWnd::GetItemCount() {
	return (int)_problems->_problems.size();
}

void ProblemListWnd::PaintItem(int id, tj::shared::graphics::Graphics& g, Area& row, const ColumnInfo& ci) {
	ref<Problem> problem = _problems->_problems.at(id);
	if(problem) {
		switch(problem->GetSeverity()) {
			case ExceptionTypeError:
				DrawCellIcon(g, KColIcon, row, _errorIcon);
				break;

			case ExceptionTypeSevere:
				DrawCellIcon(g, KColIcon, row, _fatalIcon);
				break;

			case ExceptionTypeMessage:
				DrawCellIcon(g, KColIcon, row, _messageIcon);
				break;

			case ExceptionTypeWarning:
				DrawCellIcon(g, KColIcon, row, _notifyIcon);
		}

		StringFormat sf;
		sf.SetTrimming(StringTrimmingEllipsisPath);
		sf.SetFormatFlags(StringFormatFlagsLineLimit);

		strong<Theme> theme = ThemeManager::GetTheme();
		SolidBrush textBr(theme->GetColor(Theme::ColorText));

		DrawCellText(g, &sf, &textBr, theme->GetGUIFontBold(), KColType, row, problem->GetTypeName());
		DrawCellText(g, &sf, &textBr, theme->GetGUIFont(), KColDescription, row, problem->GetProblemDescription());
	}
}

void ProblemListWnd::OnClickItem(int id, int col, Pixels x, Pixels y) {
	if(id>=0 && id < int(_problems->_problems.size())) {
		ref<Problem> problem = _problems->_problems.at(id);
		if(problem) {
			ref<Inspectable> subject = problem->GetSubject();
			if(subject) {
				Application::Instance()->GetView()->Inspect(subject);
			}
		}
	}
	SetSelectedRow(id);
}

/** CheckShowAction **/
CheckShowAction::CheckShowAction(ref<Model> model, ref<Application> app, bool deploy): Action(TL(command_check_resources), Action::UndoIgnore), _alsoDeploy(deploy), _app(app), _model(model) {
}

CheckShowAction::~CheckShowAction() {
}

void RecursiveGetResources(strong<Timeline> tl, std::map< GroupID, std::vector<ResourceIdentifier> >& resourcesPerGroup) {
	ref< Iterator< ref<TrackWrapper> > > tit = tl->GetTracks();
	while(tit->IsValid()) {
		ref<TrackWrapper> tw = tit->Get();
		std::vector<ResourceIdentifier> res;
		tw->GetTrack()->GetResources(res);

		if(res.size()>0) {
			GroupID gid = tw->GetGroup();

			if(resourcesPerGroup.find(gid)==resourcesPerGroup.end()) {
				resourcesPerGroup[gid] = std::vector<ResourceIdentifier>();
			}

			std::vector<ResourceIdentifier>::const_iterator it = res.begin();
			std::vector<ResourceIdentifier>& groupResources = resourcesPerGroup[gid];
			while(it!=res.end()) {
				groupResources.push_back(*it);
				++it;
			}

			ref<SubTimeline> sub = tw->GetSubTimeline();
			if(sub) {
				RecursiveGetResources(sub->GetTimeline(), resourcesPerGroup);
			}
		}
		tit->Next();
	}
}

void CheckShowAction::Execute() {
	ref<ProblemList> problems = GC::Hold(new ProblemList());
	ref<Network> nw = _app->GetNetwork();
	ref<ResourceProvider> rmg = _model->GetResourceManager();

	/* Make a list of all resources per group and check if they are available */
	{
		std::map< GroupID, std::vector<ResourceIdentifier> > resourcesPerGroup;
		RecursiveGetResources(_model->GetTimeline(), resourcesPerGroup);

		/** Check if resources are available */
		std::map< Channel, std::vector<ResourceIdentifier> >::iterator it = resourcesPerGroup.begin();
		while(it!=resourcesPerGroup.end()) {
			GroupID c = it->first;

			std::vector<ResourceIdentifier>::const_iterator rit = it->second.begin();
			while(rit!=it->second.end()) {
				const ResourceIdentifier& rid = *rit;

				// don't bother sending resource advertisements if even the master doesn't have the file
				std::wstring path;
				if(!rmg->GetPathToLocalResource(rid, path)) {
					problems->_problems.push_back(GC::Hold(new ResourceNotFoundProblem(null, rid)));
				}
				else {
					// only publish resource if this resource is really needed on a client
					if(_alsoDeploy) {
						nw->PushResource(c, rid);
					}
				}
				++rit;
			}
			++it;
		}
	}

	// Check show info fields
	{
		if(_model->GetAuthor().length()<1) {
			problems->_problems.push_back(GC::Hold(new NoShowInfoProblem(_model, TL(model_author))));
		}

		if(_model->GetTitle().length()<1) {
			problems->_problems.push_back(GC::Hold(new NoShowInfoProblem(_model, TL(model_title))));
		}
	}
	
	if(problems->_problems.size()>0) {
		// Create some window
		ref<ProblemListWnd> plw = GC::Hold(new ProblemListWnd(problems));
		ref<view::View> view = Application::Instance()->GetView();
		if(view) {
			std::wstring title;

			if(_model->GetFileName().length()>0) {
				title = TL(problem_report_for) + std::wstring(L" ") + File::GetFileName(_model->GetFileName());
			}
			else {
				title = TL(problem_report);
			}

			if(_alsoDeploy) {
				Alert::Show(TL(show_deployer),TL(show_deployed_with_errors), Alert::TypeWarning);
			}
			view->AddProblemReport(title, plw);
		}
	}
	else {
		if(_alsoDeploy) {
			Alert::Show(TL(show_deployer),TL(show_deployed_successfully), Alert::TypeInformation);
		}
		else {
			Alert::Show(TL(show_checker),TL(show_checker_no_errors), Alert::TypeInformation);
		}
	}
}

/** ProblemList **/
ProblemList::ProblemList() {
}

ProblemList::~ProblemList() {
}