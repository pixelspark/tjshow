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
#include "../include/internal/tjshow.h"
using namespace tj::show;

Deck::~Deck() {
}

Mesh::~Mesh() {
}

void Mesh::SetMixEffect(const std::wstring& ident, EffectParameter ep, float value) {
	ThreadLock lock(&_effectsLock);
	if(_effects.find(ep)==_effects.end()) {
		_effects[ep] = Mixed<float>();
	}
	_effects[ep].SetMixValue(ident, value);
}

bool Mesh::IsEffectPresent(EffectParameter ep) {
	ThreadLock lock(&_effectsLock);

	std::map<EffectParameter, Mixed<float> >::const_iterator it = _effects.find(ep);
	if(it!=_effects.end()) {
		return it->second.HasMixValues();
	}
	return false;
}

float Mesh::GetMixEffect(const std::wstring& ident, EffectParameter ep, float defaultValue) {
	ThreadLock lock(&_effectsLock);
	std::map<EffectParameter, Mixed<float> >::const_iterator it = _effects.find(ep);
	if(it!=_effects.end()) {
		return it->second.GetMixValue(ident, defaultValue);
	}
	return defaultValue;
}

void Mesh::RemoveMixEffect(const std::wstring& ident, EffectParameter ep) {
	ThreadLock lock(&_effectsLock);
	std::map<EffectParameter, Mixed<float> >::iterator it = _effects.find(ep);
	if(it!=_effects.end()) {
		it->second.RemoveMixValue(ident);
	}
}

/** Iterates over all effect values and removes values for a certain identifier when the effect
parameter is not in the set of effect parameters **/
void Mesh::RestrictMixEffects(const std::wstring& ident, std::set<EffectParameter> eps) {
	ThreadLock lock(&_effectsLock);
	std::map< EffectParameter, Mixed<float> >::iterator it = _effects.begin();
	while(it!=_effects.end()) {
		if(eps.find(it->first)==eps.end()) {
			it->second.RemoveMixValue(ident);
		}
		++it;
	}
}

float Mesh::GetEffectValue(EffectParameter ep, float source) {
	ThreadLock lock(&_effectsLock);
	std::map<EffectParameter, Mixed<float> >::const_iterator it = _effects.find(ep);
	if(it!=_effects.end()) {
		return it->second.GetMultiplyMixValue(source);
	}
	return source;
}

Surface::~Surface() {
}

SurfacePainter::~SurfacePainter() {
}

Keyable::~Keyable() {
}

/** Interactive **/
Interactive::Interactive(): _canFocus(false) {
}

Interactive::~Interactive() {
}

bool Interactive::CanFocus() const {
	return _canFocus;
}

void Interactive::SetCanFocus(bool can) {
	_canFocus = can;
}

void Interactive::SetFocus(bool f) {
	if(!_canFocus && f) {
		Throw(L"This interactive object cannot receive focus!", ExceptionTypeError);
	}

	FocusNotification fn;
	fn._focused = f;
	EventFocusChanged.Fire(null, fn);
}