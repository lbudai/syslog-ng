/*
 * Copyright (c) 2002-2016 Balabit
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * As an additional exemption you are allowed to compile & link against the
 * OpenSSL libraries as published by the OpenSSL project. See the file
 * COPYING for details.
 *
 */

#ifndef OPENSSL_SUPPORT_H_INCLUDED
#define OPENSSL_SUPPORT_H_INCLUDED

#include "compat/compat.h"
#include <openssl/ssl.h>

#if !SYSLOG_NG_HAVE_DECL_SSL_CTX_GET0_PARAM
X509_VERIFY_PARAM *SSL_CTX_get0_param(SSL_CTX *ctx);
#endif

#if !SYSLOG_NG_HAVE_DECL_X509_STORE_CTX_GET0_CERT
X509 *X509_STORE_CTX_get0_cert(X509_STORE_CTX *ctx);
#endif

#if !SYSLOG_NG_HAVE_DECL_X509_GET_EXTENSION_FLAGS
#include <stdint.h>
uint32_t X509_get_extension_flags(X509 *x);
#endif

#if SYSLOG_NG_HAVE_DECL_EVP_MD_CTX_RESET
#include <openssl/evp.h>
#define EVP_MD_CTX_cleanup EVP_MD_CTX_reset
#define DECLARE_EVP_MD_CTX(md_ctx) EVP_MD_CTX md_ctx
#else
#define DECLARE_EVP_MD_CTX(md_ctx) EVP_MD_CTX _##md_ctx; EVP_MD_CTX * md_ctx = & _##md_ctx
#endif

#endif

