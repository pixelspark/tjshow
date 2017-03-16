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
#ifndef _TJCHECKER_H
#define _TJCHECKER_H

namespace tj {
	namespace show {
		namespace checker {
			class Problem: public virtual tj::shared::Object {
				public:
					Problem(ExceptionType severity, ref<Inspectable> subject);
					virtual ~Problem();
					virtual std::wstring GetTypeName() const = 0;
					virtual std::wstring GetProblemDescription() const = 0;
					virtual ExceptionType GetSeverity() const;
					virtual ref<Inspectable> GetSubject();

				private:
					ExceptionType _severity;
					weak<Inspectable> _subject;
			};

			class ResourceNotFoundProblem: public Problem {
				public:
					ResourceNotFoundProblem(ref<Inspectable> subject, const std::wstring& rid);
					virtual ~ResourceNotFoundProblem();
					virtual std::wstring GetTypeName() const;
					virtual std::wstring GetProblemDescription() const;

				protected:
					std::wstring _rid;
			};

			class ChannelDoublyUsedProblem: public Problem {
				public:
					ChannelDoublyUsedProblem(Channel c);
					virtual ~ChannelDoublyUsedProblem();
					virtual std::wstring GetTypeName() const;
					virtual std::wstring GetProblemDescription() const;

				protected:
					Channel _channel;
			};

			class NoShowInfoProblem: public Problem {
				public:
					NoShowInfoProblem(ref<Model> model, const std::wstring& field);
					virtual ~NoShowInfoProblem();
					virtual std::wstring GetTypeName() const;
					virtual std::wstring GetProblemDescription() const;

				protected:
					std::wstring _field;
			};

			class ProblemList {
				public:
					ProblemList();
					virtual ~ProblemList();

					std::vector< ref<Problem> > _problems;
			};

			class ProblemListWnd: public ListWnd {
				public:	
					ProblemListWnd(strong<ProblemList> pl);
					virtual ~ProblemListWnd();
					virtual int GetItemCount();
					virtual void PaintItem(int id, tj::shared::graphics::Graphics& g, Area& row, const ColumnInfo& ci);
					virtual void OnClickItem(int id, int col, Pixels x, Pixels y);

				protected:
					enum {
						KColIcon = 1,
						KColType = 2,
						KColDescription = 3,
					};

					strong<ProblemList> _problems;
					Icon _notifyIcon, _fatalIcon, _errorIcon, _messageIcon;
			};

			/** Check show action; runs several checks on the show **/
			class CheckShowAction: public Action {
				public:
					CheckShowAction(ref<Model> model, ref<Application> app, bool alsoDeploy);
					virtual ~CheckShowAction();
					virtual void Execute();

				protected:
					bool _alsoDeploy;
					ref<Application> _app;
					ref<Model> _model;
			};
		}
	}
}

#endif