/*****************************************************************************
#                                                                            *
# Copyright 2024 Alexandre Huff                                              *
#                                                                            *
# Licensed under the Apache License, Version 2.0 (the "License");            *
# you may not use this file except in compliance with the License.           *
# You may obtain a copy of the License at                                    *
#                                                                            *
#      http://www.apache.org/licenses/LICENSE-2.0                            *
#                                                                            *
# Unless required by applicable law or agreed to in writing, software        *
# distributed under the License is distributed on an "AS IS" BASIS,          *
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
# See the License for the specific language governing permissions and        *
# limitations under the License.                                             *
#                                                                            *
******************************************************************************/

#include "utils.hpp"
#include "logger.h"

/*
    This function checks and encodes a given asn1 type descriptor.
    Returns NULL in case of error.
    It is the caller responsibility to free the returned struct pointer.
*/
OCTET_STRING_t *common::utils::asn1_check_and_encode(const asn_TYPE_descriptor_s *type_to_encode, const void *structure_to_encode) {
    LOGGER_TRACE_FUNCTION_IN

    const int BUF_SIZE = 8192;

    OCTET_STRING_t *encoded_ostr = NULL;

    char error_buf[300] = {0, };
    size_t errlen = 300;
    int ret = asn_check_constraints(type_to_encode, structure_to_encode, error_buf, &errlen);
    if (ret == 0) { // constraints are met
        uint8_t e2smbuffer[BUF_SIZE] = {0, };
        size_t e2smbuffer_size = BUF_SIZE;

        asn_codec_ctx_t *opt_cod = NULL;
        asn_enc_rval_t er = asn_encode_to_buffer(opt_cod,
                                ATS_ALIGNED_BASIC_PER,
                                type_to_encode, structure_to_encode,
                                e2smbuffer, e2smbuffer_size);

        if (er.encoded == -1) {
            logger_error("Failed to encode %s", er.failed_type->name);

        } else if (er.encoded > BUF_SIZE) {
            logger_error("Size of encoded data is greater than buffer size: %ld > %lu", er.encoded, BUF_SIZE);

        } else {
            encoded_ostr = OCTET_STRING_new_fromBuf(type_to_encode, (const char *)e2smbuffer, er.encoded);
            if (encoded_ostr == NULL) {
                logger_error("Unable to allocate allocate new OCTET_STRING for %s", type_to_encode->name);
            }
        }

    } else {
        logger_error("Check constraints failed for %s: error length = %lu, error buf = %s", type_to_encode->name, errlen, error_buf);
    }

    LOGGER_TRACE_FUNCTION_OUT

    return encoded_ostr;
}

/*
    This function decodes and checks a given asn1 type descriptor.
    It is the caller responsibility to free the returned struct pointer in case of success.
*/
bool common::utils::asn1_decode_and_check(const asn_TYPE_descriptor_s *type_to_decode, void **structure_to_decode, const uint8_t *buffer, size_t bufsize) {
    LOGGER_TRACE_FUNCTION_IN

    // asn_dec_rval_t dval = aper_decode_complete(nullptr, type_to_decode, structure_to_decode, buffer, bufsize);
    asn_dec_rval_t dval = asn_decode(nullptr, ATS_ALIGNED_BASIC_PER, type_to_decode, structure_to_decode, buffer, bufsize);
    if (dval.code != RC_OK) {
        logger_error("Failed to decode %s. Length of data = %lu, return code = %d", type_to_decode->name, dval.consumed, dval.code);
        return false;
    }

    logger_debug("%s length of data = %lu, return code = %d", type_to_decode->name, dval.consumed, dval.code);

    char error_buf[300] = {0, };
    size_t errlen = 300;
    int ret = asn_check_constraints(type_to_decode, *structure_to_decode, error_buf, &errlen);

    if (ret != 0) {
        logger_error("Check constraints failed for %s: error length = %lu, error buf = %s", type_to_decode->name, errlen, error_buf);
        ASN_STRUCT_FREE(*type_to_decode, *structure_to_decode);
        return false;
    }

    if (LOGGER_LEVEL >= LOGGER_DEBUG) {
        asn_fprint(stderr, type_to_decode, *structure_to_decode);
    }

    LOGGER_TRACE_FUNCTION_OUT

    return true;
}

// FIXME: remove this in incorrect
// PLMN_Identity_t *common::utils::encodePlmnId_old(const char *mcc, const char *mnc) {
//     LOGGER_TRACE_FUNCTION_IN

//     PLMN_Identity_t *plmn = (PLMN_Identity_t *)calloc(1, sizeof(PLMN_Identity_t));
//     plmn->size = 3;  // the size according to E2AP specification
//     plmn->buf = (uint8_t *)calloc(3, sizeof(uint8_t));

//     char buf = mcc[1];
//     plmn->buf[0] |= (atoi(&buf)) << 4;
//     buf = mcc[0];
//     plmn->buf[0] |= (atoi(&buf));

//     if (strlen((char *)mnc) == 3) {
//         buf = mnc[0];
//         plmn->buf[1] |= (atoi(&buf)) << 4;
//         buf = mcc[2];
//         plmn->buf[1] |= (atoi(&buf));
//         buf = mnc[2];
//         plmn->buf[2] |= (atoi(&buf)) << 4;
//         buf = mnc[1];
//         plmn->buf[2] |= (atoi(&buf));

//     } else {
//         plmn->buf[1] |= 0x0F << 4;
//         buf = mcc[2];
//         plmn->buf[1] |= (atoi(&buf));
//         buf = mnc[1];
//         plmn->buf[2] |= (atoi(&buf)) << 4;
//         buf = mnc[0];
//         plmn->buf[2] |= (atoi(&buf));
//     }

//     logger_debug("PLMN Identity encoded for mcc=%s mnc=%s", mcc, mnc);

//     if (LOGGER_LEVEL >= LOGGER_DEBUG) {
//         xer_fprint(stderr, &asn_DEF_PLMN_Identity, plmn);
//     }

//     LOGGER_TRACE_FUNCTION_OUT

//     return plmn;
// }

PLMN_Identity_t *common::utils::encodePlmnId(const char *mcc, const char *mnc) {
    LOGGER_TRACE_FUNCTION_IN

    PLMN_Identity_t *plmn = (PLMN_Identity_t *)calloc(1, sizeof(PLMN_Identity_t));
    plmn->size = 3;  // the size according to E2AP specification
    plmn->buf = (uint8_t *)calloc(3, sizeof(uint8_t));

    plmn->buf[0] = ((int)(mcc[1])) << 4;
    plmn->buf[0] |= ((int)(mcc[0])) & 0x0F;

    if (strlen((char *)mnc) == 3) {
        plmn->buf[1] = ((int)mnc[0]) << 4;
        plmn->buf[1] |= ((int)mcc[2]) & 0x0F;
        plmn->buf[2] = ((int)mnc[2]) << 4;
        plmn->buf[2] |= ((int)mnc[1]) & 0x0F;

    } else {
        plmn->buf[1] = 0xF0;
        plmn->buf[1] |= ((int)mcc[2]) & 0x0F;
        plmn->buf[2] = ((int)mnc[1]) << 4;
        plmn->buf[2] |= ((int)mnc[0]) & 0x0F;
    }

    logger_debug("PLMN Identity encoded for mcc=%s mnc=%s", mcc, mnc);

    if (LOGGER_LEVEL >= LOGGER_DEBUG) {
        xer_fprint(stderr, &asn_DEF_PLMN_Identity, plmn);
    }

    LOGGER_TRACE_FUNCTION_OUT

    return plmn;
}
