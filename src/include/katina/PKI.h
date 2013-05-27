#pragma once
#ifndef _OASTATS_PKI_H
#define	_OASTATS_PKI_H
/*
 * File:   PKI.h
 * Author: SooKee oasookee@gmail.com
 *
 * Created on May 3, 2013, 7:40 AM
 */

/*-----------------------------------------------------------------.
| Copyright (C) 2013 SooKee oasookee@gmail.com               |
'------------------------------------------------------------------'

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301, USA.

http://www.gnu.org/licenses/gpl-2.0.html

'-----------------------------------------------------------------*/

#include <gcrypt.h>
#include "types.h"

namespace oastats { namespace pki {

using namespace oastats::types;

class PKI
{
public:
	typedef gcry_sexp_t key_t;
	typedef std::map<str, key_t> key_map;
	typedef std::pair<const str, key_t> key_map_pair;
	typedef key_map::iterator key_map_iter;
	typedef key_map::const_iterator key_map_citer;

private:
	key_t skey;
	key_t pkey;

	std::map<str, gcry_sexp_t> pkeys;

	bool set_keys(const key_t keypair);
	bool create_sexp_from_text(const str& exp, gcry_sexp_t& sexp) const;
	bool get_sexp_as_text(const key_t& sexp, str& text) const;
	bool get_signature(const gcry_sexp_t& sexp, str& signature) const;

public:
	PKI();
	virtual ~PKI();

	bool generate_keypair(siz bits = 512);

	/**
	 * Add a public key for encrypting data to its owner.
     * @param id A means to identify the public key's owner
     * @param s public key in sexp form
     * @return true on success else false
     */
	bool read_public_key(const str& id, const str& s);

	/**
	 * Add a public key for encrypting data to its owner.
     * @param id A means to identify the public key's owner
     * @param is input must contain the public key in sexp form
     * @return true on success else false
     */
	bool read_public_key(const str& id, std::istream& is);

	/**
	 * Add a public key for encrypting data to its owner.
	 * @param id A means to identify the public key's owner
     * @param file name of a file that must contain the public key in sexp form
     * @return true on success else false
     */
	bool load_public_key(const str& id, const str& file);

	/**
	 * Set the keypair
     * @param s
     * @return
     */
	bool read_keypair(const str& s);
	bool read_keypair(std::istream& is);
	bool load_keypair(const str& file);

	bool get_keypair_as_text(str& keypair) const;
	bool get_public_key_as_text(str& text) const;
	bool get_private_key_as_text(str& text) const;

	/**
	 * Encrypt text using public key belonging to id
	 * @param id The public key owner's id
	 * @param text The clear-text message to encrypt
	 * @param code Either the encrypted version of text or
	 * unchanged on error.
	 * @return true = success else false
	 */
	bool encrypt(const str& id, const str& text, str& code) const;

	/**
	 * Decrypt text using private key
	 */
	bool decrypt(const str& code, str& text) const;

	bool get_signature(str& signature) const; // use pkey
	bool get_signature(const str& text, str& signature) const;

	bool verify_signature(const str& id, const str& signature, bool& is_good) const; // pkey
	bool verify_signature(const str& id, const str& signature, const str& text, bool& is_good) const;
};

}} // oastats::pki

#endif	// _OASTATS_PKI_H

