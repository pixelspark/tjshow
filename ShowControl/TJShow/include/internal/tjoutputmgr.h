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
#ifndef _TJOUTPUTMGR_H
#define _TJOUTPUTMGR_H

interface IDirect3D9;

namespace tj {
	namespace show {
		namespace view {
			class PlayerWnd;
		}

		enum ScaleMode {
			ScaleModeNone = 0,		// 1 pixel always equals 1 screen pixel
			ScaleModeAll = 1,		// Scale, but keep all stuff visible (=> black bars)
			ScaleModeMax = 2,		// Scale and crop parts that are not visible (overscan)
			ScaleModeStretch = 3,	// Just stretch (changes aspect ratio, but always fits)
		};

		class ScreenDefinition: public Serializable, public Inspectable {
			public:
				ScreenDefinition();
				ScreenDefinition(strong<Settings> templ);
				ScreenDefinition(bool isTouchScreen, unsigned int docWidth, unsigned int docHeight, ScaleMode scale = ScaleModeNone);
				virtual ~ScreenDefinition();
				virtual bool IsTouchScreen() const;
				virtual unsigned int GetDocumentHeight() const;
				virtual unsigned int GetDocumentWidth() const;
				virtual ScaleMode GetScaleMode() const;
				virtual bool IsBuiltIn() const;
				virtual void SetBuiltIn(bool b);
				virtual bool ShowGuides() const;
				virtual void SetShowGuides(bool t);
				virtual const RGBColor& GetBackgroundColor() const;
				virtual ref<PropertySet> GetProperties();

				virtual void Load(TiXmlElement* el);
				virtual void Save(TiXmlElement* el);

			protected:
				unsigned int _documentWidth;
				unsigned int _documentHeight;
				bool _isTouchScreen;
				bool _isBuiltIn;
				bool _showGuides;
				ScaleMode _scaleMode;
				RGBColor _backgroundColor;
		};

		class ScreenDefinitions: public Serializable {
			public:
				ScreenDefinitions();
				virtual ~ScreenDefinitions();
				virtual ref<ScreenDefinition> GetDefinitionByID(const std::wstring& screenDefID);
				virtual void SetDefinition(const std::wstring& id, ref<ScreenDefinition> def);
				virtual void Save(TiXmlElement* parent);
				virtual void Load(TiXmlElement* parent);
				virtual void Clear();
				virtual unsigned int GetDefinitionCount() const;

			protected:
				std::map< std::wstring, ref<ScreenDefinition> > _defs;
		};

		class OutputManager: public virtual tj::shared::Object, public Playback {
			friend class view::PlayerWnd;
			
			public:
				OutputManager();
				virtual ~OutputManager();

				// Playback
				virtual ref<Deck> CreateDeck(bool visible, ref<Device> screen = 0);
				virtual ref<Surface> CreateSurface(int w, int h, bool visible, ref<Device> screen = 0);
				virtual bool IsKeyingSupported(ref<Device> screen = 0);
				virtual ref<Device> GetDeviceByPatch(const PatchIdentifier& t);
				virtual PatchIdentifier GetPatchByDevice(ref<Device> dev);
				virtual ref<Property> CreateSelectPatchProperty(const std::wstring& name, ref<Inspectable> holder, PatchIdentifier* pi);
				virtual std::wstring ParseVariables(const std::wstring& source);
				virtual strong<Property> CreateParsedVariableProperty(const std::wstring& name, ref<Inspectable> holder, std::wstring* source, bool multiline);
				virtual void QueueDownload(const std::wstring& rid);
				virtual strong<ResourceProvider> GetResources();
				virtual void SetOutletValue(ref<Outlet> outlet, const Any& value);

				/* The 'dirty' status determines whether PlayerWnd should rebuild geometry. Rebuilding geometry is needed in 
				the following cases:
				- A mesh has been added or removed
				- The texture width or height of one of the meshes has changed
				- The horizontal/vertical flip status of one of the meshes has changed
				
				VideoDeck normally doesn't change texture w/h and flip status, but Surface does. Therefore, Surface calls OutputManager::SetDirty
				when it does so. PlayerWnd calls 'IsDirty', which returns true when the output manager is 'dirty', and also resets the dirty status. */
				bool IsDirty(); // modifies dirty state!
				void SetDirty();
				void SortMeshes();

				// Screen management
				ref<view::PlayerWnd> AddScreen(const std::wstring& name, strong<ScreenDefinition> sd);	// Adds a simple screen (typically used on the master)
				void AddAllScreens(std::vector< ref<view::PlayerWnd> >& created);	// Adds all available screens, full-screen (on client)
				int GetScreenCount() const;
				ref<view::PlayerWnd> GetScreenWindow(int screen);

				virtual bool IsFeatureAvailable(const std::wstring& ft);

				virtual void ListDevices(std::vector< ref<Device> >& devs);

			protected:
				void CleanMeshes();

				IDirect3D9* _d3d;
				std::vector< ref<view::PlayerWnd> > _screens;
				std::vector< ref<Device> > _screenDevices;
				std::map< std::wstring, ref<Device> > _existingVideoDevices;
				mutable CriticalSection _lock;
				volatile bool _dirty;
		};
	}
}

#endif
