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
#include "../include/internal/tjdashboardserver.h"
#include "../include/internal/tjdashboard.h"

using namespace tj::show;
using namespace tj::np;

std::ostream& operator<< (std::ostream& out, const TiXmlNode& doc) {
	TiXmlPrinter printer;
	doc.Accept(&printer);
	out << printer.Str();
	return out;
}

DashboardWebItem::DashboardWebItem(ref<Model> model): WebItemResource(L"text/xml"),  _model(model) {
}

DashboardWebItem::~DashboardWebItem() {
}

Resolution DashboardWebItem::Get(ref<WebRequest> frq, std::wstring& error, char** data, Bytes& dataLength) {
	if(!_model) {
		error = L"No model in DashboardRequestResolver!";
		return ResolutionNone;
	}

	strong<Dashboard> db = _model->GetDashboard();
	TiXmlDocument doc;
	TiXmlDeclaration decl("1.0", "", "no");
	doc.InsertEndChild(decl);
	TiXmlElement dashboardElement("dashboard");
	db->Save(&dashboardElement);
	doc.InsertEndChild(dashboardElement);

	std::ostringstream xos;
	xos << doc;
	std::string dataString = xos.str();

	dataLength = dataString.length();
	char* nd = new char[(unsigned int)(dataLength+2)];
	strcpy_s(nd, (size_t)(dataLength+1), dataString.c_str());
	*data = nd;

	return ResolutionData;
}