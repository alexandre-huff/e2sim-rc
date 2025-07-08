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

#include "e2sm_utils.hpp"
#include "logger.h"
#include "utils.hpp"
#include "global_data.hpp"

extern "C" {
    #include "PLMN-Identity.h"
}

NR_CGI_t *e2sm::utils::encode_NR_CGI(const std::string& mcc, const std::string& mnc, const uint32_t gnb_id, const uint16_t pci) {
    LOGGER_TRACE_FUNCTION_IN

    if (gnb_id > ((1 << GNB_ID_LENGTH) - 1)) {
        logger_error("gNB ID value cannot be higher than %d bits to encode NR CGI", GNB_ID_LENGTH);
        return nullptr;
    }

    if (pci > (1 << (36 - GNB_ID_LENGTH) - 1)) {
        logger_error("PCI value cannot be higher than %d bits to encode NR CGI", 36 - GNB_ID_LENGTH);
        return nullptr;
    }

    PLMNIdentity_t *plmnid = common::utils::encodePlmnId(mcc.c_str(), mnc.c_str()); // validates mcc and mnc
    if (!plmnid) {
        logger_error("Unable to encode PLMN Identity for NR CGI");
        return nullptr;
    }

    NR_CGI_t *nr_cgi = (NR_CGI_t *) calloc(1, sizeof(NR_CGI_t));
    nr_cgi->pLMNIdentity = *plmnid;
    if (plmnid) free(plmnid);

    // NCI
    nr_cgi->nRCellIdentity.buf = (uint8_t *) calloc(5, sizeof(uint8_t)); // we need to allocate 40 bits to store 36 bits
    nr_cgi->nRCellIdentity.size = 5;
    nr_cgi->nRCellIdentity.bits_unused = 4; // 40 - 4 = 36 bits
    // Taking into account that GNB_ID Length is 29 bits, we have the following formula: NCI = gnbId * 2^(36-29) + cellid
    // we leave 7 bits for cellid (pci) as long as GNB_ID length is 29 bits
    // uint64_t nci = gnb_id << (36 - GNB_ID_LENGTH) + pci;
    uint64_t nci = (gnb_id << (36 - GNB_ID_LENGTH)) | pci;
    // Source: 3GPP TS 38.413, Sections 9.3.1.6 and 9.3.1.7
    // Source: https://nrcalculator.firebaseapp.com/nrgnbidcalc.html
    // Source: https://www.telecomhall.net/t/what-is-the-formula-for-cell-id-nci-in-5g-nr-networks/12623/2
    // Source: https://nrcalculator.web.app/nrgnbidcalc_tw.html

    nci = nci << nr_cgi->nRCellIdentity.bits_unused;    // we need to put the first byte at the 40th position (36 + 4) in the bit string
    nr_cgi->nRCellIdentity.buf[0] = (nci >> 32) & 0xFF;
    nr_cgi->nRCellIdentity.buf[1] = (nci >> 24) & 0xFF;
    nr_cgi->nRCellIdentity.buf[2] = (nci >> 16) & 0xFF;
    nr_cgi->nRCellIdentity.buf[3] = (nci >> 8) & 0xFF;
    nr_cgi->nRCellIdentity.buf[4] = nci & 0xFF;

    if (LOGGER_LEVEL >= LOGGER_DEBUG) {
        asn_fprint(stderr, &asn_DEF_NR_CGI, nr_cgi);
    }

    LOGGER_TRACE_FUNCTION_OUT

    return nr_cgi;
}

bool e2sm::utils::decode_NR_CGI(const NR_CGI_t *nr_cgi, std::string &mcc, std::string &mnc, uint32_t &gnb_id, uint16_t &pci) {
    LOGGER_TRACE_FUNCTION_IN

    if (!nr_cgi) {
        logger_error("Unable to decode NR CGI, nil?");
        return false;
    }

    // ASN1 has different names for PLMN Identity types in E2AP and E2SM, so we have to typecast
    if (!common::utils::decodePlmnId((PLMN_Identity_t *)&nr_cgi->pLMNIdentity, mcc, mnc)) {
        logger_error("Unable to decode PLMN Identity from NR CGI");
        return false;
    }

    // Source: 3GPP TS 38.413, Sections 9.3.1.6 and 9.3.1.7
    uint64_t nci;   // NR Cell Idendity
    nci = (uint64_t)nr_cgi->nRCellIdentity.buf[0] << 32;
    nci |= (uint64_t)nr_cgi->nRCellIdentity.buf[1] << 24;
    nci |= (uint64_t)nr_cgi->nRCellIdentity.buf[2] << 16;
    nci |= (uint64_t)nr_cgi->nRCellIdentity.buf[3] << 8;
    nci |= (uint64_t)nr_cgi->nRCellIdentity.buf[4];

    // bit rotation based on 3GPP TS 38.413, Sections 9.3.1.6 and 9.3.1.7
    nci = nci >> nr_cgi->nRCellIdentity.bits_unused;    // first, we have to rotate unused bits to the right
    pci = nci & (36 - GNB_ID_LENGTH);                   // extract only the remaining bits from cell id
    gnb_id = nci >> (36 - GNB_ID_LENGTH);

    if (gnb_id > ((1 << GNB_ID_LENGTH) - 1)) {
        logger_error("Unable to decode Cell ID from NR CGI, value cannot be higher than %d bits", GNB_ID_LENGTH);
        return false;
    }

    logger_debug("NR CGI decoded to MCC=%s, MNC=%s, gNB_ID=%u, Cell_ID=%u", mcc.c_str(), mnc.c_str(), gnb_id, pci);

    LOGGER_TRACE_FUNCTION_OUT

    return true;
}
