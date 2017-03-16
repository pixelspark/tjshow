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
#include "../include/internal/tjnetwork.h"
#include "../include/internal/tjclientcachemgr.h"
#include "../include/internal/tjfileserver.h"

using namespace tj::shared;
using namespace tj::show;
using namespace tj::show::network;

namespace tj {
	namespace show {
		namespace network {
			class AnnounceThread: public Thread {
				public:
					AnnounceThread(ref<Network> nw);
					virtual ~AnnounceThread();

					virtual void Run();

				protected:
					ref<Network> _network;
			};

			class ChannelTalkback: public virtual tj::shared::Object, public Talkback {
				public:
					ChannelTalkback(strong<Network> net, Channel ch, GroupID gid);
					virtual ~ChannelTalkback();
					
					virtual void SendOutletChange(const std::wstring& outletID, const Any& value);

				protected:
					Channel _channel;
					GroupID _group;
					weak<Network> _network;
			};
		}
	}
}

Network::Network(): _role(RoleNone), _isPrimaryMaster(false), _tryBecomePrimary(false) {
	// Generate instance ID
	_instance = Util::RandomInt();
	_auth = GC::Hold(new Authorizer());
	_filter = GC::Hold(new Filter());
	Log::Write(L"TJShow/Networking", L"Instance ID is "+StringifyHex(_instance));

	// Initialize client cache manager (but not on masters)
	wchar_t tp[MAX_PATH+2];
	GetTempPath(MAX_PATH, tp);
	std::wstring cacheDir = std::wstring(tp) + L"TJShow\\";
	CreateDirectory(cacheDir.c_str(), NULL);
	_ccm = GC::Hold(new ClientCacheManager(cacheDir));
}

void Network::Connect(ref<Settings> st) {
	ThreadLock lock(&_lock);

	std::string netAddress  = Mbs(st->GetValue(L"net.address"));
	_socket = GC::Hold(new ShowSocket(StringTo<int>(st->GetValue(L"net.port"), 0), netAddress.c_str(), this));
	_settings = st;

	// only set 'tryBecomePrimary' if the settings say so
	if(_role==RoleMaster) {
		_tryBecomePrimary = st->GetFlag(L"net.server.try-become-primary");
	}
}

void Network::AddEvent(const std::wstring& message, ExceptionType e, bool read) {
	if(_socket) {
		_socket->SendError(FeatureEventLogger, e, message);
	}
}

int Network::GetInstanceID() const {
	return _instance;
}

void Network::SetRole(Role r) {
	_role = r;
	Announce();
}

bool Network::IsPrimaryMaster() const {
	return _isPrimaryMaster;
}

bool Network::IsConnected() const {
	return _socket;
}

void Network::Disconnect() {
	ThreadLock lock(&_lock);

	if(_socket) {
		_socket->SendLeave();
		_socket = 0;
		_isPrimaryMaster = false;
	}
}

void Network::OnReceive(int instance, in_addr from, const PacketHeader& ph, ref<DataReader> code) {
	ThreadLock lock(&_lock);
	unsigned int pos = 0;

	if(ph._action==ActionNothing) {
		return;
	}	
	else if(ph._action==ActionAnnounce) {
		Role role = code->Get<Role>(pos);
		Features feats = code->Get<Features>(pos);
		TransactionIdentifier ti = code->Get<TransactionIdentifier>(pos);
		std::wstring address = code->Get<std::wstring>(pos);		

		OnReceiveAnnounce(instance, role, address, from, feats, ti);
	}
	else if(ph._action==ActionAnnounceReply) {
		Role role = code->Get<Role>(pos);
		Features feats = code->Get<Features>(pos);
		std::wstring address = code->Get<std::wstring>(pos);			
		OnReceiveAnnounceReply(instance, role, address, from, feats);
	}
	else if(ph._action==ActionLeave) {
		OnReceiveLeave(ph._from, from);
	}
	else if(ph._action==ActionUpdate||ph._action==ActionUpdatePlugin) {
		OnReceive(code, ph, ph._action==ActionUpdatePlugin);
	}
	else if(ph._action==ActionSetAddress) {
		InstanceID id = code->Get<InstanceID>(pos);
		std::wstring newAddress = code->Get<std::wstring>(pos);
		OnReceiveSetAddress(id, newAddress, from);
	}
	else if(ph._action==ActionFindResource) {
		std::wstring ident = code->Get<std::wstring>(pos);
		TransactionIdentifier tid = code->Get<TransactionIdentifier>(pos);
		OnReceiveResourceFind(instance, ident, tid);
	}
	else if(ph._action==ActionAdvertiseResource) {
		// Ignore resource advertisements that we send ourselves
		if(ph._from!=_instance) {
			unsigned short port = code->Get<unsigned short>(pos);
			std::wstring rid = code->Get<std::wstring>(pos);
			std::wstring url = code->Get<std::wstring>(pos);
			OnReceiveResourceAdvertise(rid, url, from, port);
		}
	}
	else if(ph._action==ActionPushResource) {
		if(ph._from==_instance) {
			std::wstring rid = code->Get<std::wstring>(pos);
			OnReceiveResourcePush(rid, from);
		}
	}
	else if(ph._action==ActionReportError) {
		Features fs = code->Get<Features>(pos);
		ExceptionType type = code->Get<ExceptionType>(pos);
		std::wstring msg = code->Get<std::wstring>(pos);
		OnReceiveError(instance, fs, type, msg);
	}
	else if(ph._action==ActionListDevices) {
		InstanceID to = code->Get<InstanceID>(pos);
		TransactionIdentifier ti = code->Get<TransactionIdentifier>(pos);
		OnReceiveListDevices(ph, to, from, ti);
	}
	else if(ph._action==ActionListPatches) {
		InstanceID to = code->Get<InstanceID>(pos);
		TransactionIdentifier ti = code->Get<TransactionIdentifier>(pos);
		OnReceiveListPatches(ph, to, from, ti);
	}
	else if(ph._action==ActionResetAll) {
		OnReceiveResetAll();
	}
	else if(ph._action==ActionResetChannel) {
		OnReceiveResetChannel(ph._group, ph._channel);
	}
	else if(ph._action==ActionSetPatch) {
		InstanceID id = code->Get<InstanceID>(pos);
		if(id==GetInstanceID()) {
			PatchIdentifier pi = code->Get<PatchIdentifier>(pos);
			DeviceIdentifier di = code->Get<DeviceIdentifier>(pos);
			OnReceiveSetPatch(pi,di);
		}
	}
	else if(ph._action==ActionInput) {
		PatchIdentifier pi = code->Get<PatchIdentifier>(pos);
		InputID path = code->Get<InputID>(pos);
		float value = code->Get<float>(pos);
		OnReceiveInput(pi, path, value);
	}
	else if(ph._action==ActionPromoted) {
		if(ph._from!=GetInstanceID()) {
			Demote(true);
		}
		else {
			Log::Write(L"TJShow/Network", L"I, for one, welcome my new overlord (primary master): "+StringifyHex(ph._from));
		}
	}
	else if(ph._action==ActionDemoted) {
		Log::Write(L"TJShow/Network", L"Primary master was actively demoted: "+StringifyHex(ph._from)+L"; failovers may start");
	}
	else if(ph._action==ActionOutletChange) {
		Channel ch = code->Get<Channel>(pos);
		GroupID gid = code->Get<GroupID>(pos);
		Any::Type type = (Any::Type)code->Get<unsigned int>(pos);
		Any value(type, code->Get<std::wstring>(pos));
		OutletHash outletID = code->Get<OutletHash>(pos);
		OnReceiveOutletChange(ph._from, ch, gid, outletID, value);
	}
	else {
		Log::Write(L"TJShow/Network", L"Packet received with an unknown action");
	}
}

bool Network::IsExpired() const {
	return false;
}

void Network::GetCachedClients(std::vector< ref<Client> >& lst) {
	std::set< ref<Client> >::iterator it = _cachedClients.begin();
	while(it!=_cachedClients.end()) {
		lst.push_back(*it);
		++it;
	}
}

void Network::OnReceiveListDevices(const PacketHeader& ph, InstanceID to, in_addr from, TransactionIdentifier ti) {
	if(to==GetInstanceID()) {
		std::map<DeviceIdentifier, ref<Device> >* devs =  PluginManager::Instance()->GetDevices();
		if(devs!=0) {
			std::map<DeviceIdentifier, ref<Device> >::iterator it = devs->begin();
			while(it!=devs->end()) {
				const DeviceIdentifier& di = it->first;
				ref<Device> dev = it->second;
				if(dev) {
					std::map<DeviceIdentifier, ref<Device> >::iterator cit = it;
					++cit;
					_socket->SendListDevicesReply(di, dev->GetFriendlyName(), ti, from, (unsigned int)devs->size());
				}
				++it;
			}
		}
	}
}

void Network::OnReceiveListPatches(const PacketHeader& ph, InstanceID to, in_addr from, TransactionIdentifier ti) {
	if(to==GetInstanceID()) {
		ref<Patches> p = Application::Instance()->GetModel()->GetPatches();
		if(p) {
			std::map<PatchIdentifier, DeviceIdentifier>& patches = p->_patches;
			std::map<PatchIdentifier, DeviceIdentifier>::const_iterator it = patches.begin();
			
			while(it!=patches.end()) {
				std::map<PatchIdentifier, DeviceIdentifier>::const_iterator cit = it;
				++cit;
				_socket->SendListPatchesReply(it->first, it->second, ti, from, (unsigned int)patches.size());
				++it;
			}
		}
	}
}

Role Network::GetRole() const {
	return _role;
}

ref<Filter> Network::GetFilter() {
	return _filter;
}

void Network::Announce() {
	ThreadLock lock(&_lock);

	// Masters do a periodic announce using the announce thread, clients can do it instantaneously
	if(_role==RoleMaster) {
		if(!_announceThread) {
			_announceThread = GC::Hold(new AnnounceThread(this));
			_announceThread->Start();
		}
		
		_announceEvent.Signal();
	}
	else {
		DoAnnounce();
	}
}

std::map<InstanceID, ref<Client> >* Network::GetClients() {
	return &_clients;
}

ref<ClientCacheManager> Network::GetClientCacheManager() {
	return _ccm;
}

Features Network::GetFeatures() const {
	Features fs = 0;

	// If we are primary master, tell the others
	if(IsPrimaryMaster()) {
		fs |= FeaturePrimaryMaster;
	}

	// If we have a file server, set feature
	if(Application::Instance()->GetFileServer()) {
		fs |= FeatureFileServer;
	}

	// Power features
	Flags<Power::Status> powerStatus = Power::GetStatus();
	if(powerStatus.IsSet(Power::PowerHasBattery)) {
		fs |= FeatureBatteryPower;
	}
	if(powerStatus.IsSet(Power::PowerHasAC)) {
		fs |= FeatureACPower;
	}
	if(powerStatus.IsSet(Power::PowerIsOnACBackup)) {
		fs |= FeatureBackupPower;
	}
	
	return fs;
}

std::wstring Network::FormatFeatures(const Features& fs) {
	std::wostringstream wos;

	if((fs & FeaturesUnknown)!=0) {
		return TL(feature_unknown);
	}

	if((fs & FeaturePrimaryMaster)!=0) {
		wos << TL(feature_primary_master) << L" ";
	}

	if((fs & FeatureAutomaticFailover)!=0) {
		wos << TL(feature_automatic_failover) << L" ";
	}

	if((fs & FeatureFileServer)!=0) {
		wos << TL(feature_file_server) << L" ";
	}

	if((fs & FeatureACPower) !=0) {
		wos << TL(feature_ac_power) << L" ";
	}

	if((fs & FeatureBackupPower) != 0) {
		wos << TL(feature_backup_power) << L" ";
	}

	if((fs & FeatureBatteryPower) != 0) {
		wos << TL(feature_battery_power) << L" ";
	}

	return wos.str();
}

namespace tj {
	namespace show {
		namespace network {
			// This transaction handles incoming ActionAnnounceReply messages,and if it sees a primary master, it sets a flag.
			// When it expires, and it hasn't seen a primary master, then it will promote this node to be the new primary master.
			class PromotionTransaction: public Transaction {
				public:
					PromotionTransaction(ref<Network> net): Transaction(5000), _network(net), _primaryFound(false) {
						assert(net);
						Log::Write(L"TJShow/Network", L"Promotion transaction started");
					}

					virtual ~PromotionTransaction() {
					}

					virtual void OnReceive(int instance, in_addr from, const PacketHeader& header, ref<DataReader> packet) {
						if(header._action==ActionAnnounceReply) {
							Log::Write(L"TJShow/Network", L"PromotionTransaction received announce reply from "+StringifyHex(instance));
							unsigned int pos = 0;
							Role r = packet->Get<Role>(pos);
							Features f = packet->Get<Features>(pos);
							if(r==RoleMaster && (f & FeaturePrimaryMaster)!=0) {
								_primaryFound = true;
							}

							ref<Network> net = _network;
							if(net) {
								net->OnReceive(instance, from, header, packet);
							}	
						}
						else {
							Log::Write(L"TJShow/Network", L"Invalid packet received in promotion transaction; ignoring");
						}
					}

					virtual void OnExpire() {
						// If no primary master was found after waiting a little, we can safely become primary
						ref<Network> net = _network;
						if(net) {
							if(!_primaryFound) {
								net->BecomePrimaryMaster();
							}
							else {
								Log::Write(L"TJShow/Network", L"Another primary master found; cannot promote");
								net->Demote(true);
							}
						}
					}

				protected:
					bool _primaryFound;
					weak<Network> _network;
			};

			class ListDevicesTransaction: public Transaction {
				public:
					ListDevicesTransaction(ref<Client> cl): _client(cl), _received(0), _done(false) {
						assert(cl);
					}

					virtual ~ListDevicesTransaction() {
					}

					virtual void OnReceive(int instance, in_addr from, const PacketHeader& header, ref<DataReader> packet) {
						if(header._action==ActionListDevicesReply) {
							InstanceID client = _client->GetInstanceID();
							if(client!=instance) {
								Log::Write(L"TJShow/Networking", L"Wrong client reporting to this transaction!");
								return;
							}

							unsigned int pos = 0;
							++_received;
							DeviceIdentifier di = packet->Get<DeviceIdentifier>(pos);
							std::wstring friendly = packet->Get<std::wstring>(pos);
							unsigned int total = packet->Get<unsigned int>(pos);
							_devices[di] = GC::Hold(new RemoteDevice(instance, di, friendly));

							if(_received>=total) {
								_done = true;
								_client->_devices = _devices;
								_devices.clear();
							}
						}
					}

					virtual bool IsExpired() const {
						return _done || Transaction::IsExpired();
					}

				protected:
					unsigned int _received;
					bool _done;
					ref<Client> _client;
					std::map<DeviceIdentifier, ref<Device> > _devices;
			};

			class ListPatchesTransaction: public Transaction {
				public:
					ListPatchesTransaction(ref<Client> cl): _client(cl), _received(0), _done(false) {
						assert(cl);
					}

					virtual ~ListPatchesTransaction() {
					}

					virtual void OnReceive(int instance, in_addr from, const PacketHeader& header, ref<DataReader> packet) {
						if(header._action==ActionListPatchesReply) {
							InstanceID client = _client->GetInstanceID();
							if(client!=instance) {
								Log::Write(L"TJShow/Networking", L"Wrong client reporting to this transaction!");
								return;
							}

							++_received;
							unsigned int pos = 0;
							PatchIdentifier pi = packet->Get<PatchIdentifier>(pos);
							DeviceIdentifier di = packet->Get<DeviceIdentifier>(pos);
							unsigned int total = packet->Get<unsigned int>(pos);
							_patches[pi] = di;

							if(_received >= total) {
								_client->_patches = _patches;
								_patches.clear();
								_done = true;
							}
						}
					}

					virtual bool IsExpired() const {
						return _done || Transaction::IsExpired();
					}

				protected:
					bool _done;
					unsigned int _received;
					ref<Client> _client;
					std::map<PatchIdentifier, DeviceIdentifier> _patches;
			};

			FindResourceTransaction::FindResourceTransaction(const std::wstring& rid): Transaction(KTimeOut), _rid(rid) {
			}

			FindResourceTransaction::~FindResourceTransaction() {
			}

			Time FindResourceTransaction::GetTimeOut() const {
				return KTimeOut;
			}

			void FindResourceTransaction::OnReceive(int instance, in_addr from, const PacketHeader& header, ref<DataReader> code) {
				if(header._action==ActionAdvertiseResource) {
					unsigned int pos = 0;
					unsigned short port = code->Get<unsigned short>(pos);
					std::wstring rid = code->Get<std::wstring>(pos);
					std::wstring url = code->Get<std::wstring>(pos);

					if(rid==_rid) {
						_foundOn.push_back(instance);
						_url[instance] = url;

						ResourceFoundNotification rfn;
						rfn._rid = rid;
						rfn._instanceID = instance;
						EventResourceFound.Fire(this, rfn);
					}		
				}
			}

		}
	}
}

void Network::OnReceiveResetAll() {
	if(_role==RoleClient) {
		ref<Patches> p = Application::Instance()->GetModel()->GetPatches();
		if(p) {
			p->Clear();
		}
		_filter->Clear();
	}
}

void Network::ForcePromotion() {
	ThreadLock lock(&_lock);
	_tryBecomePrimary = true;
	BecomePrimaryMaster();
}

// Called from PromotionTransaction
void Network::BecomePrimaryMaster() {
	ThreadLock lock(&_lock);

	if(!_tryBecomePrimary) {
		return; // promotion was probably cancelled
	}

	if(!_isPrimaryMaster) {
		_isPrimaryMaster = true;
		ref<EventLogger> el = Application::Instance()->GetEventLogger();
		if(el) {
			el->AddEvent(TL(node_promoted), ExceptionTypeMessage, false);
		}

		if(_socket) {
			_socket->SendPromoted();
		}

		EventPromoted.Fire(this, Notification(true));
	}
	else {
		Throw(L"This node already is the primary master!", ExceptionTypeWarning);
	}
}

void Network::OnReceiveSetPatch(const PatchIdentifier& pi, const DeviceIdentifier& di) {
	if(_role==RoleClient) {
		ref<Patches> p = Application::Instance()->GetModel()->GetPatches();
		if(p) {
			p->SetPatch(pi,di);
		}
	}
}

void Network::CancelPromotion() {
	ThreadLock lock(&_lock);
	_tryBecomePrimary = false;
}

bool Network::IsPromotionInProgress() const {
	return _tryBecomePrimary;
}

void Network::Demote(bool silent) {
	ThreadLock lock(&_lock);
	_tryBecomePrimary = false;

	if(_isPrimaryMaster) {
		ref<EventLogger> el = Application::Instance()->GetEventLogger();
		if(el) {
			if(silent) {
				el->AddEvent(TL(node_demoted_passive), ExceptionTypeMessage, false);
			}
			else {
				el->AddEvent(TL(node_demoted_active), ExceptionTypeMessage, false);
			}
	}

		_isPrimaryMaster = false;

		if(!silent) {
			_socket->SendDemoted();
		}

		EventDemoted.Fire(this, Notification(false));
	}
}

Network::Notification::Notification(bool promoted): _promoted(promoted) {
}

void Network::Promote() {
	ThreadLock lock(&_lock);
	if(!_isPrimaryMaster) {
		_tryBecomePrimary = true;
		Announce();
	}
	else {
		Log::Write(L"TJShow/Network", L"Cannot promote: this node already is the primary master!");
	}
}

/* TODO: this doesn't completely work with dynamically allocated channels and instances. Problem is that it is not easy to find
the track that is playing this channel. Even if we know that, we need to do special magic to find the right variables, etc..
For now, outlet changes over the network only work for 'main'/'first' instances */
void Network::OnReceiveOutletChange(InstanceID instance, Channel ch, GroupID gid, const OutletHash& outletID, const tj::shared::Any& value) {
	if(_role == RoleMaster) {
		ref<Application> app = Application::InstanceReference();
		ref<Group> group = app->GetModel()->GetGroups()->GetGroupById(gid);
		if(group) {
			ref<Instance> instance = group->GetInstanceByChannel(ch);

			if(instance) {
				ref<TrackWrapper> tw = instance->GetTrackByChannel(ch);
				if(tw) {
					ref<Outlet> outlet = tw->GetOutletByHash(outletID);
					if(outlet) {
						instance->GetPlayback()->SetOutletValue(outlet, value);
					}
				}
			}
		}
	}
}

void Network::OnReceiveAnnounce(InstanceID instance, Role role, const std::wstring& address, in_addr from, Features feats, TransactionIdentifier ti) {
	OnReceiveAnnounceReply(instance, role, address, from, feats);
	
	// If somebody else sent this, send our own reply
	if(_socket && instance!=GetInstanceID()) {
		_socket->SendAnnounceReply(GetRole(), _filter->Dump(), GetFeatures(), ti);
	}
}

void Network::OnReceiveAnnounceReply(int instance, Role role, const std::wstring& address, in_addr from, Features feats) {
	if(_role==RoleMaster && instance != GetInstanceID()) {
		// Check if somebody else is thinking that he is the primary master; if so, silently demote ourselves
		if((feats & FeaturePrimaryMaster) != 0) {
			Demote(true);
		}

		// Clients database is only maintained at master; update it here
		ref<Client> cl;
		bool firstDiscovery = false;
		std::map< InstanceID, ref<Client> >::iterator it = _clients.find(instance);
		if(it==_clients.end()) {
			std::wstring hostname = Networking::GetHostName(&from);
			
			// The client is not 'active' yet. Try to find the client from cache and if it is there,
			// load its settings
			std::set< ref<Client> >::iterator it = _cachedClients.begin();
			while(it!=_cachedClients.end()) {
				ref<Client> client = *it;
				if(client && client->GetHostName() == hostname) {
					cl = client;
					cl->SetInstanceID(instance);
					cl->SetIP(Util::IPToString(from));

					_cachedClients.erase(client);
					firstDiscovery = true;
					break;
				}
				++it;
			}

			// Client not found in cache, create a new one
			if(!firstDiscovery) { 
				cl = GC::Hold(new Client(role, Timestamp(true), Util::IPToString(from), address, instance));
				firstDiscovery = true;
			}

			_clients[instance] = cl;
		}
		else {
			cl = it->second;	
		}

		// Load attributes that are 'decided' on the client, such as role and features
		cl->SetFeatures(feats);
		cl->SetRole(role);
		cl->SetLastSeen(Timestamp(true));

		if(firstDiscovery) {
			// Push our cached attributes to the client
			_socket->SendSetClientAddress(cl, cl->GetAddressing());
		}
		else {
			cl->SetAddressing(address);
			// Ask for devices and patches
			_socket->SendListDevices(instance, GC::Hold(new tj::show::network::ListDevicesTransaction(cl)));
			_socket->SendListPatches(instance, GC::Hold(new tj::show::network::ListPatchesTransaction(cl)));
		}
	}
}

ref<FindResourceTransaction> Network::SendFindResource(const std::wstring& rid) {
	ref<FindResourceTransaction> frt = GC::Hold(new tj::show::network::FindResourceTransaction(rid));
	if(_socket) {
		_socket->SendResourceFind(rid, frt);
		return frt;
	}
	return 0;
}

void Network::OnReceiveLeave(int instance, in_addr from) {
	ref<Client> cl = _clients[instance];
	_clients.erase(instance);
	Application::Instance()->OnClientLeft(cl);
}

unsigned int Network::GetActiveTransactionCount() const {
	if(_socket) {
		return _socket->GetActiveTransactionCount();
	}
	return 0;
}

unsigned int Network::GetWaitingPacketsCount() const {
	if(_socket) {
		return _socket->GetWishListSize();
	}
	return 0;
}

void Network::OnReceiveSetAddress(int instance, const std::wstring& address, in_addr from) {
	if(_instance==instance && _role!=RoleMaster) {
		// meant for us
		_filter->Clear();
		_filter->Parse(address.c_str());
			
		Announce();
	}
}

void Network::ReportError(Features fs, ExceptionType type, const std::wstring& message) {
	if(_socket) {
		_socket->SendError( fs, type, message);
	}
}

strong<ResourceProvider> Network::GetResourceProvider() {
	return _ccm;
}

void Network::SendInput(const PatchIdentifier& patch, const InputID& path, float value) {
	if(_socket && _role==RoleClient) {
		_socket->SendInput(patch,path, value);
	}
}

void Network::OnReceiveError(int instance, Features fs, ExceptionType type, const std::wstring& message) {
	if(_role==RoleMaster && _instance!=instance) {
		ref<Client> client = GetClientByInstanceID(instance);
		if(client) {
			std::wostringstream wos;
			wos << TL(error_on_client) << client->GetIP() << L"("+ StringifyHex(instance) +L") :";
			wos << message;

			ref<EventLogger> el = Application::Instance()->GetEventLogger();
			if(el) {
				el->AddEvent(wos.str(), type, false);
			}
		}
	}
}

void Network::OnReceiveInput(const PatchIdentifier& pi, const InputID& path, float value) {
	if(_role==RoleMaster) {
		ref<Model> model = Application::Instance()->GetModel();
		if(model) {
			ref<input::Rules> rules = model->GetInputRules();
			if(rules) {
				rules->Dispatch(pi, path, value);
			}
		}
	}
}

void Network::OnReceiveResourceAdvertise(const std::wstring& rid, const std::wstring& url, in_addr from, unsigned short port) {
	Log::Write(L"TJShow/Network" , L"Receive resource advertise for rid="+rid+L" url="+url);
	// Tell the CCM that the specified resource (identified by the rid) can be found at the specified url
	_ccm->StartDownload(rid, url, from, port);
}

void Network::Save(TiXmlElement* me) {
	ThreadLock lock(&_lock);

	std::map<InstanceID, ref<Client> >::iterator it = _clients.begin();
	while(it!=_clients.end()) {
		ref<Client> c = it->second;
		if(c && c->GetInstanceID()!=_instance) {
			TiXmlElement client("client");
			c->Save(&client);
			me->InsertEndChild(client);
		}
		++it;
	}

	std::set< ref<Client> >::iterator cit = _cachedClients.begin();
	while(cit!=_cachedClients.end()) {
		ref<Client> client = *cit;
		if(client) {
			TiXmlElement ce("client");
			client->Save(&ce);
			me->InsertEndChild(ce);
		}
		++cit;
	}
}

void Network::Load(TiXmlElement* you) {
	ThreadLock lock(&_lock);

	TiXmlElement* client = you->FirstChildElement("client");
	while(client!=0) {
		ref<Client> cl = GC::Hold(new Client(RoleNone, 0, L"", L"", 0));
		cl->Load(client);
		Log::Write(L"TJShow/Networking", L"Loaded client-cache with hostname "+cl->GetHostName());
		_cachedClients.insert(cl);
		client = client->NextSiblingElement("client");
	}
}

void Network::PushResource(const GroupID& gid, const ResourceIdentifier& rid) {
	if(_role==RoleMaster && _socket) {
		_socket->SendResourcePush(gid, rid);
	}
}

void Network::NeedResource(const std::wstring& ident) {
	// Enqueue this file on the CCM 'wish list'
	_ccm->NeedFile(ident);

	if(_socket) {
		// Find a file server that is able to provide this resource
		_socket->SendResourceFind(ident);
	}
}

void Network::OnReceiveResourcePush(const std::wstring& rid, in_addr from) {
	// This message is also accepted by masters; the sending master will do nothing, since it already has the file

	ref<ResourceProvider> showResources = Application::Instance()->GetModel()->GetResourceManager();
	std::wstring localPath;
	if(!showResources->GetPathToLocalResource(rid, localPath)) {
		NeedResource(rid);
	}
}

ref<Client> Network::GetClientByInstanceID(InstanceID ident) {
	ThreadLock lock(&_lock);

	if(_clients.find(ident)!=_clients.end()) {
		return _clients[ident];
	}
	else {
		return 0;
	}
}

void Network::OnReceiveResourceFind(int ident, const std::wstring& r, TransactionIdentifier tid) {
	if(_role==RoleMaster) {
		if(ident==GetInstanceID()) {
			// Never advertise resources to ourselves
			return;
		}

		ref<Settings> st = Application::Instance()->GetSettings();

		//  Check if this resource is in our resource list
		ref<Model> md = Application::Instance()->GetModel();

		// If we have a file server running, we assume here that it runs at port net.web.port. If this has changed, resource distribution
		// will not work correctly. TODO: maybe store the current port number in fileserver itself (and move FileServer to the Network class
		// anyway?)
		if(md && (md->GetResources()->ContainsResource(r)||md->GetTimeline()->IsResourceRequired(r)) && Application::Instance()->GetFileServer()) {
			if(_socket && st->GetFlag(L"net.web.advertise-resources")) {
				_socket->SendResourceAdvertise(r, CreateResourceURL(r), StringTo<unsigned short>(st->GetValue(L"net.web.port"),0), tid);
			}
		}
	}
}

ref<Authorizer> Network::GetAuthorizer() {
	return _auth;
}

std::wstring Network::CreateResourceURL(const std::wstring& rid) {
	// Create URL for retrieving this resource
	SecurityToken st = _auth->CreateToken(rid);
	strong<Settings> settings = Application::Instance()->GetSettings();
	std::wostringstream url;
	url << settings->GetValue(L"net.web.resources.path") << L"?key=" << Stringify(st) << L"&rid=" << rid;
	return url.str();
}

void Network::OnReceiveResetChannel(GroupID group, Channel ch) {
	ThreadLock lock(&_lock);

	unsigned int cig = group | (ch << 16);
	std::map<unsigned int, ref<StreamPlayer> >::iterator it = _streamPlayers.find(cig);
	if(it!=_streamPlayers.end()) {
		_streamPlayers.erase(it);
	}
}

void Network::OnReceive(ref<DataReader> code, const PacketHeader& ph, bool isPlugin) {
	ThreadLock lock(&_lock);

	if(_role!=RoleClient || (!_filter->IsMemberOf(ph._group) && ph._group!=0)) { 
		return;
	}

	ref<Application> app = Application::InstanceReference();
	if(!app) {
		return; // probably won't happen... if it does, there is trouble anyway
	}

	ref<PluginWrapper> plugin = PluginManager::Instance()->GetPluginByHash(ph._plugin);
	if(!plugin) {
		// notify master that we don't have the required plug-in
		ref<ShowSocket> socket = this->GetSocket();
		if(socket) {
			socket->SendError(FeaturePlugin, ExceptionTypeError, L"Plug-in is not present on client (plug-in ID is "+StringifyHex(ph._plugin)+L")");
		}
		return;
	}

	unsigned int cig = ph._group | (ph._channel << 16);

	if(!isPlugin) {
		std::map<unsigned int, ref<StreamPlayer> >::iterator it = _streamPlayers.find(cig);
		if(it==_streamPlayers.end()) {
			// create streamplayer
			ref<Talkback> talk = GC::Hold(new ChannelTalkback(ref<Network>(this), ph._channel, ph._group));
			ref<StreamPlayer> spl = plugin->CreateStreamPlayer(app->GetOutputManager(), talk);
			if(!spl) {
				Log::Write(L"TJShow/Networking", L"StreamPlayer creation failed!");
				
				// Notify the other nodes 
				std::wostringstream error;
				error << TL(error_in_streamplayer_creation) << ph._channel;
				ReportError(FeaturePlugin, ExceptionTypeError, error.str());
				return;
			}

			spl->Message(code, app->GetOutputManager());
			_streamPlayers[cig] = spl;
		}
		else {
			ref<PluginWrapper> expectedPlugin = PluginManager::Instance()->GetPluginByHash(ph._plugin);

			ref<StreamPlayer> sp = (*it).second;
			if(sp->GetPlugin()!=expectedPlugin->GetPlugin()) {
				// create streamplayer
				ref<Talkback> talk = GC::Hold(new ChannelTalkback(ref<Network>(this), ph._channel, ph._group));
				sp = plugin->CreateStreamPlayer(app->GetOutputManager(), talk);
				if(!sp) {
					Log::Write(L"TJShow/Networking", L"StreamPlayer creation failed!");

					// Notify the other nodes 
					std::wostringstream error;
					error << TL(error_in_streamplayer_creation) << ph._channel;
					ReportError(FeaturePlugin, ExceptionTypeError, error.str());
					return;
				}
				_streamPlayers[cig] = sp;
			}
			sp->Message(code, app->GetOutputManager());
		}
	}
	else {
		// Message meant for plug-in
		plugin->Message(code);
	}
}

Network::~Network() {
	if(_socket) {
		_socket->SendLeave();
	}
	_stopAnnounceEvent.Signal();
	if(_announceThread) {
		_announceThread->WaitForCompletion();
	}
}

ref<ShowSocket> Network::GetSocket() {
	return _socket;
}

void Network::SendUpdate(ref<Message> msg, bool reliable) {
	ref<ShowSocket> socket = _socket;
	if(socket) {
		if(!IsPrimaryMaster()) {
			return; // will not send updates when not a primary master
		}

		socket->Send(msg, reliable);
	}
}

int Network::GetBytesSent() const {
	if(_socket) {
		return _socket->GetBytesSent();
	}
	return 0;
}

int Network::GetBytesReceived() const {
	if(_socket) {
		return _socket->GetBytesReceived();
	}
	return 0;
}

void Network::SetClientAddress(ref<Client> client, std::wstring na) {
	if(_socket) {
		_socket->SendSetClientAddress(client,na);
	}
	client->SetAddressing(na);
}

void Network::SetClientPatch(ref<Client> client, const PatchIdentifier& pi, const DeviceIdentifier& di) {
	if(_socket) {
		_socket->SendSetPatch(client,pi,di);
	}
	client->SetPatch(pi,di); // Update our local cache
}

void Network::Clear() {
	ThreadLock lock(&_lock);

	if(_socket && _isPrimaryMaster) {
		_socket->SendResetAll();
	}

	_clients.clear();
	_cachedClients.clear();
}

/// TODO FIXME: this logic is duplicated in Filter::Parse...
std::set<Channel> Network::GetPresentGroups() const {
	std::set<GroupID> groups;
	ThreadLock lock(&_lock);

	std::map< InstanceID, ref<Client> >::const_iterator it = _clients.begin();
	while(it!=_clients.end()) {
		ref<Client> client = it->second;
		if(client) { 
			std::wstring addressing = client->GetAddressing();
			std::wistringstream is(addressing);
			while(!is.eof()) {
				GroupID gid = -1;
				is >> gid;
				if(gid>=0) {
					groups.insert(gid);
				}
			}
		}
		++it;
	}

	return groups;
}

// This gets called either directly from some timer on clients or from AnnounceThread on masters.
void Network::DoAnnounce() {
	ThreadLock lock(&_lock);

	if(_socket) {
		// If we are trying to become primary master, do the announce in a special transaction
		ref<Transaction> trans = this;

		// If we want to promote and a promotion transaction is not yet running, create one
		if(_role==RoleMaster && _tryBecomePrimary && !_isPrimaryMaster) {
			ref<Transaction> prt = _promotionTransaction;
			if(!prt) {
				prt = GC::Hold(new network::PromotionTransaction(this));
				trans = prt;
			}
			_promotionTransaction = prt;
		}
		_socket->SendAnnounce(_role, _filter->Dump(), GetFeatures(), trans);

		if(_role==RoleMaster) {
			Timestamp t(true);
			// Set last announce time on all clients
			std::map< InstanceID, ref<Client> >::iterator it = _clients.begin();
			while(it!=_clients.end()) {
				ref<Client> client = it->second;
				client->SetLastAnnounce(t);
				it++;
			}
		}
	}
}

/* RemoteDevice */
RemoteDevice::RemoteDevice(int instance, const DeviceIdentifier& di, const std::wstring& friendly): _instance(instance), _di(di), _friendly(friendly) {
}

RemoteDevice::~RemoteDevice() {
}

std::wstring RemoteDevice::GetFriendlyName() const {
	return _friendly;
}

DeviceIdentifier RemoteDevice::GetIdentifier() const {
	return _di;
}

ref<tj::shared::Icon> RemoteDevice::GetIcon() {
	return 0;
}

bool RemoteDevice::IsMuted() const {
	return false;
}

void RemoteDevice::SetMuted(bool t) const {
}

/** Client **/
Client::Client(Role r, const Timestamp& last, const std::wstring& ip, const std::wstring& addressing, const InstanceID& instance): BasicClient(r,last,ip,addressing,instance) {
}

Client::~Client() {
}

void Client::Save(TiXmlElement* me) {
	BasicClient::Save(me);

	std::map<PatchIdentifier, DeviceIdentifier>::const_iterator it = _patches.begin();
	while(it!=_patches.end()) {
		TiXmlElement p("patch");
		SaveAttributeSmall(&p, "id", it->first);
		SaveAttributeSmall(&p, "device", it->second);
		me->InsertEndChild(p);
		++it;
	}
}

void Client::Load(TiXmlElement* you) {
	BasicClient::Load(you);

	TiXmlElement* patch = you->FirstChildElement("patch");
	while(patch!=0) {
		PatchIdentifier pi = LoadAttributeSmall<PatchIdentifier>(patch, "id", L"");
		DeviceIdentifier di = LoadAttributeSmall<DeviceIdentifier>(patch, "device", L"");
		_patches[pi] = di;
		patch = patch->NextSiblingElement("patch");
	}
}

void Client::AddDevice(const DeviceIdentifier& di, ref<Device> dev) {
	_devices[di] = dev;
}

void Client::SetPatch(const PatchIdentifier& pi, const DeviceIdentifier& di) {
	_patches[pi] = di;
}

unsigned int Client::GetDeviceCount() const {
	return (unsigned int)_devices.size();
}

ref<Device> Client::GetDeviceByIdentifier(const DeviceIdentifier& di) {
	std::map<DeviceIdentifier, ref<Device> >::iterator it = _devices.find(di);
	if(it!=_devices.end()) {
		return it->second;
	}
	return 0;
}

DeviceIdentifier Client::GetDeviceByPatch(const PatchIdentifier& p) {
	std::map< PatchIdentifier, DeviceIdentifier >::iterator it = _patches.find(p);
	if(it!=_patches.end()) {
		return it->second;
	}
	return L"";
}

std::map<DeviceIdentifier, ref<Device> >* Client::GetDevices() {
	return &_devices;
}

/** AnnounceThread **/

AnnounceThread::AnnounceThread(ref<Network> nw): _network(nw) {
}

AnnounceThread::~AnnounceThread() {
}

void AnnounceThread::Run()  {
	SetName(L"AnnounceThread");
	Log::Write(L"TJShow/Networking", L"Announce thread started");

	Wait wait; wait[_network->_announceEvent][_network->_stopAnnounceEvent];

	while(true) {
		Time announceTimer = -1;

		if(_network->GetRole()!=RoleClient) {
			ref<Settings> st = _network->_settings;
			if(st) {
				announceTimer = Time(StringTo<int>(st->GetValue(L"net.announce-period"), 1000));
			}
		}

		// When announcetimer == -1, Wait::ForAny will wait an infinite amount of time, which is good
		int result = wait.ForAny(announceTimer);
		if(result<0 || result==0) {
			try {
				// Before announcing, time to clean up our transactions
				ref<ShowSocket> s = _network->GetSocket();
				if(s) {
					s->SendRedeliveryRequests();
					s->CleanTransactions();
				}

				// just announce
				_network->_announceEvent.Reset();
				_network->DoAnnounce();	
			}
			catch(Exception& e) {
				Log::Write(L"TJShow/Network/AnnounceThread", L"Caught exception: "+e.GetMsg());
			}
			catch(...) {
				Log::Write(L"TJShow/Network/AnnounceThread", L"Caught some unknown exception!");
			}
		}
		else if(result==1) {
			// Stop announce thread
			Log::Write(L"TJShow/Networking", L"Announce thread ended");
			return;
		}
	}
}

/** ChannelTalkback **/
ChannelTalkback::ChannelTalkback(strong<Network> net, Channel ch, GroupID gid): _network(ref<Network>(net)), _channel(ch), _group(gid) {
}

ChannelTalkback::~ChannelTalkback() {
}

void ChannelTalkback::SendOutletChange(const std::wstring &outletID, const Any &value) {
	ref<Network> net = _network;
	if(net) {
		ref<ShowSocket> socket = net->GetSocket();
		if(socket) {
			socket->SendOutletChange(_channel, _group, outletID, value);
		}
	}
}

/** Talkback **/
Talkback::~Talkback() {
}

/** Roles **/
std::wstring Roles::GetRoleName(Role r) {
	switch(r) {
		case RoleMaster:
			return TL(role_master);

		case RoleClient:
			return TL(role_client);

		default:
		case RoleNone:
			return TL(role_none);
	}
}