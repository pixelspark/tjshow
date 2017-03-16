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
#ifndef _TJDMXCOLORTRACK_H
#define _TJDMXCOLORTRACK_H

namespace tj {
	namespace dmx {
		namespace color {
			enum ColorChannel {
				ColorChannelRed = 0,
				ColorChannelGreen,
				ColorChannelBlue,
				ColorChannelCyan,
				ColorChannelMagenta,
				ColorChannelYellow,
				ColorChannelHue,
				ColorChannelSaturation,
				ColorChannelValue,
				_ColorChannelLast,
			};

			class DMXColorTrack: public MultifaderTrack, public Listener<ColorPopupWnd::NotificationChanged> {
				friend class DMXColorPlayer;
				friend class DMXColorLiveWnd;
				friend class DMXColorLiveControl;

				public:
					DMXColorTrack(ref<DMXPlugin> dp);
					virtual ~DMXColorTrack();
					virtual std::wstring GetTypeName() const;
					virtual tj::shared::Flags<RunMode> GetSupportedRunModes();
					virtual ref<Player> CreatePlayer(ref<Stream> stream);
					virtual ref<PropertySet> GetProperties();
					virtual ref<Item> GetItemAt(Time position,unsigned int h, bool rightClick, int trackHeight, float pixelsPerMs);
					virtual void Paint(tj::shared::graphics::Graphics* g, Time position, float pixelsPerMs, unsigned int x, unsigned int y, unsigned int h, Time start, Time end, ref<Item> focus);
					virtual void Save(TiXmlElement* parent);
					virtual void Load(TiXmlElement* you);
					virtual void Notify(ref<Object> source, const ColorPopupWnd::NotificationChanged& ev);
					virtual ref<LiveControl> CreateControl(ref<Stream> str);

					enum Faders {
						KFaderHue = 1,
						KFaderSaturation,
						KFaderValue,
					};

				protected:
					std::wstring _dmx[_ColorChannelLast];
					ref<DMXPlugin> _plugin;

					Time _colorPopupCurrent;
					ref<ColorPopupWnd> _colorPopup;
					HSVColor _lastColor;
			};
		}
	}
}

#endif