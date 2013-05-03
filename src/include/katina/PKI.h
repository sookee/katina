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
	gcry_sexp_t skey;
	gcry_sexp_t pkey;

public:
	PKI();
	virtual ~PKI();

	bool generate_keys();
	bool generate_keys(str& keydata);

	bool set_skey(const str& s);
	bool set_pkey(const str& s);
	bool load_keys(const str& file);
	bool read_keys(std::istream& is);

	/**
	 * Encrypt text using public key
	 */
	bool encrypt(const str& text, str& code);

	/**
	 * Decrypt text using private key
	 */
	bool decrypt(const str& code, str& text);

private:

};

}} oastats::pki

#endif	// _OASTATS_PKI_H

