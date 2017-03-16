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
#ifndef _TJSHOW_H
#define _TJSHOW_H

/** This is the main header file for TJShow. It defines classes and stuff. Take care that whatever
is put or included in here is also visible for dll-plugins. They will have to include this to know
about types that are used in TJShow.

Policy: don't add any 'using'-declarations for external classes in any header file included in here,
but use full qualified name-form (tj::shared::graphics::Classname) so classname conflicts will never happen,
even if someone includes one of our header files in their own project. **/

#undef TJSHARED_EXPORTS
#include <TJShared/include/tjshared.h>
#include <TJSharedUI/include/tjsharedui.h>
#include <TJNP/include/tjprotocol.h>
#include <TJNP/include/tjstream.h>

using tj::shared::ref;
using tj::shared::weak;
using tj::shared::strong;
using tj::shared::Time;

#include "tjfader.h"
#include "tjpatching.h"
#include "tjtalkback.h"
#include "tjplayback.h"
#include "tjplugin.h"

#endif