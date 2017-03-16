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
#include "../include/tjdataplugin.h"
#include <algorithm>

DataCue::DataCue(Time pos): _time(pos) {
}

DataCue::~DataCue() {
}

void DataCue::Load(TiXmlElement* you) {
	_time = LoadAttributeSmall(you, "time", _time);
	_query = LoadAttribute(you, "query", _query);
}

void DataCue::Save(TiXmlElement* me) {
	TiXmlElement cue("cue");
	SaveAttributeSmall(&cue, "time", _time);
	SaveAttribute(&cue, "query", _query);
	me->InsertEndChild(cue);
}

void DataCue::Move(Time t, int h) {
	_time = t;
}

ref<DataCue> DataCue::Clone() {
	ref<DataCue> dc = GC::Hold(new DataCue(_time));
	dc->_query = _query;
	return dc;
}

ref<PropertySet> DataCue::GetProperties(ref<Playback> pb, strong< CueTrack<DataCue> > track) {
	ref<PropertySet> ps = GC::Hold(new PropertySet());
	ps->Add(GC::Hold(new GenericProperty<Time>(TL(data_cue_time), this, &_time, _time)));
	ps->Add(GC::Hold(new TextProperty(TL(data_query), this, &_query, 200))); // TODO: maybe use Scintilla for a SQLProperty?
	return ps;
}

Time DataCue::GetTime() const {
	return _time;
}

void DataCue::SetTime(Time t) {
	_time = t;
}

ref<Query> DataCue::Fire(ref<Database> db, ref<Playback> pb, ref<Outlet> tupleOut, ref<Stream> stream, ref<Query> current, int& rowNumberOut, bool& hasNextRow, ref<DataQueryHistory> hist) {
	ref<Query> q = current;

	if(_query.length()>0) {
		std::wstring queryTranslated = pb->ParseVariables(_query);
		q = db->CreateQuery(queryTranslated);
		if(hist) {
			hist->AddQuery(queryTranslated);
		}
		q->Execute();
		rowNumberOut = 0;
	}
	else {
		// Fetch a row
		if(q&& q->HasRow()) {
			q->Next();
			++rowNumberOut;
		}
	}

	hasNextRow = q->HasRow();

	// If there is data, write it to the outlet
	if(q && q->HasRow()) {
		unsigned int nc = (unsigned int)q->GetColumnCount();
		strong<Tuple> tuple = GC::Hold(new Tuple(nc));
		for(unsigned int a=0;a<nc;a++) {
			Any value = q->GetAny(a);
			tuple->Set(a, value);
		}
		pb->SetOutletValue(tupleOut, Any(tuple));
	}

	return q;
}

void DataCue::Paint(tj::shared::graphics::Graphics* g, ref<Theme> theme, float pixelLeft, unsigned int y, unsigned int h, ref< CueTrack<DataCue> > track, bool focus) {
	Pen pn(theme->GetColor(Theme::ColorCurrentPosition), focus? 2.0f : 1.0f);
	tj::shared::graphics::SolidBrush cueBrush = theme->GetColor(Theme::ColorCurrentPosition);
	tj::shared::graphics::SolidBrush onBrush(Color(1.0,0.0,0.0));
	tj::shared::graphics::SolidBrush offBrush(Color(0.5, 0.5, 0.5));
	g->DrawLine(&pn, pixelLeft, (float)y, pixelLeft, float(y+(h/4)));

	StringFormat sf;
	sf.SetAlignment(StringAlignmentCenter);

	std::wstring::size_type firstSpace = _query.find_first_of(L' ');
	std::wstring str(_query, 0, firstSpace);
	std::transform(str.begin(), str.end(), str.begin(), toupper);
	g->DrawString(str.c_str(), (int)str.length(), theme->GetGUIFontSmall(), PointF(pixelLeft+(DataTrack::KCueWidth/2), float(y+h/2)), &sf, &cueBrush);
}


DataTrack::DataTrack(ref<Playback> pb): _pb(pb) {
	_hist = GC::Hold(new DataQueryHistory());
}

DataTrack::~DataTrack() {
}

std::wstring DataTrack::GetTypeName() const {
	return L"Data";
}

ref<Player> DataTrack::CreatePlayer(ref<Stream> str) {
	return GC::Hold(new DataPlayer(this, str));
}

void DataTrack::GetResources(std::vector<tj::shared::ResourceIdentifier>& rids) {
	rids.push_back(_databaseIdentifier);
}

void DataTrack::CreateOutlets(OutletFactory& of) {
	_tupleOut = of.CreateOutlet(L"tuple-out", TL(data_tuple_outlet));
	_currentRowNumberOut = of.CreateOutlet(L"row-number-out", TL(data_row_number_outlet));
	_hasNextRowOut = of.CreateOutlet(L"has-row-out", TL(data_has_next_outlet));
}

ref<Outlet> DataTrack::GetHasNextRowOutlet() {
	return _hasNextRowOut;
}

ref<Outlet> DataTrack::GetTupleOutlet() {
	return _tupleOut;
}

ref<Database> DataTrack::GetDatabase() {
	ThreadLock lock(&_lock);

	if(_loadedDatabase==_databaseIdentifier) {
		return _db;
	}

	std::wstring localPath;
	if(_pb->GetResources()->GetPathToLocalResource(_databaseIdentifier, localPath)) {
		_db = Database::Open(localPath);
	}
	
	return _db;
}

Flags<RunMode> DataTrack::GetSupportedRunModes() {
	Flags<RunMode> rm;
	rm.Set(RunModeMaster, true);
	rm.Set(RunModeDont, true);
	return rm;
}

ref<Playback> DataTrack::GetPlayback() {
	return _pb;
}

ref<PropertySet> DataTrack::GetProperties() {
	ref<PropertySet> ps = GC::Hold(new PropertySet());
	ps->Add(GC::Hold(new FileProperty(TL(data_database_file), this, &_databaseIdentifier, _pb->GetResources(), L"*.db (Database)\x0000*.db\x0000\x0000\x0000")));
	return ps;
}

void DataTrack::SetDatabase(const std::wstring& path) {
	// Convert path to rid
	strong<ResourceProvider> res = _pb->GetResources();
	_databaseIdentifier = res->GetRelative(path);

	// Rest of the loading is done in GetDatabase(); people still busy with the old database
	// will continue to use it as long as they hold a reference to it.
}

strong<DataQueryHistory> DataTrack::GetHistory() {
	return _hist;
}

ref<LiveControl> DataTrack::CreateControl(ref<Stream> str) {
	return GC::Hold(new DataLiveControl(this, str));
}

void DataTrack::Save(TiXmlElement* parent) {
	CueTrack<DataCue>::Save(parent);
	SaveAttributeSmall(parent, "database", _databaseIdentifier);
}

void DataTrack::Load(TiXmlElement* you) {
	CueTrack<DataCue>::Load(you);
	_databaseIdentifier = LoadAttributeSmall(you, "database", _databaseIdentifier);
}

std::wstring DataTrack::GetEmptyHintText() const {
	return TL(data_track_empty_text);
}

ref<Outlet> DataTrack::GetCurrentRowNumberOutlet() {
	return _currentRowNumberOut;
}

/** DataQueryHistory **/
DataQueryHistory::DataQueryHistory(unsigned int size): _history(size) {

}

DataQueryHistory::~DataQueryHistory() {
}

void DataQueryHistory::AddQuery(const std::wstring& q) {
	_history.push_front(q);
	if(_history.size()>_size) {
		_history.pop_back();
	}
}