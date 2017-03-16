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
#include "../include/tjtransfer.h"
using namespace tj::transfer;

TransferPlugin::TransferPlugin() {
}

TransferPlugin::~TransferPlugin() {
}

std::wstring TransferPlugin::GetName() const {
	return L"Transfer";
}

ref<Track> TransferPlugin::CreateTrack(ref<Playback> pb) {
	return GC::Hold(new TransferTrack(this));
}

ref<StreamPlayer> TransferPlugin::CreateStreamPlayer(ref<Playback> pb, ref<Talkback> talk) {
	return GC::Hold(new TransferStreamPlayer(this, talk));
}

std::wstring TransferPlugin::GetFriendlyName() const {
	return TL(transfer_plugin_friendly_name);
}

std::wstring TransferPlugin::GetFriendlyCategory() const {
	return TL(other_category);	
}

std::wstring TransferPlugin::GetDescription() const {
	return TL(transfer_plugin_description);
}

std::wstring TransferPlugin::GetVersion() const {
	std::wostringstream os;
	os << __DATE__ << " @ " << __TIME__;
	#ifdef UNICODE
	os << L" Unicode";
	#endif

	#ifdef NDEBUG
	os << " Release";
	#endif

	return os.str();
}

std::wstring TransferPlugin::GetAuthor() const {
	return L"Tommy van der Vorst";
}