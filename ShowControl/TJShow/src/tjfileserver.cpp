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
#include "../include/internal/tjfileserver.h"

using namespace tj::np;
using namespace tj::show;

/** ResourcesRequestResolver **/
ResourcesRequestResolver::ResourcesRequestResolver(): WebItemResource(L"") {
}

ResourcesRequestResolver::~ResourcesRequestResolver() {
}

Resolution ResourcesRequestResolver::Get(ref<WebRequest> frq, String& error, char** data, Bytes& length) {
	if(frq.IsCastableTo<HTTPRequest>()) {
		ref<HTTPRequest> hrp = frq;
		std::wstring channelString = hrp->GetParameter("key", L"");
		SecurityToken st = StringTo<SecurityToken>(channelString, 0);
		std::wstring rid = hrp->GetParameter("rid", L"");

		// Check if this request is authorized
		ref<Authorizer> auth = Application::Instance()->GetNetwork()->GetAuthorizer();
		if(auth && auth->CheckToken(rid, st)) {
			strong<ResourceProvider> showResources = Application::Instance()->GetModel()->GetResourceManager();

			Log::Write(L"TJShow/FileServer/ResponseThread", L"Serve resource (authorized): rid="+rid);
			if(!showResources->GetPathToLocalResource(rid, error)) {
				error = L"Couldn't find resource to serve: rid="+rid;
				return ResolutionNotFound;
			}
			else {
				return ResolutionFile;
			}
		}
		else {
			error = L"Cannot serve this file, unauthorized!";
			return ResolutionNone;
		}
	}
	else {
		error = L"Resources can only be served in response to an HTTP request";
		return ResolutionNone;
	}
}