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

#ifndef DECODE_RC_HPP
#define DECODE_RC_HPP

#include <cstdint>
#include <string>

extern "C" {
    #include "OCTET_STRING.h"
    #include "E2SM-RC-ControlHeader.h"
    #include "E2SM-RC-ControlMessage.h"
}

// Standard E2SM-RC RAN Parameter IDs for Control Style 2, Action 6 (Slice-level PRB quota)
// These IDs follow the srsRAN/O-RAN standard implementation
#define RANP_ID_RRM_POLICY_RATIO_LIST       1   // RRM Policy Ratio List (LIST)
#define RANP_ID_RRM_POLICY_RATIO_GROUP      2   // RRM Policy Ratio Group (STRUCTURE) - not explicitly used in messages
#define RANP_ID_RRM_POLICY                  3   // RRM Policy (STRUCTURE)
#define RANP_ID_RRM_POLICY_MEMBER_LIST      5   // RRM Policy Member List (LIST)
#define RANP_ID_RRM_POLICY_MEMBER           6   // RRM Policy Member (STRUCTURE) - not explicitly used in messages
#define RANP_ID_PLMN_IDENTITY               7   // PLMN Identity (OCTET_STRING, 3 bytes)
#define RANP_ID_SNSSAI                      8   // S-NSSAI (STRUCTURE)
#define RANP_ID_SST                         9   // SST (OCTET_STRING, 1 byte)
#define RANP_ID_SD                          10  // SD (OCTET_STRING, 3 bytes)
#define RANP_ID_MIN_PRB_POLICY_RATIO        11  // Min PRB Policy Ratio (INTEGER, 0-100%)
#define RANP_ID_MAX_PRB_POLICY_RATIO        12  // Max PRB Policy Ratio (INTEGER, 0-100%)
#define RANP_ID_DED_PRB_POLICY_RATIO        13  // Dedicated PRB Policy Ratio (INTEGER, 0-100%)

/**
 * Structure to hold the decoded slice SLA policy received from the xApp
 * Updated to use standard E2SM-RC PRB ratios instead of throughput values
 */
typedef struct {
    // Slice identification
    uint8_t plmn_id[3];         // 3-byte PLMN Identity
    bool plmn_id_valid;
    int sst;                    // Slice/Service Type (1 byte)
    bool sst_valid;
    int sd;                     // Slice Differentiator (3 bytes, optional)
    bool sd_valid;

    // Standard E2SM-RC PRB ratios (percentages, 0-100)
    int min_prb_ratio;          // Minimum PRB ratio (ID 11)
    bool min_prb_ratio_valid;
    int max_prb_ratio;          // Maximum PRB ratio (ID 12)
    bool max_prb_ratio_valid;
    int ded_prb_ratio;          // Dedicated PRB ratio (ID 13)
    bool ded_prb_ratio_valid;

    // Legacy throughput fields (kept for backward compatibility, not used in standard E2SM-RC)
    long max_dl_thpt_per_slice;
    bool max_dl_thpt_per_slice_valid;
    long max_ul_thpt_per_slice;
    bool max_ul_thpt_per_slice_valid;
    long max_dl_thpt_per_ue;
    bool max_dl_thpt_per_ue_valid;
    long max_ul_thpt_per_ue;
    bool max_ul_thpt_per_ue_valid;

    // Slice priorities (legacy, not used in standard E2SM-RC)
    int dl_slice_priority;
    bool dl_slice_priority_valid;
    int ul_slice_priority;
    bool ul_slice_priority_valid;
} slice_sla_policy_t;

/**
 * Structure to hold the decoded E2SM-RC Control Header Format 1 fields
 */
typedef struct {
    long ric_style_type;
    long ric_control_action_id;
    long ric_control_decision;      // 0 = accept, 1 = reject, -1 = not present
    // UEID fields can be added here if needed
} control_header_fmt1_t;

/**
 * Initialize a slice_sla_policy_t structure with default/invalid values
 *
 * @param policy Pointer to the policy structure to initialize
 */
void init_slice_sla_policy(slice_sla_policy_t *policy);

/**
 * Decode E2SM-RC Control Header from OCTET STRING
 *
 * @param header_bytes The encoded control header as OCTET_STRING
 * @param header_out Pointer to store the decoded E2SM_RC_ControlHeader structure (caller must free with ASN_STRUCT_FREE)
 * @return 0 on success, -1 on failure
 */
int decode_e2sm_rc_control_header(const OCTET_STRING_t *header_bytes, E2SM_RC_ControlHeader_t **header_out);

/**
 * Decode E2SM-RC Control Message from OCTET STRING
 *
 * @param msg_bytes The encoded control message as OCTET_STRING
 * @param msg_out Pointer to store the decoded E2SM_RC_ControlMessage structure (caller must free with ASN_STRUCT_FREE)
 * @return 0 on success, -1 on failure
 */
int decode_e2sm_rc_control_message(const OCTET_STRING_t *msg_bytes, E2SM_RC_ControlMessage_t **msg_out);

/**
 * Extract Control Header Format 1 fields from decoded header
 *
 * @param header The decoded E2SM_RC_ControlHeader structure
 * @param header_fmt1 Pointer to store the extracted fields
 * @return 0 on success, -1 if not Format 1 or on failure
 */
int extract_control_header_fmt1(const E2SM_RC_ControlHeader_t *header, control_header_fmt1_t *header_fmt1);

/**
 * Extract slice SLA policy from decoded E2SM-RC Control Message Format 1
 * Walks the nested RAN Parameter structure and extracts values by ID
 *
 * @param msg The decoded E2SM_RC_ControlMessage structure
 * @param policy Pointer to store the extracted policy (should be initialized with init_slice_sla_policy first)
 * @return 0 on success, -1 if not Format 1 or on failure
 */
int extract_slice_sla_policy(const E2SM_RC_ControlMessage_t *msg, slice_sla_policy_t *policy);

/**
 * Convert PLMN ID bytes to human-readable MCC-MNC string
 *
 * @param plmn_id 3-byte PLMN Identity
 * @param mcc Output buffer for MCC (at least 4 bytes)
 * @param mnc Output buffer for MNC (at least 4 bytes)
 */
void plmn_id_to_mcc_mnc(const uint8_t plmn_id[3], char *mcc, char *mnc);

/**
 * Log the contents of a slice SLA policy
 *
 * @param policy The policy to log
 */
void log_slice_sla_policy(const slice_sla_policy_t *policy);

#endif // DECODE_RC_HPP
