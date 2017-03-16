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
#include "../include/tjmedia.h"
#include <TJScript/include/tjscript.h>
#include <shlwapi.h>
using namespace tj::shared::graphics;
using namespace tj::script;
using namespace tj::media;

std::wstring MediaTrack::KClickedOutletID = L"clicked";
std::wstring MediaTrack::KClickedXOutletID = L"clicked-x";
std::wstring MediaTrack::KClickedYOutletID = L"clicked-y";

namespace tj {
	namespace media {
		class MediaTrackItem: public Item, public Serializable {
			public:
				enum Variable {
					VariableNone=0,
					VariableBegin,
					VariableEnd,
				};

				MediaTrackItem(Variable v, ref<MediaBlock> at, Time grab, ref<MediaTrack> tr) {
					assert(at && tr);
					_v = v;
					_at = at;
					if(_v==VariableBegin) {
						_grab = grab - _at->GetTime();
					}
					else if(v==VariableEnd) {
						_grab = grab - (_at->GetTime() + _at->GetDuration(tr->_pb));
					}
					_track = tr;
				}

				Variable GetVariable() {
					return _v;
				}

				virtual ref<MediaBlock> GetBlock() {
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
					return _at->GetProperties(_track->_plugin->IsKeyingSupported(), _track->_pb);
				}

				virtual std::wstring GetTooltipText() {
					return _at->GetFile() + L" @ " + _at->GetTime().Format();
				}

				virtual void Save(TiXmlElement* me) {
					_at->Save(me);
				}

				virtual void Load(TiXmlElement* me) {
					_at->Load(me);
				}

				Variable _v;
				ref<MediaBlock> _at;
				Time _grab;
				ref<MediaTrack> _track;
		};
	}
}

MediaTrack::MediaTrack(ref<Playback> pb, ref<MediaPlugin> mp, bool isVideo): 
	_translate(0.0f, 0.0f, 0.0f),
	_scale(0.0f, 0.0f, 0.0f),
	_rotate(0.0f, 0.0f, 0.0f),
	_itranslate(0.0f, 0.0f, 0.0f),
	_iscale(1.0f, 1.0f, 1.0f),
	_irotate(0.0f, 0.0f, 0.0f),
	_lastControlVolume(1.0f),
	_lastControlOpacity(1.0f),
	_isVideo(isVideo)
	{
	assert(pb);
	_pb = pb;

	AddFader(FaderVolume, TL(media_volume));

	if(isVideo) {
		AddFader(FaderOpacity, TL(media_opacity));
		AddFader(FaderTranslate, TL(media_video_translate), 0.0f);
		AddFader(FaderRotate, TL(media_video_rotate), 0.0f);
		AddFader(FaderScale, TL(media_video_scale), 1.0f);
	}

	_balance = 0;
	_color = RGBColor(0.29,0.60,0.29);
	_selectedFadePoint = -1;
	_plugin = mp;
}

MediaTrack::~MediaTrack() {
}

void MediaTrack::CreateOutlets(OutletFactory& of) {
	if(_isVideo) {
		_clickedOutlet = of.CreateOutlet(KClickedOutletID, TL(media_outlet_clicked));
		_clickXOutlet = of.CreateOutlet(KClickedXOutletID, TL(media_outlet_clicked_x));
		_clickYOutlet = of.CreateOutlet(KClickedYOutletID, TL(media_outlet_clicked_y));
	}
}

Flags<RunMode> MediaTrack::GetSupportedRunModes() {
	return Flags<RunMode>(RunModeBoth);
}

ref< Fader<float> > MediaTrack::GetVolume() {
	return GetFaderById(FaderVolume);
}

/** This gets called by MediaControl to store the last value of the media control. This is necessary
to restore the manual values to Deck each time a new MediaPlayer is Start'ed (in MediaPlayer in Start(), a
new Deck is created; this means the old alpha/opacity mix values are lost) **/
void MediaTrack::SetLastControlValues(float volume, float opacity) {
	_lastControlVolume = volume;
	_lastControlOpacity = opacity;
}

ref<MediaBlock> MediaTrack::GetBlockAt(Time t) {
	std::vector<ref<MediaBlock> >::iterator it = _blocks.begin();
	while(it!=_blocks.end()) {
		ref<MediaBlock> bl = *it;
		Time begin = bl->GetTime();
		Time duration = Time(bl->GetDuration(_pb));
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

float MediaTrack::GetVolumeAt(Time t) {
	return GetFaderById(FaderVolume)->GetValueAt(t);
}

float MediaTrack::GetOpacityAt(Time t) {
	return _isVideo ? GetFaderById(FaderOpacity)->GetValueAt(t) : 0.0f;
}

float MediaTrack::GetScaleAt(Time t) {
	return _isVideo ? GetFaderById(FaderScale)->GetValueAt(t) : 0.0f;
}

float MediaTrack::GetRotateAt(Time t) {
	return _isVideo ? GetFaderById(FaderRotate)->GetValueAt(t) : 0.0f;
}

float MediaTrack::GetTranslateAt(Time t) {
	return _isVideo ? GetFaderById(FaderTranslate)->GetValueAt(t) : 0.0f;
}

Time MediaTrack::GetNextEvent(Time t) {
	Time closest = -1;
	std::vector<ref<MediaBlock> >::iterator it = _blocks.begin();
	while(it!=_blocks.end()) {
		ref<MediaBlock> bl = *it;
		Time begin = bl->GetTime();
		Time end = begin+Time(bl->GetDuration(_pb))+Time(1);
		
		
		if(begin>t) {
			if(closest<Time(0)) {
				closest = begin;
			}
			else if((begin-t) < (closest-t)) {
				closest = begin;
			}
		}

		if(end>t) {
			if(closest<Time(0)) {
				closest = end;
			}
			else if((end-t) < (closest-t)) {
				closest = end;
			}
		}

		++it;
	}
	
	Time fadersNext = MultifaderTrack::GetNextEvent(t);
	if(fadersNext<Time(0) || fadersNext<t) return closest;
	return Time(min(int(closest), int(fadersNext)));
}

ref<MediaBlock> MediaTrack::GetNextBlock(Time t) {
	Time closest = -1;
	ref<MediaBlock> next;

	std::vector<ref<MediaBlock> >::iterator it = _blocks.begin();
	while(it!=_blocks.end()) {
		ref<MediaBlock> bl = *it;
		Time begin = bl->GetTime();
		
		if(begin>t && ((abs(int(begin-t))<abs(int(closest-t)))||closest==Time(-1))) {
			closest = begin;
			next = bl;
		}

		++it;
	}
	
	return next;
}


void MediaTrack::RemoveBlock(ref<MediaBlock> bl) {
	std::vector<ref<MediaBlock> >::iterator it = _blocks.begin();
	while(it!=_blocks.end()) {
		ref<MediaBlock> block = *it;
		if(block==bl) {
			_blocks.erase(it);
			return;
		}
		++it;
	}
}

void MediaTrack::GetResources(std::vector< ResourceIdentifier>& rids) {
	std::vector<ref<MediaBlock> >::iterator it = _blocks.begin();
	while(it!=_blocks.end()) {
		ref<MediaBlock> block = *it;
		rids.push_back(block->GetFile());
		++it;
	}
}

ref<TrackRange> MediaTrack::GetRange(Time start, Time end) {
	return GC::Hold(new MultifaderTrackRange(this, start, end));
}

void MediaTrack::RemoveItemsBetween(Time start, Time end) {
	std::vector< ref<MediaBlock> >::iterator it = _blocks.begin();
	while(it!=_blocks.end()) {
		ref<MediaBlock> block = *it;
		if(block) {
			Time t = block->GetTime();
			Time e = t + block->GetDuration(_pb);
			if((t >= start && t<=end) || (e <= end && e>=start)) {
				it = _blocks.erase(it);
				continue;
			}
		}
		++it;
	}
	MultifaderTrack::RemoveItemsBetween(start,end);
}

ref<Item> MediaTrack::GetItemAt(Time position, unsigned int h, bool rightClick, int th, float pixelsPerMs) {
	ref<Item> faderItem = MultifaderTrack::GetItemAt(position, h, rightClick, th, pixelsPerMs);

	if(!faderItem) {
		if(rightClick && int(h)>(th-MultifaderTrack::GetFaderHeight())) {
			ref<MediaBlock> block = GC::Hold(new MediaBlock(position, _defaultFile, _isVideo));
			_blocks.push_back(block);
			return GC::Hold(new MediaTrackItem(MediaTrackItem::VariableBegin,block, position, this));
		}
		else  {
			ref<MediaBlock> block = GetBlockAt(position);
			if(block) {
				Time end = block->GetDuration(_pb)+block->GetTime();
				
				// if we're clicking near to the end of the block, drag it. Otherwise, drag
				// the beginning of the block (the block itself)
				// TODO put the 0.2f (20%) into some constant (it means '20% of the block at the end will drag the end time')
				if(position<end && position.ToInt()>=end.ToInt()-int(0.2f*block->GetDuration(_pb).ToInt())) {
					return GC::Hold(new MediaTrackItem(MediaTrackItem::VariableEnd,block, position, this));
				}
				else {
					return GC::Hold(new MediaTrackItem(MediaTrackItem::VariableBegin,block, position, this));
				}
			}
		}
		return 0;
	}
	else {
		return faderItem;
	}
}

void MediaTrack::SaveFader(TiXmlElement* parent, int id, const char* name) {
	TiXmlElement el(name);
	SaveAttributeSmall<bool>(&el, "enabled", IsFaderEnabled(id));
	GetFaderById(id)->Save(&el);
	parent->InsertEndChild(el);
}

void MediaTrack::LoadFader(TiXmlElement* parent, int id, const char* name) {
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

void MediaTrack::Save(TiXmlElement* parent) {
	SaveAttributeSmall(parent, "card", _card);
	SaveAttributeSmall(parent, "balance", _balance);
	SaveAttribute(parent, "default-file", _defaultFile);
	SaveAttribute(parent, "masters", _masters);
	_color.Save(parent);

	SaveFader(parent, FaderVolume, "volume-fader");

	if(_isVideo) {
		SaveAttributeSmall(parent, "screen", _screen);
		SaveFader(parent, FaderOpacity, "opacity-fader");
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
	}

	TiXmlElement blocks("blocks");
	std::vector< ref<MediaBlock> >::iterator it = _blocks.begin();
	while(it!=_blocks.end()) {
		ref<MediaBlock> block = *it;
		block->Save(&blocks);
		++it;
	}
	parent->InsertEndChild(blocks);
	
	TiXmlElement color("color");
	_color.Save(&color);
	parent->InsertEndChild(color);
}

bool MediaTrack::PasteItemAt(const Time& position, TiXmlElement* item) {
	TiXmlElement* blockElement = item->FirstChildElement("block");
	if(blockElement!=0) {
		ref<MediaBlock> block = GC::Hold(new MediaBlock(position, L"", _isVideo));
		block->Load(blockElement);
		block->SetTime(position);
		_blocks.push_back(block);
		return true;
	}
	return false;
}

void MediaTrack::Load(TiXmlElement* you) {
	_card = LoadAttributeSmall<PatchIdentifier>(you,"card", _card);
	_balance = LoadAttributeSmall<int>(you, "balance", 0);
	_defaultFile = LoadAttribute<std::wstring>(you, "default-file", L"");
	_masters = LoadAttribute<std::wstring>(you, "masters", _masters);
	_screen = LoadAttributeSmall<PatchIdentifier>(you, "screen", _screen);

	// Older volume faders range from [-10000, 0] and old opacity faders use [0,255]. If found, load them
	TiXmlElement* volume = you->FirstChildElement("volume");
	ref< Fader<float> > trackVolumeFader = GetFaderById(FaderVolume);

	LoadFader(you, FaderVolume, "volume-fader");

	TiXmlElement* color = you->FirstChildElement("color");
	if(color!=0) {
		_color.Load(color);
	}

	if(_isVideo) {
		LoadFader(you, FaderOpacity, "opacity-fader");
		LoadFader(you, FaderTranslate, "translate-fader");
		LoadFader(you, FaderRotate, "rotate-fader");
		LoadFader(you, FaderScale, "scale-fader");

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
	}

	_blocks.clear();
	TiXmlElement* blocks = you->FirstChildElement("blocks");
	if(blocks!=0) {
		TiXmlElement* block = blocks->FirstChildElement("block");
		while(block!=0) {
			ref<MediaBlock> mbl = GC::Hold(new MediaBlock(0,L"", _isVideo));
			mbl->Load(block);
			_blocks.push_back(mbl);
			block = block->NextSiblingElement("block");
		}
	}
}

int MediaTrack::GetBalance() {
	return _balance;
}

std::wstring MediaTrack::GetTypeName() const {
	return _isVideo ? std::wstring(TL(media_track_type)) : std::wstring(TL(media_audio_track_type));
}

ref<Device> MediaTrack::GetCardDevice() {
	 return _pb->GetDeviceByPatch(_card);
}

ref<Device> MediaTrack::GetScreenDevice() {
	return _pb->GetDeviceByPatch(_screen);
}

ref<Deck> MediaTrack::CreateDeck() {
	return _pb->CreateDeck(true, _pb->GetDeviceByPatch(_screen));
}

const PatchIdentifier& MediaTrack::GetCardPatch() {
	return _card;
}

Pixels MediaTrack::GetHeight() {
	return MultifaderTrack::GetHeight();
}

ref<PropertySet> MediaTrack::GetProperties() {
	ref<PropertySet> ps = GC::Hold(new PropertySet());

	ps->Add(_pb->CreateSelectPatchProperty(TL(media_soundcard), this, &_card));

	if(_isVideo) {
		ps->Add(_pb->CreateSelectPatchProperty(TL(media_screen), this, &_screen));
	}

	ps->Add(CreateChooseFadersProperty(TL(faders_choose), this, TL(faders_choose_hint)));
	ps->Add(_plugin->GetMasters()->CreateMasterListProperty(TL(media_masters), this, &_masters));
	
	ps->Add(GC::Hold(new GenericProperty<int>(TL(media_balance), this, &_balance, _balance)));
	ps->Add(GC::Hold(new GenericProperty<std::wstring>(TL(media_default_file), this, &_defaultFile, _defaultFile)));
	ps->Add(GC::Hold(new ColorProperty(TL(media_color), this, &_color)));

	if(_isVideo) {
		ps->Add(GC::Hold(new PropertySeparator(TL(media_video_position), true)));

		ps->Add(GC::Hold(new VectorProperty(TL(media_video_translate), &_itranslate)));
		ref<VectorProperty> svp = GC::Hold(new VectorProperty(TL(media_video_scale), &_iscale));
		svp->SetDimensionShown(2, false); // Hide the field for the Z-dimension, since that doesn't make sense to use
		
		ps->Add(svp);
		ps->Add(GC::Hold(new VectorProperty(TL(media_video_rotate), &_irotate)));

		if(IsFaderEnabled(FaderTranslate) || IsFaderEnabled(FaderRotate) || IsFaderEnabled(FaderScale)) {
			ps->Add(GC::Hold(new PropertySeparator(TL(media_video_position_faders), true)));
			if(IsFaderEnabled(FaderTranslate)) {
				ps->Add(GC::Hold(new VectorProperty(TL(media_video_translate), &_translate)));
			}
			
			if(IsFaderEnabled(FaderScale)) {
				ps->Add(GC::Hold(new VectorProperty(TL(media_video_scale), &_scale)));
			}

			if(IsFaderEnabled(FaderRotate)) {
				ps->Add(GC::Hold(new VectorProperty(TL(media_video_rotate), &_rotate)));
			}
		}
	}

	return ps;
}

ref<Player> MediaTrack::CreatePlayer(ref<Stream> str) {
	return GC::Hold(new MediaPlayer(strong<MediaTrack>(this),_plugin, str));
}

ref<LiveControl> MediaTrack::CreateControl(ref<Stream> str) {
	if(!_control) {
		_control = GC::Hold(new MediaControl(this, str, _isVideo));
	}
	return _control;
}

ref< Fader<float> > MediaTrack::GetOpacity() {
	return _isVideo ? GetFaderById(FaderOpacity) : null;
}

void MediaTrack::InsertFromControl(Time t, ref<LiveControl> control, bool fade) {
	if(control.IsCastableTo<MediaControl>()) {
		ref<MediaControl> mcc = control;
		GetFaderById(FaderVolume)->AddPoint(t, mcc->GetVolumeValue(), fade);

		if(_isVideo) {
			GetFaderById(FaderOpacity)->AddPoint(t, mcc->GetOpacityValue(), fade);
		}
	}
}

void MediaTrack::Paint(tj::shared::graphics::Graphics* g, Time position, float pixelsPerMs, unsigned int x, unsigned int y, unsigned int h, Time start, Time end, ref<Item> focus) {
	ref<MediaBlock> focused = 0;
	if(focus.IsCastableTo<MediaTrackItem>()) {
		ref<MediaTrackItem> mti = focus;
		focused = mti->GetBlock();
	}

	strong<Theme> theme = ThemeManager::GetTheme();
	std::vector< ref<MediaBlock> >::iterator it = _blocks.begin();
	int blockHeight = MultifaderTrack::GetFaderHeight()-4;

	Color a = _color;
	Color b = Theme::ChangeAlpha(a,155);
	LinearGradientBrush lbr(PointF(0.0f,(float)y), PointF(0.0f,float(y+blockHeight)), a,b);
	tj::shared::graphics::SolidBrush descBrush = theme->GetColor(Theme::ColorDescriptionText);
	
	if(_blocks.size()<=0) {
		StringFormat sf;
		sf.SetAlignment(StringAlignmentCenter);
		Font* fnt = theme->GetGUIFontSmall();

		std::wstring str(TL(media_track_add_block));
		g->DrawString(str.c_str(), (INT)str.length(), fnt, PointF((float)(((int(end-start)*pixelsPerMs)/2)+x), (float)((blockHeight/4)+y)), &sf, &descBrush);
	}
	else {
		SolidBrush tbr(theme->GetColor(Theme::ColorText));
		SolidBrush stbr(Theme::ChangeAlpha(theme->GetColor(Theme::ColorBackground),172));

		while(it!=_blocks.end()) {
			ref<MediaBlock> block = *it;

			int cx = x + int(pixelsPerMs * int(block->GetTime()-start));
			int duration = int(block->GetDuration(_pb));
			bool valid = block->IsValid(_pb);

			Area blockRC(cx,y, Pixels(duration*pixelsPerMs), blockHeight);
			g->FillRectangle(&lbr, blockRC);

			// If this block is invalid, draw a red border around it
			if(!valid) {
				Area border = blockRC;
				border.Narrow(1,1,1,1);
				// TODO: move red color to Theme::ColorError or something like that
				SolidBrush errorBrush(Color(255,0,0));
				Pen errorPen(&errorBrush, 2.0f);
				g->DrawRectangle(&errorPen, border);
			}
			else {
				// Draw block background
				block->Paint(*g, blockRC, theme, start, pixelsPerMs);
			}
			
			// draw filename
			std::wstring path = block->GetFile();

			std::wstring fn;
			if(block->IsLiveSource()) {
				fn = TL(media_live_feed) + Stringify(block->GetLiveSource());
			}
			else {
				fn = PathFindFileName(path.c_str());
			}
			StringFormat sf;
			
			sf.SetFormatFlags(StringFormatFlagsNoWrap);
			sf.SetTrimming(StringTrimmingEllipsisCharacter);
			sf.SetAlignment(StringAlignmentNear);
			Area textArea(cx+5,y+2, int(duration*pixelsPerMs)-10, blockHeight-2);
			AreaF shadowArea(float(textArea.GetLeft()), float(textArea.GetTop()), float(textArea.GetWidth()), float(textArea.GetHeight()));
			shadowArea.Translate(0.8f, 0.8f);
			g->DrawString(fn.c_str(), (int)fn.length(), theme->GetGUIFontBold(), shadowArea, &sf, &stbr);
			g->DrawString(fn.c_str(), (int)fn.length(), theme->GetGUIFontBold(), textArea, &sf, &tbr);

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

	MultifaderTrack::Paint(g,position,pixelsPerMs, x,y,h,start,end, focus);
}