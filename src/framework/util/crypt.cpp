/*
 * Copyright (c) 2010-2017 OTClient <https://github.com/edubart/otclient>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "crypt.h"
#include <framework/stdext/math.h>
#include <framework/core/logger.h>
#include <framework/core/resourcemanager.h>
#include <framework/platform/platform.h>
#include <framework/core/application.h>

#include <boost/uuid/uuid_generators.hpp>
#include <boost/functional/hash.hpp>
#include <boost/uuid/uuid_io.hpp>

#ifdef USE_GMP

#include "crypt_gmp.h"
CryptGMP g_crypt_algos;

#else

#include "crypt_openssl.h"
CryptOpenSSL g_crypt_algos;

#endif

static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static inline bool is_base64(unsigned char c) { return (isalnum(c) || (c == '+') || (c == '/')); }

Crypt g_crypt;

std::string Crypt::base64Encode(const std::string& decoded_string)
{
    std::string ret;
    int i = 0;
    int j = 0;
    uint8_t char_array_3[3];
    uint8_t char_array_4[4];
    int pos = 0;
    int len = decoded_string.size();

    while(len--) {
        char_array_3[i++] = decoded_string[pos++];
        if(i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for(i = 0; (i <4) ; i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if(i) {
        for(j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for(j = 0; (j < i + 1); j++)
            ret += base64_chars[char_array_4[j]];

        while((i++ < 3))
            ret += '=';
    }

    return ret;
}

std::string Crypt::base64Decode(const std::string& encoded_string)
{
    int len = encoded_string.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    uint8_t char_array_4[4], char_array_3[3];
    std::string ret;

    while(len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
        char_array_4[i++] = encoded_string[in_]; in_++;
        if(i ==4) {
            for(i = 0; i <4; i++)
                char_array_4[i] = base64_chars.find(char_array_4[i]);

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for(i = 0; (i < 3); i++)
                ret += char_array_3[i];
            i = 0;
        }
    }

    if(i) {
        for(j = i; j <4; j++)
            char_array_4[j] = 0;

        for(j = 0; j <4; j++)
            char_array_4[j] = base64_chars.find(char_array_4[j]);

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

        for(j = 0; (j < i - 1); j++)
            ret += char_array_3[j];
    }

    return ret;
}

std::string Crypt::xorCrypt(const std::string& buffer, const std::string& key)
{
    std::string out;
    out.resize(buffer.size());
    size_t i, j=0;
    for(i=0;i<buffer.size();++i) {
        out[i] = buffer[i] ^ key[j++];
        if(j >= key.size())
            j = 0;
    }
    return out;
}

std::string Crypt::genUUID()
{
    boost::uuids::random_generator gen;
    boost::uuids::uuid u = gen();
    return boost::uuids::to_string(u);
}

bool Crypt::setMachineUUID(std::string uuidstr)
{
    if(uuidstr.empty())
        return false;
    uuidstr = _decrypt(uuidstr, false);
    if(uuidstr.length() != 16)
        return false;
    std::copy(uuidstr.begin(), uuidstr.end(), m_machineUUID.begin());
    return true;
}

std::string Crypt::getMachineUUID()
{
    if(m_machineUUID.is_nil()) {
        boost::uuids::random_generator gen;
        m_machineUUID = gen();
    }
    return _encrypt(std::string(m_machineUUID.begin(), m_machineUUID.end()), false);
}

std::string Crypt::getCryptKey(bool useMachineUUID)
{
    boost::hash<boost::uuids::uuid> uuid_hasher;
    boost::uuids::uuid uuid;
    if(useMachineUUID) {
        uuid = m_machineUUID;
    } else {
        boost::uuids::nil_generator nilgen;
        uuid = nilgen();
    }
    boost::uuids::name_generator namegen(uuid);
    boost::uuids::uuid u = namegen(g_app.getCompactName() + g_platform.getCPUName() + g_platform.getOSName() + g_resources.getUserDir());
    std::size_t hash = uuid_hasher(u);
    std::string key;
    key.assign((const char *)&hash, sizeof(hash));
    return key;
}

std::string Crypt::_encrypt(const std::string& decrypted_string, bool useMachineUUID)
{
    std::string tmp = "0000" + decrypted_string;
    uint32_t sum = stdext::adler32((const uint8_t*)decrypted_string.c_str(), decrypted_string.size());
    stdext::writeULE32((uint8_t*)&tmp[0], sum);
    std::string encrypted = base64Encode(xorCrypt(tmp, getCryptKey(useMachineUUID)));
    return encrypted;
}

std::string Crypt::_decrypt(const std::string& encrypted_string, bool useMachineUUID)
{
    std::string decoded = base64Decode(encrypted_string);
    std::string tmp = xorCrypt(decoded, getCryptKey(useMachineUUID));
    if(tmp.length() >= 4) {
        uint32_t readsum = stdext::readULE32((const uint8_t*)tmp.c_str());
        std::string decrypted_string = tmp.substr(4);
        uint32_t sum = stdext::adler32((const uint8_t*)decrypted_string.c_str(), decrypted_string.size());
        if(readsum == sum)
            return decrypted_string;
    }
    return std::string();
}

std::string Crypt::md5Encode(const std::string& decoded_string, bool upperCase)
{
    return g_crypt_algos.md5Encode(decoded_string, upperCase);
}

std::string Crypt::sha1Encode(const std::string& decoded_string, bool upperCase)
{
    return g_crypt_algos.sha1Encode(decoded_string, upperCase);
}

std::string Crypt::sha256Encode(const std::string& decoded_string, bool upperCase)
{
    return g_crypt_algos.sha256Encode(decoded_string, upperCase);
}

std::string Crypt::sha512Encode(const std::string& decoded_string, bool upperCase)
{
    return g_crypt_algos.sha512Encode(decoded_string, upperCase);
}

void Crypt::rsaGenerateKey(int bits, int e)
{
    return g_crypt_algos.rsaGenerateKey(bits, e);
}

void Crypt::rsaSetPublicKey(const std::string& n, const std::string& e)
{
    return g_crypt_algos.rsaSetPublicKey(n, e);
}

void Crypt::rsaSetPrivateKey(const std::string& p, const std::string& q, const std::string& d)
{
    return g_crypt_algos.rsaSetPrivateKey(p, q, d);
}

bool Crypt::rsaCheckKey()
{
    return g_crypt_algos.rsaCheckKey();
}

bool Crypt::rsaEncrypt(unsigned char *msg, int size)
{
    return g_crypt_algos.rsaEncrypt(msg, size);
}

bool Crypt::rsaDecrypt(unsigned char *msg, int size)
{
    return g_crypt_algos.rsaDecrypt(msg, size);
}

int Crypt::rsaGetSize()
{
    return g_crypt_algos.rsaGetSize();
}
