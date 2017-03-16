// This is a part of the Active Template Library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#include "StdAfx.H"

extern "C" void DoInitialization();
extern "C" void DoCleanup();

#define SPACECHAR   _T(' ')
#define DQUOTECHAR  _T('\"')

extern void __WinMainCRTStartup();

/*
 * Make sure CALL/RET -> JMP optimization doesn't trigger in entrypoint, so
 * it will appear in callstacks.
 */
#pragma optimize("", off)

extern "C" void WinMainCRTStartup()
{
	/*
	 * The /GS security cookie must be initialized before any exception
	 * handling targetting the current image is registered.  No function
	 * using exception handling can be called in the current image until
	 * after __security_init_cookie has been called.
	 */
	__security_init_cookie();

	__WinMainCRTStartup();
}

#pragma optimize("", on)

extern void __WinMainCRTStartup()
{
	LPSTR lpszCommandLine = ::GetCommandLineA();
	if (lpszCommandLine == NULL)
		::ExitProcess((UINT)-1);

	DoInitialization();

	// Skip past program name (first token in command line).
	// Check for and handle quoted program name.
	if (*lpszCommandLine == DQUOTECHAR)
	{
		// Scan, and skip over, subsequent characters until
		// another double-quote or a null is encountered.
		do
		{
			lpszCommandLine = ::CharNextA(lpszCommandLine);
		}
		while ((*lpszCommandLine != DQUOTECHAR) && (*lpszCommandLine != _T('\0')));

		// If we stopped on a double-quote (usual case), skip over it.
		if (*lpszCommandLine == DQUOTECHAR)
			lpszCommandLine = ::CharNextA(lpszCommandLine);
	}
	else
	{
		while (*lpszCommandLine > SPACECHAR)
			lpszCommandLine = ::CharNextA(lpszCommandLine);
	}

	// Skip past any white space preceeding the second token.
	while (*lpszCommandLine && (*lpszCommandLine <= SPACECHAR))
		lpszCommandLine = ::CharNextA(lpszCommandLine);

	STARTUPINFOA StartupInfo;
	StartupInfo.dwFlags = 0;
	::GetStartupInfoA(&StartupInfo);

	int nRet = WinMain(::GetModuleHandleA(NULL), NULL, lpszCommandLine,
		(StartupInfo.dwFlags & STARTF_USESHOWWINDOW) ?
		StartupInfo.wShowWindow : SW_SHOWDEFAULT);

	DoCleanup();

	::ExitProcess((UINT)nRet);
}

