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
#ifndef _TJINGLEAPPLICATION_H
#define _TJINGLEAPPLICATION_H

#define JID_THEME 2

#include "tjinglelayout.h"
#include "io/tjingleconnector.h"

namespace tj {
	namespace jingle {
		class JingleView;
		class Jingle;

		enum JingleEvent {
			JingleNone = 0,
			JinglePlay,
			JingleStop,
			JingleFadeIn,
			JingleFadeOut,
		};

		class JingleApplication: public tj::shared::RunnableApplication, public tj::shared::Singleton<JingleApplication> {
			public:
				JingleApplication();
				virtual ~JingleApplication();
				virtual void Message(MSG& msg);
				std::wstring GetProgramDirectory() const;
				tj::shared::ref<JingleKeyboardLayout> GetKeyboardLayout();
				virtual void Initialize(tj::shared::ref<tj::shared::Arguments> args);
				virtual void Command(int cmd);
				virtual void OnThemeChanged();
				virtual void OnBeforeJingleStart();
				tj::shared::ref<JingleView> GetView();
				virtual void FindJinglesByName(const std::wstring& q, std::deque< tj::shared::ref<Jingle> >& results);

				virtual void OnJingleEvent(const std::wstring& jingleID, const JingleEvent& ev);

			protected:
				static tj::shared::ref<JingleApplication> _instance;
				std::wstring _programDirectory;
				tj::shared::ref<JingleView> _view;
				tj::shared::graphics::GraphicsInit _ginit;
				tj::shared::ref<tj::shared::SettingsStorage> _settings;
				tj::shared::ref<io::JingleConnector> _connector;
		};
	}
}

#endif