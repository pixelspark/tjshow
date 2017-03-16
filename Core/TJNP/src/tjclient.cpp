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
 
 #include "../include/tjclient.h"
#include <TinyXML/tinyxml.h>
#include <time.h>
#include <sstream>

#ifdef TJ_OS_WIN
	#include <ws2tcpip.h>
#endif

#ifdef TJ_OS_MAC
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <arpa/inet.h>
	#include <netdb.h>
#endif

using namespace tj::np;
using namespace tj::shared;

BasicClient::BasicClient(Role r, const Timestamp& last, const std::wstring& ip, const std::wstring& addressing, const InstanceID& instance):
	_lastSeen(last),
	_role(r),
	_addressing(addressing),
	_instance(instance) {
		SetIP(ip);
}

BasicClient::~BasicClient() {
}

Features BasicClient::GetFeatures() const {
	return _features;
}

void BasicClient::SetFeatures(Features fs) {
	_features = fs;
}

const Networking::MACAddress& BasicClient::GetMACAddress() const {
	return _mac;
}

void BasicClient::SetLastSeen(const Timestamp& ls) {
	_lastSeen = ls;
}

void BasicClient::Load(TiXmlElement* you) {
	_hostName = LoadAttributeSmall<std::wstring>(you, "hostname", _hostName);
	_addressing = LoadAttributeSmall<std::wstring>(you, "address", _addressing);
	_mac = Networking::MACAddress(LoadAttributeSmall<std::wstring>(you, "mac", L""));
}

void BasicClient::Save(TiXmlElement* me) {
	SaveAttributeSmall(me, "hostname", _hostName);
	SaveAttributeSmall(me, "address", _addressing);
	SaveAttributeSmall(me, "mac", _mac.ToString());
}

std::wstring BasicClient::GetHostName() {
	if(_hostName.length()>0) return _hostName;
	_hostName = Networking::GetHostName(_ip);
	return _hostName;
}

void BasicClient::SetRole(Role r) {
	_role = r;
}

void BasicClient::SetAddressing(const std::wstring& a) {
	_addressing = a;
}

long double BasicClient::GetLatency() const {
	/* Masters do not respond to announce messages, but instead send their own announces
	every x seconds. So calculating latency for them is tricky. We might want to do this
	another way if we're implementing failover masters some time. For now, just ignore any
	masters in the network. */
	Timestamp zero;
	if(_role!=RoleMaster && _lastAnnounce>zero && _lastSeen>_lastAnnounce) {
		return _lastSeen.Difference(_lastAnnounce).ToMilliSeconds();
	}
	return -1.0;
}

bool BasicClient::IsOnline(Time limit) const {
	Timestamp now(true);
	return  now.Difference(_lastSeen).ToMicroSeconds() < (long double)limit.ToInt()*1000;
}

Timestamp BasicClient::GetLastAnnounce() const {
	return _lastAnnounce;
}

Timestamp BasicClient::GetLastSeen() const {
	return _lastSeen;
}

void BasicClient::SetLastAnnounce(const Timestamp& la) {
	_lastAnnounce = la;
}

InstanceID BasicClient::GetInstanceID() const {
	return _instance;
}

void BasicClient::SetInstanceID(const InstanceID& id) {
	_instance = id;
}

void BasicClient::SetIP(const std::wstring& ip) {
	_ip = ip;
	Networking::GetMACAddress(ip, _mac);
}

Role BasicClient::GetRole() const {
	return _role;
}

std::wstring BasicClient::GetIP() const {
	return _ip;
}

std::wstring BasicClient::GetAddressing() const {
	return _addressing;
}