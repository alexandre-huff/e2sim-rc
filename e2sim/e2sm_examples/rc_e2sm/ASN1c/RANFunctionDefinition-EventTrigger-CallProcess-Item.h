/*
 * Generated by asn1c-0.9.29 (http://lionet.info/asn1c)
 * From ASN.1 module "E2SM-RC-IEs"
 * 	found in "e2sm-rc-v01.02.03.asn1"
 * 	`asn1c -fcompound-names -findirect-choice -fincludes-quoted -fno-include-deps -gen-PER -no-gen-OER -no-gen-example`
 */

#ifndef	_RANFunctionDefinition_EventTrigger_CallProcess_Item_H_
#define	_RANFunctionDefinition_EventTrigger_CallProcess_Item_H_


#include "asn_application.h"

/* Including external dependencies */
#include "RIC-CallProcessType-ID.h"
#include "RIC-CallProcessType-Name.h"
#include "asn_SEQUENCE_OF.h"
#include "constr_SEQUENCE_OF.h"
#include "constr_SEQUENCE.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
struct RANFunctionDefinition_EventTrigger_Breakpoint_Item;

/* RANFunctionDefinition-EventTrigger-CallProcess-Item */
typedef struct RANFunctionDefinition_EventTrigger_CallProcess_Item {
	RIC_CallProcessType_ID_t	 callProcessType_ID;
	RIC_CallProcessType_Name_t	 callProcessType_Name;
	struct RANFunctionDefinition_EventTrigger_CallProcess_Item__callProcessBreakpoints_List {
		A_SEQUENCE_OF(struct RANFunctionDefinition_EventTrigger_Breakpoint_Item) list;
		
		/* Context for parsing across buffer boundaries */
		asn_struct_ctx_t _asn_ctx;
	} callProcessBreakpoints_List;
	/*
	 * This type is extensible,
	 * possible extensions are below.
	 */
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} RANFunctionDefinition_EventTrigger_CallProcess_Item_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_RANFunctionDefinition_EventTrigger_CallProcess_Item;
extern asn_SEQUENCE_specifics_t asn_SPC_RANFunctionDefinition_EventTrigger_CallProcess_Item_specs_1;
extern asn_TYPE_member_t asn_MBR_RANFunctionDefinition_EventTrigger_CallProcess_Item_1[3];

#ifdef __cplusplus
}
#endif

#endif	/* _RANFunctionDefinition_EventTrigger_CallProcess_Item_H_ */
#include "asn_internal.h"
