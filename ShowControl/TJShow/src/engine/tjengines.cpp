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
#include "../../include/internal/tjshow.h"
#include "../../include/internal/engine/tjengine.h"
#include "../../include/internal/engine/tjmtengine.h"
#include "../../include/internal/engine/tjpoolengine.h"

using namespace tj::shared;
using namespace tj::show;
using namespace tj::show::engine;

/** Engine **/
Engine::Engine() {
}

Engine::~Engine() {
}

/** Engines **/
ref<Engine> Engines::CreateEngineOfType(Engines::EngineType t) {
	switch(t) {
		case Engines::TypeThreadPooled:
			return GC::Hold(new PoolEngine());

		case Engines::TypeThreadPerPlayer:
		default:
			return GC::Hold(new MultiThreadedEngine());
	}
}