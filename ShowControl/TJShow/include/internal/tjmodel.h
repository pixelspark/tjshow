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
#ifndef _TJMODEL_H
#define _TJMODEL_H

/* The Model-class defines all 'data' that is available to view and controller in this application. It holds
the main timeline instance, for example */
namespace tj {
	namespace script {
		class ScriptMap;
	}
}


namespace tj {
	namespace show {
		class Track;
		class Cue;
		class Network;
		class Dashboard;

		class Patches: public virtual Object, public Serializable {
			friend class Network; 

			public:
				Patches();
				virtual ~Patches();
				virtual void Load(TiXmlElement* you);
				virtual void Save(TiXmlElement* me);
				virtual bool DoesPatchExist(const PatchIdentifier& pi);
				virtual DeviceIdentifier GetDeviceByPatch(const PatchIdentifier& pi, bool createIfNotExistant = false);
				virtual void Clear();
				virtual unsigned int GetPatchCount() const;
				const std::map< PatchIdentifier, DeviceIdentifier >& GetPatches();
				virtual void AddPatch(const PatchIdentifier& pi);
				virtual void RemovePatch(const PatchIdentifier& pi);
				virtual void SetPatch(const PatchIdentifier& pi, ref<Device> di);
				virtual void SetPatch(const PatchIdentifier& pi, const DeviceIdentifier& di);
				virtual PatchIdentifier GetPatchForDevice(const DeviceIdentifier& di);
				virtual PatchIdentifier GetPatchByIndex(unsigned int i);

			protected:
				mutable CriticalSection _lock;
				std::map< PatchIdentifier, DeviceIdentifier > _patches;
		};

		class Model: public virtual Object, public Serializable, public Inspectable {
			public:
				Model(strong<Network> net);
				virtual ~Model();
				void Clear(); // empties the whole model
				virtual void Save(TiXmlElement* parent);
				virtual void Load(TiXmlElement* me);
				virtual void Save(TiXmlElement* parent, const std::wstring& filePath);
				virtual void Load(TiXmlElement* me, const std::wstring& filePath);
				
				virtual ref<PropertySet> GetProperties();
				virtual ref<Resources> GetResources();
				virtual strong<Groups> GetGroups();

				void New();
				bool IsSaved() const;
				std::wstring GetFileName() const;
				void SetFileName(const std::wstring& path);
				ref<ResourceManager> GetResourceManager();
				std::wstring GetWebDirectory() const; // empty when tsx is not saved yet
				std::wstring GetAuthor() const;
				std::wstring GetTitle() const;
				int IncrementVersion();

				strong<Timeline> GetTimeline();
				strong<CueList> GetCueList();
				ref<Crumb> CreateModelCrumb();
				strong<VariableList> GetVariables();
				strong<Capacities> GetCapacities();
				strong<ScriptMap> GetGlobals();
				strong<Patches> GetPatches();
				strong<ScreenDefinitions> GetScreens();
				strong<input::Rules> GetInputRules();
				strong<Dashboard> GetDashboard();

				strong<ScreenDefinition> GetPreviewScreenDefinition(strong<Settings> templ);
				ref<Timeline> GetTimelineByID(const TimelineIdentifier& tid, ref<Timeline> parent = null);

				std::string GetFileHash() const;
				void SetFileHash(const std::string& sh);

			protected:
				strong<Resources> _resources;
				strong<Timeline> _timeline;
				strong<VariableList> _variables;
				strong<Capacities> _caps;
				strong<Patches> _patches;
				strong<Groups> _groups;
				strong<ResourceManager> _rmg;
				strong<input::Rules> _rules;
				strong<ScreenDefinitions> _screens;
				strong<Dashboard> _dashboard;

				ref<ResourceBundle> _rb;
				std::wstring _filename;
				strong<ScriptMap> _globals;
				std::wstring _author;
				std::wstring _title;
				std::wstring _notes;
				std::string _fileHash;
				int _version;
				ShowID _showID;

		};
	}
}

#endif