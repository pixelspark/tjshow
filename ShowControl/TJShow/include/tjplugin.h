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
#ifndef _TJPLUGIN_H
#define _TJPLUGIN_H

#define SCRIPT_EXPORTED __declspec(dllimport)
#include <TJScript/include/tjscriptable.h>

/** This is probably the most important header for plugin developers. It describes most of the classes
that have to be implemented.

Every plugin has its own 'plugin ID' which is used to distinguish between different plug-ins. The plug-in ID
currently is a 32-bit hash of information like the plugin-name, module name and version. The plug-in ID of the 
plug-in at the master and the plug-in at a client must be the same in order for the two to communicate properly.

Plugin DLL's should be in the directory where PluginManager is asked to look (most likely just 'plugins' or
something like that). They should have one method:

std::vector<Plugin*>* GetPlugin();

In Visual Studio, this should be something like this:

extern "C" { 
	__declspec(dllexport) std::vector<Plugin*>* GetPlugin() {
		std::vector<Plugin*>* plugins = new std::vector<Plugin*>();
		plugins->push_back(new DummyPlugin());
		return plugins;
	}
}

If it returns null, TJShow will inform the user that the plug-in could not be loaded. If you somehow
encounter an error (for example if your plugin needs to initialize some resources), throw a 
TJShow-compatible (i.e. class Exception-derived one) from any of your methods, it will be caught by the
program. You can use TJShared's Throw-macro to do that easily.
**/
#define PluginEntryName ("GetPlugins")

namespace tj {
	namespace show {
		class Track;
		class StreamPlayer;
		class LiveControl;
		class Player;
		class Plugin;
		typedef std::vector< ref<Plugin> >* (*PluginEntry)();
		typedef std::wstring TrackID;
		
		/** Proxy instance that is given to live controls so they can change the contents of the 
		propertygrid of the application **/
		class PropertyGridProxy: public virtual tj::shared::Object {
			public:
				virtual ~PropertyGridProxy() {
				}

				virtual void SetProperties(ref<tj::shared::Inspectable> properties) = 0;
		};

		/** An outlet is a connection to a variable on a timeline. Tracks that need to return data to the timeline (such as
		a database track, which returns database results) do this through an Outlet. The Track defines which outlets it provides,
		and needs to connect them in Track::CreateOutlets. When it has data to send to the outlet, a Player should call 
		Playback::SetOutletValue with the outlet. The host application takes care of storing the association between variables
		and outlets. **/
		class Outlet: public virtual tj::shared::Object, public tj::shared::Serializable {
			public:
				virtual ~Outlet();
				virtual bool IsConnected() = 0;
				virtual void Load(TiXmlElement* e) = 0;
				virtual void Save(TiXmlElement* s) = 0;
				virtual std::wstring GetID() const = 0;
				virtual std::wstring GetName() const = 0;
				virtual void SetName(const std::wstring& name) = 0;
		};

		/** Interface that is passed as parameter to Track::CreateOutlets which the track can use to create outlets **/
		class OutletFactory {
			public:
				virtual ~OutletFactory();
				virtual ref<Outlet> CreateOutlet(const std::wstring& id, const std::wstring& name) = 0;
		};

		/** A 'live control' is a control, such as a slider, used to transmit some value over the network
		when the master is not in play-mode, effectively allowing the user to change for example DMX values
		manually ('live'). Each tick, TJShow calls the Tick-method of each LiveControl. If the live control 
		writes to the provided stream, it will send the contents of that stream over the network. If the stream
		is not touched, nothing is sent. A live control should obviously try to minimize the amount of data
		written, and should only write data when that data has changed. 
		**/
		class LiveControl: public virtual tj::shared::Object {
			public:
				struct EndpointDescription {
					inline EndpointDescription(const std::wstring& id, const std::wstring& friendly): _id(id), _friendlyName(friendly) {
					}

					std::wstring _id;
					std::wstring _friendlyName;
				};

				virtual ~LiveControl() {
				}

				virtual ref<tj::shared::Wnd> GetWindow() = 0;
				virtual void Update() = 0;
				
				virtual std::wstring GetGroupName() {
					return L"Live";
				}

				virtual bool IsSeparateTab() {
					return false;
				}

				virtual tj::shared::Pixels GetWidth() {
					return 30;
				}

				virtual void SetPropertyGrid(ref<PropertyGridProxy> pg) {
				};

				virtual ref<tj::shared::Endpoint> GetEndpoint(const std::wstring& name) {
					return 0;
				}

				virtual void GetEndpoints(std::vector<EndpointDescription>& eps) {
				}

				virtual bool IsInitiallyVisible() {
					return true;
				};
		};

		/** Base class of all plugins. The plug-in class does not need to be thread safe; however,
		if you are going to call it from your Track or Player implementations, it probably has to be.
		**/
		class Plugin: public virtual tj::shared::Object {
			friend class PluginManager;

			public:
				virtual ~Plugin() {}

				/* Returns a language-independent name for this plug-in. Do not change this,
				as file-loading and saving uses this to identify the plug-in! Use GetFriendlyName
				for a localized name */
				virtual std::wstring GetName() const = 0;

				/* This returns a localized (same as user language) name for this track. */
				virtual std::wstring GetFriendlyName() const = 0;
				virtual std::wstring GetFriendlyCategory() const = 0;
				virtual std::wstring GetVersion() const = 0;
				virtual std::wstring GetAuthor() const = 0;
				virtual std::wstring GetDescription() const = 0;
				virtual void GetRequiredFeatures(std::list<std::wstring>& fts) const {};

				/* Called when a new file is opened or created in TJShow. This can be used by a DMX-plugin
				for example to reset all output values back to zero. */
				virtual void Reset() {};
				
				virtual ref<tj::script::Scriptable> GetScriptable() { return 0; };
				virtual ref<tj::shared::Pane> GetSettingsWindow(ref<PropertyGridProxy> pg) { return 0; };

				// Plugin settings
				virtual void Load(TiXmlElement* you, bool showSpecific) {};
				virtual void Save(TiXmlElement* you, bool showSpecific) {};
		};

		class OutputPlugin: public Plugin {
			public:
				virtual ~OutputPlugin() {};
				virtual ref<Track> CreateTrack(ref<Playback> playback) = 0;
				virtual ref<StreamPlayer> CreateStreamPlayer(ref<Playback> playback, ref<Talkback> talk) = 0;
				virtual void Message(ref<tj::shared::DataReader> msg) {};
				virtual void GetDevices(std::vector< ref<Device> >& devs) {};
		};

		class InputPlugin: public Plugin {
			public:
				virtual ~InputPlugin() {};
				virtual void GetDevices(std::vector< ref<Device> >& devs, ref<input::Dispatcher> dispatcher) { };
		};

		/** An Item represents an item on a track. TJShow can query for an item at a specified time.
		This class facilitates built-in drag-and-drop features for the timeline. **/
		class Item: public virtual tj::shared::Object, public virtual tj::shared::Inspectable {
			public:
				enum Direction {
					None = 0,
					Left,
					Right,
					Up,
					Down,
				};

				virtual ~Item() {}
				virtual void MoveEnded() {};
				virtual void Move(Time t, int h) = 0;
				virtual void MoveRelative(Direction d) {};
				virtual void Remove() = 0;
				virtual ref<tj::shared::PropertySet> GetProperties() { return 0; };
				
				/* This should return the distance (to the left: negative, otherwise positive) to
				the point the item is *really* dragging. For example, if you are dragging an item
				but the underlying item is 10ms ahead, return 10 here. */
				virtual tj::shared::Pixels GetSnapOffset() { return 0; };
				virtual std::wstring GetTooltipText() { return L""; };
		};

		/** Represents a range on the timeline. **/
		class TrackRange: public virtual tj::shared::Object {
			public:
				virtual ~TrackRange() {}
				virtual void RemoveItems() = 0;
				virtual void Paste(ref<Track> other, Time start) = 0;
				virtual void InterpolateLeft(Time left, float ratio, Time right) {};
				virtual void InterpolateRight(Time left, float ratio, Time right) {};
		};

		/** The track object represents a track on the timeline. A track is created by Plugin::CreateTrack
		(your own implementation of it of course). A track usually holds some items and some properties
		for the track.  The track is also the creator of the master-side Player (client-side StreamPlayer will
		be created by Plugin::CreateStreamPlayer). 
		
		Furthermore, each track can have one LiveControl associated with it. It is created with 
		Track::CreateControl. See LiveControl documentation for more information. 
		**/
		class Track: public virtual tj::shared::Object, public virtual tj::shared::Serializable, public virtual tj::shared::Inspectable {
			public:
				virtual ~Track() {}
				virtual std::wstring GetTypeName() const = 0;

				/* Returns the preferred height for this track. If it is zero or smaller than the application setting
				for minimum track height, then the track is set to have a height specified by the application. */
				virtual tj::shared::Pixels GetHeight() { return 0; }

				virtual tj::shared::Flags<tj::np::RunMode> GetSupportedRunModes() = 0;
				virtual ref<Player> CreatePlayer(ref<tj::np::Stream> stream) = 0;

				/** Returns the item at the specified location. If no item is present at that time,
				return ref<Item>(0). Height is the height on the track at which the mouse clicked and
				trackHeight is the total height of the track (which might be different than what GetHeight returns!) **/
				virtual ref<Item> GetItemAt(Time position,unsigned int height, bool rightClick, int trackHeight, float pixelsPerMs) { return 0; }
				virtual bool PasteItemAt(const Time& position, TiXmlElement* item) { return false; };
				virtual void OnDoubleClick(Time position, unsigned int h) {};
				virtual void Paint(tj::shared::graphics::Graphics* g, Time position, float pixelsPerMs, unsigned int x, unsigned int y, unsigned int h, Time start, Time end, ref<Item> focus) {}
				
				/** Returns a list of properties for this track (not for specific items on the track). If there are no
				properties, return 0. **/
				virtual ref<tj::shared::PropertySet> GetProperties() { return 0; };
				virtual void GetResources(std::vector<tj::shared::ResourceIdentifier>& res) {};

				/** If this track wants to be scriptable, it can expose new methods through a scriptable object returned
				by this method. If the track does not implement this method, no additional functionality will be available
				through scripting (only default functionality from TrackScriptable - see tjwrappers.h) **/
				virtual ref<tj::script::Scriptable> GetScriptable() { return 0; };

				/** Return a live control window. This is meant as a way to provide the user with live, manual access
				to a plugin's output (for example, a DMX plugin should create some slider window to directly allow the user
				to control the channel's value. If this method returns 0, no window will be added to the live-tab in TJShow. 
				
				The returned window should also be a subclass of the LiveControl-class described above to enable network
				transmission. If it is not a subclass, the control might not work at all. 
				**/
				virtual ref<LiveControl> CreateControl(ref<tj::np::Stream> stream) { return 0; }

				/** Called when the 'insert current control value to track'-function is used. The ref<LiveControl> 
				can always be safely casted to the type the application created in  CreateControl. **/
				virtual void InsertFromControl(Time t, ref<LiveControl> control, bool fade=true) {}

				/** Is the track expandable/collapsable? **/
				virtual bool IsExpandable() { return false;}
				virtual void SetExpanded(bool e) {}
				virtual bool IsExpanded() { return false; }
				virtual ref<TrackRange> GetRange(Time start, Time end) { return 0; };

				/** This can be used to get the name and channel. Useful for some plug-ins (such as the DMX patching grid), but
				don not generally use it for anything other than just displaying this data **/
				virtual void OnSetInstanceName(const std::wstring& name) {};
				virtual void OnSetID(const TrackID& c) {};
				virtual void Clone() {};

				/** Outlets can be used to transfer track data to variables whenever the track wants. For example, this could be
				used for tracks that ask the user for text. This text then needs to be sent to a database track through a variable */
				virtual void CreateOutlets(OutletFactory& of) {};
		};

		class Controller;

		/** A player is created for each part of the program that wants to 'play' part of the timeline. It is different from
		the method the program views a certain point in time of the timeline, because that's for editing. The player
		can do what the TransientState does in TJBeamer. Players are create on the master side, and communicate with
		StreamPlayers, which are created on the client automatically, through a Stream. 

		The Stream (given as ref<Stream> to CreatePlayer) is responsible for message creation and delivery
		between master and client. See Message and Stream documentation for more information.

		Call sequence:
		- Track->CreatePlayer() from main-thread
		- Player->Start(Time) from main-thread. Load your resources here and do anything else that takes time. TJShow
		  will not start playing before all Start-methods have returned.

		When all players are ready for playing, a separate thread (the player-thread) does the following (looped):
			- Player->GetNextEvent(time)
			- If an event is scheduled (i.e. the return value is >= 0 and >= currentTime):
				- Player->Tick(...)
			- Else quit the thread

		- If a jump is done, the main-thread or the player-thread will call Player->Jump(). 

		- Player->Stop() when playback is stopped. After this call, the player will be destructed.

		Please note that Player is highly multithreaded. Usually, all methods (except creation and information
		methods, like GetOutputs) are called by a separate player thread. 
		*/
		class Player: public virtual tj::shared::Object {
			public:
				virtual ~Player() {}
				virtual ref<Track> GetTrack() = 0;
				
				/** Called when playback stops. You will want to free your resources here.**/
				virtual void Stop() = 0;

				/** Start is called when playback is about to start. TJShow tries to make sure that Start is 
				called and has returned for all tracks before it starts playing back the whole timeline. However,
				if Start doesn't return within a certain time, TJShow may choose to allow the other, initialized tracks
				to start already.
				**/
				virtual void Start(Time pos, ref<Playback> playback, float speed) = 0; 

				/** Called every time the client goes into pause mode **/
				virtual void Pause(Time pos) {};

				/** Called by TJShow when a scheduled event is due. Your player defines when these events occur through
				the GetNextEvent method. Most tracks use Tick() to update their output. **/
				virtual void Tick(Time currentPosition) = 0;

				/** Called when the user jumped to another time on the timeline while not in playback. 
				In playback, Stop() and then Start(new_time) would have been called. 
				If pause==true, then the Jump was done while TJShow was paused.
				**/
				virtual void Jump(Time t, bool pause) {}

				/** Multi-threaded implementations of host programs may use some sort of scheduler-strategy,
				where the Tick-function is only called when the Player has something to do. This method
				should return the next Time it wants to recieve a tick, relative to the time parameter given.**/
				virtual Time GetNextEvent(Time t) = 0;

				/** Called when the playback speed changes. Ticks are automatically sent using the new time, but if
				for example you're playing video or audio, you should speed it up/slow it down here. Otherwise,
				you can just safely ignore this and not implement this method. **/
				virtual void SetPlaybackSpeed(Time t, float c) {};
				
				/** Enable or disable output **/
				virtual void SetOutput(bool enable) = 0;
		};

		/** This is the part of your plug-in that runs on the client. It recieves messages from the master which you
		send in your Player through the ref<Stream>. **/
		class StreamPlayer: public virtual tj::shared::Object {
			public:
				virtual ref<Plugin> GetPlugin() = 0;
				virtual void Message(ref<tj::shared::DataReader> msg, ref<Playback> pb) = 0;
				virtual ~StreamPlayer() {}
		};
	}
}

#endif