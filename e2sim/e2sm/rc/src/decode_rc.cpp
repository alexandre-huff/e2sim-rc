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

#include "decode_rc.hpp"
#include "logger.h"

#include <cstring>

extern "C" {
    #include "E2SM-RC-ControlHeader-Format1.h"
    #include "E2SM-RC-ControlMessage-Format1.h"
    #include "E2SM-RC-ControlMessage-Format1-Item.h"
    #include "RANParameter-ValueType.h"
    #include "RANParameter-ValueType-Choice-ElementFalse.h"
    #include "RANParameter-ValueType-Choice-Structure.h"
    #include "RANParameter-ValueType-Choice-List.h"
    #include "RANParameter-STRUCTURE.h"
    #include "RANParameter-STRUCTURE-Item.h"
    #include "RANParameter-LIST.h"
    #include "RANParameter-Value.h"
    #include "per_decoder.h"
}

void init_slice_sla_policy(slice_sla_policy_t *policy) {
    if (!policy) return;

    memset(policy->plmn_id, 0, sizeof(policy->plmn_id));
    policy->plmn_id_valid = false;
    policy->sst = 0;
    policy->sst_valid = false;
    policy->sd = 0;
    policy->sd_valid = false;

    policy->max_dl_thpt_per_slice = 0;
    policy->max_dl_thpt_per_slice_valid = false;
    policy->max_ul_thpt_per_slice = 0;
    policy->max_ul_thpt_per_slice_valid = false;
    policy->max_dl_thpt_per_ue = 0;
    policy->max_dl_thpt_per_ue_valid = false;
    policy->max_ul_thpt_per_ue = 0;
    policy->max_ul_thpt_per_ue_valid = false;

    policy->dl_slice_priority = 0;
    policy->dl_slice_priority_valid = false;
    policy->ul_slice_priority = 0;
    policy->ul_slice_priority_valid = false;
}

int decode_e2sm_rc_control_header(const OCTET_STRING_t *header_bytes, E2SM_RC_ControlHeader_t **header_out) {
    if (!header_bytes || !header_out) {
        logger_error("Invalid parameters for decode_e2sm_rc_control_header");
        return -1;
    }

    *header_out = NULL;

    asn_dec_rval_t rval = asn_decode(NULL, ATS_ALIGNED_BASIC_PER,
                                     &asn_DEF_E2SM_RC_ControlHeader,
                                     (void **)header_out,
                                     header_bytes->buf, header_bytes->size);

    if (rval.code != RC_OK) {
        logger_error("Failed to decode E2SM-RC Control Header, rval.code=%d, consumed=%zu bytes",
                     rval.code, rval.consumed);
        if (*header_out) {
            ASN_STRUCT_FREE(asn_DEF_E2SM_RC_ControlHeader, *header_out);
            *header_out = NULL;
        }
        return -1;
    }

    logger_debug("Successfully decoded E2SM-RC Control Header, consumed=%zu bytes", rval.consumed);
    return 0;
}

int decode_e2sm_rc_control_message(const OCTET_STRING_t *msg_bytes, E2SM_RC_ControlMessage_t **msg_out) {
    if (!msg_bytes || !msg_out) {
        logger_error("Invalid parameters for decode_e2sm_rc_control_message");
        return -1;
    }

    logger_debug("Control Message bytes (size=%zu): ", msg_bytes->size);
    for (size_t i = 0; i < msg_bytes->size && i < 64; i++) {
        fprintf(stderr, "%02X ", msg_bytes->buf[i]);
    }
    fprintf(stderr, "\n");

    *msg_out = NULL;

    asn_dec_rval_t rval = asn_decode(NULL, ATS_ALIGNED_BASIC_PER,
                                     &asn_DEF_E2SM_RC_ControlMessage,
                                     (void **)msg_out,
                                     msg_bytes->buf, msg_bytes->size);

    if (rval.code != RC_OK) {
        logger_error("Failed to decode E2SM-RC Control Message, rval.code=%d, consumed=%zu bytes",
                     rval.code, rval.consumed);
        if (*msg_out) {
            ASN_STRUCT_FREE(asn_DEF_E2SM_RC_ControlMessage, *msg_out);
            *msg_out = NULL;
        }
        return -1;
    }

    logger_debug("Successfully decoded E2SM-RC Control Message, consumed=%zu bytes", rval.consumed);
    return 0;
}

int extract_control_header_fmt1(const E2SM_RC_ControlHeader_t *header, control_header_fmt1_t *header_fmt1) {
    if (!header || !header_fmt1) {
        logger_error("Invalid parameters for extract_control_header_fmt1");
        return -1;
    }

    if (header->ric_controlHeader_formats.present != E2SM_RC_ControlHeader__ric_controlHeader_formats_PR_controlHeader_Format1) {
        logger_error("E2SM-RC Control Header is not Format 1 (present=%d)",
                     header->ric_controlHeader_formats.present);
        return -1;
    }

    E2SM_RC_ControlHeader_Format1_t *fmt1 = header->ric_controlHeader_formats.choice.controlHeader_Format1;
    if (!fmt1) {
        logger_error("E2SM-RC Control Header Format 1 is NULL");
        return -1;
    }

    header_fmt1->ric_style_type = fmt1->ric_Style_Type;
    header_fmt1->ric_control_action_id = fmt1->ric_ControlAction_ID;

    if (fmt1->ric_ControlDecision) {
        header_fmt1->ric_control_decision = *fmt1->ric_ControlDecision;
    } else {
        header_fmt1->ric_control_decision = -1;  // Not present
    }

    logger_debug("Control Header Format 1: style_type=%ld, action_id=%ld, decision=%ld",
                 header_fmt1->ric_style_type, header_fmt1->ric_control_action_id,
                 header_fmt1->ric_control_decision);

    return 0;
}

/**
 * Extract an integer value from RANParameter_Value
 */
static bool extract_int_value(const RANParameter_Value_t *value, long *out) {
    if (!value || !out) return false;

    if (value->present == RANParameter_Value_PR_valueInt) {
        *out = value->choice.valueInt;
        return true;
    }
    return false;
}

/**
 * Extract an OCTET STRING value from RANParameter_Value
 */
static bool extract_octet_string_value(const RANParameter_Value_t *value, uint8_t *out, size_t max_size, size_t *out_size) {
    if (!value || !out || !out_size) return false;

    if (value->present == RANParameter_Value_PR_valueOctS) {
        size_t copy_size = value->choice.valueOctS.size;
        if (copy_size > max_size) {
            copy_size = max_size;
        }
        memcpy(out, value->choice.valueOctS.buf, copy_size);
        *out_size = copy_size;
        return true;
    }
    return false;
}

/**
 * Process a leaf RANParameter (ElementFalse) containing a value
 */
static void process_ran_parameter_value(long param_id, const RANParameter_ValueType_Choice_ElementFalse_t *elem_false, slice_sla_policy_t *policy) {
    if (!elem_false || !elem_false->ranParameter_value || !policy) return;

    const RANParameter_Value_t *value = elem_false->ranParameter_value;
    long int_val;
    uint8_t oct_buf[8];
    size_t oct_size;

    switch (param_id) {
        case RANP_ID_PLMN_IDENTITY:
            if (extract_octet_string_value(value, policy->plmn_id, 3, &oct_size) && oct_size == 3) {
                policy->plmn_id_valid = true;
                logger_debug("Extracted PLMN ID: %02X %02X %02X",
                             policy->plmn_id[0], policy->plmn_id[1], policy->plmn_id[2]);
            }
            break;

        case RANP_ID_SST:
            if (extract_octet_string_value(value, oct_buf, sizeof(oct_buf), &oct_size) && oct_size >= 1) {
                policy->sst = oct_buf[0];
                policy->sst_valid = true;
                logger_debug("Extracted SST: %d", policy->sst);
            }
            break;

        case RANP_ID_SD:
            if (extract_octet_string_value(value, oct_buf, sizeof(oct_buf), &oct_size) && oct_size >= 3) {
                // SD is 3 bytes, convert to integer
                policy->sd = (oct_buf[0] << 16) | (oct_buf[1] << 8) | oct_buf[2];
                policy->sd_valid = true;
                logger_debug("Extracted SD: %06X", policy->sd);
            }
            break;

        case RANP_ID_MAX_DL_THPT_PER_SLICE:
            if (extract_int_value(value, &int_val)) {
                policy->max_dl_thpt_per_slice = int_val;
                policy->max_dl_thpt_per_slice_valid = true;
                logger_debug("Extracted maxDlThptPerSlice: %ld", policy->max_dl_thpt_per_slice);
            }
            break;

        case RANP_ID_MAX_UL_THPT_PER_SLICE:
            if (extract_int_value(value, &int_val)) {
                policy->max_ul_thpt_per_slice = int_val;
                policy->max_ul_thpt_per_slice_valid = true;
                logger_debug("Extracted maxUlThptPerSlice: %ld", policy->max_ul_thpt_per_slice);
            }
            break;

        case RANP_ID_MAX_DL_THPT_PER_UE:
            if (extract_int_value(value, &int_val)) {
                policy->max_dl_thpt_per_ue = int_val;
                policy->max_dl_thpt_per_ue_valid = true;
                logger_debug("Extracted maxDlThptPerUe: %ld", policy->max_dl_thpt_per_ue);
            }
            break;

        case RANP_ID_MAX_UL_THPT_PER_UE:
            if (extract_int_value(value, &int_val)) {
                policy->max_ul_thpt_per_ue = int_val;
                policy->max_ul_thpt_per_ue_valid = true;
                logger_debug("Extracted maxUlThptPerUe: %ld", policy->max_ul_thpt_per_ue);
            }
            break;

        case RANP_ID_DL_SLICE_PRIORITY:
            if (extract_int_value(value, &int_val)) {
                policy->dl_slice_priority = (int)int_val;
                policy->dl_slice_priority_valid = true;
                logger_debug("Extracted dlSlicePriority: %d", policy->dl_slice_priority);
            }
            break;

        case RANP_ID_UL_SLICE_PRIORITY:
            if (extract_int_value(value, &int_val)) {
                policy->ul_slice_priority = (int)int_val;
                policy->ul_slice_priority_valid = true;
                logger_debug("Extracted ulSlicePriority: %d", policy->ul_slice_priority);
            }
            break;

        default:
            logger_trace("Unhandled RAN Parameter ID: %ld", param_id);
            break;
    }
}

// Forward declaration for recursive traversal
static void traverse_ran_parameter_structure(const RANParameter_STRUCTURE_t *structure, slice_sla_policy_t *policy);

/**
 * Traverse a RANParameter_ValueType and extract values recursively
 */
static void traverse_ran_parameter_value_type(long param_id, const RANParameter_ValueType_t *value_type, slice_sla_policy_t *policy) {
    if (!value_type || !policy) return;

    switch (value_type->present) {
        case RANParameter_ValueType_PR_ranP_Choice_ElementFalse:
            // Leaf node with actual value
            process_ran_parameter_value(param_id, value_type->choice.ranP_Choice_ElementFalse, policy);
            break;

        case RANParameter_ValueType_PR_ranP_Choice_Structure:
            // Nested structure
            if (value_type->choice.ranP_Choice_Structure &&
                value_type->choice.ranP_Choice_Structure->ranParameter_Structure) {
                traverse_ran_parameter_structure(value_type->choice.ranP_Choice_Structure->ranParameter_Structure, policy);
            }
            break;

        case RANParameter_ValueType_PR_ranP_Choice_List:
            // List of structures
            if (value_type->choice.ranP_Choice_List &&
                value_type->choice.ranP_Choice_List->ranParameter_List) {
                RANParameter_LIST_t *list = value_type->choice.ranP_Choice_List->ranParameter_List;
                int list_count = list->list_of_ranParameter.list.count;
                logger_trace("Processing RANParameter List with %d items", list_count);

                for (int i = 0; i < list_count; i++) {
                    RANParameter_STRUCTURE_t *item = list->list_of_ranParameter.list.array[i];
                    if (item) {
                        traverse_ran_parameter_structure(item, policy);
                    }
                }
            }
            break;

        case RANParameter_ValueType_PR_ranP_Choice_ElementTrue:
            // Element true (not used in this context)
            logger_trace("RANParameter ElementTrue not handled for param_id=%ld", param_id);
            break;

        default:
            logger_trace("Unknown RANParameter ValueType present=%d for param_id=%ld",
                        value_type->present, param_id);
            break;
    }
}

/**
 * Traverse a RANParameter_STRUCTURE and extract values
 */
static void traverse_ran_parameter_structure(const RANParameter_STRUCTURE_t *structure, slice_sla_policy_t *policy) {
    if (!structure || !structure->sequence_of_ranParameters || !policy) return;

    int count = structure->sequence_of_ranParameters->list.count;
    logger_trace("Traversing RANParameter STRUCTURE with %d items", count);

    for (int i = 0; i < count; i++) {
        RANParameter_STRUCTURE_Item_t *item = structure->sequence_of_ranParameters->list.array[i];
        if (!item) continue;

        long param_id = item->ranParameter_ID;
        logger_trace("Processing RANParameter STRUCTURE Item: ID=%ld", param_id);

        if (item->ranParameter_valueType) {
            traverse_ran_parameter_value_type(param_id, item->ranParameter_valueType, policy);
        }
    }
}

int extract_slice_sla_policy(const E2SM_RC_ControlMessage_t *msg, slice_sla_policy_t *policy) {
    if (!msg || !policy) {
        logger_error("Invalid parameters for extract_slice_sla_policy");
        return -1;
    }

    if (msg->ric_controlMessage_formats.present != E2SM_RC_ControlMessage__ric_controlMessage_formats_PR_controlMessage_Format1) {
        logger_error("E2SM-RC Control Message is not Format 1 (present=%d)",
                     msg->ric_controlMessage_formats.present);
        return -1;
    }

    E2SM_RC_ControlMessage_Format1_t *fmt1 = msg->ric_controlMessage_formats.choice.controlMessage_Format1;
    if (!fmt1) {
        logger_error("E2SM-RC Control Message Format 1 is NULL");
        return -1;
    }

    int ranp_count = fmt1->ranP_List.list.count;
    logger_debug("E2SM-RC Control Message Format 1 has %d RAN Parameters", ranp_count);

    for (int i = 0; i < ranp_count; i++) {
        E2SM_RC_ControlMessage_Format1_Item_t *item = fmt1->ranP_List.list.array[i];
        if (!item) continue;

        long param_id = item->ranParameter_ID;
        logger_trace("Top-level RANParameter ID: %ld", param_id);

        traverse_ran_parameter_value_type(param_id, &item->ranParameter_valueType, policy);
    }

    return 0;
}

void plmn_id_to_mcc_mnc(const uint8_t plmn_id[3], char *mcc, char *mnc) {
    if (!mcc || !mnc) return;

    // PLMN ID encoding (3GPP TS 38.413):
    // Byte 0: MCC digit 2 | MCC digit 1
    // Byte 1: MNC digit 3 | MCC digit 3  (MNC digit 3 = 0xF for 2-digit MNC)
    // Byte 2: MNC digit 2 | MNC digit 1

    int mcc1 = plmn_id[0] & 0x0F;
    int mcc2 = (plmn_id[0] >> 4) & 0x0F;
    int mcc3 = plmn_id[1] & 0x0F;

    int mnc1 = plmn_id[2] & 0x0F;
    int mnc2 = (plmn_id[2] >> 4) & 0x0F;
    int mnc3 = (plmn_id[1] >> 4) & 0x0F;

    snprintf(mcc, 4, "%d%d%d", mcc1, mcc2, mcc3);

    if (mnc3 == 0x0F) {
        // 2-digit MNC
        snprintf(mnc, 4, "%d%d", mnc1, mnc2);
    } else {
        // 3-digit MNC
        snprintf(mnc, 4, "%d%d%d", mnc1, mnc2, mnc3);
    }
}

void log_slice_sla_policy(const slice_sla_policy_t *policy) {
    if (!policy) return;

    char mcc[4] = {0};
    char mnc[4] = {0};

    if (policy->plmn_id_valid) {
        plmn_id_to_mcc_mnc(policy->plmn_id, mcc, mnc);
        logger_info("Slice SLA Policy - PLMN: MCC=%s, MNC=%s (raw: %02X %02X %02X)",
                    mcc, mnc, policy->plmn_id[0], policy->plmn_id[1], policy->plmn_id[2]);
    }

    if (policy->sst_valid) {
        logger_info("Slice SLA Policy - SST: %d", policy->sst);
    }

    if (policy->sd_valid) {
        logger_info("Slice SLA Policy - SD: %06X (%d)", policy->sd, policy->sd);
    }

    if (policy->max_dl_thpt_per_slice_valid) {
        logger_info("Slice SLA Policy - Max DL Throughput Per Slice: %ld bps", policy->max_dl_thpt_per_slice);
    }

    if (policy->max_ul_thpt_per_slice_valid) {
        logger_info("Slice SLA Policy - Max UL Throughput Per Slice: %ld bps", policy->max_ul_thpt_per_slice);
    }

    if (policy->max_dl_thpt_per_ue_valid) {
        logger_info("Slice SLA Policy - Max DL Throughput Per UE: %ld bps", policy->max_dl_thpt_per_ue);
    }

    if (policy->max_ul_thpt_per_ue_valid) {
        logger_info("Slice SLA Policy - Max UL Throughput Per UE: %ld bps", policy->max_ul_thpt_per_ue);
    }

    if (policy->dl_slice_priority_valid) {
        logger_info("Slice SLA Policy - DL Slice Priority: %d", policy->dl_slice_priority);
    }

    if (policy->ul_slice_priority_valid) {
        logger_info("Slice SLA Policy - UL Slice Priority: %d", policy->ul_slice_priority);
    }
}
