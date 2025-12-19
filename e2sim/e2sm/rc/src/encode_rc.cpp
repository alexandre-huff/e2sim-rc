/*****************************************************************************
#                                                                            *
# Copyright 2020 AT&T Intellectual Property                                  *
# Copyright 2022 Alexandre Huff                                              *
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

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <unistd.h>
#include <vector>

#include "encode_rc.hpp"
#include "logger.h"

using namespace std;

// Vendor-specific RAN Parameter IDs for cell capacity advertisement
namespace {
    constexpr long PARAM_ID_MAX_DL_CAPACITY_KBPS = 60001;
    constexpr long PARAM_ID_MAX_UL_CAPACITY_KBPS = 60002;
    constexpr long PARAM_ID_TOTAL_PRB_DL = 60003;
    constexpr long PARAM_ID_TOTAL_PRB_UL = 60004;
    constexpr long PARAM_ID_BANDWIDTH_MHZ = 60005;
    constexpr long PARAM_ID_NUM_PRBS = 60006;
    constexpr long PARAM_ID_SUBCARRIER_SPACING = 60007;
}

void add_capacity_ran_parameter(RANFunctionDefinition_Control_Action_Item_t *ctrl_act_item,
                                 long param_id, const char *param_name) {
    if (!ctrl_act_item || !param_name) {
        logger_error("Invalid arguments to add_capacity_ran_parameter");
        return;
    }

    // Allocate RAN Parameter list if not exists
    if (!ctrl_act_item->ran_ControlActionParameters_List) {
        ctrl_act_item->ran_ControlActionParameters_List =
            (RANFunctionDefinition_Control_Action_Item::RANFunctionDefinition_Control_Action_Item__ran_ControlActionParameters_List *)
            calloc(1, sizeof(RANFunctionDefinition_Control_Action_Item::RANFunctionDefinition_Control_Action_Item__ran_ControlActionParameters_List));
    }

    // Create RAN Parameter Item
    ControlAction_RANParameter_Item_t *param_item =
        (ControlAction_RANParameter_Item_t *) calloc(1, sizeof(ControlAction_RANParameter_Item_t));

    param_item->ranParameter_ID = param_id;

    size_t name_len = strlen(param_name);
    param_item->ranParameter_name.buf = (uint8_t *) calloc(name_len, sizeof(uint8_t));
    memcpy(param_item->ranParameter_name.buf, param_name, name_len);
    param_item->ranParameter_name.size = name_len;

    // Add to list
    ASN_SEQUENCE_ADD(&ctrl_act_item->ran_ControlActionParameters_List->list, param_item);

    logger_debug("Added RAN Parameter ID %ld (%s) to Control Action", param_id, param_name);
}

void encode_rc_function_definition(E2SM_RC_RANFunctionDefinition_t* ranfunc_def) {
    int ret;    // Temporary return value
    size_t len; // Temporary length to avoid unnecessary strlen calls

    logger_trace("in %s function", __func__);

    uint8_t *buf = (uint8_t*)"ORAN-E2SM-RC";	// short name
    uint8_t *buf2 = (uint8_t*)"RAN Control";	// ran function description
    uint8_t *buf3 = (uint8_t*)"1.3.6.1.4.1.53148.1.1.2.3";	// OID

    long *inst = (long *) malloc(sizeof(long));     // ran function instance (optional)
    *inst = 1;

    len = strlen((char *) buf);
    ranfunc_def->ranFunction_Name.ranFunction_ShortName.size = len;
    ranfunc_def->ranFunction_Name.ranFunction_ShortName.buf = (uint8_t *) calloc(len, sizeof(uint8_t));
    memcpy(ranfunc_def->ranFunction_Name.ranFunction_ShortName.buf, buf, len);

    len = strlen((char *) buf2);
    ranfunc_def->ranFunction_Name.ranFunction_Description.buf = (uint8_t *)calloc(len, sizeof(uint8_t));
    memcpy(ranfunc_def->ranFunction_Name.ranFunction_Description.buf, buf2, len);
    ranfunc_def->ranFunction_Name.ranFunction_Description.size = len;

    len = strlen((char *) buf3);
    ranfunc_def->ranFunction_Name.ranFunction_E2SM_OID.buf = (uint8_t *) calloc(len, sizeof(uint8_t));
    memcpy(ranfunc_def->ranFunction_Name.ranFunction_E2SM_OID.buf, buf3, len);
    ranfunc_def->ranFunction_Name.ranFunction_E2SM_OID.size = len;

    ranfunc_def->ranFunction_Name.ranFunction_Instance = inst;

    logger_trace("ranFunction_Name set up");

    ranfunc_def->ranFunctionDefinition_EventTrigger =
            (RANFunctionDefinition_EventTrigger_t *) calloc(1, sizeof(RANFunctionDefinition_EventTrigger_t));
    RANFunctionDefinition_EventTrigger_Style_Item_t *style_item =
            (RANFunctionDefinition_EventTrigger_Style_Item_t *) calloc(1, sizeof(RANFunctionDefinition_EventTrigger_Style_Item_t));
    style_item->ric_EventTriggerStyle_Type = 2; // call process breakpoint
    uint8_t *buf4 = (uint8_t *) "Call Process Breakpoint";
    len = strlen((char *) buf4);
    style_item->ric_EventTriggerStyle_Name.buf = (uint8_t *) calloc(len, sizeof(uint8_t));
    memcpy(style_item->ric_EventTriggerStyle_Name.buf, buf4, len);
    style_item->ric_EventTriggerStyle_Name.size = len;
    style_item->ric_EventTriggerFormat_Type = 2;
    ASN_SEQUENCE_ADD(&ranfunc_def->ranFunctionDefinition_EventTrigger->ric_EventTriggerStyle_List.list, style_item);
    logger_trace("ranFunction_EventTrigger set up");

    ranfunc_def->ranFunctionDefinition_Insert =
            (RANFunctionDefinition_Insert_t *) calloc(1, sizeof(RANFunctionDefinition_Insert_t));
    RANFunctionDefinition_Insert_Item_t *insert_item =
            (RANFunctionDefinition_Insert_Item_t *) calloc(1, sizeof(RANFunctionDefinition_Insert_Item_t));

    insert_item->ric_InsertStyle_Type = 4;
    uint8_t *insert_name = (uint8_t *) "Radio Access Control Request";
    len = strlen((char *) insert_name);
    insert_item->ric_InsertStyle_Name.buf = (uint8_t *) calloc(len, sizeof(uint8_t));
    memcpy(insert_item->ric_InsertStyle_Name.buf, insert_name, len);
    insert_item->ric_InsertStyle_Name.size = len;

    insert_item->ric_SupportedEventTriggerStyle_Type = 2;
    insert_item->ric_ActionDefinitionFormat_Type = 3;
    insert_item->ric_IndicationHeaderFormat_Type = 2;
    insert_item->ric_IndicationMessageFormat_Type = 5;
    insert_item->ric_CallProcessIDFormat_Type = 1;

    ASN_SEQUENCE_ADD(&ranfunc_def->ranFunctionDefinition_Insert->ric_InsertStyle_List.list, insert_item);
    logger_trace("ranFunction_Definition_Insert set up");

    ranfunc_def->ranFunctionDefinition_Control =
            (RANFunctionDefinition_Control_t *) calloc(1, sizeof(RANFunctionDefinition_Control_t));
    RANFunctionDefinition_Control_Item_t *ctrl_item =
            (RANFunctionDefinition_Control_Item_t *) calloc(1, sizeof(RANFunctionDefinition_Control_Item_t));
    ctrl_item->ric_ControlStyle_Type = 2;  // Radio Resource Allocation Control (standard E2SM-RC)
    uint8_t *ctrl_name = (uint8_t *) "Radio Resource Allocation Control";
    len = strlen((char *) ctrl_name);
    ctrl_item->ric_ControlStyle_Name.buf = (uint8_t *) calloc(len, sizeof(uint8_t));
    memcpy(ctrl_item->ric_ControlStyle_Name.buf, ctrl_name, len);
    ctrl_item->ric_ControlStyle_Name.size = len;

    ctrl_item->ric_ControlHeaderFormat_Type = 1;
    ctrl_item->ric_ControlMessageFormat_Type = 1;
    ctrl_item->ric_CallProcessIDFormat_Type = (RIC_Format_Type_t *) calloc(1, sizeof(RIC_Format_Type_t));
    *ctrl_item->ric_CallProcessIDFormat_Type = 1;

    // Control Action List - defines supported control actions for this style
    ctrl_item->ric_ControlAction_List =
            (RANFunctionDefinition_Control_Item::RANFunctionDefinition_Control_Item__ric_ControlAction_List *)
            calloc(1, sizeof(RANFunctionDefinition_Control_Item::RANFunctionDefinition_Control_Item__ric_ControlAction_List));

    RANFunctionDefinition_Control_Action_Item_t *ctrl_act_item =
            (RANFunctionDefinition_Control_Action_Item_t *) calloc(1, sizeof(RANFunctionDefinition_Control_Action_Item_t));
    ctrl_act_item->ric_ControlAction_ID = 6;
    uint8_t *ctrl_act_name = (uint8_t *) "Slice-level PRB quota";
    len = strlen((char *) ctrl_act_name);
    ctrl_act_item->ric_ControlAction_Name.buf = (uint8_t *) calloc(len, sizeof(uint8_t));
    memcpy(ctrl_act_item->ric_ControlAction_Name.buf, ctrl_act_name, len);
    ctrl_act_item->ric_ControlAction_Name.size = len;

    // Add vendor-specific capacity RAN Parameters (60001-60007)
    // These advertise the E2 Node's cell capability parameters to xApps
    add_capacity_ran_parameter(ctrl_act_item, PARAM_ID_MAX_DL_CAPACITY_KBPS, "maxDlCapacityKbps");
    add_capacity_ran_parameter(ctrl_act_item, PARAM_ID_MAX_UL_CAPACITY_KBPS, "maxUlCapacityKbps");
    add_capacity_ran_parameter(ctrl_act_item, PARAM_ID_TOTAL_PRB_DL, "totalPrbDl");
    add_capacity_ran_parameter(ctrl_act_item, PARAM_ID_TOTAL_PRB_UL, "totalPrbUl");
    add_capacity_ran_parameter(ctrl_act_item, PARAM_ID_BANDWIDTH_MHZ, "bandwidthMhz");
    add_capacity_ran_parameter(ctrl_act_item, PARAM_ID_NUM_PRBS, "numPrbs");
    add_capacity_ran_parameter(ctrl_act_item, PARAM_ID_SUBCARRIER_SPACING, "subcarrierSpacing");

    logger_info("Added vendor-specific capacity RAN Parameters (60001-60007) to RAN Function Definition");

    ASN_SEQUENCE_ADD(&ctrl_item->ric_ControlAction_List->list, ctrl_act_item);

    ASN_SEQUENCE_ADD(&ranfunc_def->ranFunctionDefinition_Control->ric_ControlStyle_List.list, ctrl_item);
    logger_trace("ranFunction_Definition_Control set up");

    if(LOGGER_LEVEL >= LOGGER_DEBUG) {
        xer_fprint(stderr, &asn_DEF_E2SM_RC_RANFunctionDefinition, ranfunc_def);
    }

    logger_trace("end of %s", __func__);
}

void encode_rc_indication_message(E2SM_RC_IndicationMessage_t *ind_msg, PLMNIdentity_t *plmn_id, BIT_STRING_t *gnb_id) {
    logger_trace("in %s function", __func__);

    char error_buf[300] = {0, };
    size_t errlen = 0;
    int ret;

    ASN_STRUCT_RESET(asn_DEF_E2SM_RC_IndicationMessage, ind_msg);

    ind_msg->ric_indicationMessage_formats.present = E2SM_RC_IndicationMessage__ric_indicationMessage_formats_PR_indicationMessage_Format5;
    E2SM_RC_IndicationMessage_Format5_t *indicationMessage_Format5 = (E2SM_RC_IndicationMessage_Format5_t *) calloc(1, sizeof(E2SM_RC_IndicationMessage_Format5_t));
    ind_msg->ric_indicationMessage_formats.choice.indicationMessage_Format5 = indicationMessage_Format5;

    E2SM_RC_IndicationMessage_Format5_Item_t *format_item =
            (E2SM_RC_IndicationMessage_Format5_Item_t *) calloc(1, sizeof(E2SM_RC_IndicationMessage_Format5_Item_t));
    ASN_SEQUENCE_ADD(&indicationMessage_Format5->ranP_Requested_List.list, format_item);

    format_item->ranParameter_ID = 1; // Primary Cell ID as in E2SM-RC v01.02 section 8.4.5.1
    format_item->ranParameter_valueType.present = RANParameter_ValueType_PR_ranP_Choice_Structure;
    format_item->ranParameter_valueType.choice.ranP_Choice_Structure =
            (RANParameter_ValueType_Choice_Structure_t *) calloc(1, sizeof(RANParameter_ValueType_Choice_Structure_t));

    RANParameter_STRUCTURE_t *ranp_struct_item1 =
            (RANParameter_STRUCTURE_t *) calloc(1, sizeof(RANParameter_STRUCTURE_t));
    format_item->ranParameter_valueType.choice.ranP_Choice_Structure->ranParameter_Structure = ranp_struct_item1;

    ranp_struct_item1->sequence_of_ranParameters = (struct RANParameter_STRUCTURE::RANParameter_STRUCTURE__sequence_of_ranParameters *)
                                    calloc(1, sizeof(struct RANParameter_STRUCTURE::RANParameter_STRUCTURE__sequence_of_ranParameters));

    RANParameter_STRUCTURE_Item_t *ranp_struct_item2 = (RANParameter_STRUCTURE_Item_t *) calloc(1, sizeof(RANParameter_STRUCTURE_Item_t));
    ASN_SEQUENCE_ADD(&ranp_struct_item1->sequence_of_ranParameters->list, ranp_struct_item2);

    ranp_struct_item2->ranParameter_ID = 2; // CHOICE Primary Cell as in E2SM-RC v01.02 section 8.4.5.1
    ranp_struct_item2->ranParameter_valueType = (RANParameter_ValueType_t *) calloc(1, sizeof(RANParameter_ValueType_t));
    ranp_struct_item2->ranParameter_valueType->present = RANParameter_ValueType_PR_ranP_Choice_Structure;
    ranp_struct_item2->ranParameter_valueType->choice.ranP_Choice_Structure =
            (RANParameter_ValueType_Choice_Structure_t *) calloc(1, sizeof(RANParameter_ValueType_Choice_Structure_t));

    RANParameter_STRUCTURE_t *ranp_struct2 = (RANParameter_STRUCTURE_t *) calloc(1, sizeof(RANParameter_STRUCTURE_t));
    ranp_struct_item2->ranParameter_valueType->choice.ranP_Choice_Structure->ranParameter_Structure = ranp_struct2;

    ranp_struct2->sequence_of_ranParameters =
            (struct RANParameter_STRUCTURE::RANParameter_STRUCTURE__sequence_of_ranParameters *) calloc(1, sizeof(struct RANParameter_STRUCTURE::RANParameter_STRUCTURE__sequence_of_ranParameters));

    RANParameter_STRUCTURE_Item *ranp_struct_item3 =
            (RANParameter_STRUCTURE_Item *) calloc(1, sizeof(RANParameter_STRUCTURE_Item));
    ASN_SEQUENCE_ADD(&ranp_struct2->sequence_of_ranParameters->list, ranp_struct_item3);

    ranp_struct_item3->ranParameter_ID = 3; // NR Cell as in E2SM-RC v01.02 section 8.4.5.1
    ranp_struct_item3->ranParameter_valueType = (RANParameter_ValueType_t *) calloc(1, sizeof(RANParameter_ValueType_t));
    ranp_struct_item3->ranParameter_valueType->present = RANParameter_ValueType_PR_ranP_Choice_Structure;
    ranp_struct_item3->ranParameter_valueType->choice.ranP_Choice_Structure =
            (RANParameter_ValueType_Choice_Structure_t *) calloc(1, sizeof(RANParameter_ValueType_Choice_Structure_t));

    RANParameter_STRUCTURE_t *ranp_struct3 = (RANParameter_STRUCTURE_t *) calloc(1, sizeof(RANParameter_STRUCTURE_t));
    ranp_struct_item3->ranParameter_valueType->choice.ranP_Choice_Structure->ranParameter_Structure = ranp_struct3;

    ranp_struct3->sequence_of_ranParameters =
            (struct RANParameter_STRUCTURE::RANParameter_STRUCTURE__sequence_of_ranParameters *) calloc(1, sizeof(struct RANParameter_STRUCTURE::RANParameter_STRUCTURE__sequence_of_ranParameters));

    RANParameter_STRUCTURE_Item_t *ranp_struct_item4 = (RANParameter_STRUCTURE_Item_t *) calloc(1, sizeof(RANParameter_STRUCTURE_Item_t));
    ASN_SEQUENCE_ADD(&ranp_struct3->sequence_of_ranParameters->list, ranp_struct_item4);

    ranp_struct_item4->ranParameter_ID = 4; // NR CGI as in E2SM-RC v01.02 section 8.4.5.1
    ranp_struct_item4->ranParameter_valueType = (RANParameter_ValueType_t *) calloc(1, sizeof(RANParameter_ValueType_t));
    ranp_struct_item4->ranParameter_valueType->choice.ranP_Choice_ElementFalse =
            (RANParameter_ValueType_Choice_ElementFalse_t *) calloc(1, sizeof(RANParameter_ValueType_Choice_ElementFalse_t));

    ranp_struct_item4->ranParameter_valueType->present = RANParameter_ValueType_PR_ranP_Choice_ElementFalse;

    ranp_struct_item4->ranParameter_valueType->choice.ranP_Choice_ElementFalse->ranParameter_value =
            (RANParameter_Value_t *) calloc(1, sizeof(RANParameter_Value_t));

    ranp_struct_item4->ranParameter_valueType->choice.ranP_Choice_ElementFalse->ranParameter_value->present = RANParameter_Value_PR_valueOctS;

    NR_CGI_t *nr_cgi = (NR_CGI_t *) calloc(1, sizeof(NR_CGI_t));

    nr_cgi->pLMNIdentity = *plmn_id;    // Is this as same as the plmn id from Global gNodeB IE? or from a given UE?
    if(plmn_id) free(plmn_id);

    if(gnb_id == NULL) {
        logger_fatal("gnb_id must have a value. nil?");
        exit(1);
    }

    nr_cgi->nRCellIdentity.buf = (uint8_t*)calloc(1,5); // required to have room for 36 bits
    if(nr_cgi->nRCellIdentity.buf) {
        // currently we use a dummy value for cell id, should we get this from e2sim base class?
        uint8_t cellid = 127; // for now we leave 7 bits to identity cells on each gNodeB, so uint8_t is enough

        nr_cgi->nRCellIdentity.size = 5;
        nr_cgi->nRCellIdentity.bits_unused = 4;   // 40-36
        assert(nr_cgi->nRCellIdentity.size >= gnb_id->size);
        memcpy(nr_cgi->nRCellIdentity.buf, gnb_id->buf, gnb_id->size); // copied 32 bytes into a 40 bytes variable (we need only 29)
        nr_cgi->nRCellIdentity.buf[3] |= ((cellid & 0X0070) >> 4);       // we get only the 3 most significant of 7 bits
        nr_cgi->nRCellIdentity.buf[4] = ((cellid & 0X000F) << 4) ;       // we get only the 4 least significant bits of 7 bits
    }
    ASN_STRUCT_FREE(asn_DEF_BIT_STRING, gnb_id);

    if(LOGGER_LEVEL >= LOGGER_DEBUG) {
        xer_fprint(stdout, &asn_DEF_NR_CGI, nr_cgi);
    }

    memset(error_buf, 0, sizeof(error_buf));    // ensuring it is clean
    errlen = 0;

    logger_trace("about to check constraints of NR_CGI");
    ret = asn_check_constraints(&asn_DEF_NR_CGI, nr_cgi, error_buf, &errlen);
    if(ret != 0) {
        logger_error("NR_CGI check constraints failed. error length = %lu, error buf = %s", errlen, error_buf);
    }

    logger_trace("NR_CGI set up");

    asn_codec_ctx_t *opt_cod;

    uint8_t nr_cgi_buffer[8192] = {0, };
    size_t nr_cgi_buffer_size = 8192;

    asn_enc_rval_t er =
        asn_encode_to_buffer(opt_cod,
                ATS_ALIGNED_BASIC_PER,
                &asn_DEF_NR_CGI,
                nr_cgi, nr_cgi_buffer, nr_cgi_buffer_size);

    logger_debug("er encded is %ld", er.encoded);
    logger_trace("after encoding NR_CGI");

    ASN_STRUCT_FREE(asn_DEF_NR_CGI, nr_cgi);

    OCTET_STRING_t *ostr = &ranp_struct_item4->ranParameter_valueType->choice.ranP_Choice_ElementFalse->ranParameter_value->choice.valueOctS;
    OCTET_STRING_fromBuf(ostr, (char *) nr_cgi_buffer, er.encoded);

//     fprintf(stderr, "here is the NR_CGI ostr: %s\n", ostr->buf);
//     fprintf(stderr, "after printing NR_CGI ostr\n");

    // asn_fprint(stderr, &asn_DEF_E2SM_RC_IndicationMessage_Format5_Item, format_item);

    // memset(error_buf, 0, sizeof(error_buf));    // ensuring it is clean
    // errlen = 0;
    // fprintf(stderr, "INFO %s:%d - about to check constraints of E2SM_RC_IndicationMessage_Format5_Item\n", __FILE__, __LINE__);
    // ret = asn_check_constraints(&asn_DEF_E2SM_RC_IndicationMessage_Format5_Item, format_item, error_buf, &errlen);
    // printf("error length %lu\n", errlen);
    // printf("error buf %s\n", error_buf);
    // assert(ret == 0);

    // memset(error_buf, 0, sizeof(error_buf));    // ensuring it is clean
    // errlen = 0;
    // fprintf(stderr, "INFO %s:%d - about to check constraints of E2SM_RC_IndicationMessage\n", __FILE__, __LINE__);
    // asn_check_constraints(&asn_DEF_E2SM_RC_IndicationMessage, ind_msg, error_buf, &errlen);
    // printf("error length %lu\n", errlen);
    // printf("error buf %s\n", error_buf);
    // assert(ret == 0);

    // asn_fprint(stderr, &asn_DEF_E2SM_RC_IndicationMessage, ind_msg);

    logger_trace("E2SM_RC_IndicationMessage set up");

    if(LOGGER_LEVEL >= LOGGER_DEBUG) {
        xer_fprint(stderr, &asn_DEF_E2SM_RC_IndicationMessage, ind_msg);
    }

    logger_trace("end of %s", __func__);
}

void encode_rc_indication_header(E2SM_RC_IndicationHeader_t *ind_header, PLMNIdentity_t *plmn_id) {
    logger_trace("in %s function", __func__);

    ind_header->ric_indicationHeader_formats.choice.indicationHeader_Format2 =
            (E2SM_RC_IndicationHeader_Format2_t *) calloc(1, sizeof(E2SM_RC_IndicationHeader_Format2_t));
    ind_header->ric_indicationHeader_formats.present =
            E2SM_RC_IndicationHeader__ric_indicationHeader_formats_PR_indicationHeader_Format2;
    ind_header->ric_indicationHeader_formats.choice.indicationHeader_Format2->ric_InsertStyle_Type = 4;
    ind_header->ric_indicationHeader_formats.choice.indicationHeader_Format2->ric_InsertIndication_ID = 1;

    UEID_GNB_t *ueid_gnb = (UEID_GNB_t *) calloc(1, sizeof(UEID_GNB_t));
    ASN_STRUCT_RESET(asn_DEF_UEID_GNB, ueid_gnb);
    ind_header->ric_indicationHeader_formats.choice.indicationHeader_Format2->ueID.choice.gNB_UEID = ueid_gnb;
    ind_header->ric_indicationHeader_formats.choice.indicationHeader_Format2->ueID.present = UEID_PR_gNB_UEID;
    // an integer between 0..2^40-1, but we only alloc 1 byte to store values between 0..255
    ueid_gnb->amf_UE_NGAP_ID.buf = (uint8_t *) calloc(1, sizeof(uint8_t));
    ueid_gnb->amf_UE_NGAP_ID.buf[0] = (uint8_t) 1;
    ueid_gnb->amf_UE_NGAP_ID.size = sizeof(uint8_t);

    ueid_gnb->guami.pLMNIdentity = *plmn_id;    // Is this as same as the plmn id from Global gNodeB IE? or from a given UE?
    if (plmn_id) free(plmn_id);

    ueid_gnb->guami.aMFRegionID.buf = (uint8_t *) calloc(1, sizeof(uint8_t)); // (8 bits)
    ueid_gnb->guami.aMFRegionID.buf[0] = (uint8_t) 128; // this is a dummy value
    ueid_gnb->guami.aMFRegionID.size = 1;
    ueid_gnb->guami.aMFRegionID.bits_unused = 0;

    ueid_gnb->guami.aMFSetID.buf = (uint8_t *) calloc(2, sizeof(uint8_t)); // (10 bits)
    uint16_t v = (uint16_t) 4; // this is a dummy vale (uint16_t is required to have room for 10 bits)
    v = v << 6; // we are only interested in 10 bits, so rotate them to the correct place
    ueid_gnb->guami.aMFSetID.buf[0] = (v >> 8); // only interested in the most significant bits (& 0x00ff only required for signed)
    ueid_gnb->guami.aMFSetID.buf[1] = v & 0x00ff; // we are only interested in the least significant bits
    ueid_gnb->guami.aMFSetID.size = 2;
    ueid_gnb->guami.aMFSetID.bits_unused = 6;

    ueid_gnb->guami.aMFPointer.buf = (uint8_t *) calloc(1, sizeof(uint8_t)); // (6 bits)
    ueid_gnb->guami.aMFPointer.buf[0] = (uint8_t) 1 << 2; // this is a dummy value
    ueid_gnb->guami.aMFPointer.size = 1;
    ueid_gnb->guami.aMFPointer.bits_unused = 2;

    char error_buf[300] = {0, };
    size_t errlen = 0;

    logger_trace("about to check constraints of E2SM_RC_IndicationHeader");
    int ret = asn_check_constraints(&asn_DEF_E2SM_RC_IndicationHeader, ind_header, error_buf, &errlen);
    if(ret != 0) {
        printf("E2SM_RC_IndicationHeader check constraints failed. error length = %lu, error buf = %s", errlen, error_buf);
    }

    logger_trace("E2SM_RC_IndicationHeader set up");

    if(LOGGER_LEVEL >= LOGGER_DEBUG) {
        xer_fprint(stderr, &asn_DEF_E2SM_RC_IndicationHeader, ind_header);
    }

    logger_trace("end of %s", __func__);
}

// Include for E2SM-RC Query Outcome encoding
extern "C" {
    #include "E2SM-RC-QueryOutcome.h"
    #include "E2SM-RC-QueryOutcome-Format1.h"
    #include "E2SM-RC-QueryOutcome-Format1-ItemCell.h"
    #include "RANParameter-ValueType.h"
    #include "RANParameter-Value.h"
    #include "RANParameter-STRUCTURE.h"
    #include "RANParameter-STRUCTURE-Item.h"
    #include "RANParameter-Testing-Item.h"
    #include "RANParameter-Testing-Item-Choice-ElementFalse.h"
}

#include "e2sim_rc.hpp"  // For node_capacity_t

/**
 * Helper function to add a RAN Parameter with INTEGER value to the Query Outcome
 */
static void add_ran_parameter_int(E2SM_RC_QueryOutcome_Format1_ItemCell_t *cell_item,
                                   long param_id, long value) {
    RANParameter_Testing_Item_t *param = (RANParameter_Testing_Item_t *) calloc(1, sizeof(RANParameter_Testing_Item_t));
    if (!param) return;

    param->ranParameter_ID = param_id;

    // Set value type to Element (elementary value)
    param->ranParameter_Type.present = RANParameter_Testing_Item__ranParameter_Type_PR_ranP_Choice_ElementFalse;
    param->ranParameter_Type.choice.ranP_Choice_ElementFalse =
        (RANParameter_Testing_Item_Choice_ElementFalse_t *) calloc(1, sizeof(RANParameter_Testing_Item_Choice_ElementFalse_t));

    if (param->ranParameter_Type.choice.ranP_Choice_ElementFalse) {
        param->ranParameter_Type.choice.ranP_Choice_ElementFalse->ranParameter_Value =
            (RANParameter_Value_t *) calloc(1, sizeof(RANParameter_Value_t));

        if (param->ranParameter_Type.choice.ranP_Choice_ElementFalse->ranParameter_Value) {
            param->ranParameter_Type.choice.ranP_Choice_ElementFalse->ranParameter_Value->present = RANParameter_Value_PR_valueInt;
            param->ranParameter_Type.choice.ranP_Choice_ElementFalse->ranParameter_Value->choice.valueInt = value;
        }
    }

    ASN_SEQUENCE_ADD(&cell_item->ranP_List.list, param);
}

int encode_e2sm_rc_query_outcome_fmt1(OCTET_STRING_t *outcome_ostr, node_capacity_t *capacity,
                                       PLMNIdentity_t *plmn_id, BIT_STRING_t *gnb_id) {
    logger_trace("in %s function", __func__);

    if (!outcome_ostr || !capacity) {
        logger_error("Invalid arguments to encode_e2sm_rc_query_outcome_fmt1");
        return -1;
    }

    E2SM_RC_QueryOutcome_t *outcome = (E2SM_RC_QueryOutcome_t *) calloc(1, sizeof(E2SM_RC_QueryOutcome_t));
    if (!outcome) {
        logger_error("Failed to allocate E2SM_RC_QueryOutcome");
        return -1;
    }

    // Set to Format 1
    outcome->ric_queryOutcome_formats.present = E2SM_RC_QueryOutcome__ric_queryOutcome_formats_PR_queryOutcome_Format1;
    outcome->ric_queryOutcome_formats.choice.queryOutcome_Format1 =
        (E2SM_RC_QueryOutcome_Format1_t *) calloc(1, sizeof(E2SM_RC_QueryOutcome_Format1_t));

    if (!outcome->ric_queryOutcome_formats.choice.queryOutcome_Format1) {
        logger_error("Failed to allocate E2SM_RC_QueryOutcome_Format1");
        ASN_STRUCT_FREE(asn_DEF_E2SM_RC_QueryOutcome, outcome);
        return -1;
    }

    E2SM_RC_QueryOutcome_Format1_t *fmt1 = outcome->ric_queryOutcome_formats.choice.queryOutcome_Format1;

    // Create a cell item with NR-CGI and RAN parameters
    E2SM_RC_QueryOutcome_Format1_ItemCell_t *cell_item =
        (E2SM_RC_QueryOutcome_Format1_ItemCell_t *) calloc(1, sizeof(E2SM_RC_QueryOutcome_Format1_ItemCell_t));

    if (!cell_item) {
        logger_error("Failed to allocate E2SM_RC_QueryOutcome_Format1_ItemCell");
        ASN_STRUCT_FREE(asn_DEF_E2SM_RC_QueryOutcome, outcome);
        return -1;
    }

    // Set NR-CGI (NR Cell Global Identity) - new API uses pointers
    cell_item->cellGlobal_ID.present = CGI_PR_nR_CGI;
    cell_item->cellGlobal_ID.choice.nR_CGI = (struct NR_CGI *) calloc(1, sizeof(struct NR_CGI));

    if (!cell_item->cellGlobal_ID.choice.nR_CGI) {
        logger_error("Failed to allocate NR_CGI");
        ASN_STRUCT_FREE(asn_DEF_E2SM_RC_QueryOutcome, outcome);
        free(cell_item);
        return -1;
    }

    struct NR_CGI *nr_cgi = cell_item->cellGlobal_ID.choice.nR_CGI;

    if (plmn_id && plmn_id->buf) {
        nr_cgi->pLMNIdentity.buf = (uint8_t *) calloc(plmn_id->size, sizeof(uint8_t));
        memcpy(nr_cgi->pLMNIdentity.buf, plmn_id->buf, plmn_id->size);
        nr_cgi->pLMNIdentity.size = plmn_id->size;
    } else {
        // Default PLMN (001-01)
        nr_cgi->pLMNIdentity.buf = (uint8_t *) calloc(3, sizeof(uint8_t));
        nr_cgi->pLMNIdentity.buf[0] = 0x00;
        nr_cgi->pLMNIdentity.buf[1] = 0xF1;
        nr_cgi->pLMNIdentity.buf[2] = 0x10;
        nr_cgi->pLMNIdentity.size = 3;
    }

    // NR Cell Identity (36 bits)
    nr_cgi->nRCellIdentity.buf = (uint8_t *) calloc(5, sizeof(uint8_t));
    nr_cgi->nRCellIdentity.size = 5;
    nr_cgi->nRCellIdentity.bits_unused = 4;
    if (gnb_id && gnb_id->buf) {
        // Copy gNB ID (29 bits) and add cell ID (7 bits)
        memcpy(nr_cgi->nRCellIdentity.buf, gnb_id->buf, 4);
        nr_cgi->nRCellIdentity.buf[4] = 0x00;  // Cell ID = 0
    } else {
        // Default cell ID
        nr_cgi->nRCellIdentity.buf[0] = 0x00;
        nr_cgi->nRCellIdentity.buf[1] = 0x00;
        nr_cgi->nRCellIdentity.buf[2] = 0x00;
        nr_cgi->nRCellIdentity.buf[3] = 0x08;  // gNB ID = 1
        nr_cgi->nRCellIdentity.buf[4] = 0x00;
    }

    // Add RAN Parameters with actual values (IDs 60001-60007)
    add_ran_parameter_int(cell_item, PARAM_ID_MAX_DL_CAPACITY_KBPS, capacity->max_dl_capacity_kbps);
    add_ran_parameter_int(cell_item, PARAM_ID_MAX_UL_CAPACITY_KBPS, capacity->max_ul_capacity_kbps);
    add_ran_parameter_int(cell_item, PARAM_ID_TOTAL_PRB_DL, capacity->total_prb_dl);
    add_ran_parameter_int(cell_item, PARAM_ID_TOTAL_PRB_UL, capacity->total_prb_ul);
    add_ran_parameter_int(cell_item, PARAM_ID_BANDWIDTH_MHZ, capacity->bandwidth_mhz);
    add_ran_parameter_int(cell_item, PARAM_ID_NUM_PRBS, capacity->num_prbs);
    add_ran_parameter_int(cell_item, PARAM_ID_SUBCARRIER_SPACING, capacity->subcarrier_spacing_khz);

    // Add cell item to the list
    ASN_SEQUENCE_ADD(&fmt1->cellInfo_List.list, cell_item);

    logger_info("Encoded E2SM-RC Query Outcome Format 1 with capacity: DL=%ld kbps, UL=%ld kbps, BW=%ld MHz, PRBs=%ld",
                capacity->max_dl_capacity_kbps, capacity->max_ul_capacity_kbps,
                capacity->bandwidth_mhz, capacity->num_prbs);

    // Check constraints
    char error_buf[300] = {0, };
    size_t errlen = 0;

    int ret = asn_check_constraints(&asn_DEF_E2SM_RC_QueryOutcome, outcome, error_buf, &errlen);
    if (ret != 0) {
        logger_error("E2SM_RC_QueryOutcome check constraints failed. error length = %lu, error buf = %s", errlen, error_buf);
        ASN_STRUCT_FREE(asn_DEF_E2SM_RC_QueryOutcome, outcome);
        return -1;
    }

    if (LOGGER_LEVEL >= LOGGER_DEBUG) {
        xer_fprint(stderr, &asn_DEF_E2SM_RC_QueryOutcome, outcome);
    }

    // Encode to OCTET_STRING using APER
    uint8_t *buffer = NULL;
    ssize_t encoded_size = aper_encode_to_new_buffer(&asn_DEF_E2SM_RC_QueryOutcome, NULL, outcome, (void **)&buffer);

    if (encoded_size < 0 || !buffer) {
        logger_error("Failed to APER encode E2SM_RC_QueryOutcome");
        ASN_STRUCT_FREE(asn_DEF_E2SM_RC_QueryOutcome, outcome);
        return -1;
    }

    // Copy to output OCTET_STRING
    outcome_ostr->buf = buffer;
    outcome_ostr->size = encoded_size;

    logger_debug("E2SM-RC Query Outcome encoded to %zd bytes", encoded_size);

    ASN_STRUCT_FREE(asn_DEF_E2SM_RC_QueryOutcome, outcome);

    logger_trace("end of %s", __func__);
    return 0;
}
