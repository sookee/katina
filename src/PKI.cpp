/*
 * File:   PKI.cpp
 * Author: SooKee oasookee@gmail.com
 *
 * Created on May 3, 2013, 7:40 AM
 */

#include "PKI.h"

#include <fstream>
#include <gcrypt.h>

#include <katina/log.h>
#include <katina/types.h>
#include <katina/utils.h>

namespace oastats { namespace pki {

using namespace oastats::log;
using namespace oastats::types;
using namespace oastats::utils;

PKI::PKI()
{
	if(!gcry_check_version(GCRYPT_VERSION))
	{
		log("PKI: ERROR version check failed: " << GCRYPT_VERSION);
	}
	gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);
}

PKI::~PKI()
{
}

bool PKI::generate_keys()
{
	str keydata;
	return generate_keys(keydata);
}

bool PKI::generate_keys(str& keydata)
{
	siz errof;
	gcry_sexp_t params;
	if(gcry_error_t e = gcry_sexp_build(&params, &errof, "(genkey (rsa (nbits 3:256)))"))
	{
		log("PKI: ERROR setting parameters: " << gcry_strerror(e));
		return false;
	}

	gcry_sexp_t keys;
	if(gcry_error_t e = gcry_pk_genkey(&keys, params))
	{
		log("PKI: ERROR generating keys: " << gcry_strerror(e));
		return false;
	}

	if(!(skey = gcry_sexp_find_token(keys, "private-key", 0)))
	{
		log("PKI: ERROR retrieving private key");
		return false;
	}

	if(!(pkey = gcry_sexp_find_token(keys, "public-key", 0)))
	{
		log("PKI: ERROR retrieving public key");
		return false;
	}

	return true;
}

bool PKI::set_skey(const str& s)
{
	siz errof = 0;
	if(gcry_error_t e = gcry_sexp_build(&skey, &errof, s.c_str()))
	{
		log("PKI: ERROR creating private key: " << gcry_strerror(e));
		log("PKI: " << s);
		return false;
	}
	return true;
}

bool PKI::set_pkey(const str& s)
{
	siz errof = 0;
	if(gcry_error_t e = gcry_sexp_build(&pkey, &errof, s.c_str()))
	{
		log("PKI: ERROR creating public key: " << gcry_strerror(e));
		log("PKI: " << s);
		return false;
	}
	return true;
}

bool PKI::load_keys(const str& file)
{
	std::ifstream ifs(file.c_str());
	if(!ifs.is_open())
	{
		log("PKI: Error opening file; " << file);
		return false;
	}
	return read_keys(ifs);
}

bool PKI::read_keys(std::istream& is)
{
	str line;
	str_vec parts;
	while(std::getline(is, line))
		parts.push_back(line);

	if(parts.size() != 6)
	{
		log("PKI: ERROR: reading keys");
		return false;
	}

	soss oss;
	oss << "(private-key (rsa ";
	oss << "(n #" << parts[0] << "#)";
	oss << "(e #" << parts[1] << "#)";
	oss << "(d #" << parts[2] << "#)";
	oss << "(p #" << parts[3] << "#)";
	oss << "(q #" << parts[4] << "#)";
	oss << "(u #" << parts[5] << "#)";
	oss << "))";

	if(!set_skey(oss.str().c_str()))
		return false;

	oss.str("");

	oss << "(public-key (rsa ";
	oss << "(n #" << parts[0] << "#)";
	oss << "(e #" << parts[1] << "#)";
	oss << "))";

	if(!set_pkey(oss.str().c_str()))
		return false;

	return true;
}

bool PKI::encrypt(const str& text, str& code)
{
	gcry_sexp_t data;

	// convert text into data

	siz errof = 0;
	if(gcry_error_t e = gcry_sexp_build(&data, &errof, "%s", text.c_str()))
	{
		log("PKI: ERROR: converting text to sexp: " << gcry_strerror(e));
		return false;
	}

	gcry_sexp_t result;

	if(gcry_error_t e = gcry_pk_encrypt(&result, data, pkey))
	{
		log("PKI: encrypt error: " << gcry_strerror(e));
		return false;
	}

	//con("DUMP: " << __func__);
	gcry_sexp_dump(result);

	siz len = 0;
	char* buff = new char[(len = gcry_sexp_sprint(result, GCRYSEXP_FMT_CANON, 0, 0))];

	if(!buff)
	{
		log("PKI: ERROR: creating buffer");
		return false;
	}

	if(!(len = gcry_sexp_sprint(result, GCRYSEXP_FMT_CANON, buff, len)))
	{
		log("PKI: ERROR: converting sexp to string");
		delete[] buff;
		return false;
	}

	code.assign(buff, len);
	delete[] buff;

	return true;
}

bool PKI::decrypt(const str& code, str& text)
{
	gcry_sexp_t data;

	// convert code into data

	siz errof = 0;
//	if(gcry_error_t e = gcry_sexp_build(&data, &errof, "%s", code.c_str()))
	if(gcry_error_t e = gcry_sexp_new(&data, code.c_str(), code.size(), 0))
	{
		log("PKI: ERROR: converting code to sexp: " << gcry_strerror(e));
		return false;
	}

	gcry_sexp_t result;

	if(gcry_error_t e = gcry_pk_decrypt(&result, data, skey))
	{
		log("PKI: decrypt error: " << gcry_strerror(e));
		return false;
	}

	//con("DUMP: " << __func__);
	gcry_sexp_dump(result);

	siz len = 0;
	char* buff = new char[(len = gcry_sexp_sprint(result, GCRYSEXP_FMT_DEFAULT, 0, 0))];

	if(!buff)
	{
		log("PKI: ERROR: creating buffer");
		return false;
	}

	if(!(len = gcry_sexp_sprint(result, GCRYSEXP_FMT_DEFAULT, buff, len)))
	{
		log("PKI: ERROR: converting sexp to string");
		delete[] buff;
		return false;
	}

	str line;
	siss iss(str(buff, len));
	if(!std::getline(std::getline(iss, line, ':'), line))
	{
		log("PKI: ERROR: extracting text from sexp: " << str(buff, len));
		delete[] buff;
		return false;
	}

	text = line;
	delete[] buff;

	return true;
}

}} //oastats::pki
