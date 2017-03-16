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
#ifndef _TJDATAPLUGIN_H
#define _TJDATAPLUGIN_H

#include <TJShow/include/tjshow.h>
#include <TJShow/include/extra/tjextra.h>
#include <TJDB/include/tjdb.h>
using namespace tj::shared;
using namespace tj::show;
using namespace tj::np;
using namespace tj::db;

class DataQueryHistory: public virtual Object {
	public:
		DataQueryHistory(unsigned int size = 20);
		virtual ~DataQueryHistory();
		virtual void AddQuery(const std::wstring& q);

		std::deque<std::wstring> _history;

	private:
		unsigned int _size;
};

class DataPlugin: public OutputPlugin {
	public:
		DataPlugin();
		virtual ~DataPlugin();
		virtual std::wstring GetName() const;
		virtual std::wstring GetFriendlyName() const;
		virtual std::wstring GetFriendlyCategory() const;
		virtual std::wstring GetVersion() const;
		virtual std::wstring GetAuthor() const;
		virtual std::wstring GetDescription() const;
		virtual ref<Track> CreateTrack(ref<Playback> playback);
		virtual ref<StreamPlayer> CreateStreamPlayer(ref<Playback> playback, ref<Talkback> talk);
		virtual void GetRequiredFeatures(std::list<std::wstring>& fts) const;
};

class DataCue: public Serializable, public virtual Inspectable {
	public:
		DataCue(Time pos = 0);
		virtual ~DataCue();

		virtual void Load(TiXmlElement* you);
		virtual void Save(TiXmlElement* me);
		virtual void Move(Time t, int h);
		virtual ref<DataCue> Clone();
		virtual ref<PropertySet> GetProperties(ref<Playback> pb, strong< CueTrack<DataCue> > track);
		Time GetTime() const;
		void SetTime(Time t);
		virtual void Paint(tj::shared::graphics::Graphics* g, ref<Theme> theme, float pixelLeft, unsigned int y, unsigned int h, ref< CueTrack<DataCue> > track, bool focus);
		virtual ref<Query> Fire(ref<Database> db, ref<Playback> pb, ref<Outlet> tupleOut, ref<Stream> stream, ref<Query> current, int& rowNumOut, bool& hasNextRow, ref<DataQueryHistory> hist);

	protected:
		Time _time;
		std::wstring _query;
};

class DataTrack: public CueTrack<DataCue> {
	public:
		DataTrack(ref<Playback> pb);
		virtual ~DataTrack();
		virtual std::wstring GetTypeName() const;
		virtual ref<Player> CreatePlayer(ref<Stream> str);
		virtual Flags<RunMode> GetSupportedRunModes();
		virtual ref<PropertySet> GetProperties();
		virtual ref<LiveControl> CreateControl(ref<Stream> str);
		virtual void Save(TiXmlElement* parent);
		virtual void Load(TiXmlElement* you);
		virtual void CreateOutlets(OutletFactory& of);
		virtual std::wstring GetEmptyHintText() const;
		virtual void GetResources(std::vector<ResourceIdentifier>& rids);

		virtual ref<Database> GetDatabase();
		virtual void SetDatabase(const std::wstring& path);
		virtual ref<Outlet> GetTupleOutlet();
		virtual ref<Outlet> GetCurrentRowNumberOutlet();
		virtual ref<Outlet> GetHasNextRowOutlet();

		virtual strong<DataQueryHistory> GetHistory();
		virtual ref<Playback> GetPlayback();

	protected:
		ref<Playback> _pb;
		ref<Outlet> _tupleOut;
		ref<Outlet> _currentRowNumberOut;
		ref<Outlet> _hasNextRowOut;
		ResourceIdentifier _databaseIdentifier;

		CriticalSection _lock;
		std::wstring _loadedDatabase;
		ref<Database> _db;
		ref<DataQueryHistory> _hist;
};

class DataPlayer: public Player {
	public:
		DataPlayer(ref<DataTrack> tr, ref<Stream> str);
		virtual ~DataPlayer();

		ref<Track> GetTrack();
		virtual void Stop();
		virtual void Start(Time pos, ref<Playback> playback, float speed);
		virtual void Tick(Time currentPosition);
		virtual void Jump(Time newT, bool paused);
		virtual void SetOutput(bool enable);
		virtual Time GetNextEvent(Time t);

	protected:
		ref<DataTrack> _track;
		ref<Playback> _pb;
		ref<Database> _db;
		ref<Stream> _stream;
		ref<Query> _query;
		bool _output;
		Time _last;
		int _n;
		bool _hasNextRow;
};

class DataLiveControl: public LiveControl {
	public:
		DataLiveControl(ref<DataTrack> track, ref<Stream> str);
		virtual ~DataLiveControl();
		virtual ref<Wnd> GetWindow();
		virtual void Update();
		virtual std::wstring GetGroupName();
		virtual bool IsSeparateTab();
		virtual int GetWidth();
		virtual void SetPropertyGrid(ref<PropertyGridProxy> pg);

	protected:
		ref<DataTrack> _track;
		ref<Wnd> _wnd;
};

#endif