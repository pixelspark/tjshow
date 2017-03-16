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
#include "../../include/tjmediamasters.h"
#include <shlwapi.h>
using namespace tj::show;

std::wstring ImageTrack::KClickedOutletID = L"clicked";
std::wstring ImageTrack::KClickedXOutletID = L"clicked-x";
std::wstring ImageTrack::KClickedYOutletID = L"clicked-y";
std::wstring ImageTrack::KTextOutletID = L"text";

class ImageTrackItem: public Item {
	public:
		enum Variable {
			VariableNone=0,
			VariableBegin,
			VariableEnd,
		};

		ImageTrackItem(Variable v, ref<ImageBlock> at, Time grab, ref<ImageTrack> tr) {
			assert(at && tr);
			_v = v;
			_at = at;
			if(_v==VariableBegin) {
				_grab = grab - _at->GetTime();
			}
			else if(v==VariableEnd) {
				_grab = grab - (_at->GetTime() + _at->GetDuration());
			}
			_track = tr;
		}

		Variable GetVariable() {
			return _v;
		}

		virtual ref<ImageBlock> GetBlock() {
			return _at;
		}

		virtual void Remove() {
			_track->RemoveBlock(_at);
		}

		virtual void MoveRelative(Item::Direction dir) {
			switch(dir) {
				case Item::Left:
					_at->SetTime(_at->GetTime()-Time(1));
					break;

				case Item::Right:
					_at->SetTime(_at->GetTime()+Time(1));
					break;
			}
		}

		virtual void Move(Time t, int h) {
			if(h<-25) {
				_track->RemoveBlock(_at);
			}

			if(_v==VariableBegin) {
				_at->SetTime(t-_grab);
			}
			else if(_v==VariableEnd) {
				_at->SetDuration(t-_grab-_at->GetTime());
			}
		}

		virtual int GetSnapOffset() {
			return int(_grab);
		}

		virtual ref<PropertySet> GetProperties() {
			return _at->GetProperties(_track->_pb);
		}

		virtual std::wstring GetTooltipText() {
			return _at->GetTooltipText();
		}

		Variable _v;
		ref<ImageBlock> _at;
		Time _grab;
		ref<ImageTrack> _track;
};


ImageTrack::ImageTrack(strong<ImagePlugin> ip, ref<Playback> pb):
	_translate(0.0f, 0.0f, 0.0f),
	_scale(0.0f, 0.0f, 0.0f),
	_rotate(0.0f, 0.0f, 0.0f),
	_itranslate(0.0f, 0.0f, 0.0f),
	_iscale(1.0f, 1.0f, 1.0f),
	_irotate(0.0f, 0.0f, 0.0f),
	_color(200,125, 125),
	_pb(pb),
	_plugin(ref<ImagePlugin>(ip)),
	_resetOnStop(true) {

	AddFader(FaderAlpha, TL(media_opacity));
	AddFader(FaderTranslate, TL(media_video_translate), 0.0f);
	AddFader(FaderRotate, TL(media_video_rotate), 0.0f);
	AddFader(FaderScale, TL(media_video_scale), 0.0f);
}

ImageTrack::~ImageTrack() {
}

ref<TrackRange> ImageTrack::GetRange(Time start, Time end) {
	return GC::Hold(new MultifaderTrackRange(this, start, end));
}

void ImageTrack::RemoveItemsBetween(Time start, Time end) {
	std::vector< ref<ImageBlock> >::iterator it = _blocks.begin();
	while(it!=_blocks.end()) {
		ref<ImageBlock> block = *it;
		if(block) {
			Time t = block->GetTime();
			Time e = t + block->GetDuration();
			if((t >= start && t<=end) || (e <= end && e>=start)) {
				it = _blocks.erase(it);
				continue;
			}
		}
		++it;
	}
	MultifaderTrack::RemoveItemsBetween(start,end);
}

void ImageTrack::CreateOutlets(OutletFactory& of) {
	_clickedOutlet = of.CreateOutlet(KClickedOutletID, TL(media_outlet_clicked));
	_clickXOutlet = of.CreateOutlet(KClickedXOutletID, TL(media_outlet_clicked_x));
	_clickYOutlet = of.CreateOutlet(KClickedYOutletID, TL(media_outlet_clicked_y));
}

Flags<RunMode> ImageTrack::GetSupportedRunModes() {
	Flags<RunMode> flags;
	flags.Set(RunModeMaster,true);
	flags.Set(RunModeClient, true);
	flags.Set(RunModeBoth, true);
	return flags;
}

std::wstring ImageTrack::GetTypeName() const {
	return TL(media_image_track_type);
}

ref<Player> ImageTrack::CreatePlayer(ref<Stream> str) {
	return GC::Hold(new ImagePlayer(this, str));
}

ref<PropertySet> ImageTrack::GetProperties() {
	ref<PropertySet> ps = GC::Hold(new PropertySet());

	ps->Add(GC::Hold(new ColorProperty(TL(media_color), this, &_color)));
	ps->Add(_pb->CreateSelectPatchProperty(TL(media_screen), this, &_screen));
	ps->Add(CreateChooseFadersProperty(TL(faders_choose), this, TL(faders_choose_hint)));
	ps->Add(GC::Hold(new GenericProperty<bool>(TL(media_reset_on_stop), this, &_resetOnStop, _resetOnStop)));
	
	ref<ImagePlugin> ip = _plugin;
	if(ip) {
		ps->Add(ip->GetMasters()->CreateMasterListProperty(TL(media_masters), this, &_masters));
	}

	ps->Add(GC::Hold(new PropertySeparator(TL(media_video_position), true)));
	ps->Add(GC::Hold(new VectorProperty(TL(media_video_translate), &_itranslate)));
	ref<VectorProperty> svp = GC::Hold(new VectorProperty(TL(media_video_scale), &_iscale));
	svp->SetDimensionShown(2,false); // Hide the field for the Z-dimension of the scale vector, since it is useless
	ps->Add(svp);
	ps->Add(GC::Hold(new VectorProperty(TL(media_video_rotate), &_irotate)));

	ps->Add(GC::Hold(new PropertySeparator(TL(media_video_position_faders), true)));
	ps->Add(GC::Hold(new VectorProperty(TL(media_video_translate), &_translate)));
	ps->Add(GC::Hold(new VectorProperty(TL(media_video_scale), &_scale)));
	ps->Add(GC::Hold(new VectorProperty(TL(media_video_rotate), &_rotate)));

	return ps;
}

void ImageTrack::Paint(tj::shared::graphics::Graphics* g, Time position, float pixelsPerMs, unsigned int x, unsigned int y, unsigned int h, Time start, Time end, ref<Item> focus) {
	strong<Theme> theme = ThemeManager::GetTheme();
	std::vector< ref<ImageBlock> >::iterator it = _blocks.begin();
	int blockHeight = MultifaderTrack::GetFaderHeight()-4;

	ref<ImageBlock> focused = 0;
	if(focus.IsCastableTo<ImageTrackItem>()) {
		focused = ref<ImageTrackItem>(focus)->GetBlock();
	}

	Color a = _color;
	Color b = Theme::ChangeAlpha(a,155);

	LinearGradientBrush lbr(PointF(0.0f,(float)y), PointF(0.0f,float(y+blockHeight)), a,b);
	tj::shared::graphics::SolidBrush descBrush = theme->GetColor(Theme::ColorDescriptionText);
	
	if(_blocks.size()<=0) {
		StringFormat sf;
		sf.SetAlignment(StringAlignmentCenter);
		Font* fnt = theme->GetGUIFontSmall();

		std::wstring str(TL(media_track_add_block));
		g->DrawString(str.c_str(), str.length(), fnt, PointF((float)(((int(end-start)*pixelsPerMs)/2)+x), (float)((blockHeight/4)+y)), &sf, &descBrush);
	}
	else {
		while(it!=_blocks.end()) {
			ref<ImageBlock> block = *it;

			int cx = x + int(pixelsPerMs * int(block->GetTime()-start));
			int duration = int(block->GetDuration());
			if(duration<100) {
				duration = 100;
			}

			Area blockRC(cx,y, Pixels(duration*pixelsPerMs), blockHeight);
			g->FillRectangle(&lbr, blockRC);
			
			// draw thumbnail
			Bitmap* thumb = block->GetThumbnail(_pb);
			if(thumb!=0) {
				g->DrawImage(thumb, RectF(float(cx+2), float(y+1), float(min(int(duration)*pixelsPerMs-10, blockHeight-2)), float(blockHeight-2)));
			}

			// draw filename
			SolidBrush tbr(theme->GetColor(Theme::ColorText));
			std::wstring fn = block->GetBlockDisplayText();
			StringFormat sf;
			sf.SetAlignment(StringAlignmentNear);
			sf.SetFormatFlags(StringFormatFlagsNoWrap);
			g->DrawString(fn.c_str(), (int)fn.length(), theme->GetGUIFontSmall(), RectF(float(cx+blockHeight+10), float(y+1), float(int(duration)*pixelsPerMs)-32, float(blockHeight)-2), &sf, &tbr);

			int gripStart = cx + int(int(duration)*pixelsPerMs) - 15;
			if(gripStart>cx+5) {
				Color text = ThemeManager::GetTheme()->GetColor(Theme::ColorText);
				Pen gripPen(Color((unsigned char)100,text.GetRed(), text.GetGreen(), text.GetBlue()), 1.0);
				g->DrawLine(&gripPen, float(gripStart), float(y+5), float(gripStart+5), float(y+5));
				g->DrawLine(&gripPen, float(gripStart), float(y+7), float(gripStart+5), float(y+7));
				g->DrawLine(&gripPen, float(gripStart), float(y+9), float(gripStart+5), float(y+9));
			}

			if(block==focused) {
				theme->DrawFocusRectangle(*g, blockRC);
			}
			++it;
		}
	}

	MultifaderTrack::Paint(g,position,pixelsPerMs, x,y,h,start,end,focus);
}

ref<LiveControl> ImageTrack::CreateControl() {
	return 0;
}

Time ImageTrack::GetNextEvent(Time t) {
	Time nf = MultifaderTrack::GetNextEvent(t);

	ref<ImageBlock> current = GetBlockAt(t);
	if(current) {
		Time blockEnd = current->GetTime() + current->GetDuration();
		nf = Time::Earliest(blockEnd, nf);
	}
	else {
		// try next block
		ref<ImageBlock> next = GetNextBlock(t);
		if(next) {
			nf = Time::Earliest(next->GetTime(), nf);
		}
	}

	return nf;
}

void ImageTrack::GetResources(std::vector<ResourceIdentifier>& rids) {
	std::vector<ref<ImageBlock> >::iterator it = _blocks.begin();
	while(it!=_blocks.end()) {
		ref<ImageBlock> block = *it;
		block->GetResources(rids);
		++it;
	}
}

float ImageTrack::GetTranslateAt(Time t) {
	return GetFaderById(FaderTranslate)->GetValueAt(t);
}

float ImageTrack::GetRotateAt(Time t) {
	return GetFaderById(FaderRotate)->GetValueAt(t);
}

float ImageTrack::GetScaleAt(Time t) {
	return GetFaderById(FaderScale)->GetValueAt(t);
}

float ImageTrack::GetOpacityAt(Time t) {
	return GetFaderById(FaderAlpha)->GetValueAt(t);
}

ref<ImageBlock> ImageTrack::GetNextBlock(Time t) {
	Time closest = -1;
	ref<ImageBlock> next;

	std::vector<ref<ImageBlock> >::iterator it = _blocks.begin();
	while(it!=_blocks.end()) {
		ref<ImageBlock> bl = *it;
		Time begin = bl->GetTime();
		
		if(begin>t && ((abs(int(begin-t))<abs(int(closest-t)))||closest==Time(-1))) {
			closest = begin;
			next = bl;
		}

		++it;
	}
	
	return next;
}

void ImageTrack::RemoveBlock(ref<ImageBlock> bl) {
	std::vector<ref<ImageBlock> >::iterator it = _blocks.begin();
	while(it!=_blocks.end()) {
		ref<ImageBlock> block = *it;
		if(block==bl) {
			_blocks.erase(it);
			return;
		}
		++it;
	}
}


ref<Item> ImageTrack::GetItemAt(Time position, unsigned int h, bool rightClick, int th, float pixelsPerMs) {
	ref<Item> faderItem = MultifaderTrack::GetItemAt(position, h ,rightClick, th, pixelsPerMs);

	if(!faderItem) {
		if(rightClick && int(h)>(th-MultifaderTrack::GetFaderHeight())) {
			ref<ImageBlock> block = GC::Hold(new FileImageBlock(position, L""));
			_blocks.push_back(block);
			return GC::Hold(new ImageTrackItem(ImageTrackItem::VariableBegin,block, position, this));
		}
		else  {
			ref<ImageBlock> block = GetBlockAt(position);
			if(block) {
				Time end = block->GetDuration()+block->GetTime();
				
				// if we're clicking near to the end of the block, drag it. Otherwise, drag
				// the beginning of the block (the block itself)
				// TODO put the 0.2f (20%) into some constant (it means '20% of the block at the end will drag the end time')
				if(position<end && position.ToInt()>=end.ToInt()-int(0.2f*block->GetDuration().ToInt())) {
					return GC::Hold(new ImageTrackItem(ImageTrackItem::VariableEnd,block, position, this));
				}
				else {
					return GC::Hold(new ImageTrackItem(ImageTrackItem::VariableBegin,block, position, this));
				}
			}
		}
		return 0;
	}
	else {
		return faderItem;
	}
}


void ImageTrack::SaveFader(TiXmlElement* parent, int id, const char* name) {
	TiXmlElement el(name);
	SaveAttributeSmall<bool>(&el, "enabled", IsFaderEnabled(id));
	GetFaderById(id)->Save(&el);
	parent->InsertEndChild(el);
}

void ImageTrack::LoadFader(TiXmlElement* parent, int id, const char* name) {
	TiXmlElement* faderElement  = parent->FirstChildElement(name);
	if(faderElement!=0) {
		bool enabled = LoadAttributeSmall<bool>(faderElement, "enabled", true);
		SetFaderEnabled(id, enabled);
		TiXmlElement* fader = faderElement->FirstChildElement("fader");
		if(fader!=0) {
			GetFaderById(id)->Load(fader);
		}
	}
}

void ImageTrack::Save(TiXmlElement* parent) {
	_color.Save(parent);
	SaveAttributeSmall(parent, "reset-on-stop", _resetOnStop);
	SaveAttributeSmall(parent, "screen", _screen);
	SaveAttributeSmall(parent, "masters", _masters);

	SaveFader(parent, FaderAlpha, "opacity-fader");
	SaveFader(parent, FaderTranslate, "translate-fader");
	SaveFader(parent, FaderRotate, "rotate-fader");
	SaveFader(parent, FaderScale, "scale-fader");

	TiXmlElement translate("translate");
	_translate.Save(&translate);
	parent->InsertEndChild(translate);

	TiXmlElement rotate("rotate");
	_rotate.Save(&rotate);
	parent->InsertEndChild(rotate);

	TiXmlElement scale("scale");
	_scale.Save(&scale);
	parent->InsertEndChild(scale);

	// init values
	TiXmlElement itranslate("initial-translate");
	_itranslate.Save(&itranslate);
	parent->InsertEndChild(itranslate);

	TiXmlElement irotate("initial-rotate");
	_irotate.Save(&irotate);
	parent->InsertEndChild(irotate);

	TiXmlElement iscale("initial-scale");
	_iscale.Save(&iscale);
	parent->InsertEndChild(iscale);

	TiXmlElement blocks("blocks");
	std::vector< ref<ImageBlock> >::iterator it = _blocks.begin();
	while(it!=_blocks.end()) {
		ref<ImageBlock> block = *it;
		TiXmlElement blockElement("block");
		block->Save(&blockElement);
		blocks.InsertEndChild(blockElement);
		++it;
	}
	parent->InsertEndChild(blocks);
	
	TiXmlElement color("color");
	_color.Save(&color);
	parent->InsertEndChild(color);
}

void ImageTrack::Load(TiXmlElement* you) {
	_resetOnStop = LoadAttributeSmall(you, "reset-on-stop", _resetOnStop);
	_screen = LoadAttributeSmall<PatchIdentifier>(you, "screen", _screen);
	_masters = LoadAttributeSmall(you, "masters", _masters);

	// transformation faders
	LoadFader(you, FaderAlpha, "opacity-fader");
	LoadFader(you, FaderTranslate, "translate-fader");
	LoadFader(you, FaderRotate, "rotate-fader");
	LoadFader(you, FaderScale, "scale-fader");

	TiXmlElement* color = you->FirstChildElement("color");
	if(color!=0) {
		_color.Load(color);
	}

	// translate
	TiXmlElement* translate = you->FirstChildElement("translate");
	if(translate!=0) {
		_translate.Load(translate);
	}

	// rotate
	TiXmlElement* rotate = you->FirstChildElement("rotate");
	if(rotate!=0) {
		_rotate.Load(rotate);
	}

	// scale
	TiXmlElement* scale = you->FirstChildElement("scale");
	if(scale!=0) {
		_scale.Load(scale);
	}

	// initial values
	TiXmlElement* iscale = you->FirstChildElement("initial-scale");
	if(iscale!=0) {
		_iscale.Load(iscale);
	}
	
	TiXmlElement* itranslate = you->FirstChildElement("initial-translate");
	if(itranslate!=0) {
		_itranslate.Load(itranslate);
	}

	TiXmlElement* irotate = you->FirstChildElement("initial-rotate");
	if(irotate!=0) {
		_irotate.Load(irotate);
	}

	_blocks.clear();
	TiXmlElement* blocks = you->FirstChildElement("blocks");
	if(blocks!=0) {
		TiXmlElement* block = blocks->FirstChildElement("block");
		while(block!=0) {
			std::wstring type = LoadAttributeSmall<std::wstring>(block, "type", L"file");
			ref<ImageBlock> mbl;
			if(type==L"file") {
				mbl = GC::Hold(new FileImageBlock(0,L""));
			}
			else if(type==L"text") {
				mbl = GC::Hold(new TextImageBlock(0));
			}

			mbl->Load(block);
			_blocks.push_back(mbl);
			block = block->NextSiblingElement("block");
		}
	}
}

ref<ImageBlock> ImageTrack::GetBlockAt(Time t) {
	std::vector<ref<ImageBlock> >::iterator it = _blocks.begin();
	while(it!=_blocks.end()) {
		ref<ImageBlock> bl = *it;
		Time begin = bl->GetTime();
		Time duration = Time(bl->GetDuration());
		if(duration<Time(100)) {
			duration = Time(100);
		}
		if(t>=begin && t<=(begin+duration)) {
			return bl;
		}
		++it;
	}

	return 0;
}

/** TextImageTrack **/
TextImageTrack::TextImageTrack(strong<ImagePlugin> ip, ref<Playback> pb): ImageTrack(ip, pb) {
}

TextImageTrack::~TextImageTrack() {
}

ref<Item> TextImageTrack::GetItemAt(Time position, unsigned int h, bool rightClick, int th, float pixelsPerMs) {
	ref<Item> faderItem = MultifaderTrack::GetItemAt(position, h, rightClick, th, pixelsPerMs);

	if(!faderItem && rightClick && int(h)>(th-MultifaderTrack::GetFaderHeight())) {
		ref<ImageBlock> block = GC::Hold(new TextImageBlock(position));
		_blocks.push_back(block);
		return GC::Hold(new ImageTrackItem(ImageTrackItem::VariableBegin,block, position, this));
	}

	return ImageTrack::GetItemAt(position, h, rightClick, th, pixelsPerMs);
}

std::wstring TextImageTrack::GetTypeName() const {
	return TL(media_image_text_track_type);
}

void TextImageTrack::CreateOutlets(OutletFactory& of) {
	ImageTrack::CreateOutlets(of);
	_textOutlet = of.CreateOutlet(KTextOutletID, TL(media_outlet_text));
}