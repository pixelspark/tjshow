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
 
 #include "../include/tjsettings.h"
#include "../include/tjthread.h"
#include "../include/tjutil.h"
#include "../include/tjlog.h"
#include "../include/tjfile.h"

#ifdef TJ_OS_WIN
	#include <shlwapi.h>
	#include <shlobj.h>
#endif

using namespace tj::shared;

class SettingsNamespace: public Settings {
	public:
		SettingsNamespace(ref<Settings> st, const String& ns): _settings(st), _ns(ns+L".") {
			assert(_settings);
		}

		virtual ~SettingsNamespace() {
		}

		virtual bool GetFlag(const String& key) const {
			return _settings->GetFlag(_ns+key);
		}

		virtual bool GetFlag(const String& key, bool def) const {
			return _settings->GetFlag(_ns+key, def);
		}

		virtual String GetValue(const String& key) const {
			return _settings->GetValue(_ns+key);
		}

		virtual void SetValue(const String& key, const String& value) {
			_settings->SetValue(_ns+key, value);
		}

		virtual void SetFlag(const String& key, bool t) {
			_settings->SetFlag(_ns+key, t);
		}

		virtual ref<Settings> GetNamespace(const String& ns) {
			return GC::Hold(new SettingsNamespace(_settings, _ns+ns));
		}

		virtual String GetValue(const String& key, const String& defaultValue) const {
			return _settings->GetValue(_ns+key, defaultValue);
		}

	protected:
		ref<Settings> _settings;
		String _ns;
};

Settings::~Settings() {
}

SettingsStorage::SettingsStorage() {
}

SettingsStorage::~SettingsStorage() {
}

void SettingsStorage::Load(TiXmlElement* you) {
	TiXmlElement* pref = you->FirstChildElement("pref");
	while(pref!=0) {
		const char* aKey = pref->Attribute("key");
		if(aKey!=0) {
			TiXmlNode* textNode = pref->FirstChild();
			if(textNode!=0) {
				const char* aValue = textNode->Value();
				if(aValue!=0) {
					_data[Wcs(aKey)] = Wcs(aValue);
				}
			}
		}
		pref = pref->NextSiblingElement("pref");
	}
}

void SettingsStorage::Save(TiXmlElement* parent) const {
	std::map< String, String >::const_iterator it = _data.begin();
	while(it!=_data.end()) {
		TiXmlElement pref("pref");
		pref.SetAttribute("key", Mbs(it->first).c_str());

		TiXmlText text(Mbs(it->second).c_str());
		pref.InsertEndChild(text);
		parent->InsertEndChild(pref);
		++it;
	}
}

void SettingsStorage::SaveFile(const String& path) const {
	TiXmlDocument doc;

	TiXmlElement root("settings");
	Save(&root);
	doc.InsertEndChild(root);

	doc.SaveFile(Mbs(path).c_str());
}

void SettingsStorage::LoadFile(const String& path) {
	ThreadLock lock(&_lock);
	TiXmlDocument doc;
	doc.LoadFile(Mbs(path).c_str());

	if(!doc.Error()) {
		TiXmlElement* root = doc.FirstChildElement();
		if(root!=0) {
			Load(root);
		}
	}
	else {
		// If we could not load, well... bummer. Just use the default values
		Log::Write(L"TJShared/SettingsStorage", L"Could not load XML SettingsStorage file ("+path+L")");
	}
}

ref<Settings> SettingsStorage::GetNamespace(const String& ns) {
	return GC::Hold(new SettingsNamespace(this, ns));
}

void SettingsStorage::SetValue(const String& key, const String& value) {
	ThreadLock lock(&_lock);
	_data[key] = value;
}

void SettingsStorage::SetFlag(const String& key, bool t) {
	if(t) {
		SetValue(key, String(L"yes"));
	}
	else {
		SetValue(key, String(L"no"));
	}
}

bool SettingsStorage::GetFlag(const String& key) const {
	std::map< String, String >::const_iterator it = _data.find(key);
	if(it!=_data.end()) {
		return it->second == String(L"yes");
	}
	
	Throw(L"SettingsStorage flag "+key+L" does not exist", ExceptionTypeWarning);
}

bool SettingsStorage::GetFlag(const String& key, bool def) const {
	std::map< String, String >::const_iterator it = _data.find(key);
	if(it!=_data.end()) {
		return it->second == String(L"yes");
	}

	return def;
}

String SettingsStorage::GetValue(const String& key, const String& defaultValue) const {
	std::map< String, String >::const_iterator it = _data.find(key);
	if(it!=_data.end()) {
		return it->second;
	}

	return defaultValue;
}

String SettingsStorage::GetValue(const String &key) const {
	std::map< String, String >::const_iterator it = _data.find(key);
	if(it!=_data.end()) {
		return it->second;
	}
	
	Throw(L"SettingsStorage value "+key+L" does not exist", ExceptionTypeWarning);
}

#ifdef TJ_OS_WIN
/* Creates the path to a user-specific settings file.
- On Windows: %USERPROFILE%\Application Data\TJ\TJShow\file.xml
- On Unices, this would probably be something like /home/%USER%/.tj/tjshow/file.xml */
String SettingsStorage::GetSettingsPath(const String& vendor, const String& app, const String& file) {
	std::string suffix = "\\" + Mbs(vendor) + "\\" + Mbs(app) + "\\";
	char buffer[MAX_PATH+2];
	SHGetSpecialFolderPathA(NULL, buffer, CSIDL_APPDATA, TRUE);
	SHCreateDirectoryExA(NULL, std::string(std::string(buffer)+suffix).c_str(),NULL);

	return Wcs(std::string(buffer) + suffix + Mbs(file) + ".xml");
}

String SettingsStorage::GetSystemSettingsPath(const String& vendor, const String& app, const String& file) {
	std::string suffix = "\\" + Mbs(vendor) + "\\" + Mbs(app) + "\\";
	char buffer[MAX_PATH+2];
	SHGetSpecialFolderPathA(NULL, buffer, CSIDL_COMMON_APPDATA, TRUE);
	SHCreateDirectoryExA(NULL, std::string(std::string(buffer)+suffix).c_str(),NULL);
	
	return Wcs(std::string(buffer) + suffix + Mbs(file) + ".xml");
}
#endif

#ifdef TJ_OS_LINUX
String SettingsStorage::GetSettingsPath(const String& vendor, const String& app, const String& file) {
	std::wostringstream wos;
	wchar_t sep = File::GetPathSeparator();
	String lowVendor = vendor;
	Util::StringToLower(lowVendor);
	String lowApp = app;
	Util::StringToLower(lowApp);
	String lowFile = file;
	Util::StringToLower(lowFile);
	
	wos << Wcs(std::string(getenv("HOME"))) << sep << L'.' << lowVendor << sep << lowApp << sep << lowFile << L".xml";
	return wos.str();
}

String SettingsStorage::GetSystemSettingsPath(const String& vendor, const String& app, const String& file) {
	std::wostringstream wos;
	wchar_t sep = File::GetPathSeparator();
	String lowVendor = vendor;
	Util::StringToLower(lowVendor);
	String lowApp = app;
	Util::StringToLower(lowApp);
	String lowFile = file;
	Util::StringToLower(lowFile);
	
	wos << sep << L"etc" << sep << lowVendor << sep << lowApp << sep << lowFile << L".xml";
	return wos.str();
}
#endif
