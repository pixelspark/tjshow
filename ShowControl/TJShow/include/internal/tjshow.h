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
#ifndef _TJINTERNAL_H
#define _TJINTERNAL_H

/* All include files that define classes and other stuff that plug-ins don't need should
be placed in here. Source files from TJShow that need internal stuff should include this 
header instead of tjshow.h 
*/
#include "../tjshow.h"
#include <atlbase.h>
#include <TJScript/include/tjscript.h>

using namespace tj::shared;
using namespace tj::script;
using namespace tj::np;

#include "../../resource.h"
#include "tjversion.h"
#include "tjresources.h"
#include "tjplugin.h"
#include "tjpluginmgr.h"
#include "tjgroup.h"
#include "tjtrack.h"
#include "tjcapacity.h"
#include "tjinput.h"
#include "tjoutputmgr.h"
#include "tjdeck.h"
#include "tjtimeline.h"
#include "tjcue.h"
#include "tjcuelist.h"
#include "tjexpression.h"
#include "tjvariable.h"
#include "tjmodel.h"
#include "tjview.h"
#include "tjinstance.h"
#include "tjapplication.h"

using namespace tj::show;

#endif