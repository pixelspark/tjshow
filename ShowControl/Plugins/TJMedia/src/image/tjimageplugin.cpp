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
#include "../../include/tjmedia.h"
#include "../../include/tjmediamaster.h"

using namespace tj::media;

ImagePlugin::ImagePlugin(strong<master::MediaMasters> ms): _masters(ms) {
}

ImagePlugin::~ImagePlugin() {
}

void ImagePlugin::GetRequiredFeatures(std::list<std::wstring>& fts) const {
	fts.push_back(L"MEDIA");
}

std::wstring ImagePlugin::GetName() const {
	return std::wstring(L"Image");
}

strong<master::MediaMasters> ImagePlugin::GetMasters() {
	return _masters;
}

ref<Track> ImagePlugin::CreateTrack(ref<Playback> pb) {
	return GC::Hold(new ImageTrack(ref<ImagePlugin>(this), pb));
}

ref<StreamPlayer> ImagePlugin::CreateStreamPlayer(ref<Playback> pb, ref<Talkback> t) {
	return GC::Hold(new ImageStreamPlayer(this, t));
}

std::wstring ImagePlugin::GetFriendlyName() const {
	return TL(media_image_plugin_friendly_name);
}

std::wstring ImagePlugin::GetFriendlyCategory() const {
	return TL(media_category);
}

std::wstring ImagePlugin::GetDescription() const {
	return TL(media_image_plugin_description);
}

std::wstring ImagePlugin::GetVersion() const {
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

std::wstring ImagePlugin::GetAuthor() const {
	return std::wstring(L"Tommy van der Vorst");
}

/** TextImagePlugin **/
TextImagePlugin::TextImagePlugin(strong<tj::media::master::MediaMasters> ms): ImagePlugin(ms) {
}

TextImagePlugin::~TextImagePlugin() {
}

std::wstring TextImagePlugin::GetName() const {
	return std::wstring(L"Text");
}

ref<Track> TextImagePlugin::CreateTrack(ref<Playback> pb) {
	return GC::Hold(new TextImageTrack(ref<ImagePlugin>(this), pb));
}

std::wstring TextImagePlugin::GetFriendlyName() const {
	return TL(media_image_text_plugin_friendly_name);
}

std::wstring TextImagePlugin::GetDescription() const {
	return TL(media_image_text_plugin_description);
}