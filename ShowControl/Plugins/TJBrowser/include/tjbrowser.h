#ifndef _TJBROWSER_H
#define _TJBROWSER_H

#include "../../../TJShow/include/tjshow.h"
using namespace tj::shared;
using namespace tj::show;
using namespace tj::np;

namespace Awesomium {
	class WebCore;
	class WebView;
}

namespace tj {
	namespace browser {
		class BrowserPlugin: public OutputPlugin {
			public:
				BrowserPlugin();
				virtual ~BrowserPlugin();
				virtual std::wstring GetName() const;
				virtual ref<Track> CreateTrack(ref<Playback> playback);
				virtual ref<StreamPlayer> CreateStreamPlayer(ref<Playback> playback, ref<Talkback> talk);
				virtual std::wstring GetVersion() const;
				virtual std::wstring GetAuthor() const;
				virtual std::wstring GetFriendlyName() const;
				virtual std::wstring GetFriendlyCategory() const;
				virtual std::wstring GetDescription() const;
		};

		class BrowserTrack;

		class BrowserCue: public Item, public Serializable {
			public:
				BrowserCue(ref<BrowserTrack> track, Time t=0);
				virtual ~BrowserCue();
				Time GetTime() const;
				virtual void SetTime(Time t);
				std::wstring GetURL() const;
				virtual void Move(Time t, int h);
				virtual void Remove();
				virtual void MoveRelative(Item::Direction dir);
				virtual void Save(TiXmlElement* parent);
				virtual void Load(TiXmlElement* you);
				virtual ref<PropertySet> GetProperties();

			protected:
				Time _time;
				weak<BrowserTrack> _track;
				std::wstring _url;
		};

		class BrowserTrack: public Track {
			friend class BrowserPlayer;
			friend class BrowserTrackRange;
			friend class BrowserSurfacePlayer;

			public:
				BrowserTrack(ref<BrowserPlugin> plug);
				virtual ~BrowserTrack();

				virtual void Save(TiXmlElement* parent);
				virtual void Load(TiXmlElement* you);
				virtual std::wstring GetTypeName() const;
				virtual Flags<RunMode> GetSupportedRunModes();

				virtual ref<Player> CreatePlayer(ref<Stream> str);
				virtual ref<PropertySet> GetProperties();
				virtual void Paint(tj::shared::graphics::Graphics* g, Time position, float pixelsPerMs, unsigned int x, unsigned int y, unsigned int h, Time start, Time end, ref<Item> focus);
				virtual ref<Item> GetItemAt(Time pos,unsigned int h, bool right, int th, float pixelsPerMs);
				virtual ref<LiveControl> CreateControl(ref<Stream> stream);
				virtual ref<TrackRange> GetRange(Time s, Time e);

				ref<BrowserCue> GetCueBefore(Time pos);
				ref<BrowserCue> GetCueAfter(Time pos);
				void RemoveItemAt(Time t);

			protected:
				std::vector< ref<BrowserCue> > _cues;
				bool _hideOnStop;
		};

		enum BrowserAction {
			BrowserActionNone = 0,
			BrowserActionShow,
			BrowserActionHide,
			BrowserActionNavigate,
		};

		class BrowserWebViewListener;

		class BrowserSurfacePlayer: public Player, public Listener<Interactive::MouseNotification>, public Listener<Interactive::KeyNotification> {
			public:
				BrowserSurfacePlayer(ref<BrowserTrack> tr, ref<Stream> str);
				virtual ~BrowserSurfacePlayer();
				virtual ref<Track> GetTrack();
				virtual void Stop();
				virtual void Start(Time pos, ref<Playback> playback, float speed);
				virtual void Pause(Time pos);
				virtual void Tick(Time currentPosition);
				virtual void Jump(Time t, bool pause);
				virtual Time GetNextEvent(Time t);
				virtual void SetPlaybackSpeed(Time t, float c);
				virtual void SetOutput(bool enable);
				virtual void RepaintBrowser();
				virtual void Notify(ref<Object> source, const Interactive::MouseNotification& mn);
				virtual void Notify(ref<Object> source, const Interactive::KeyNotification& mn);

			protected:
				void Navigate(const std::wstring& url);

				CriticalSection _browserLock;
				ref<BrowserTrack> _track;
				ref<Stream> _stream;
				std::wstring _url;
				bool _output;
				ref<BrowserCue> _current;
				Awesomium::WebCore* _core;
				Awesomium::WebView* _view;
				BrowserWebViewListener* _listener;
				Pixels _width, _height;
				ref<Surface> _surface;
		};

		class BrowserStreamPlayer: public StreamPlayer {
			public:
				BrowserStreamPlayer(ref<BrowserPlugin> plugin);
				virtual ~BrowserStreamPlayer();
				virtual ref<Plugin> GetPlugin();
				virtual void Message(ref<Code> msg, ref<Playback> pb);

			protected:
				ref<BrowserPlugin> _plugin;
		};
	}
}
#endif
