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
 
 #ifndef _TJLISTENER_H
#define _TJLISTENER_H

#include "tjsharedinternal.h"
#include "tjthread.h"
#include <vector>

namespace tj {
	namespace shared {
		/** Listener **/
		template<typename NotificationType> class EXPORTED Listener {
			public:
				virtual ~Listener() {
				}

				virtual void Notify(ref<Object> source, const NotificationType& data) = 0;
		};

		template<typename NotificationType> class EXPORTED Listenable {
			public:
				inline Listenable() {
				}

				inline ~Listenable() {
				}

				inline void Fire(ref<Object> source, const NotificationType& data) {
					ThreadLock lock(&_lock);

					typename std::vector< weak< Listener< NotificationType > > >::iterator it = _listeners.begin();
					while(it!=_listeners.end()) {
						ref< Listener<NotificationType> > listener = *it;
						if(listener) {
							listener->Notify(source, data);
						}
						++it;
					}
				}

				inline bool HasListener() const {
					return _listeners.size() > 0;
				}

				inline void AddListener(strong< Listener<NotificationType> > listener) {
					ThreadLock lock(&_lock);
					_listeners.push_back(ref <Listener<NotificationType> >(listener));
				}

				inline void AddListener(ref< Listener<NotificationType> > listener) {
					ThreadLock lock(&_lock);
					if(listener) {
						_listeners.push_back(weak <Listener<NotificationType> >(listener));
					}
				}

				inline void RemoveListener(ref< Listener<NotificationType> > listener) {
					ThreadLock lock(&_lock);
					if(listener) {
						typename std::vector< weak<Listener<NotificationType> > >::iterator it = _listeners.begin();
						while(it!=_listeners.end()) {
							if(*it == listener) {
								it = _listeners.erase(it);
							}
							else {
								++it;
							}
						}
					}
				}

			private:
				CriticalSection _lock;
				std::vector< weak< Listener<NotificationType> > > _listeners;
		};
	}
}

#endif