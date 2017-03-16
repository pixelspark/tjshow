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
using namespace tj::shared::graphics;

const float ImageBlock::KThumbnailSize = 64.0f;

FileImageBlock::FileImageBlock(Time start, const std::wstring& rid): ImageBlock(start), _file(rid), _bitmap(0), _downBitmap(0), _thumbnail(0) {
}

FileImageBlock::~FileImageBlock() {
	delete _thumbnail;
	delete _bitmap;
	delete _downBitmap;
}

std::wstring FileImageBlock::GetFile() const {
	return _file;
}

std::wstring FileImageBlock::GetDownFile() const {
	return _downFile;
}

ref<PropertySet> FileImageBlock::GetProperties(ref<Playback> pb) {
	ref<PropertySet> parent = ImageBlock::GetProperties(pb);

	ref<PropertySet> ps = GC::Hold(new PropertySet());
	ps->Add(GC::Hold(new PropertySeparator(TL(media_image_properties))));
	ps->Add(GC::Hold(new FileProperty(TL(media_image_file), this, &_file, pb->GetResources())));

	ps->MergeAdd(parent);

	ps->Add(GC::Hold(new PropertySeparator(TL(media_image_advanced), _downFile.length()<1)));
	ps->Add(GC::Hold(new FileProperty(TL(media_image_file_down), this, &_downFile, pb->GetResources())));
	return ps;
}

void FileImageBlock::Send(ref<Stream> stream, ref<Playback> pb, const PatchIdentifier& screen) {
	ref<Message> msg = stream->Create();
	msg->Add(ImageStreamPlayer::KMessageLoad);
	msg->Add(pb->ParseVariables(GetFile()));
	msg->Add(screen);
	stream->Send(msg);
}

bool FileImageBlock::HasDifferentDownImage() const {
	return _downFile.length() > 0;
}

void FileImageBlock::Save(TiXmlElement* parent) {
	SaveAttributeSmall<std::wstring>(parent, "type", L"file");
	SaveAttributeSmall(parent, "file", _file);
	SaveAttributeSmall(parent, "file-down", _downFile);
	ImageBlock::Save(parent);
}

void FileImageBlock::Load(TiXmlElement* you) {
	_file = LoadAttributeSmall(you, "file", std::wstring(L""));
	_downFile = LoadAttributeSmall(you, "file-down", std::wstring(L""));
	ImageBlock::Load(you);
}

Bitmap* FileImageBlock::GetBitmap(ref<Playback> pb) {
	GetThumbnail(pb); // This loads our bitmap into _bitmap and _thumbnail
	return _bitmap;
}

Bitmap* FileImageBlock::GetDownBitmap(ref<Playback> pb) {
	GetThumbnail(pb); // This loads our bitmap into _bitmap and _thumbnail
	return HasDifferentDownImage() ? _downBitmap : _bitmap;
}

std::wstring FileImageBlock::GetTooltipText() const {
	return GetFile() + L" @ " + GetTime().Format();
}

std::wstring FileImageBlock::GetBlockDisplayText() const {
	return File::GetFileName(GetFile());
}

std::wstring FileImageBlock::GetBlockText() const {
	return L"";
}

void FileImageBlock::GetResources(std::vector<ResourceIdentifier>& rc) {
	rc.push_back(GetFile());
	if(HasDifferentDownImage()) {
		rc.push_back(GetDownFile());
	}
}

Bitmap* FileImageBlock::GetThumbnail(ref<Playback> pb) {
	ThreadLock lock(&_thumbLock);
	std::wstring translated = pb->ParseVariables(_file);
	std::wstring downTranslated = pb->ParseVariables(_downFile);

	Hash h;
	int hash = h.Calculate(translated+downTranslated);

	if(_thumbnail!=0 && _thumbnailHash==hash) {
		return _thumbnail;
	}

	delete _thumbnail;
	delete _bitmap;
	delete _downBitmap;
	_thumbnail = 0;
	_bitmap = 0;
	_downBitmap = 0;

	// load the file and transform it to a smaller bitmap
	std::wstring path;
	if(pb->GetResources()->GetPathToLocalResource(translated, path)) {
		_bitmap = dynamic_cast<graphics::Bitmap*>(graphics::Bitmap::FromFile(path.c_str(), true));

		// if we have a down image, load it
		if(HasDifferentDownImage()) {
			std::wstring downPath;
			if(pb->GetResources()->GetPathToLocalResource(downTranslated, downPath)) {
				_downBitmap = dynamic_cast<graphics::Bitmap*>(graphics::Bitmap::FromFile(downPath.c_str(), true));
			}
		}

		_thumbnailHash = hash;

		if(_bitmap!=0) {
			_thumbnail = new graphics::Bitmap(int(KThumbnailSize), int(KThumbnailSize));
			{
				graphics::Graphics g(_thumbnail);
				g.DrawImage(_bitmap, RectF(0.0f, 0.0f, KThumbnailSize, KThumbnailSize));		
			}
			return _thumbnail;
		}
		else {
			Log::Write(L"TJMedia/FileImageBlock", L"Bitmap::FromFile failed");
		}
	}
	return 0;
}

ImageBlock::ImageBlock(Time start): _duration(2000), _start(start) {
}

ImageBlock::~ImageBlock() {
}

ref<PropertySet> ImageBlock::GetProperties(ref<Playback> pb) {
	ref<PropertySet> ps = GC::Hold(new PropertySet());
	ps->Add(GC::Hold(new PropertySeparator(TL(media_block_timing), true)));
	ps->Add(GC::Hold(new GenericProperty<Time>(TL(media_block_begin_time), this, &_start, _start)));
	ps->Add(GC::Hold(new GenericProperty<Time>(TL(media_block_length), this, &_duration, _duration)));
	return ps;
}

void ImageBlock::OnFocus(bool f) {
}

void ImageBlock::Move(Time t, int h) {
	_start = t;
}

Time ImageBlock::GetDuration() {
	return _duration;
}

Time ImageBlock::GetTime() const {
	return _start;
}

void ImageBlock::SetDuration(Time t) {
	_duration = t;
}

void ImageBlock::SetTime(Time start) {
	_start = start;
}

void ImageBlock::Prepare(ref<Playback> pb) {
}

void ImageBlock::OnKey(const Interactive::KeyNotification& kn) {
}

void ImageBlock::Save(TiXmlElement* parent) {
	SaveAttributeSmall(parent, "start", _start);
	SaveAttributeSmall(parent, "length", _duration);
}

void ImageBlock::Load(TiXmlElement* you) {
	_start = LoadAttributeSmall(you, "start", Time(0));
	_duration = LoadAttributeSmall<int>(you, "length", 0);
}

/** TextImageBlock **/
TextImageBlock::TextImageBlock(Time start): ImageBlock(start), _thumbnail(0), _bitmap(0), _thumbnailHash(0), _isTextField(false), _cursorPosition(-1), _width(256), _height(64), _fontSize(10), _showBorder(false), _color(1.0f, 1.0f, 1.0f) {
}

TextImageBlock::~TextImageBlock() {
	delete _bitmap;
	delete _thumbnail;
}

tj::shared::graphics::Bitmap* TextImageBlock::GetThumbnail(ref<Playback> pb) {
	ThreadLock lock(&_thumbLock);
	std::wstring translated = IsTextField() ? _currentText : pb->ParseVariables(_currentText);

	// Calculate thumbnail hash
	Hash h;
	int hash = h.Calculate(translated+Stringify(_width)+Stringify(_height)+Stringify(_fontSize)+Stringify(_showBorder)+_fontName+Stringify(_isTextField)+Stringify(_cursorPosition));

	if(_thumbnail!=0 && _thumbnailHash==hash) {
		return _thumbnail;
	}

	delete _thumbnail;
	delete _bitmap;
	_thumbnail = 0;
	_bitmap = 0;

	// try to measure the text and create a bitmap accordingly
	strong<Theme> theme = ThemeManager::GetTheme();
	Font* font = 0;

	if(_fontName.length()>0) {
		font = new Font(_fontName.c_str(), float(_fontSize), FontStyleRegular);
	}
	else {
		strong<Theme> theme = ThemeManager::GetTheme();
		std::wstring uiFontName = theme->GetGUIFontName();
		font = new Font(uiFontName.c_str(), float(_fontSize), FontStyleRegular);
	}

	SolidBrush tbr(_color);
	PointF origin(0.0f, 0.0f);
	
	{
		Graphics desktopGraphics(GetDesktopWindow());
		_bitmap = new Bitmap(_width, _height, &desktopGraphics); // size *2 for oversize sampling
		Graphics g(_bitmap);
		///g.ScaleTransform(2.0f, 2.0f);

		if(font!=0) {
			g.DrawString(translated.c_str(), (int)translated.length(), font, origin, &tbr);

			if(IsTextField() && _cursorPosition >= 0 && _cursorPosition <= int(translated.length())) {
				std::wstring pre = translated.substr(0,_cursorPosition);
				RectF bound;
				g.MeasureString(pre.c_str(), pre.length(), font, origin, &bound);
				SolidBrush cursorBrush(Theme::ChangeAlpha(_color, 127));
				g.FillRectangle(&cursorBrush, Area(Pixels(bound.GetRight())-10, 0, 3, _height));
			}
		}

		if(_showBorder) {
			Pen pn(&tbr,2.0f);
			g.DrawRectangle(&pn, Area(2, 1, _width-3, _height-2));
		}
	}

	_thumbnailHash = hash;

	if(_bitmap!=0) {
		_thumbnail = new Bitmap(int(KThumbnailSize), int(KThumbnailSize));

		{
			Graphics g(_thumbnail);
			g.DrawImage(_bitmap, RectF(0.0f, 0.0f, KThumbnailSize, KThumbnailSize));
		}
		return _thumbnail;
	}
	return 0;
}

void TextImageBlock::OnFocus(bool f) {
	if(f) {
		_cursorPosition = _currentText.length();
	}
	else {
		_cursorPosition = -1;
	}
}

tj::shared::graphics::Bitmap* TextImageBlock::GetBitmap(ref<Playback> pb) {
	GetThumbnail(pb);
	return _bitmap;
}

tj::shared::graphics::Bitmap* TextImageBlock::GetDownBitmap(ref<Playback> pb) {
	GetThumbnail(pb);
	return _bitmap;
}

void TextImageBlock::OnKey(const Interactive::KeyNotification& kn) {
	if(IsTextField()) {
		if(kn._down) {
			if(kn._key==KeyLeft) {
				_cursorPosition = max(0,_cursorPosition-1);
			}
			else if(kn._key==KeyRight) {
				_cursorPosition = min(int(_currentText.length()), _cursorPosition+1);
			}
			else if(kn._key==KeyDelete) {
				int n = (int)_currentText.length();
				std::wostringstream wos;
				wos << _currentText.substr(0,min(n,max(0,_cursorPosition)));
				if(_cursorPosition<int(_currentText.length())-2) {
					wos << _currentText.substr(max(0,_cursorPosition+1));
				}
				_currentText = wos.str();
				_cursorPosition;
			}
			else if(kn._key==KeyBackspace) {
				int n = (int)_currentText.length();
				std::wostringstream wos;
				wos << _currentText.substr(0,min(n,max(0,_cursorPosition-1)));
				if(_cursorPosition<int(_currentText.length())-2) {
					wos << _currentText.substr(max(0,_cursorPosition));
				}
				_currentText = wos.str();
				--_cursorPosition;
			}
			else if(kn._key==KeyCharacter && kn._code!=L'\b' && kn._code!=L'\r' && kn._code!=L'\n') {
				int n = (int)_currentText.length();
				if(n==0) {
					_currentText = Stringify(kn._code);
					_cursorPosition = 1;
				}
				else {
					std::wostringstream wos;
					if(_cursorPosition>0) {
						wos << _currentText.substr(0,min(n, _cursorPosition));
					}
					wos << kn._code;
					wos << _currentText.substr(min(n,max(0, _cursorPosition)));
					_currentText = wos.str();
					++_cursorPosition;
				}
			}
		}
	}
}

bool TextImageBlock::IsTextField() const {
	return _isTextField;
}

void TextImageBlock::SetIsTextField(bool t) {
	_isTextField = t;
}

void TextImageBlock::Prepare(ref<Playback> pb) {
	_currentText = pb->ParseVariables(_text);
	ImageBlock::Prepare(pb);
}

ref<PropertySet> TextImageBlock::GetProperties(ref<Playback> pb) {
	ref<PropertySet> parent = ImageBlock::GetProperties(pb);
	ref<PropertySet> ps = GC::Hold(new PropertySet());

	ps->Add(GC::Hold(new PropertySeparator(TL(media_image_properties))));
	ps->Add(pb->CreateParsedVariableProperty(TL(image_block_text), this, &_text, true));
	ps->Add(GC::Hold(new ColorProperty(TL(image_block_text_color), this, &_color)));
	ps->Add(GC::Hold(new GenericProperty<std::wstring>(TL(image_block_font_name), this, &_fontName, _fontName)));

	ps->Add(GC::Hold(new PropertySeparator(TL(media_image_dimensions))));
	ps->Add(GC::Hold(new GenericProperty<Pixels>(TL(media_image_width), this, &_width, _width)));
	ps->Add(GC::Hold(new GenericProperty<Pixels>(TL(media_image_height), this, &_height, _height)));
	ps->Add(GC::Hold(new GenericProperty<Pixels>(TL(media_image_font_size), this, &_fontSize, _fontSize)));
	ps->Add(GC::Hold(new GenericProperty<bool>(TL(media_image_text_border), this, &_showBorder, false)));
	ps->Add(GC::Hold(new GenericProperty<bool>(TL(media_image_text_is_field), this, &_isTextField, false)));
	ps->MergeAdd(parent);
	return ps;
}

std::wstring TextImageBlock::GetTooltipText() const {
	return L"\"" + _text + L"\" @ " + GetTime().Format();
}

std::wstring TextImageBlock::GetBlockDisplayText() const {
	return L"\"" + _text + L"\"";
}

std::wstring TextImageBlock::GetBlockText() const {
	return _currentText;
}

void TextImageBlock::GetResources(std::vector<ResourceIdentifier>& rc) {
}

void TextImageBlock::Save(TiXmlElement* parent) {
	SaveAttribute(parent, "text", _text);
	SaveAttributeSmall<std::wstring>(parent, "type", L"text");
	SaveAttributeSmall(parent, "width", _width);
	SaveAttributeSmall(parent, "height", _height);
	SaveAttributeSmall(parent, "font-size", _fontSize);
	SaveAttributeSmall(parent, "border", _showBorder);
	SaveAttributeSmall(parent, "font", _fontName);
	SaveAttributeSmall(parent, "is-field", _isTextField);
	TiXmlElement color("color");
	_color.Save(&color);
	parent->InsertEndChild(color);
	ImageBlock::Save(parent);
}

void TextImageBlock::Load(TiXmlElement* you) {
	_text = LoadAttribute(you, "text", _text);
	_width = LoadAttribute(you, "width", _width);
	_height = LoadAttributeSmall(you, "height", _height);
	_fontSize = LoadAttributeSmall(you,"font-size", _fontSize);
	_showBorder = LoadAttributeSmall(you, "border", _showBorder);
	_fontName = LoadAttributeSmall(you, "font", _fontName);
	_isTextField = LoadAttributeSmall(you, "is-field", _isTextField);

	TiXmlElement* color = you->FirstChildElement("color");
	if(color!=0) {
		_color.Load(color);
	}
	ImageBlock::Load(you);
}

void TextImageBlock::Send(ref<Stream> stream, ref<Playback> pb, const PatchIdentifier& screen) {
	ref<Message> msg = stream->Create();
	msg->Add(ImageStreamPlayer::KMessageLoadText);
	msg->Add(_width);
	msg->Add(_height);
	msg->Add(_fontSize);
	msg->Add<unsigned int>(_showBorder?1:0);
	msg->Add(_color._r);
	msg->Add(_color._g);
	msg->Add(_color._b);
	msg->Add(pb->ParseVariables(_text));
	msg->Add(screen);
	msg->Add(_fontName);
	msg->Add<unsigned int>(_isTextField?1:0);
	stream->Send(msg);
}