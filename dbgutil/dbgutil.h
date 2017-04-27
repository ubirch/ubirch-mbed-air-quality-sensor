/*!
 * @file
 * @brief debug helpers.
 *
 * @author Matthias L. Jugel
 * @date 2016-04-09
 *
 * @copyright &copy; 2015 ubirch GmbH (https://ubirch.com)
 *
 * ```
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * ```
 */

#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef NDEBUG
#define PRINTF(...)
#else
#define PRINTF printf
#endif
/*!
 * @brief Dump the buffer in hex and ascii format.
 * @param prefix printed in front of the array lines
 * @param b the byte array
 * @param size the length of the array
 */
void hex_dump(const char *prefix, const uint8_t *b, size_t size);


/*!
 * @brief Dump the buffer in hex format readable by xxd.
 * @param prefix printed in front of the array lines
 * @param b the byte array
 * @param size the length of the array
 */
void dbg_xxd(const char *prefix, const uint8_t *b, size_t size);

//#ifdef NDEBUG
//#  define CIODEBUG(...)
//#  define CIODUMP(...)
//#else
//#  define CIODEBUG(...)  PRINTF(__VA_ARGS__)                  /*!< Debug I/O message (AT commands) */
//#  define CIODUMP(buffer, size) hex_dump("GSM", buffer, size) /*!< Debug and dump a buffer */
//#endif
//
//#ifdef NCSTDEBUG
//#  define CSTDEBUG(...)
//#else
//#  define CSTDEBUG(...)  PRINTF(__VA_ARGS__)                  /*!< Standard debug message (info) */
//#endif
//
//
//
////! textual representation of the registration status
//const char *reg_status[6] = {
//        "NOT SEARCHING",
//        "HOME",
//        "SEARCHING",
//        "DENIED",
//        "UNKNOWN",
//        "ROAMING"
//};
#ifdef __cplusplus
}
#endif

#endif // _UBIRCH_DEBUG_H_
