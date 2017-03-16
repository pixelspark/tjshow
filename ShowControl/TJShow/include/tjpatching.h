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
#ifndef _TJPATCHING_H
#define _TJPATCHING_H

/** This header defines the hardware patching mechanism classes. Hardware patching is the way
hardware devices are connected to plug-in output. Plug-ins implement their own hardware, and TJShow
only acts as a 'broker'. 

How does this work? Plug-ins expose several Device-instances (each can implement interfaces only the plug-in
knows about). Each device has a unique ID, friendly name, icon and in most cases can be 'muted'. The TJShow
patch-manager maintains 'patches'. Each 'patch' has a name and an associated device (identified by the unique identifier).

When a plug-in needs a certain device (for example, a sound card) it asks the Playback interface to retrieve 
that device. If the device is not patched, this function returns null. Otherwise, it returns a reference to the device. 
Since the plug-in created that device, it can use any interface it knows that device implements. So for example, a sound
card 'Device' could implement an extra interface, which allows creation of DirectShow devices or the like.

For TJMedia, the story will be different: since most playback stuff is done in TJShow (through Deck and Surface), these
objects will need a ref<Device> as parameter (or a patch identifier at least).
**/

namespace tj {
	namespace show {
		class Device {
			public:
				virtual ~Device() {}
				virtual std::wstring GetFriendlyName() const = 0;
				virtual tj::np::DeviceIdentifier GetIdentifier() const = 0;
				virtual ref<tj::shared::Icon> GetIcon() = 0;
				virtual bool IsMuted() const = 0;
				virtual void SetMuted(bool t) const = 0;
		};

		// Input
		namespace input {
			class Dispatcher: public virtual tj::shared::Object {
				public:
					virtual ~Dispatcher() {}
					virtual void Dispatch(ref<Device> device, const tj::np::InputID& path, const tj::shared::Any& value) = 0;
			};
		}
	}	
}

#endif