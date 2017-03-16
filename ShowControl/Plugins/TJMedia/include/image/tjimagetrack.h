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
#ifndef _TJIMAGETRACK_H
#define _TJIMAGETRACK_H

#include "tjmediamasters.h"

class ImageBlock: public Serializable, public virtual Inspectable {
	public:
		ImageBlock(Time start);
		virtual ~ImageBlock();
		virtual ref<PropertySet> GetProperties(ref<Playback> pb);
		virtual void Move(Time t, int h);
		virtual void Save(TiXmlElement* parent);
		virtual void Load(TiXmlElement* you);

		virtual Time GetDuration();
		virtual Time GetTime() const;
		virtual void SetTime(Time start);
		virtual void SetDuration(Time t);
		virtual void Prepare(ref<Playback> pb);

		virtual std::wstring GetTooltipText() const = 0;
		virtual std::wstring GetBlockDisplayText() const = 0;
		virtual std::wstring GetBlockText() const = 0;
		virtual tj::shared::graphics::Bitmap* GetThumbnail(ref<Playback> pb) = 0;
		virtual tj::shared::graphics::Bitmap* GetBitmap(ref<Playback> pb) = 0;
		virtual tj::shared::graphics::Bitmap* GetDownBitmap(ref<Playback> pb) = 0;
		virtual void GetResources(std::vector<ResourceIdentifier>& rids) = 0;
		virtual void Send(ref<Stream> stream, ref<Playback> pb, const PatchIdentifier& screen) = 0;

		virtual void OnKey(const Interactive::KeyNotification& kn);
		virtual void OnFocus(bool f);
	private:
		Time _start;
		Time _duration;

	protected:
		const static float KThumbnailSize;
};

class FileImageBlock: public ImageBlock {
	public:
		FileImageBlock(Time start, const std::wstring& rid);
		virtual ~FileImageBlock();
		ResourceIdentifier GetFile() const;
		ResourceIdentifier GetDownFile() const;
		bool HasDifferentDownImage() const;
		virtual tj::shared::graphics::Bitmap* GetThumbnail(ref<Playback> pb);
		virtual tj::shared::graphics::Bitmap* GetBitmap(ref<Playback> pb);
		virtual tj::shared::graphics::Bitmap* GetDownBitmap(ref<Playback> pb);
		virtual ref<PropertySet> GetProperties(ref<Playback> pb);
		virtual std::wstring GetTooltipText() const;
		virtual std::wstring GetBlockDisplayText() const;
		virtual std::wstring GetBlockText() const;
		virtual void GetResources(std::vector<ResourceIdentifier>& rc);
		virtual void Save(TiXmlElement* parent);
		virtual void Load(TiXmlElement* you);
		virtual void Send(ref<Stream> stream, ref<Playback> pb, const PatchIdentifier& screen);

	protected:
		CriticalSection _thumbLock;
		ResourceIdentifier _file;
		ResourceIdentifier _downFile;
		tj::shared::graphics::Bitmap* _thumbnail;
		tj::shared::graphics::Bitmap* _bitmap;
		tj::shared::graphics::Bitmap* _downBitmap;
		int _thumbnailHash;
};

class TextImageBlock: public ImageBlock {
	public:
		TextImageBlock(Time start);
		virtual ~TextImageBlock();
		virtual tj::shared::graphics::Bitmap* GetThumbnail(ref<Playback> pb);
		virtual tj::shared::graphics::Bitmap* GetBitmap(ref<Playback> pb);
		virtual tj::shared::graphics::Bitmap* GetDownBitmap(ref<Playback> pb);
		virtual ref<PropertySet> GetProperties(ref<Playback> pb);
		virtual std::wstring GetTooltipText() const;
		virtual std::wstring GetBlockText() const;
		virtual std::wstring GetBlockDisplayText() const;
		virtual void GetResources(std::vector<ResourceIdentifier>& rc);
		virtual void Save(TiXmlElement* parent);
		virtual void Load(TiXmlElement* you);
		virtual void Send(ref<Stream> stream, ref<Playback> pb, const PatchIdentifier& screen);
		virtual void OnKey(const Interactive::KeyNotification& kn);
		virtual void OnFocus(bool f);

		virtual bool IsTextField() const;
		virtual void SetIsTextField(bool t);
		virtual void Prepare(ref<Playback> pb);

	protected:
		CriticalSection _thumbLock;
		std::wstring _currentText;
		std::wstring _text;
		std::wstring _fontName;
		Pixels _width;
		Pixels _height;
		Pixels _fontSize;
		RGBColor _color;
		bool _showBorder;
		bool _isTextField;
		int _cursorPosition;
		tj::shared::graphics::Bitmap* _thumbnail;
		tj::shared::graphics::Bitmap* _bitmap;
		int _thumbnailHash;
};

class ImagePlugin;

class ImageTrack: public tj::show::MultifaderTrack {
	friend class ImagePlayer;
	friend class ImageTrackItem;

	public:
		ImageTrack(strong<ImagePlugin> ip, ref<Playback> pb);
		virtual ~ImageTrack();
		virtual Flags<RunMode> GetSupportedRunModes();
		virtual std::wstring GetTypeName() const;
		virtual ref<Player> CreatePlayer(ref<Stream> str);
		virtual ref<PropertySet> GetProperties();
		virtual void Paint(tj::shared::graphics::Graphics* g, Time position, float pixelsPerMs, unsigned int x, unsigned int y, unsigned int h, Time start, Time end, ref<Item> focus);
		virtual ref<LiveControl> CreateControl();
		virtual void GetResources(std::vector<ResourceIdentifier>& rids);
		virtual ref<TrackRange> GetRange(Time start, Time end);
		virtual void RemoveItemsBetween(Time start, Time end);
		virtual void CreateOutlets(OutletFactory& of);

		virtual ref<Item> GetItemAt(Time pos, unsigned int h, bool rightClick, int th, float pixelsPerMs);

		float GetTranslateAt(Time t);
		float GetRotateAt(Time t);
		float GetScaleAt(Time t);
		float GetOpacityAt(Time t);
		virtual Time GetNextEvent(Time t);

		// Block stuff
		ref<ImageBlock> GetBlockAt(Time t);
		ref<ImageBlock> GetNextBlock(Time t);
		void RemoveBlock(ref<ImageBlock> block);

		// Serializable
		virtual void Save(TiXmlElement* parent);
		virtual void Load(TiXmlElement* you);

		static std::wstring KClickedOutletID;
		static std::wstring KClickedXOutletID;
		static std::wstring KClickedYOutletID;
		static std::wstring KTextOutletID;

	protected:
		void SaveFader(TiXmlElement* parent, int id, const char* name);
		void LoadFader(TiXmlElement* parent, int id, const char* name);

		enum WhichFader {
			FaderNone = 0,
			FaderAlpha,
			FaderTranslate,
			FaderRotate,
			FaderScale,
		};

		Vector _translate, _rotate, _scale;
		Vector _itranslate, _irotate, _iscale;
		ref<Playback> _pb;
		std::vector< ref<ImageBlock> > _blocks;
		RGBColor _color;
		bool _resetOnStop;
		PatchIdentifier _screen;
		weak<ImagePlugin> _plugin;
		tj::media::master::MediaMasterList _masters;

		ref<Outlet> _clickedOutlet;
		ref<Outlet> _clickXOutlet;
		ref<Outlet> _clickYOutlet;
		ref<Outlet> _textOutlet;
};

/** Simple proxy class that instead of creating image blocks creates text blocks when right-clicking. We want
to have text and image tracks separated, so a track that is a text track will remain a text track, but it shares
the same code under the hood **/
class TextImageTrack: public ImageTrack {
	public:
		TextImageTrack(strong<ImagePlugin> ip, ref<Playback> pb);
		virtual ~TextImageTrack();
		virtual ref<Item> GetItemAt(Time pos, unsigned int h, bool rightClick, int th, float pixelsPerMs);
		virtual std::wstring GetTypeName() const;
		virtual void CreateOutlets(OutletFactory& of);
};

#endif