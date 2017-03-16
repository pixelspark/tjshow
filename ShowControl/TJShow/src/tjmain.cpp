#include "../include/internal/tjshow.h"
#include "../include/internal/tjsubtimeline.h"
#include "../include/internal/tjstats.h"
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
#include "../include/internal/tjinstancer.h"

#include <time.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <commdlg.h>
#include <shellapi.h>

using namespace tj::show::stats;

/** Main entry method for TJShow executable. This is the pre-initialization code, which means that this doesn't 
actually initialize the application, but just checks things like licenses and what application we should start.
**/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR cmd, int nShow) {
	srand((int)time(0)+1); // Initialize random number generator (this is also done in tjshared Thread::Run code)
	ref<Arguments> args = GC::Hold(new Arguments(GetCommandLine()));

	// Start me up, the usual way...
	Log::Write(L"TJShow/Main", std::wstring(L"TJShow starting @ ")+Stringify(int(time(NULL))));
	SharedDispatcher sd;
	
	// splash window
	std::wostringstream pws;
	wchar_t programDir[MAX_PATH+1];
	GetModuleFileName(GetModuleHandle(NULL), programDir, MAX_PATH);
	PathRemoveFileSpec(programDir);
	pws << programDir << L"\\icons\\splash\\tjshow.jpg";

	ref<SplashThread> splash = GC::Hold(new SplashThread(pws.str(), 400, 300));
	splash->Start();

	bool asClient = args->IsSet(L"client");

	try {
		ref<Application> application = Application::InstanceReference();

		PluginManager::Instance()->AddInternalPlugin(GC::Hold(new StatsPlugin()));
		PluginManager::Instance()->AddInternalPlugin(GC::Hold(new SubTimelinePlugin()));
		PluginManager::Instance()->AddInternalPlugin(GC::Hold(new instancer::InstancerPlugin()));

		ref<Core> core = Core::Instance();
		application->Initialize(args, splash);
		
		core->Run(dynamic_cast<RunnableApplication*>(application.GetPointer()), args);

		Application::Close();
	}
	catch(Exception& e) {
		splash->Hide();
		MessageBox(0L, e.GetMsg().c_str(), TL(error), MB_OK|MB_ICONERROR);
	}

	return 0;
}
