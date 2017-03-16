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
#include "../../include/internal/tjshow.h"
#include "../../include/internal/view/tjtricks.h"
using namespace tj::shared;
using namespace tj::show;
using namespace tj::show::view;

ref<ChoiceWnd> Tricks::LoadTricksFromFile(const ResourceIdentifier& rid) {
	std::wstring xmlPath;
	if(ResourceManager::Instance()->GetPathToLocalResource(rid, xmlPath)) {
		std::string xmlPathMbs = Mbs(xmlPath);
		TiXmlDocument doc;
		doc.LoadFile(xmlPathMbs.c_str());
		TiXmlElement* helpElement = doc.RootElement();
		if(helpElement!=0) {
			TiXmlElement* tricksElement = helpElement->FirstChildElement("tricks");
			if(tricksElement!=0) {
				ref<ChoiceWnd> tricks = GC::Hold(new ChoiceWnd());
				tricks->SetTitle(TL(tricks));

				TiXmlElement* trickElement = tricksElement->FirstChildElement("trick");
				while(trickElement!=0) {
					std::wstring title = LoadAttributeSmall<std::wstring>(trickElement, "title", L"");
					std::wstring text = LoadAttributeSmall<std::wstring>(trickElement, "text", L"");
					std::wstring image = LoadAttributeSmall<std::wstring>(trickElement, "image", L"");
					ref<Choice> choice = GC::Hold(new Choice(Language::Get(title), Language::Get(text), image));
					tricks->AddChoice(choice);

					trickElement = trickElement->NextSiblingElement("trick");
				}

				return tricks;
			}
		}
	}

	return null;
}