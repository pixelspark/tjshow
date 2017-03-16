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
#ifndef _TJAUDIOANALYZER_H
#define _TJAUDIOANALYZER_H

#include <TJShared/include/tjshared.h>

namespace tj {
	namespace media {
		namespace analyzer {
			class AudioAnalysis: public virtual tj::shared::Object {
				friend class AudioAnalyzer;
				friend class AudioAnalysisTask;

				public:
					virtual ~AudioAnalysis();
					virtual std::wstring GetFilePath() const;
					virtual tj::shared::Time GetDuration();
					virtual bool HasPeaks() const;

					const static int KResolutionMS;
					const static tj::shared::Time KMaximumLength;

					tj::shared::CriticalSection _lock;
					unsigned char* _peaks;
					unsigned int _peaksSize;

				protected:
					bool LoadPeaksFromFile(const std::wstring& realPath);
					void SavePeaksToFile(const std::wstring& realPath);
					void SetPeakData(unsigned char* data, unsigned int dataSize);
					AudioAnalysis(const std::wstring& file);
					
					std::wstring _file;
					tj::shared::Time _duration;
			};

			class AudioAnalyzer: public virtual tj::shared::Object {
				public:
					virtual ~AudioAnalyzer();
					static tj::shared::ref<AudioAnalysis> Analyze(const std::wstring& realPath);
				
				protected:
					virtual tj::shared::ref<AudioAnalysis> _Analyze(const std::wstring& realPath);
				private:
					AudioAnalyzer();
					static tj::shared::strong<AudioAnalyzer> Instance();
					static tj::shared::ref<AudioAnalyzer> _instance;
			};
		}
	}
}

#endif