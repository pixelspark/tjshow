/* This file is part of TJShow. TJShow is free software: you 
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
 
 #include "../include/tjutil.h"
using namespace tj::shared;

Arguments::Arguments(const wchar_t* cmdLine) {
    m_cmdline = new wchar_t[wcslen (cmdLine) + 2];
    if(m_cmdline) {
		#ifdef TJ_OS_WIN
			wcscpy_s(m_cmdline, wcslen(cmdLine)+1, cmdLine);
		#endif
		
		#ifdef TJ_OS_POSIX
			wcsncpy(m_cmdline, cmdLine, wcslen(cmdLine)+1);
		#endif
        Parse();
    }
}

Arguments::~Arguments() {
    delete m_cmdline;
}

bool Arguments::IsSet(const String& option) {
	std::vector<wchar_t*>::iterator it = _options.begin();
	while(it!=_options.end()) {
		wchar_t* str = *it;
		if(String(str)==option) {
			return true;
		}
		++it;
	}
	return false;
}

 ////////////////////////////////////////////////////////////////////////////////
    // Parse m_cmdline into individual tokens, which are delimited by spaces. If a
    // token begins with a quote, then that token is terminated by the next quote
    // followed immediately by a space or terminator.  This allows tokens to contain
    // spaces.
    // This input string:     This "is" a ""test"" "of the parsing" alg"o"rithm.
    // Produces these tokens: This, is, a, "test", of the parsing, alg"o"rithm
    ////////////////////////////////////////////////////////////////////////////////A
void Arguments::Parse() {
	enum { TERM  = L'\0', QUOTE = L'\"' };

    bool bInQuotes = false;
    wchar_t* pargs = m_cmdline;
	
	while (*pargs) {
		while (isspace (*pargs)) {       // skip leading whitespace
                pargs++;
		}

        bInQuotes = (*pargs == QUOTE);  // see if this token is quoted

		if (bInQuotes) {                 // skip leading quote
			pargs++;
		}

		_options.push_back (pargs);              // store position of current token

		// Find next token.
		// NOTE: Args are normally terminated by whitespace, unless the
		// arg is quoted.  That's why we handle the two cases separately,
		// even though they are very similar.
		if(bInQuotes) {
		// find next quote followed by a space or terminator
			while(*pargs && !(*pargs == QUOTE && (isspace (pargs[1]) || pargs[1] == TERM))) {
				pargs++;
			}
		
			if(*pargs) {
				*pargs = TERM;  // terminate token
				if(pargs[1]) {   // if quoted token not followed by a terminator
					pargs += 2; // advance to next token
				}
			}
		}
		else {
			// skip to next non-whitespace character
			while (*pargs && !isspace (*pargs)) {
				pargs++;
			}
			if (*pargs && isspace (*pargs)) {
				*pargs = TERM;    // terminate token
				pargs++;         // advance to next token or terminator
			}
        }
    }
}

std::vector<wchar_t*>* Arguments::GetOptions() {
	return &_options;
}
