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
using namespace tj::show;

void Application::LoadDefaultSettings() {
	_settings->SetFlag(L"debug", false);
	_settings->SetValue(L"locale", L"en");

	// Controller
	_settings->SetValue(L"controller.tick-length", L"30");
	_settings->SetValue(L"controller.min-tick-length", L"5");

	// Networking
	_settings->SetValue(L"net.address", L"224.0.0.2");
	_settings->SetValue(L"net.port", L"7959");
	_settings->SetValue(L"net.announce-period", L"10001");
	_settings->SetValue(L"net.web.port", L"7960");
	_settings->SetValue(L"net.web.resources.path", L"/_resources");
	_settings->SetValue(L"net.web.dashboard.path", L"/_dashboard");
	_settings->SetFlag(L"net.web.advertise-resources", true); // If true, this server will advertise resources on the network
	_settings->SetFlag(L"net.server.try-become-primary", true); // If true, a server will always try to become the primary server
	_settings->SetFlag(L"net.ep.enabled", true); // If true, starts an EP server for the TJShow remote

	// View
	_settings->SetValue(L"view.theme", L"0");
	_settings->SetValue(L"view.min-track-height", L"19");
	_settings->SetFlag(L"view.toolbar.show", true);
	_settings->SetFlag(L"view.tooltips", true);
	_settings->SetFlag(L"view.save-prompt", true);
	_settings->SetFlag(L"view.deny-exit-when-playing", true);
	_settings->SetValue(L"view.video.fps", L"25");
	_settings->SetValue(L"view.video.fx", L"player.fx");
	_settings->SetValue(L"view.video.max-texture-width", L"1024");
	_settings->SetValue(L"view.video.max-texture-height", L"1024");

	_settings->SetFlag(L"view.video.default-screen-definition.is-touch", false);
	_settings->SetValue(L"view.video.default-screen-definition.document-width", L"1024");
	_settings->SetValue(L"view.video.default-screen-definition.document-height", L"768");
	_settings->SetValue(L"view.video.default-screen-definition.scale", L"all");
	_settings->SetValue(L"view.video.default-screen-definition.background-color", L"000000");

	// Warnings
	_settings->SetFlag(L"warnings.no-addressed-client", true);
	_settings->SetFlag(L"warnings.resources.missing",true);
	_settings->SetFlag(L"view.show-notifications", true);
	_settings->SetFlag(L"view.enable-animations", true);
}