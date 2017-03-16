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
#include "../include/internal/tjshow.h"
#include "../include/internal/tjclientcachemgr.h"
#include "../include/internal/tjnetwork.h"

#include <shlobj.h>
#include <shellapi.h>
#include <winioctl.h>

using namespace tj::show::network;

namespace tj {
	namespace show {
		namespace network {
			class DownloadThread: public Thread {
				public:
					DownloadThread(ClientCacheManager* ccm) {
						_ccm = ccm;
					}

					virtual ~DownloadThread() {
					}

					static std::string URLEncode(const std::wstring& rid) {
						std::ostringstream os;
						std::wstring::const_iterator it = rid.begin();
						while(it!=rid.end()) {
							wchar_t current = *it;
							if(current==L' ') {
								os << '+';
							}
							else if(iswascii(current)==0) {
								os << '%' << std::hex << current;
							}
							else {
								os << (char)current;
							}
							++it;
						}
						return os.str();
					}

					virtual void Run() {
						Log::Write(L"TJShow/ClientCacheManager", L"Download thread started");

						Wait wait; 
						wait[_ccm->_downloadAdded][_ccm->_stopDownloadThread];

						while(true) {
							int r = wait.ForAny();
							if(r==0) {
								// download added
								ref<Download> download;
								
								{
									ThreadLock lock(&(_ccm->_lock));
									download =  *(_ccm->_downloads.begin());
									_ccm->_downloads.pop_front();
									_ccm->_downloadAdded.Reset();
								}

								SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
								if(sock==INVALID_SOCKET) {
									Log::Write(L"TJShow/ClientCacheManager/Download", L"Could not create socket!");
								}
								else {
									ref<Settings> settings = Application::Instance()->GetSettings();
									if(settings) {
										int webPort = StringTo<int>(settings->GetValue(L"net.web.port"), 0);

										sockaddr_in address;
										address.sin_addr = download->_from;
										address.sin_family = AF_INET;
										address.sin_port = htons(download->_port);

										if(connect(sock, (sockaddr*)&address, sizeof(address))==0) {
											Log::Write(L"TJShow/ClientCacheManager/Download", L"Connected to server");
											
											// Compose a nice request
											std::ostringstream request;
											request << "GET " << URLEncode(download->_url) << " HTTP/1.0\r\n\r\n";
											std::string requestString = request.str();

											if(send(sock, requestString.c_str(), (int)requestString.length()*sizeof(char), 0)<=0) {
												Log::Write(L"TJShow/ClientCacheManager/Download", L"Could not send request");
												
											}
											else {
												// Let's get the file names and create the destination directory
												std::wstring dir = _ccm->_dir + L"\\" + File::GetDirectory(download->_rid);
												std::wstring fn = _ccm->_dir + L"\\" + download->_rid;
												SHCreateDirectoryEx(NULL, dir.c_str(),NULL);
												
												// Make some temporary file name
												wchar_t tempPath[MAX_PATH+2];
												wchar_t tempFilePath[MAX_PATH+2];
												GetTempPath(MAX_PATH, &(tempPath[0]));
												if(GetTempFileName(tempPath, L"TJS", 0, &(tempFilePath[0]))==0) {
													Log::Write(L"TJShow/ClientCacheManager" , L"Could not create local temporary file!");
												}
												else {
													Log::Write(L"TJShow/ClientCacheManager", L"Local temp file path is "+std::wstring(tempFilePath));
												}
												
												HANDLE file = CreateFile(tempFilePath, FILE_WRITE_ACCESS, FILE_SHARE_READ, NULL, CREATE_ALWAYS, NULL, NULL);
												if(file!=INVALID_HANDLE_VALUE) {
													char buffer[4097];

													int bytes = 0;
													int entersRead = 0;
													std::ostringstream header;

													while(true) {
														int r = recv(sock, buffer, 4096,0);
												
														if(r>0) {
															char* dataStart = 0;
															unsigned int dataLength = 0;

															// Do not write the header to the file, wait until we recieve \r\n\r\n
															if(entersRead<2) {
																for(int a=0;a<r;a++) {
																	header << buffer[a];
																	if(buffer[a]==L'\r' || buffer[a]==L'\n') {
																		entersRead++;
																		if(entersRead>=4) {
																			dataStart = &(buffer[a+1]);
																			dataLength = r-a-1;
																		}
																	}
																}
															}
															// Header was already sent, just dump this packet to the file
															else {
																dataStart = buffer;
																dataLength = r;
															}

															if(dataStart!=0 && dataLength>0) {
																DWORD written = 0;
																WriteFile(file, dataStart, r, &written, NULL);
																bytes += r;
															}
														}
														else {
															break;
														}

													}
													Log::Write(L"TJShow/ClientCacheManager/Download", L"Header:" + Wcs(header.str())); 
													Log::Write(L"TJShow/ClientCacheManager/Download", L"File recieved ("+download->_rid+L": "+Stringify(bytes)+L" bytes)");
													
													// Move the file to the final destination
													CloseHandle(file);
													MoveFile(tempFilePath, fn.c_str());
													DeleteFile(tempFilePath);
													download->OnFinished();
												}
												closesocket(sock);
											}
										}
										else {
											Log::Write(L"TJShow/ClientCacheManager/Download", L"Could not connect to file server");
										}
									}
								}
							}
							else if(r==1) {
								// stop thread
								break;
							}
						}

						Log::Write(L"TJShow/ClientCacheManager", L"Download thread ended");
					}

				protected:
					ClientCacheManager* _ccm;
			};

			class ClientCachedResource: public Resource {
				public:
					ClientCachedResource(const ResourceIdentifier& rid, ref<ClientCacheManager> ccm): _ccm(ccm), _rid(rid) {
					}

					virtual ~ClientCachedResource() {
					}

					virtual bool Exists() const {
						return true;
					}

					virtual bool IsScript() const {
						return false;
					}

					virtual Bytes GetSize() {
						return -1;
					}

					virtual ResourceIdentifier GetIdentifier() const {
						return _rid;
					}

				private:
					weak<ClientCacheManager> _ccm;
					std::wstring _rid;
			};
		}
	}
}

ClientCacheManager::ClientCacheManager(const std::wstring& dir): _dir(dir), _localResources(GC::Hold(new LocalFileResourceProvider(dir))) {
	Log::Write(L"TJShow/ClientCacheManager", L"Using cache dir "+dir);
	_downloadThread = GC::Hold(new DownloadThread(this));
	_downloadThread->Start();
}

ClientCacheManager::~ClientCacheManager() {
	_stopDownloadThread.Signal();

	if(_dir.length()>0) {
		File::DeleteFiles(_dir, L"*.*");
	}
}

Bytes ClientCacheManager::GetCacheSize() {
	return File::GetDirectorySize(_dir);
}

void ClientCacheManager::NeedFile(const ResourceIdentifier& rid) {
	ThreadLock lock(&_lock);
	_wishList.insert(rid);
}

ref<Resource> ClientCacheManager::GetResource(const ResourceIdentifier& rid) {
	ref<Resource> cached = _localResources->GetResource(rid);
	if(cached) {
		return cached;
	}

	/* This has to call Network::NeedResource, which in turn will call ClientCacheManager::NeedFile,
	since otherwise the resource gets added to the wishlist (through NeedFile), but no resource find
	message is sent over the network */
	Application::Instance()->GetNetwork()->NeedResource(rid);

	return GC::Hold(new ClientCachedResource(rid, this));
}

bool ClientCacheManager::GetPathToLocalResource(const ResourceIdentifier& rid, std::wstring& path) {
	if(_localResources->GetPathToLocalResource(rid, path)) {
		return true;
	}
	
	// File not downloaded yet
	/* This has to call Network::NeedResource, which in turn will call ClientCacheManager::NeedFile,
	since otherwise the resource gets added to the wishlist (through NeedFile), but no resource find
	message is sent over the network */
	Application::Instance()->GetNetwork()->NeedResource(rid);
	return false; 
}

ResourceIdentifier ClientCacheManager::GetRelative(const std::wstring& path) {
	return L""; // TODO: do we have to implement this anyway?
}

void ClientCacheManager::StartDownload(const ResourceIdentifier& rid, const std::wstring& url, in_addr from, unsigned short port) {
	ThreadLock lock(&_lock);

	// check if this is on our wish list, and if it is, remove and download
	{
		std::set< std::wstring >::iterator it = _wishList.find(rid);
		if(it==_wishList.end()) {
			return; // not wished for
		}
		_wishList.erase(it);
	}

	// check if we're not already downloading this file
	std::deque< ref<Download> >::iterator it = _downloads.begin();
	while(it!=_downloads.end()) {
		ref<Download> dl = *it;
		if(dl) {
			if(dl->GetResource()==rid) {
				return;
			}
		}
		++it;
	}

	// add to a download queue
	ref<Download> download = GC::Hold(new Download(rid,url,from,port));
	_downloads.push_back(download);
	_downloadAdded.Signal();
}

tj::show::network::Download::Download(const std::wstring& rid, const std::wstring& url, in_addr from, unsigned short port) {
	_rid = rid;
	_url = url;
	_from = from;
	_port = port;
}

tj::show::network::Download::~Download() {
}

const std::wstring& tj::show::network::Download::GetResource() const {
	return _rid;
}

void tj::show::network::Download::OnFinished() {
}