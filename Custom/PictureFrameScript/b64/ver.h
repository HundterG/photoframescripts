// SPDX-FileCopyrightText: 2021 Comcast Cable Communications Management, LLC
// SPDX-License-Identifier: Apache-2.0

#ifndef _TROWER_BASE64_VERSION_H_
#define _TROWER_BASE64_VERSION_H_ 1

#define TROWER_BASE64_VERSION          "Custom"

#define TROWER_BASE64_VERSION_MAJOR    0
#define TROWER_BASE64_VERSION_MINOR    0
#define TROWER_BASE64_VERSION_PATCH    0

/* This is the numeric version of the trowner-base64 version number, meant for easier
 * parsing and comparisons by programs.  The TROWER_BASE64_VERSION_NUM define
 * will always follow this syntax:
 *
 *      0xAABBCC
 *
 * Where AA, BB, and CC are each major version, minor version, patch in hex
 * using 8 bits each.
 *
 * It should be safe to compare two different versions with the greater number
 * being the newer version.
 */
#define TROWER_BASE64_VERSION_NUM      0

#endif
