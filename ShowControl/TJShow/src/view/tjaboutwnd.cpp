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
#include "../../include/internal/view/tjaboutwnd.h"
using namespace tj::show;
using namespace tj::show::view;
using namespace tj::shared::graphics;

Copyright KCRTinyXML(L"TJShow", L"TinyXML", L"© 2000-2002 Lee Thomason (www.grinninglizard.com)");
Copyright KCRScintilla(L"TJShow", L"Scintilla", L"© 1998-2003 by Neil Hodgson <neilh@scintilla.org>");
Copyright KCRIcons(L"TJShow", L"The famfamfam icon set", L"© Mark James (www.famfamfam.com) licensed under the Creative Commons Attribution 2.5-license.");

const int AboutWnd::KParticleCount = 100;
const int AboutWnd::KTimerID = 1;

const wchar_t* KAboutText = L"TJShow v4.0 (trunk) © Tommy van der Vorst, Pixelspark, 2005-2017."
							L"Based on ideas by Tommy van der Vorst and Joost Wijgers ('TJ'). This computer program is protected by copyright law and international treaties. Unauthorized reproduction or distribution of this program, or any portion of it, may result in severe civil and criminal penalties, and will be prosecuted to the maximum extent possible under the law.\r\n\r\n";

AboutWnd::AboutWnd(): ChildWnd(L""), _logo(L"icons/splash/logo.png") {
	for(int a=0;a<KParticleCount;a++) {
		Particle p;
		p._x = Util::RandomFloat();
		p._y = Util::RandomFloat();
		p._dir = (Util::RandomFloat()-0.5f)*3.214159f; // only forward
		p._size = Util::RandomFloat();
		p._speed = Util::RandomFloat()/2000.0f;
		p._color = RGBColor(1.0f, Util::RandomFloat(), Util::RandomFloat());
		_particles.push_back(p);
	}

	StartTimer(Time(30), KTimerID);
}

AboutWnd::~AboutWnd() {
}

void AboutWnd::Layout() {
}

void AboutWnd::OnSize(const tj::shared::Area &ns) {
}

void AboutWnd::OnTimer(unsigned int id) {
	if(id==KTimerID) {
		std::vector< Particle >::iterator it = _particles.begin();
		while(it!=_particles.end()) {
			Particle& p = *it;
			p._x += cos(p._dir)*p._speed;
			p._y += sin(p._dir)*p._speed;
			++it;
		}
		Repaint();
	}	
}

void AboutWnd::Paint(Graphics& g, strong<Theme> theme) {
	Area rc = GetClientArea();
	g.Clear(theme->GetColor(Theme::ColorBackground));

	GraphicsPath path;
	PointF points[3] = {PointF(0.0f, 25.0f), PointF(0.0f, -25.0f), PointF(45.0f, 0.0f)};
	path.AddPolygon(points, 3);

	SolidBrush disabled(theme->GetColor(Theme::ColorDisabledOverlay));

	// Draw particles
	float scale = (float)min(rc.GetWidth(), rc.GetHeight());
	std::vector< Particle >::iterator it = _particles.begin();
	while(it!=_particles.end()) {
		Particle& p = *it;
		float x = fmod(p._x, 1.0f)*rc.GetWidth();
		float y = fmod(p._y, 1.0f) * rc.GetHeight(); /* *sin(p._x*2.0f*3.14159f)*0.5f* rc.GetHeight()) + rc.GetHeight()/2 */
		Pixels s = Pixels(p._size*scale)/2;

		LinearGradientBrush particleBrush(PointF(float(0.0f), float(0.0f)), PointF(float(rc.GetWidth()), float(rc.GetHeight())), p._color, Theme::ChangeAlpha(p._color, int((1.0f-p._size)*255)));
		Pen pn(&particleBrush, 2.0f);

		GraphicsContainer gc = g.BeginContainer();
		g.TranslateTransform(float(x),float(y));
		g.RotateTransform(float(p._x)*45.0f);
		//g.ScaleTransform(float(s), float(s));
		
		g.FillPath(&disabled, &path);
		g.DrawPath(&pn, &path);
		g.EndContainer(gc);

		++it;
	}

	
	Area disabledRc = rc;
	disabledRc.Narrow(0,35,0,0);
	theme->DrawInsetRectangle(g, disabledRc);
	g.FillRectangle(&disabled, rc);

	// Draw text info
	StringFormat sf;
	sf.SetAlignment(StringAlignmentNear);

	SolidBrush tbr(theme->GetColor(Theme::ColorText));
	std::wstring name = TL(application_name) + std::wstring(L" ") +Version::GetRevisionName()+ std::wstring(L" ") + std::wstring(L" (") + Version::GetRevisionDate() + std::wstring(L")");
	g.DrawString(name.c_str(), (int)name.length(), theme->GetGUIFontBold(), PointF(20.0f, 40.0f), &sf, &tbr);
	//g.DrawImage(_logo, PointF(0.0f, 0.0f));

	Area largeRC(rc.GetLeft()+20, rc.GetTop(), rc.GetWidth(), 30);
	std::wstring productName = TL(application_name);
	LinearGradientBrush ltbr(PointF(0.0f, 0.0f), PointF(0.0f, 30.0f), Color(1.0, 0.0, 0.0), Color(0.5, 0.0, 0.0));
	std::wstring uiFontName = theme->GetGUIFontName();
	Font ltf(uiFontName.c_str(), 30.0f, FontStyleRegular);
	g.DrawString(productName.c_str(), (int)productName.length(), &ltf, largeRC, &sf, &ltbr);

	std::wstring about = TL(application_about);

	std::wstring aboutText = KAboutText + Copyrights::Dump();
	rc.Narrow(20, 70, 10, 10);
	g.DrawString(aboutText.c_str(), (int)aboutText.length(), theme->GetGUIFont(), rc, &sf, &tbr);
}