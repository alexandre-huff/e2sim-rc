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
#include "utils.hpp"

extern "C" {
    #include "E2SM-RC-IndicationHeader.h"
    #include "E2SM-RC-IndicationHeader-Format1.h"
    #include "E2SM-RC-IndicationMessage-Format2.h"
    #include "E2SM-RC-IndicationMessage-Format2-Item.h"
}

using namespace std;


/**
 * Encodes the RANFunctionDefinition_Report of E2SM_RC_RANFunctionDefinition_t
*/
void common::rc::encode_report_function_definition(void *report_func_def, std::vector<std::shared_ptr<ServiceStyle>> styles) {
    LOGGER_TRACE_FUNCTION_IN

    E2SM_RC_RANFunctionDefinition_t *ranfunc_def = (E2SM_RC_RANFunctionDefinition_t *) report_func_def;
    ranfunc_def->ranFunctionDefinition_Report = (RANFunctionDefinition_Report_t *) calloc(1, sizeof(RANFunctionDefinition_Report_t));

    for (auto &style : styles) {
        RANFunctionDefinition_Report_Item_t *report_item = (RANFunctionDefinition_Report_Item_t *) calloc(1, sizeof(RANFunctionDefinition_Report_Item_t));
        report_item->ric_ReportStyle_Type = style->getRicStyleType();
        OCTET_STRING_fromBuf(&report_item->ric_ReportStyle_Name, style->getRicStyleName().c_str(), style->getRicStyleName().length());
        report_item->ric_IndicationHeaderFormat_Type = style->getRicHeaderFormatType();
        report_item->ric_IndicationMessageFormat_Type = style->getRicMessageFormatType();
        report_item->ric_SupportedEventTriggerStyle_Type = style->getTriggerDefinition()->getStyleType();
        report_item->ric_ReportActionFormat_Type = style->getActionDefinition()->getFormat();

        for (auto &param : style->getActionDefinition()->getRanParameters()) {
            Report_RANParameter_Item_t *item = (Report_RANParameter_Item_t *) calloc(1, sizeof(Report_RANParameter_Item_t));
            item->ranParameter_ID = param->getParamId();
            OCTET_STRING_fromBuf(&item->ranParameter_name, param->getParamName().c_str(), param->getParamName().length());
            ASN_SEQUENCE_ADD(&report_item->ran_ReportParameters_List->list, item);
        }

        ASN_SEQUENCE_ADD(&ranfunc_def->ranFunctionDefinition_Report->ric_ReportStyle_List.list, report_item);
    }

    LOGGER_TRACE_FUNCTION_OUT
}

/*
    Encodes the E2SM-RC Indication Header Format 1

    Params:
        *condition: it is OPTIONAL as per E2SM-RC-R003-v03.00 so that NULL is also accepted

    Returns a pointer with the encoded data, or NULL on error.
*/
RICindicationHeader_t *common::rc::encode_indication_header_format1(RIC_EventTriggerCondition_ID_t *condition) {
    E2SM_RC_IndicationHeader_t *header = (E2SM_RC_IndicationHeader_t *) calloc(1, sizeof(E2SM_RC_IndicationHeader_t));
    header->ric_indicationHeader_formats.present = E2SM_RC_IndicationHeader__ric_indicationHeader_formats_PR_indicationHeader_Format1;
    header->ric_indicationHeader_formats.choice.indicationHeader_Format1 =
            (E2SM_RC_IndicationHeader_Format1_t *) calloc(1, sizeof(E2SM_RC_IndicationHeader_Format1_t));

    if (condition) {
        header->ric_indicationHeader_formats.choice.indicationHeader_Format1->ric_eventTriggerCondition_ID =
                (RIC_EventTriggerCondition_ID_t *) calloc(1, sizeof(RIC_EventTriggerCondition_ID_t));
        *header->ric_indicationHeader_formats.choice.indicationHeader_Format1->ric_eventTriggerCondition_ID = *condition;
    }

    RICindicationHeader_t *ric_header = utils::asn1_check_and_encode(&asn_DEF_E2SM_RC_IndicationHeader, header); // returns NULL on error

    return ric_header;
}

/*
    Encodes the E2SM-RC Indication Message Format 2

    Returns a pointer with the encoded data, or NULL on error.
*/
RICindicationMessage_t *common::rc::encode_indication_message_format2(const std::vector<indication_msg_format2_ueid_t> &ue_ids) {
    E2SM_RC_IndicationMessage_t *msg =
            (E2SM_RC_IndicationMessage_t *) calloc(1, sizeof(E2SM_RC_IndicationMessage_t));
    msg->ric_indicationMessage_formats.present = E2SM_RC_IndicationMessage__ric_indicationMessage_formats_PR_indicationMessage_Format2;
    msg->ric_indicationMessage_formats.choice.indicationMessage_Format2 =
            (E2SM_RC_IndicationMessage_Format2_t *) calloc(1, sizeof(E2SM_RC_IndicationMessage_Format2_t));
    E2SM_RC_IndicationMessage_Format2_t *msg_fmt2 = msg->ric_indicationMessage_formats.choice.indicationMessage_Format2;

    for (auto &ue_id : ue_ids) {
        // Sequence of UE Identifiers as per 9.2.1.4.2 in E2SM-RC-R003-v03.00
        E2SM_RC_IndicationMessage_Format2_Item_t *fmt2_item =
                (E2SM_RC_IndicationMessage_Format2_Item_t *) calloc(1, sizeof(E2SM_RC_IndicationMessage_Format2_Item_t));
        ASN_SEQUENCE_ADD(&msg_fmt2->ueParameter_List.list, fmt2_item);

        /* ################ UE ID ################ */
        fmt2_item->ueID = ue_id.ueid;

        /* ###### Sequence of RAN Parameters for each UE ID ###### */
        for (auto &param : ue_id.ranp_list) {
            ASN_SEQUENCE_ADD(&fmt2_item->ranP_List.list, param);
        }
    }

    RICindicationMessage_t *ric_msg = utils::asn1_check_and_encode(&asn_DEF_E2SM_RC_IndicationMessage, msg); // returns NULL on error

    return ric_msg;
}

void encode_rc_function_definition(E2SM_RC_RANFunctionDefinition_t *ranfunc_def) {
    int ret;    // Temporary return value
    size_t len; // Temporary length to avoid unnecessary strlen calls

    LOGGER_TRACE_FUNCTION_IN

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
    ctrl_item->ric_ControlStyle_Type = 4;
    uint8_t *ctrl_name = (uint8_t *) "Radio access control";
    len = strlen((char *) ctrl_name);
    ctrl_item->ric_ControlStyle_Name.buf = (uint8_t *) calloc(len, sizeof(uint8_t));
    memcpy(ctrl_item->ric_ControlStyle_Name.buf, ctrl_name, len);
    ctrl_item->ric_ControlStyle_Name.size = len;

    ctrl_item->ric_ControlHeaderFormat_Type = 1;
    ctrl_item->ric_ControlMessageFormat_Type = 1;
    ctrl_item->ric_CallProcessIDFormat_Type = (RIC_Format_Type_t *) calloc(1, sizeof(RIC_Format_Type_t));
    *ctrl_item->ric_CallProcessIDFormat_Type = 1;

    // RANFunctionDefinition_Control_Action_Item_t *ctrl_act_item =
    //         (RANFunctionDefinition_Control_Action_Item_t *) calloc(1, sizeof(RANFunctionDefinition_Control_Action_Item_t));
    // ctrl_act_item->ric_ControlAction_ID = 1;
    // uint8_t *ctrl_act_name = (uint8_t *) "UE Admission Control";
    // len = strlen((char *) ctrl_act_name);
    // ctrl_act_item->ric_ControlAction_Name.buf = (uint8_t *) calloc(len, sizeof(uint8_t));
    // memcpy(ctrl_act_item->ric_ControlAction_Name.buf, ctrl_act_name, len);
    // ctrl_act_item->ric_ControlAction_Name.size = len;
    // ASN_SEQUENCE_ADD(&ctrl_item->ric_ControlAction_List->list, ctrl_act_item);

    ASN_SEQUENCE_ADD(&ranfunc_def->ranFunctionDefinition_Control->ric_ControlStyle_List.list, ctrl_item);
    logger_trace("ranFunction_Definition_Control set up");

    if(LOGGER_LEVEL >= LOGGER_DEBUG) {
        xer_fprint(stderr, &asn_DEF_E2SM_RC_RANFunctionDefinition, ranfunc_def);
    }


	LOGGER_TRACE_FUNCTION_OUT
}

void encode_rc_indication_message(E2SM_RC_IndicationMessage_t *ind_msg, PLMNIdentity_t *plmn_id, BIT_STRING_t *gnb_id)
{
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
