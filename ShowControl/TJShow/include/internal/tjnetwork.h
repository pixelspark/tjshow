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
#ifndef _TJNETWORKING_H
#define _TJNETWORKING_H

#include <TJNP/include/tjclient.h>
#include <TJNP/include/tjauthorizer.h>
#include <TJNP/include/tjshowsocket.h>
#include <TJNP/include/tjtransaction.h>

namespace tj {
	namespace show {
		class Roles {
			public:
				static std::wstring GetRoleName(tj::np::Role r);
		};

		namespace network {
			class ListDevicesTransaction;
			class ListPatchesTransaction;
			class PromotionTransaction;
			class AnnounceThread;
			class ChannelTalkback;
			class ClientCacheManager;

			class FindResourceTransaction: public tj::np::Transaction {
				public:
					FindResourceTransaction(const std::wstring& rid);
					virtual ~FindResourceTransaction();
					virtual void OnReceive(int instance, in_addr from, const tj::np::PacketHeader& header, ref<DataReader> code);
					virtual Time GetTimeOut() const;

					struct ResourceFoundNotification {
						std::wstring _rid;
						InstanceID _instanceID;
					};
					Listenable<ResourceFoundNotification> EventResourceFound;
					std::vector<InstanceID> _foundOn;
					std::map<InstanceID, std::wstring> _url;

				protected:
					const static int KTimeOut = 5000; // in ms
					std::wstring _rid;
			};
		}

		/** The Network class is the receiving end of the protocol. Messages are formatted and sent by the ShowSocket class.
		When ShowSocket receives a message, it first checks the packet (size, destination channel, etc). It that's all OK, it will
		look for a transaction object that handles the object. If the transaction id in the received packet equals 0, Network will
		handle the packet. If not 0, the ShowSocket class will find the appropriate Transaction object and call its OnReceive. 
		
		An example of a transaction based packet is ActionListDevices. A server creates a ListDevicesTransaction, generates an ID
		and sends ActionListDevices to a client with that specific transaction ID. The ActionListDevicesReply packets are then handles
		by the appropriate transaction. When all device replies have been received, this transaction will (atomically) update the
		client's cached device list in the Client object.
		**/
		class Network: public virtual Object, public Node, public Serializable, public EventLogger {
			friend class TrackStream;
			friend class LiveStream;
			friend class Client;
			friend class network::AnnounceThread;
			friend class network::ChannelTalkback;

			public:
				Network();
				virtual ~Network();
				ref<Filter> GetFilter();
				virtual InstanceID GetInstanceID() const;
				void Connect(ref<Settings> st);
				void SetRole(Role r);
				Role GetRole() const;
				bool IsPrimaryMaster() const;
				void Demote(bool silent);
				void Promote();
				bool IsPromotionInProgress() const;
				void CancelPromotion();
				bool IsConnected() const;
				void Disconnect();
				ref<network::ClientCacheManager> GetClientCacheManager();
				void BecomePrimaryMaster();
				void ForcePromotion();

				// Network actions
				void Announce();
				std::map<InstanceID, ref<Client> >* GetClients();
				void NeedResource(const std::wstring& r); // starts search for a resource on the network
				void SetClientAddress(ref<Client> client, std::wstring na);
				void SetClientPatch(ref<Client> client, const PatchIdentifier& pi, const DeviceIdentifier& di);
				void Clear();
				void PushResource(const GroupID& gid, const ResourceIdentifier& rid);
				void ReportError(Features involved, ExceptionType type, const std::wstring& message);
				void SendInput(const tj::np::PatchIdentifier& patch, const tj::np::InputID& path, float value);
				void SendUpdate(ref<Message> msg, bool reliable = false);
				ref<network::FindResourceTransaction> SendFindResource(const std::wstring& rid);

				/** Returns a set of all channels that clients in this network accept (multiple clients
				may accept the same channel and play it, but if a channel is in this set, it is guaranteed
				to play at least on one client) **/
				std::set<GroupID> GetPresentGroups() const;
				int GetBytesSent() const;
				int GetBytesReceived() const;
				unsigned int GetActiveTransactionCount() const;
				unsigned int GetWaitingPacketsCount() const;
				ref<Client> GetClientByInstanceID(InstanceID instance);
				void GetCachedClients(std::vector< ref<Client> >& lst);
				ref<tj::np::Authorizer> GetAuthorizer();

				// For resources
				strong<ResourceProvider> GetResourceProvider();

				static std::wstring FormatFeatures(const Features& f);
				virtual void OnReceive(int instance, in_addr from, const PacketHeader& header, ref<DataReader> packet);
				virtual bool IsExpired() const;

				// For saving cached client settings
				virtual void Save(TiXmlElement* me);
				virtual void Load(TiXmlElement* you);

				// EventLogger
				virtual void AddEvent(const std::wstring& message, ExceptionType e, bool read);

				// Events
				struct Notification {
					Notification(bool promoted);
					bool _promoted;
				};

				Listenable<Notification> EventPromoted;
				Listenable<Notification> EventDemoted;

			protected:
				ref<ShowSocket> GetSocket();
				Features GetFeatures() const;
				void DoAnnounce(); // this does the real work
				std::wstring CreateResourceURL(const std::wstring& url);

				// Callbacks from socket through OnReceive (implementation of Transaction::OnReceive)
				virtual void OnReceiveAnnounce(InstanceID instance, Role role, const std::wstring& address, in_addr from, Features feats, TransactionIdentifier ti);
				virtual void OnReceiveAnnounceReply(InstanceID instance, Role role, const std::wstring& address, in_addr from, Features feats);
				virtual void OnReceiveLeave(InstanceID instance, in_addr from);
				virtual void OnReceiveSetAddress(InstanceID instance, const std::wstring& address, in_addr from);
				virtual void OnReceive(ref<DataReader> code, const PacketHeader& ph, bool isPlugin);
				virtual void OnReceiveResourceAdvertise(const std::wstring& rid, const std::wstring& url, in_addr from, unsigned short port);
				virtual void OnReceiveResourcePush(const std::wstring& rid, in_addr from);
				virtual void OnReceiveResourceFind(InstanceID instance, const std::wstring& resource, TransactionIdentifier tid);
				virtual void OnReceiveError(InstanceID instance, Features involved, ExceptionType type, const std::wstring& msg);
				virtual void OnReceiveListDevices(const PacketHeader& ph, InstanceID to, in_addr from, TransactionIdentifier ti);
				virtual void OnReceiveListPatches(const PacketHeader& ph, InstanceID to, in_addr from, TransactionIdentifier ti);
				virtual void OnReceiveResetAll();
				virtual void OnReceiveSetPatch(const PatchIdentifier& pi, const DeviceIdentifier& di);
				virtual void OnReceiveInput(const PatchIdentifier& pi, const InputID& path, float value);
				virtual void OnReceiveOutletChange(InstanceID instance, Channel ch, GroupID gid, const OutletHash& outletID, const tj::shared::Any& value);
				virtual void OnReceiveResetChannel(GroupID gid, Channel ch);

				ref<ShowSocket> _socket;
				ref<tj::np::Authorizer> _auth;
				ref<network::ClientCacheManager> _ccm;
				ref<Filter> _filter;
				ref<Settings> _settings;
				ref<network::AnnounceThread> _announceThread;

				InstanceID _instance;
				Role _role;
				bool _isPrimaryMaster;
				bool _tryBecomePrimary;
				weak<Transaction> _promotionTransaction;
				
				std::map<InstanceID, ref<Client> > _clients;
				std::set< ref<Client> > _cachedClients;
				std::map<unsigned int,  ref<StreamPlayer> > _streamPlayers;
				mutable CriticalSection _lock;				
				Event _announceEvent;
				Event _stopAnnounceEvent;
		};

		/* Device residing on a client */
		class RemoteDevice: public Device {
			public:
				RemoteDevice(int instance, const DeviceIdentifier& di, const std::wstring& friendly);
				virtual ~RemoteDevice();
				virtual std::wstring GetFriendlyName() const;
				virtual DeviceIdentifier GetIdentifier() const;
				virtual ref<tj::shared::Icon> GetIcon();
				virtual bool IsMuted() const;
				virtual void SetMuted(bool t) const;

			protected:
				int _instance;
				DeviceIdentifier _di;
				std::wstring _friendly;
		};

		class Client: public tj::np::BasicClient {
			friend class Network;
			friend class tj::show::network::ListDevicesTransaction;
			friend class tj::show::network::ListPatchesTransaction;

			public:
				Client(Role r, const Timestamp& lastSeen, const std::wstring& ip, const std::wstring& addressing, const InstanceID& instance);
				virtual ~Client();

				/* Remove device & patch management */
				void AddDevice(const DeviceIdentifier& di, ref<Device> dev);
				void SetPatch(const PatchIdentifier& pi, const DeviceIdentifier& di);
				unsigned int GetDeviceCount() const;
				ref<Device> GetDeviceByIdentifier(const DeviceIdentifier& di);
				DeviceIdentifier GetDeviceByPatch(const PatchIdentifier& p);
				std::map<DeviceIdentifier, ref<Device> >* GetDevices();

				virtual void Save(TiXmlElement* me);
				virtual void Load(TiXmlElement* you);

			protected:
				std::map<DeviceIdentifier, ref<Device> > _devices;
				std::map<PatchIdentifier, DeviceIdentifier> _patches;
		};
	}
}

#endif