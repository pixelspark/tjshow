// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include "stdafx.h"
#include <dlgs.h>       // for standard control IDs for commdlg



#define new DEBUG_NEW

////////////////////////////////////////////////////////////////////////////
// FileOpen/FileSaveAs common dialog helper

CFileDialog::CFileDialog(BOOL bOpenFileDialog,
	LPCTSTR lpszDefExt, LPCTSTR lpszFileName, DWORD dwFlags,
	LPCTSTR lpszFilter, CWnd* pParentWnd, DWORD dwSize) : CCommonDialog(pParentWnd)
{
	// determine size of OPENFILENAME struct if dwSize is zero
	if (dwSize == 0)
	{
		OSVERSIONINFO vi;
		ZeroMemory(&vi, sizeof(OSVERSIONINFO));
		vi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		::GetVersionEx(&vi);
		// if running under NT and version is >= 5
		if (vi.dwPlatformId == VER_PLATFORM_WIN32_NT && vi.dwMajorVersion >= 5)
			dwSize = sizeof(OPENFILENAME);
		else
			dwSize = OPENFILENAME_SIZE_VERSION_400;
	}

	// size of OPENFILENAME must be at least version 4
	ASSERT(dwSize >= OPENFILENAME_SIZE_VERSION_400);
	// allocate memory for OPENFILENAME struct based on size passed in
	m_pOFN = static_cast<LPOPENFILENAME>(malloc(dwSize));
	ASSERT(m_pOFN != NULL);
	if (m_pOFN == NULL)
		AfxThrowMemoryException();

	memset(&m_ofn, 0, dwSize); // initialize structure to 0/NULL
	m_szFileName[0] = '\0';
	m_szFileTitle[0] = '\0';
	m_pofnTemp = NULL;

	m_bOpenFileDialog = bOpenFileDialog;
	m_nIDHelp = bOpenFileDialog ? AFX_IDD_FILEOPEN : AFX_IDD_FILESAVE;

	m_ofn.lStructSize = dwSize;
	m_ofn.lpstrFile = m_szFileName;
	m_ofn.nMaxFile = _countof(m_szFileName);
	m_ofn.lpstrDefExt = lpszDefExt;
	m_ofn.lpstrFileTitle = (LPTSTR)m_szFileTitle;
	m_ofn.nMaxFileTitle = _countof(m_szFileTitle);
	m_ofn.Flags |= dwFlags | OFN_ENABLEHOOK | OFN_EXPLORER;
	if(dwFlags & OFN_ENABLETEMPLATE)
		m_ofn.Flags &= ~OFN_ENABLESIZING;
	m_ofn.hInstance = AfxGetResourceHandle();
	m_ofn.lpfnHook = (COMMDLGPROC)_AfxCommDlgProc;

	// setup initial file name
	if (lpszFileName != NULL)
		Checked::tcsncpy_s(m_szFileName, _countof(m_szFileName), lpszFileName, _TRUNCATE);

	// Translate filter into commdlg format (lots of \0)
	if (lpszFilter != NULL)
	{
		m_strFilter = lpszFilter;
		LPTSTR pch = m_strFilter.GetBuffer(0); // modify the buffer in place
		// MFC delimits with '|' not '\0'
		while ((pch = _tcschr(pch, '|')) != NULL)
			*pch++ = '\0';
		m_ofn.lpstrFilter = m_strFilter;
		// do not call ReleaseBuffer() since the string contains '\0' characters
	}
}

CFileDialog::~CFileDialog()
{
	free(m_pOFN);
}

const OPENFILENAME& CFileDialog::GetOFN() const
{
	return *m_pOFN;
}

OPENFILENAME& CFileDialog::GetOFN()
{
	return *m_pOFN;
}

INT_PTR CFileDialog::DoModal()
{
	ASSERT_VALID(this);
	ASSERT(m_ofn.Flags & OFN_ENABLEHOOK);
	ASSERT(m_ofn.lpfnHook != NULL); // can still be a user hook

	// zero out the file buffer for consistent parsing later
	ASSERT(AfxIsValidAddress(m_ofn.lpstrFile, m_ofn.nMaxFile));
	DWORD nOffset = lstrlen(m_ofn.lpstrFile)+1;
	ASSERT(nOffset <= m_ofn.nMaxFile);
	memset(m_ofn.lpstrFile+nOffset, 0, (m_ofn.nMaxFile-nOffset)*sizeof(TCHAR));

	//  This is a special case for the file open/save dialog,
	//  which sometimes pumps while it is coming up but before it has
	//  disabled the main window.
	HWND hWndFocus = ::GetFocus();
	BOOL bEnableParent = FALSE;
	m_ofn.hwndOwner = PreModal();
	AfxUnhookWindowCreate();
	if (m_ofn.hwndOwner != NULL && ::IsWindowEnabled(m_ofn.hwndOwner))
	{
		bEnableParent = TRUE;
		::EnableWindow(m_ofn.hwndOwner, FALSE);
	}

	_AFX_THREAD_STATE* pThreadState = AfxGetThreadState();
	ASSERT(pThreadState->m_pAlternateWndInit == NULL);

	if (m_ofn.Flags & OFN_EXPLORER)
		pThreadState->m_pAlternateWndInit = this;
	else
		AfxHookWindowCreate(this);

	INT_PTR nResult;
	if (m_bOpenFileDialog)
		nResult = ::AfxCtxGetOpenFileName(&m_ofn);
	else
		nResult = ::AfxCtxGetSaveFileName(&m_ofn);

	if (nResult)
		ASSERT(pThreadState->m_pAlternateWndInit == NULL);
	pThreadState->m_pAlternateWndInit = NULL;

	// Second part of special case for file open/save dialog.
	if (bEnableParent)
		::EnableWindow(m_ofn.hwndOwner, TRUE);
	if (::IsWindow(hWndFocus))
		::SetFocus(hWndFocus);

	PostModal();
	return nResult ? nResult : IDCANCEL;
}

CString CFileDialog::GetPathName() const
{
	if ((m_ofn.Flags & OFN_EXPLORER) && m_hWnd != NULL)
	{
		ASSERT(::IsWindow(m_hWnd));
		CString strResult;
		if (GetParent()->SendMessage(CDM_GETSPEC, (WPARAM)MAX_PATH,
			(LPARAM)strResult.GetBuffer(MAX_PATH)) < 0)
		{
			strResult.Empty();
		}
		else
		{
			strResult.ReleaseBuffer();
		}

		if (!strResult.IsEmpty())
		{
			if (GetParent()->SendMessage(CDM_GETFILEPATH, (WPARAM)MAX_PATH,
				(LPARAM)strResult.GetBuffer(MAX_PATH)) < 0)
				strResult.Empty();
			else
			{
				strResult.ReleaseBuffer();
				return strResult;
			}
		}
	}
	return m_ofn.lpstrFile;
}

CString CFileDialog::GetFileName() const
{
	if ((m_ofn.Flags & OFN_EXPLORER) && m_hWnd != NULL)
	{
		ASSERT(::IsWindow(m_hWnd));
		CString strResult;
		if (GetParent()->SendMessage(CDM_GETSPEC, (WPARAM)MAX_PATH,
			(LPARAM)strResult.GetBuffer(MAX_PATH)) < 0)
		{
			strResult.Empty();
		}
		else
		{
			strResult.ReleaseBuffer();
			return strResult;
		}
	}
	return m_ofn.lpstrFileTitle;
}

CString CFileDialog::GetFileExt() const
{
	if ((m_ofn.Flags & OFN_EXPLORER) && m_hWnd != NULL)
	{
		ASSERT(::IsWindow(m_hWnd));
		CString strResult;
		LPTSTR pszResult = strResult.GetBuffer(MAX_PATH);
		LRESULT nResult = GetParent()->SendMessage(CDM_GETSPEC, MAX_PATH,
			reinterpret_cast<LPARAM>(pszResult));
		strResult.ReleaseBuffer();
		if (nResult >= 0)
		{
			LPTSTR pszExtension = ::PathFindExtension(strResult);
			if (pszExtension != NULL && *pszExtension == _T('.'))
			{
				return pszExtension+1;
			}
		}

		strResult.Empty();
		return strResult;
	}

	if (m_pofnTemp != NULL)
		if (m_pofnTemp->nFileExtension == 0)
			return _T("");
		else
			return m_pofnTemp->lpstrFile + m_pofnTemp->nFileExtension;

	if (m_ofn.nFileExtension == 0)
		return _T("");
	else
		return m_ofn.lpstrFile + m_ofn.nFileExtension;
}

CString CFileDialog::GetFileTitle() const
{
	CString strResult = GetFileName();
	LPTSTR pszBuffer = strResult.GetBuffer();
	::PathRemoveExtension(pszBuffer);
	strResult.ReleaseBuffer();
	return strResult;
}

CString CFileDialog::GetNextPathName(POSITION& pos) const
{
	BOOL bExplorer = m_ofn.Flags & OFN_EXPLORER;
	TCHAR chDelimiter;
	if (bExplorer)
		chDelimiter = '\0';
	else
		chDelimiter = ' ';

	LPTSTR lpsz = (LPTSTR)pos;
	if (lpsz == m_ofn.lpstrFile) // first time
	{
		if ((m_ofn.Flags & OFN_ALLOWMULTISELECT) == 0)
		{
			pos = NULL;
			return m_ofn.lpstrFile;
		}

		// find char pos after first Delimiter
		while(*lpsz != chDelimiter && *lpsz != '\0')
			lpsz = _tcsinc(lpsz);
		lpsz = _tcsinc(lpsz);

		// if single selection then return only selection
		if (*lpsz == 0)
		{
			pos = NULL;
			return m_ofn.lpstrFile;
		}
	}

	CString strBasePath = m_ofn.lpstrFile;
	if (!bExplorer)
	{
		LPTSTR lpszPath = m_ofn.lpstrFile;
		while(*lpszPath != chDelimiter)
			lpszPath = _tcsinc(lpszPath);
		strBasePath = strBasePath.Left(int(lpszPath - m_ofn.lpstrFile));
	}

	LPTSTR lpszFileName = lpsz;
	CString strFileName = lpsz;

	// find char pos at next Delimiter
	while(*lpsz != chDelimiter && *lpsz != '\0')
		lpsz = _tcsinc(lpsz);

	if (!bExplorer && *lpsz == '\0')
		pos = NULL;
	else
	{
		if (!bExplorer)
			strFileName = strFileName.Left(int(lpsz - lpszFileName));

		lpsz = _tcsinc(lpsz);
		if (*lpsz == '\0') // if double terminated then done
			pos = NULL;
		else
			pos = (POSITION)lpsz;
	}

	TCHAR strDrive[_MAX_DRIVE], strDir[_MAX_DIR], strName[_MAX_FNAME], strExt[_MAX_EXT];
	Checked::tsplitpath_s(strFileName, strDrive, _MAX_DRIVE, strDir, _MAX_DIR, strName, _MAX_FNAME, strExt, _MAX_EXT);
	TCHAR strPath[_MAX_PATH];
	if (*strDrive || *strDir)
	{
		Checked::tcscpy_s(strPath, _countof(strPath), strFileName);
	}
	else
	{
		Checked::tsplitpath_s(strBasePath+_T("\\"), strDrive, _MAX_DRIVE, strDir, _MAX_DIR, NULL, 0, NULL, 0);
		Checked::tmakepath_s(strPath, _MAX_PATH, strDrive, strDir, strName, strExt);
	}
	
	return strPath;
}

void CFileDialog::SetTemplate(LPCTSTR lpWin3ID, LPCTSTR lpWin4ID)
{
	if (m_ofn.Flags & OFN_EXPLORER)
		m_ofn.lpTemplateName = lpWin4ID;
	else
		m_ofn.lpTemplateName = lpWin3ID;
	m_ofn.Flags |= OFN_ENABLETEMPLATE;
}

CString CFileDialog::GetFolderPath() const
{
	ASSERT(::IsWindow(m_hWnd));
	ASSERT(m_ofn.Flags & OFN_EXPLORER);

	CString strResult;
	if (GetParent()->SendMessage(CDM_GETFOLDERPATH, (WPARAM)MAX_PATH, (LPARAM)strResult.GetBuffer(MAX_PATH)) < 0)
		strResult.Empty();
	else
		strResult.ReleaseBuffer();
	return strResult;
}

#ifdef UNICODE
void CFileDialog::SetControlText(int nID, const wchar_t  *lpsz)
{
	ASSERT(::IsWindow(m_hWnd));
	ASSERT(m_ofn.Flags & OFN_EXPLORER);
	GetParent()->SendMessage(CDM_SETCONTROLTEXT, (WPARAM)nID, (LPARAM)lpsz);
}
#endif 
void CFileDialog::SetControlText(int nID, LPCSTR lpsz)
{
	ASSERT(::IsWindow(m_hWnd));
	ASSERT(m_ofn.Flags & OFN_EXPLORER);

#ifdef UNICODE
	 
	CStringW dest(lpsz);
	GetParent()->SendMessage(CDM_SETCONTROLTEXT, (WPARAM)nID, (LPARAM)dest.GetString());

#else
	GetParent()->SendMessage(CDM_SETCONTROLTEXT, (WPARAM)nID, (LPARAM)lpsz);
#endif 
}

void CFileDialog::HideControl(int nID)
{
	ASSERT(::IsWindow(m_hWnd));
	ASSERT(m_ofn.Flags & OFN_EXPLORER);
	GetParent()->SendMessage(CDM_HIDECONTROL, (WPARAM)nID, 0);
}

void CFileDialog::SetDefExt(LPCSTR lpsz)
{
	ASSERT(::IsWindow(m_hWnd));
	ASSERT(m_ofn.Flags & OFN_EXPLORER);
	GetParent()->SendMessage(CDM_SETDEFEXT, 0, (LPARAM)lpsz);
}

UINT CFileDialog::OnShareViolation(LPCTSTR)
{
	ASSERT_VALID(this);

	// Do not call Default() if you override
	return OFN_SHAREWARN; // default
}

BOOL CFileDialog::OnFileNameOK()
{
	ASSERT_VALID(this);

	// Do not call Default() if you override
	return FALSE;
}

void CFileDialog::OnLBSelChangedNotify(UINT, UINT, UINT)
{
	ASSERT_VALID(this);

	// Do not call Default() if you override
	// no default processing needed
}

void CFileDialog::OnInitDone()
{
	ASSERT_VALID(this);
	GetParent()->CenterWindow();

	// Do not call Default() if you override
	// no default processing needed
}

void CFileDialog::OnFileNameChange()
{
	ASSERT_VALID(this);

	// Do not call Default() if you override
	// no default processing needed
}

void CFileDialog::OnFolderChange()
{
	ASSERT_VALID(this);

	// Do not call Default() if you override
	// no default processing needed
}

void CFileDialog::OnTypeChange()
{
	ASSERT_VALID(this);

	// Do not call Default() if you override
	// no default processing needed
}

BOOL CFileDialog::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	ASSERT(pResult != NULL);

	// allow message map to override
	if (CCommonDialog::OnNotify(wParam, lParam, pResult))
		return TRUE;

	OFNOTIFY* pNotify = (OFNOTIFY*)lParam;
	switch(pNotify->hdr.code)
	{
	case CDN_INITDONE:
		OnInitDone();
		return TRUE;
	case CDN_SELCHANGE:
		OnFileNameChange();
		return TRUE;
	case CDN_FOLDERCHANGE:
		OnFolderChange();
		return TRUE;
	case CDN_SHAREVIOLATION:
		*pResult = OnShareViolation(pNotify->pszFile);
		return TRUE;
	case CDN_HELP:
		if (!SendMessage(WM_COMMAND, ID_HELP))
			SendMessage(WM_COMMANDHELP, 0, 0);
		return TRUE;
	case CDN_FILEOK:
		*pResult = OnFileNameOK();
		return TRUE;
	case CDN_TYPECHANGE:
		OnTypeChange();
		return TRUE;
	}

	return FALSE;   // not handled
}

////////////////////////////////////////////////////////////////////////////
// CFileDialog diagnostics

#ifdef _DEBUG
void CFileDialog::Dump(CDumpContext& dc) const
{
	CDialog::Dump(dc);

	if (m_bOpenFileDialog)
		dc << "File open dialog";
	else
		dc << "File save dialog";
	dc << "\nm_ofn.hwndOwner = " << (void*)m_ofn.hwndOwner;
	dc << "\nm_ofn.nFilterIndex = " << m_ofn.nFilterIndex;
	dc << "\nm_ofn.lpstrFile = " << m_ofn.lpstrFile;
	dc << "\nm_ofn.nMaxFile = " << m_ofn.nMaxFile;
	dc << "\nm_ofn.lpstrFileTitle = " << m_ofn.lpstrFileTitle;
	dc << "\nm_ofn.nMaxFileTitle = " << m_ofn.nMaxFileTitle;
	dc << "\nm_ofn.lpstrTitle = " << m_ofn.lpstrTitle;
	dc << "\nm_ofn.Flags = ";
	dc.DumpAsHex(m_ofn.Flags);
	dc << "\nm_ofn.lpstrDefExt = " << m_ofn.lpstrDefExt;
	dc << "\nm_ofn.nFileOffset = " << m_ofn.nFileOffset;
	dc << "\nm_ofn.nFileExtension = " << m_ofn.nFileExtension;

	dc << "\nm_ofn.lpstrFilter = ";
	LPCTSTR lpstrItem = m_ofn.lpstrFilter;
	LPTSTR lpszBreak = _T("|");

	while (lpstrItem != NULL && *lpstrItem != '\0')
	{
		dc << lpstrItem << lpszBreak;
		lpstrItem += lstrlen(lpstrItem) + 1;
	}
	if (lpstrItem != NULL)
		dc << lpszBreak;

	dc << "\nm_ofn.lpstrCustomFilter = ";
	lpstrItem = m_ofn.lpstrCustomFilter;
	while (lpstrItem != NULL && *lpstrItem != '\0')
	{
		dc << lpstrItem << lpszBreak;
		lpstrItem += lstrlen(lpstrItem) + 1;
	}
	if (lpstrItem != NULL)
		dc << lpszBreak;

	if (m_ofn.lpfnHook == (COMMDLGPROC)_AfxCommDlgProc)
		dc << "\nhook function set to standard MFC hook function";
	else
		dc << "\nhook function set to non-standard hook function";

	dc << "\n";
}
#endif //_DEBUG


IMPLEMENT_DYNAMIC(CFileDialog, CDialog)

////////////////////////////////////////////////////////////////////////////
