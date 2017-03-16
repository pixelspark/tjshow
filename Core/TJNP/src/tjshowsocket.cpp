/* This file is part of TJShow. TJShow is free software: you 
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
 
 #include "../include/tjshowsocket.h"

#include <time.h>
#include <sstream>
#include <limits>

#ifndef TJ_OS_WIN
	#include <sys/socket.h>
	#include <arpa/inet.h>
	#define INVALID_SOCKET -1
	#define SOCKET_ERROR -1
	#define _strdup strdup
#else
	typedef int socklen_t;
#endif

using namespace tj::shared;
using namespace tj::np;

#pragma pack(push,1)

NetworkInitializer ShowSocket::_initializer;

ShowSocket::ShowSocket(int port, const char* address, ref<Node> nw): _lastPacketID(0), _bytesSent(0), _bytesReceived(0), _network(nw), _maxReliablePacketCount(KDefaultMaxReliablePacketCount) {
	// Create a random transaction counter id
	_transactionCounter = rand();
	assert(address!=0 && port > 0 && port < 65536);
	_recieveBuffer = new char[Packet::maximumSize];

	_port = port;
	_bcastAddress = _strdup(address);

	sockaddr_in addr;
	_client = 0;
	_server = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if(_server == INVALID_SOCKET) {
		Throw(L"Couldn't create a server socket", ExceptionTypeError);
	}

	// Fill in the interface information
	addr.sin_family = AF_INET;
	addr.sin_port = htons((u_short)port);
	addr.sin_addr.s_addr = INADDR_ANY;

	int on = 1;
	setsockopt(_server,SOL_SOCKET,SO_REUSEADDR,(const char*)&on, sizeof(int));

	if(bind(_server,(sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
		Log::Write(L"TJShow/Network", Stringify(_bcastAddress) + L" could not connect");
		Throw(L"Couldn't create socket for listening. ", ExceptionTypeError);
		return;
	}

	// make us member of the multicast group for TJShow
	struct ip_mreq mreq;
	mreq.imr_multiaddr.s_addr = inet_addr((const char*)_bcastAddress);
	mreq.imr_interface.s_addr = INADDR_ANY;
	setsockopt(_server, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq));

	_client = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(_client == INVALID_SOCKET) {
		Throw(L"Couldn't open socket for broadcasting",ExceptionTypeError);
		return;
	}

	setsockopt(_client,SOL_SOCKET,SO_BROADCAST,(const char*)&on, sizeof(int));
	setsockopt(_client,SOL_SOCKET,SO_REUSEADDR,(const char*)&on, sizeof(int));
}

void ShowSocket::OnCreated() {
	_listenerThread = GC::Hold(new SocketListenerThread());
	_listenerThread->AddListener(_server, this);
	_listenerThread->Start();
}

ShowSocket::~ShowSocket() {
	_listenerThread->Stop();
	
	#ifdef TJ_OS_WIN
		closesocket(_client);
		closesocket(_server);
	#else
		close(_client);
		close(_server);
	#endif
	
	delete[] _recieveBuffer;
	delete _bcastAddress;
}

int ShowSocket::GetPort() const {
	return _port; 
}

std::wstring ShowSocket::GetAddress() const {
	return Wcs(std::string(_bcastAddress));
}

void ShowSocket::CleanTransactions() {
	ThreadLock lock(&_lock);

	std::map<TransactionIdentifier, ref<Transaction> >::iterator it = _transactions.begin();
	while(it!=_transactions.end()) {
		ref<Transaction> tx = it->second;
		if(!tx) {
			_transactions.erase(it++);
		}
		else if(tx->IsExpired()) {
			tx->OnExpire();
			_transactions.erase(it++);
		}
		else {
			++it;
		}
	}
}

unsigned int ShowSocket::GetActiveTransactionCount() const {
	return (unsigned int)_transactions.size();
}

void ShowSocket::OnReceive(NativeSocket ns) {
	ref<Node> nw = _network;
	if(!nw) {
		Log::Write(L"TJNP/Socket", L"Internal error: no network instance set");
		return;
	}

	ref<DataReader> code = 0;
	ref<Transaction> tx = 0;
	sockaddr_in from;
	PacketHeader ph;

	// Receive stuff
	{
		ThreadLock lock(&_lock);
		memset(_recieveBuffer,0,sizeof(char)*Packet::maximumSize);
		socklen_t size = (int)sizeof(from);
		int ret = recvfrom(_server, _recieveBuffer, Packet::maximumSize-1, 0, (sockaddr*)&from, &size);
		
		if(ret == SOCKET_ERROR) {
			// This seems to happen on packets that come from us
			return;
		}
		_bytesReceived += ret;

		// Extract packet header
		ph = *((PacketHeader*)_recieveBuffer);
		
		// Check if this actually is a T4 packet
		if(ph._version[0]!='T' || ph._version[1] != '4') {
			if(ph._version[0]=='T') {
				Log::Write(L"TJNP/Socket", L"Received a packet which has a different protocol version; cannot process this packet");
			}
			else {
				Log::Write(L"TJNP/Socket", L"Received invalid packets from the network; maybe network link is broken or other applications are running on this port");
			}
			return;
		}

		// Check size
		if(int(ph._size+sizeof(PacketHeader)) > ret) {
			Log::Write(L"TJNP/Socket", L"Packet smaller than it says it is; ignoring it!");
			return;
		}
		
		// Reject loopback messages
		if(ph._from == nw->GetInstanceID()) {
			return;
		}
	
		// If this is a 'special packet', process the flags
		if(ph._flags!=0) {
			// Check if this packet is a re-delivery or a cannot-re-delivery
			if((ph._flags & (PacketFlagCannotRedeliver|PacketFlagRedelivery))!=0) {
				if(ph._flags & PacketFlagCannotRedeliver) {
					///Log::Write(L"TJNP/Socket", L"Packet with rpid="+Stringify(ph._rpid)+L" from "+StringifyHex(ph._from)+L" could not be redelivered; removing from wishlist");
				}
				else {
					///Log::Write(L"TJNP/Socket", L"Packet with rpid="+Stringify(ph._rpid)+L" from "+StringifyHex(ph._from)+L" was redelivered; removing from wishlist");
				}
				std::deque< std::pair<InstanceID, ReliablePacketID> >::iterator it = _reliableWishList.begin();
				bool weRequestedRedelivery = false;
				while(it!=_reliableWishList.end()) {
					if(it->first == ph._from && it->second == ph._rpid) {
						it = _reliableWishList.erase(it);
						weRequestedRedelivery = true;
					}
					else {
						++it;
					}
				}

				if(!weRequestedRedelivery) {
					return; // Packet was redelivered for a different instance on this pc
				}
			}
			else if((ph._flags & PacketFlagRequestRedelivery)!=0) {
				// Redeliver packet with ph._rpid to the sending node and return
				std::map<ReliablePacketID, ref<Packet> >::iterator it = _reliableSentPackets.find(ph._rpid);
				if(it==_reliableSentPackets.end()) {
					///Log::Write(L"TJNP/Socket", L"Sending cannot redeliver (rpid requested="+Stringify(ph._rpid)+L")");
					PacketHeader rph;
					rph._flags = PacketFlagCannotRedeliver;
					rph._from = nw->GetInstanceID();
					rph._transaction = ph._transaction;
					rph._rpid = ph._rpid;
					ref<Packet> response = GC::Hold(new Packet(ph, (const char*)0, 0));
					Send(response, &from, false);
					// Send 'cannot redeliver'
				}
				else {
					if(!(it->second)) {
						///Log::Write(L"TJNP/Socket", L"Cannot redeliver, packet is null!");
					}

					if(ph._plugin==nw->GetInstanceID()) {
						///Log::Write(L"TJNP/Socket", L"Redelivering (rpid requested="+Stringify(ph._rpid)+L")");
						// Send packet with extra 'redelivery' flag, and *only* to the requesting computer
						ref<Packet> packet = it->second;
						packet->_header->_flags = PacketFlagRedelivery;
						packet->_header->_from = nw->GetInstanceID();
						Send(packet, &from, false);
					}
				}
				return;
			}
		}

		// Check reliability properties
		if((ph._flags & PacketFlagReliable) != 0) {
			std::map<InstanceID, ReliablePacketID>::iterator it = _reliableLastReceived.find(ph._from);
			if(it!=_reliableLastReceived.end()) {
				ReliablePacketID predictedNext = ++(it->second);

				if(predictedNext!=ph._rpid) {
					/** TODO: if the packet indicates that it may not be delivered out-of-order, then also put ph._rpid on the wishlist **/
					///Log::Write(L"TJNP/Socket", L"Received packet is out of order (predicted rpid="+Stringify(predictedNext)+L", received rpid="+Stringify(ph._rpid));
					unsigned int n = 0;
					for(ReliablePacketID r = predictedNext; r < ph._rpid; ++r) {
						++n;
						if(_reliableWishList.size() > _maxReliablePacketCount) {
							///Log::Write(L"TJNP/Socket", L"Wish list length exceeded; packets are being dropped!");
							_reliableWishList.pop_front();
						}
						else if(n > _maxReliablePacketCount) {
							///Log::Write(L"TJNP/Socket", L"Wish list length exceeded for this batch; packets are being dropped!");
							break;
						}
						///Log::Write(L"TJNP/Socket", L"Adding reliable packet "+Stringify(r)+L" from "+StringifyHex(ph._from)+L" to receive wish list");
						_reliableWishList.push_back(std::pair<InstanceID, ReliablePacketID>(ph._from, r));
					}
				}
			}
			_reliableLastReceived[ph._from] = ph._rpid;
		}

		// Extract packet contents
		code = GC::Hold(new DataReader(_recieveBuffer+sizeof(PacketHeader), ph._size));

		// Find our transaction, if it exists. Otherwise, use the 'default transaction' (which happens to be _network)
		if(ph._transaction==0) {
			tx = nw;
		}
		else {
			if(_transactions.find(ph._transaction)!=_transactions.end()) {
				tx = _transactions[ph._transaction];
			}
		}
	}

	// Handle the message
	if(tx && !tx->IsExpired()) {
		tx->OnReceive(ph._from, from.sin_addr, ph, code);
	}
}

void ShowSocket::SendRedeliveryRequests() {
	ThreadLock lock(&_lock);

	ref<Node> nw = _network;
	if(!nw) return;
	InstanceID iid = nw->GetInstanceID();

	PacketHeader rph;
	rph._flags = PacketFlagRequestRedelivery;
	ref<Packet> rp = GC::Hold(new Packet(rph, (const char*)0, 0));
	std::deque< std::pair<InstanceID, ReliablePacketID> >::iterator it = _reliableWishList.begin();
	while(it!=_reliableWishList.end()) {
		rp->_header->_rpid = it->second;
		rp->_header->_from = iid;
		rp->_header->_plugin = it->first;
		///Log::Write(L"TJNP/Socket", L"Requesting redelivery (iid="+StringifyHex(it->first)+L" rpid="+Stringify(it->second)+L")");
		Send(rp, false);
		++it;
	}
}

void ShowSocket::SendDemoted() {
	ThreadLock lock(&_lock);

	ref<Message> stream = GC::Hold(new Message(ActionDemoted));
	Send(stream, true);
}

void ShowSocket::SendError(Features fs, ExceptionType type, const std::wstring& msg) {
	ThreadLock lock(&_lock);

	ref<Message> stream = GC::Hold(new Message(ActionReportError));
	stream->Add(fs);
	stream->Add(type);
	stream->Add<std::wstring>(msg);
	Send(stream, true);
}

void ShowSocket::SendAnnounce(Role r, const std::wstring& address, Features feats, strong<Transaction> ti) {
	ThreadLock lock(&_lock);

	ref<Message> stream = GC::Hold(new Message(ActionAnnounce));
	stream->Add(r);
	stream->Add(feats);
	
	// Since announce replies can either be handled by the default transaction (_network) or a separate transaction,
	// choose here.
	if(ref<Transaction>(ti)==ref<Transaction>(ref<Node>(_network))) {
		stream->Add<TransactionIdentifier>(0);
	}
	else {
		++_transactionCounter;
		_transactions[_transactionCounter] = ti;
		stream->Add<TransactionIdentifier>(_transactionCounter);
	}

	stream->Add<std::wstring>(address);
	Send(stream);
}

void ShowSocket::SendAnnounceReply(Role r, const std::wstring& address, Features feats, TransactionIdentifier ti) {
	ThreadLock lock(&_lock);

	ref<Message> stream = GC::Hold(new Message(ActionAnnounceReply,ti));
	stream->Add(r);
	stream->Add(feats);
	stream->Add<std::wstring>(address);
	Send(stream);
}

void ShowSocket::SendPromoted() {
	ThreadLock lock(&_lock);

	ref<Message> stream = GC::Hold(new Message(ActionPromoted));
	Send(stream, true);
}

void ShowSocket::SendResetAll() {
	ThreadLock lock(&_lock);

	ref<Message> stream = GC::Hold(new Message(ActionResetAll));
	Send(stream, true);
}

void ShowSocket::SendResetChannel(GroupID gid, Channel ch) {
	ThreadLock lock(&_lock);

	ref<Message> stream = GC::Hold(new Message(ActionResetChannel));
	stream->GetHeader()->_channel = ch;
	stream->GetHeader()->_group = gid;
	Send(stream, true);
}

void ShowSocket::SendSetPatch(ref<BasicClient> c, const PatchIdentifier& pi, const DeviceIdentifier& di) {
	ThreadLock lock(&_lock);

	ref<Message> stream = GC::Hold(new Message(ActionSetPatch));
	stream->Add<InstanceID>(c->GetInstanceID());
	stream->Add<PatchIdentifier>(pi);
	stream->Add<DeviceIdentifier>(di);
	Send(stream, true);
}

void ShowSocket::SendListPatchesReply(const PatchIdentifier& pi, const DeviceIdentifier& di, TransactionIdentifier ti, in_addr to, unsigned int count) {
	ThreadLock lock(&_lock);

	ref<Message> stream = GC::Hold(new Message(ActionListPatchesReply, ti));
	stream->Add<PatchIdentifier>(pi);
	stream->Add<DeviceIdentifier>(di);
	stream->Add<unsigned int>(count);
	Send(stream, true);
}

void ShowSocket::SendListDevicesReply(const DeviceIdentifier& di, const std::wstring& friendly, TransactionIdentifier ti, in_addr to, unsigned int count) {
	ThreadLock lock(&_lock);

	ref<Message> stream = GC::Hold(new Message(ActionListDevicesReply, ti));
	stream->Add<DeviceIdentifier>(di);
	stream->Add<std::wstring>(friendly);
	stream->Add<unsigned int>(count);
	Send(stream, true);
}

void ShowSocket::SendSetClientAddress(ref<BasicClient> client, std::wstring na) {
	ThreadLock lock(&_lock);
	ref<Message> stream = GC::Hold(new Message(ActionSetAddress));
	stream->Add(client->GetInstanceID());
	stream->Add<std::wstring>(na);
	Send(stream, true);
}

void ShowSocket::SendInput(const PatchIdentifier& patch, const InputID& path, float value) {
	ThreadLock lock(&_lock);

	ref<Message> stream = GC::Hold(new Message(ActionInput));
	stream->Add(patch);
	stream->Add(path);
	stream->Add(value);
	Send(stream, true);
}

void ShowSocket::SendListDevices(InstanceID to, ref<Transaction> ti) {
	ThreadLock lock(&_lock);

	if(ti) {
		++_transactionCounter;
		_transactions[_transactionCounter] = ti;

		ref<Message> msg = GC::Hold(new Message(ActionListDevices));
		msg->Add<InstanceID>(to);
		msg->Add<TransactionIdentifier>(_transactionCounter);
		Send(msg, true);
	}
}

void ShowSocket::SendListPatches(InstanceID to, ref<Transaction> ti) {
	ThreadLock lock(&_lock);

	if(ti) {
		++_transactionCounter;
		_transactions[_transactionCounter] = ti;

		ref<Message> msg = GC::Hold(new Message(ActionListPatches));
		msg->Add<InstanceID>(to);
		msg->Add<TransactionIdentifier>(_transactionCounter);
		Send(msg, true);
	}
}

void ShowSocket::SendLeave() {
	ThreadLock lock(&_lock);
	PacketHeader ph;
	ph._action = ActionLeave;
	ref<Packet> p = GC::Hold(new Packet(ph, (const char*)0, 0));
	Send(p, true);
}

void ShowSocket::SendResourceFind(const std::wstring& ident, ref<Transaction> ti) {
	ThreadLock lock(&_lock);
	ref<Message> stream = GC::Hold(new Message(ActionFindResource));
	stream->Add(ident);

	if(ti) {
		++_transactionCounter;
		_transactions[_transactionCounter] = ti;
		stream->Add<TransactionIdentifier>(_transactionCounter);
	}
	else {
		stream->Add<TransactionIdentifier>(0);
	}
	Send(stream);
}

void ShowSocket::SendResourcePush(const GroupID& gid, const ResourceIdentifier& ident) {
	ThreadLock lock(&_lock);
	ref<Message> stream = GC::Hold(new Message(ActionPushResource));
	stream->GetHeader()->_group = gid;
	stream->Add(ident);
	Send(stream, true);
}

void ShowSocket::SendResourceAdvertise(const ResourceIdentifier& rid, const std::wstring& url, unsigned short port, TransactionIdentifier tid) {
	ThreadLock lock(&_lock);
	ref<Message> stream = GC::Hold(new Message(ActionAdvertiseResource, tid));
	stream->Add(port);
	stream->Add(rid);
	stream->Add(url);
	Send(stream);
}

void ShowSocket::SendOutletChange(Channel ch, GroupID gid, const std::wstring& outletName, const tj::shared::Any& value) {
	ThreadLock lock(&_lock);
	ref<Message> stream = GC::Hold(new Message(ActionOutletChange));
	stream->Add<Channel>(ch);
	stream->Add<GroupID>(gid);
	stream->Add<unsigned int>(value.GetType());
	stream->Add(value.ToString());
	Hash hash;
	stream->Add<OutletHash>(hash.Calculate(outletName));
	Send(stream, true);
}

void ShowSocket::Send(strong<Packet> p, bool reliable) {
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons((u_short)_port);      
	u_long dir_bcast_addr = inet_addr(_bcastAddress);
	addr.sin_addr.s_addr = dir_bcast_addr;
	Send(p, &addr, reliable);
}

ReliablePacketID ShowSocket::RegisterReliablePacket(strong<Packet> p) {
	ThreadLock lock(&_lock);
	ReliablePacketID next = _lastPacketID + 1;

	if(_reliableSentPackets.size() > _maxReliablePacketCount) {
		/* Remove the entry with the id next - _maxReliablePacketCount. When next=101, we remove 101-100=1 */
		std::map<ReliablePacketID, ref<Packet> >::iterator it = _reliableSentPackets.find(next - _maxReliablePacketCount);
		if(it!=_reliableSentPackets.end()) {
			_reliableSentPackets.erase(it);
		}
		else {
			Log::Write(L"TJNP/Socket", L"Could not remove sent reliable packet from list; probably causing memory leaks!");
		}
	}

	if(_reliableSentPackets.size() > 2*_maxReliablePacketCount) {
		_reliableSentPackets.clear();
		Log::Write(L"TJNP/Socket", L"List of sent packets became too large; removed all of them!");
	}

	_reliableSentPackets[next] = p;
	p->_header->_flags |= PacketFlagReliable;
	p->_header->_rpid = next;
	_lastPacketID = next;
	return next;
}

void ShowSocket::Send(strong<Packet> p, const sockaddr_in* address, bool reliable) {
	ThreadLock lock(&_lock);
	ref<Node> nw = _network;
	if(!nw) return;

	/* If packet needs to be sent 'reliably', create a ReliablePacketID */
	if(reliable) {
		RegisterReliablePacket(p);
		p->_header->_flags |= PacketFlagReliable;
	}

	/** For testing, drop a few packets **/
	///if(rand()%4 == 0) {
	///	Log::Write(L"TJNP/Socket", L"Deliberately dropping packet (rpid="+Stringify(p->_header->_rpid)+L") ");
	///	return;
	///}

	unsigned int size = ((unsigned int)(Packet::maximumSize-sizeof(PacketHeader)), (unsigned int)(p->GetSize() + sizeof(PacketHeader)));
	p->_header->_from = nw->GetInstanceID();
	int ret = sendto(_client, reinterpret_cast<char*>(p->_header), size, 0, (const sockaddr*)address, sizeof(sockaddr_in));

	if(ret != SOCKET_ERROR) {
		_bytesSent += (int)size;
	}
}

void ShowSocket::Send(strong<Message> s, bool reliable) {
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons((u_short)_port);      
	u_long dir_bcast_addr = inet_addr(_bcastAddress);
	addr.sin_addr.s_addr = dir_bcast_addr;

	s->SetSent();
	Send(s->ConvertToPacket(), &addr, reliable);
}

unsigned int ShowSocket::GetWishListSize() const {
	return (unsigned int)_reliableWishList.size();
}

int ShowSocket::GetBytesSent() const {
	return _bytesSent;
}

int ShowSocket::GetBytesReceived() const {
	return _bytesReceived;
}

#pragma pack(pop)