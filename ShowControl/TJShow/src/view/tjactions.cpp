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
#include "../../include/internal/tjshow.h"
#include "../../include/internal/view/tjactions.h"
#include "../../include/internal/view/tjtimelinewnd.h"
#include "../../include/internal/tjclientcachemgr.h"
#include "../../include/internal/tjfileserver.h"
#include "../../include/internal/tjsubtimeline.h"
#include "../../include/internal/tjnetwork.h"
#include <TJZip/include/tjzip.h>
#include <shlobj.h>

using namespace tj::show;
using namespace tj::show::view;
using namespace tj::zip;
using namespace tj::shared::graphics;

/** OpenFileAction **/
OpenFileAction::OpenFileAction(Application* app, std::wstring path, ref<Model> model, bool import): Action(TL(command_open_file), Action::UndoBlocking), _app(app), _import(import), _file(path), _model(model) {
	assert(app!=0 && model);
}

OpenFileAction::~OpenFileAction() {
}

void OpenFileAction::Execute() {
	if(_file.length()>0) {
		// If we're not importing, playback will be stopped
		FileReader fr;
		if(!_import) {
			_model->New();
		}

		fr.Read(Mbs(_file), _model);
		SHAddToRecentDocs(SHARD_PATH, _file.c_str());

		if(!_import) {
			_app->GetView()->OnFileLoaded(_model, _file);
			_model->SetFileName(_file);
		}
		_app->Update();
	}
}

/** SaveFileAction **/
SaveFileAction::SaveFileAction(Application* app, ref<Model> model, Type type, const std::wstring& fn): Action(TL(command_save_file), Action::UndoBlocking), _type(type), _fn(fn), _app(app), _model(model) {
	assert(app!=0 && model);
}

SaveFileAction::~SaveFileAction() {
}

void SaveFileAction::Execute() {
	FileWriter fw("tjshow");
	fw.Add(_model);

	// Compute hash of the file we just generated
	SecureHash savedHashCalculator;
	strong<TiXmlElement> el = fw.GetRoot();
	TiXmlElement* modelElement = el->FirstChildElement("model");
	if(modelElement==0) return;

	XML::GetElementHash(dynamic_cast<TiXmlNode*>(modelElement), savedHashCalculator);
	std::string savedHash = savedHashCalculator.GetHashAsString();

	// Get the right file name; if there is any
	std::wstring fn;
	if(_fn.length()!=0) {
		fn = _fn;
	}
	else {
		fn = _model->GetFileName();
	}

	if(_type==TypeSaveOnExit) {
		// The file we have generated is different from the file that was loaded; ask to save
		if(savedHash!=_model->GetFileHash()) {
			std::wstring text(std::wstring(TL(save_prompt)));
			std::wstring fn = _model->GetFileName();
			if(fn.length()>0) {
				text = text + L" (" + File::GetFileName(fn) + L")";
			}

			if(!Alert::ShowYesNo(TL(application_name), text, Alert::TypeQuestion)) {
				return;
			}
		}
		else {
			return; // No need to save
		}
	}

	if(fn.length()<1 || _type==TypeSaveAs) {
		fn = Dialog::AskForSaveFile(_app->GetView()->GetRootWindow(), TL(save_file_select), L"TJShow (*.tsx)\0*.tsx\0\0", L"tsx");
	}

	if(fn.length()>0) {
		/* Update the version number (we can't do this in Model::Save, since then the XML would always have changed,
		and our check with the hashes above won't work). */
		ref<TiXmlElement> root = fw.GetRoot();
		TiXmlElement* model = root->FirstChildElement("model");
		if(model!=0) {
			SaveAttributeSmall(model, "version", _model->IncrementVersion());
		}

		fw.Save(Mbs(fn));
		_model->SetFileHash(savedHash);
		_model->SetFileName(fn);
		_app->GetView()->OnFileSaved(_model, fn);
	}

	_app->Update();
}

/** InfoAction **/
InfoAction::InfoAction(ref<Application> app): Action(Action::UndoIgnore) {
	_app = app;
}

void InfoAction::Execute() {
	std::wostringstream info;
	info << L"GC: " << tj::shared::intern::Resource::GetResourceCount() << L"Threads: " << Thread::GetThreadCount();
	Log::Write(TL(application_name), info.str());

	wchar_t buf[MAX_PATH+1];
	_wgetcwd(buf, MAX_PATH);
	Log::Write(TL(application_name), L"CWD: "+std::wstring(buf));

	ref<Network> nw = _app->GetNetwork();
	if(nw) {
		Log::Write(TL(application_name), std::wstring(L"TNP Sent:")+ Util::GetSizeString(nw->GetBytesSent()) + L" Recvd:" + Util::GetSizeString(nw->GetBytesReceived())+L" Running Txs:"+Stringify(nw->GetActiveTransactionCount())+std::wstring(L" Waiting for redelivery: ")+Stringify(nw->GetWaitingPacketsCount()));
	}

	ref<tj::np::WebServer> fs = _app->GetFileServer();
	if(fs) {
		Log::Write(TL(application_name), L"FS Sent:"+Util::GetSizeString(fs->GetBytesSent())+L" Recvd: "+Util::GetSizeString(fs->GetBytesReceived()));
	}

	ref<network::ClientCacheManager> ccm = _app->GetNetwork()->GetClientCacheManager();
	if(ccm) {
		Log::Write(TL(application_name), L"Client cache size: "+Util::GetSizeString(ccm->GetCacheSize()));
	}

	Core::Instance()->ShowLogWindow(true);
}

/** BundleResourcesAction **/
namespace tj {
	namespace show {
		class BundleThread: public Thread {
			public:
				BundleThread(std::wstring r, const std::set<ResourceIdentifier>& resources, std::wstring tsx) {
					_file = r;
					_resources = resources;
					_tsx = tsx;
				}

				virtual ~BundleThread() {
					delete this;
				}

				virtual void Run() {
					Application::Instance()->GetEventLogger()->AddEvent(TL(resource_bundling_in_progress), ExceptionTypeMessage);

					{
						ref<Package> pkg = GC::Hold(new Package(_file));
						strong<ResourceProvider> showResources = Application::Instance()->GetModel()->GetResourceManager();
						std::set<ResourceIdentifier>::iterator it = _resources.begin();
						while(it!=_resources.end()) {
							ResourceIdentifier rid = *it;
							std::wstring fn;
							if(showResources->GetPathToLocalResource(rid, fn)) {
								pkg->Add(rid, fn);
							}
							else {
								Log::Write(L"TJShow/BundleThread", L"Could not add resource '"+rid+L"' to bundle, since it is non-local!");
							}
							++it;
						}

						// package some meta data
						std::wostringstream meta;
						meta << TL(application_name) << L" " << Version::GetRevisionDate() << L" " << Version::GetRevisionID() << L" \r\n";
						pkg->AddData(L".version", meta.str());

						// add show tsx
						pkg->Add(L"show.tsx", _tsx);
						DeleteFile(_tsx.c_str());
						
					}

					Application::Instance()->GetEventLogger()->AddEvent(TL(resource_bundling_done), ExceptionTypeMessage);
					Log::Write(L"TJShow/BundleThread", std::wstring(L"Archiving done:")+_file);
				}

				std::set<std::wstring> _resources;
				std::wstring _file;
				std::wstring _tsx;
		};
	}
}

void BundleResourcesAction::Execute() {
	std::wstring file = Dialog::AskForSaveFile(Application::Instance()->GetView()->GetRootWindow(), TL(resource_bundle_select_file), L"Zip (*.zip)\0*.zip\0\0", L"zip");
	if(file.length()<1) return;

	// gather files
	std::set<ResourceIdentifier> files;

	// export show to xml
	wchar_t fn[MAX_PATH+2];
	GetTempFileName(L"%TEMP%", L"TJ_", (unsigned int)(Timestamp(true).ToMilliSeconds()),fn); 
	FileWriter fw("tjshow");
	fw.Add(Application::Instance()->GetModel());
	fw.Save(Mbs(std::wstring(fn)));	

	ref<Model> model = _app->GetModel();
	if(model) {
		// track resources
		std::vector<ResourceIdentifier> res;
		model->GetTimeline()->GetResources(res);
		std::vector<ResourceIdentifier>::const_iterator it = res.begin();
		while(it!=res.end()) {
			files.insert(*it);
			++it;
		}

		// user resources
		ref<Resources> resources = model->GetResources();
		if(resources) {
			std::set<ResourceIdentifier>::iterator it = resources->GetResourceList()->begin();
			while(it!=resources->GetResourceList()->end()) {
				files.insert(*it);
				++it;
			}
		}
	}

	BundleThread* bpt = new BundleThread(file, files, std::wstring(fn));
	bpt->Start();
	// bpt will delete itself
}

BundleResourcesAction::BundleResourcesAction(Application* app): Action(TL(command_bundle_resources), Action::UndoIgnore), _app(app) {
}

BundleResourcesAction::~BundleResourcesAction() {
}

