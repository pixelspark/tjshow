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
#ifndef _TJDMXPATCHABLE_H
#define _TJDMXPATCHABLE_H

class DMXPatchable: public virtual Inspectable {
	public:
		virtual ~DMXPatchable();
		virtual void SetDMXAddress(const std::wstring& address) = 0;
		virtual std::wstring GetDMXAddress() const = 0;
		virtual std::wstring GetTypeName() const = 0;
		virtual std::wstring GetInstanceName() const = 0;
		virtual TrackID GetID() const = 0;
		virtual bool GetResetOnStop() const = 0;
		virtual void SetResetOnStop(bool t) = 0;
		virtual int GetSliderType() = 0;
		virtual int GetSubChannelID() const;

		enum PropertyID {
			PropertyAddress = 1,
			PropertyResetOnStop,
		};

		virtual ref<Property> GetPropertyFor(PropertyID pid) = 0;
};

#endif