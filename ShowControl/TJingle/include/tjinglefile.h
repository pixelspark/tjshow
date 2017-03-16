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
#ifndef _TJINGLEFILE_H
#define _TJINGLEFILE_H

namespace tj {
	namespace jingle {
		class Jingle: public tj::shared::Inspectable, public tj::shared::Serializable {
			public:
				Jingle();
				virtual ~Jingle();
				virtual bool IsPlaying();
				virtual void Play();
				virtual tj::shared::ref<tj::shared::PropertySet> GetProperties();
				virtual std::wstring GetFile() const;
				virtual void SetFile(std::wstring fn);
				virtual std::wstring GetName();
				virtual void Stop();
				virtual void Load(bool asStream);
				virtual void Clear();
				virtual void Save(TiXmlElement* parent);
				virtual void Load(TiXmlElement* you);
				virtual bool IsEmpty();
				virtual bool IsLoaded();
				virtual bool IsLoadedAsSample();
				virtual bool IsLoadedAsStream();
				virtual void Unload();
				virtual void FadeOut();
				virtual void FadeIn();
				virtual float GetPosition();
				virtual int GetRemainingSeconds();
				virtual tj::shared::Time GetRemainingTime();

			protected:
				virtual std::wstring GetJingleID() const;

				std::wstring _file;
				HSAMPLE _sample;
				HSTREAM _stream;
				HCHANNEL _channel;
				std::wstring _loadedFile;
		};
	}
}

#endif