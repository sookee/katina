/*
 * File:   PKI.h
 * Author: SooKee oasookee@gmail.com
 *
 * Created on May 3, 2013, 7:40 AM
 */
#pragma once
#ifndef _OASTATS_PKI_H
#define	_OASTATS_PKI_H

#include <gcrypt.h>

namespace oastats { namespace pki {

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
	bool create_sexp_from_text(const str& exp, gcry_sexp_t& sexp);
	bool get_sexp_as_text(const key_t& sexp, str& text);
	bool create_signature(const gcry_sexp_t& sexp, str& signature);

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

	bool get_keypair_as_text(str& keypair);
	bool get_public_key_as_text(str& text);
	bool get_private_key_as_text(str& text);

	/**
	 * Encrypt text using public key belonging to id
	 * @param id The public key owner's id
	 * @param text The clear-text message to encrypt
	 * @param code Either the encrypted version of text or
	 * unchanged on error.
	 * @return true = success else false
	 */
	bool encrypt(const str& id, const str& text, str& code);

	/**
	 * Decrypt text using private key
	 */
	bool decrypt(const str& code, str& text);

	bool create_signature(str& signature); // use pkey
	bool create_signature(const str& text, str& signature);

	bool verify_signature(const str& id, const str& signature, bool& is_good); // pkey
	bool verify_signature(const str& id, const str& signature, const str& text, bool& is_good);
};

}} oastats::pki

#endif	// _OASTATS_PKI_H

