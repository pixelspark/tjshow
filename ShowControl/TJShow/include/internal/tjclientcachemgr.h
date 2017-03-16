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
#ifndef _TJCLIENTCACHEMGR_H
#define _TJCLIENTCACHEMGR_H

namespace tj {
	namespace show {
		namespace network {
			class Download {
				friend class DownloadThread;

				public:
					Download(const std::wstring& rid, const std::wstring& url, in_addr from, unsigned short port);
					virtual ~Download();
					const std::wstring& GetResource() const;
					virtual void OnFinished();

				protected:
					std::wstring _rid;
					std::wstring _url;
					in_addr _from;
					unsigned short _port;
			};

			// Class for managing the resource cache on clients
			class ClientCacheManager: public virtual Object, public ResourceProvider {
				friend class DownloadThread;

				public:
					ClientCacheManager(const std::wstring& cacheDir);
					virtual ~ClientCacheManager();
					void NeedFile(const ResourceIdentifier& rid);
					void StartDownload(const ResourceIdentifier& rid, const std::wstring& url, in_addr from, unsigned short port);
					Bytes GetCacheSize(); // Take care, this can be expensive as it is read from the FS

					// ResourceProvider
					virtual ref<Resource> GetResource(const ResourceIdentifier& rid);
					virtual bool GetPathToLocalResource(const ResourceIdentifier& rid, std::wstring& path);
					virtual ResourceIdentifier GetRelative(const std::wstring& path);

				protected:
					ref<Thread> _downloadThread;
					CriticalSection _lock;
					Event _downloadAdded;
					Event _stopDownloadThread;
					std::set< std::wstring > _wishList;
					std::deque< ref<Download> > _downloads;
					strong<LocalFileResourceProvider> _localResources;
					std::wstring _dir;
			};
		}
	}
}

#endif