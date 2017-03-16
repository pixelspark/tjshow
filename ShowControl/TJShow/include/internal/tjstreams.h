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
#ifndef _TJTRACKSTREAM_H
#define _TJTRACKSTREAM_H

#include "tjnetwork.h"

namespace tj {
	namespace show {
		class Network;

		/* Stream implementation for track players */
		class TrackStream: public Stream {
			public:
				// TrackStream will allocate a channel itself (using a ChannelAllocation)
				TrackStream(strong<TrackWrapper> track, ref<Network> network, ref<Instance> instance, bool isInstance);
				virtual ~TrackStream();
				virtual strong<Message> Create();
				virtual void Send(ref<Message> msg, bool reliable);
				virtual Channel GetChannel();

			protected:
				Channel _channel;
				ChannelAllocation _ca;
				weak<Network> _network;
				weak<TrackWrapper> _track;
		};

		/* Stream implementation for live controls */
		class LiveStream: public Stream {
			public:
				LiveStream(ref<TrackWrapper> track, ref<Network> net);
				virtual ~LiveStream();

				virtual strong<Message> Create();
				virtual void Send(ref<Message> msg, bool reliable);

			protected:
				weak<Network> _network;
				weak<TrackWrapper> _track;
		};
	}
}

#endif