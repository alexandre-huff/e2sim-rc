/*
 * Generated by asn1c-0.9.29 (http://lionet.info/asn1c)
 * From ASN.1 module "E2SM-RC-IEs"
 * 	found in "e2sm-rc-v01.02.03.asn1"
 * 	`asn1c -fcompound-names -findirect-choice -fincludes-quoted -fno-include-deps -gen-PER -no-gen-OER -no-gen-example`
 */

#ifndef	_E2SM_RC_EventTrigger_Format5_H_
#define	_E2SM_RC_EventTrigger_Format5_H_


#include "asn_application.h"

/* Including external dependencies */
#include "NativeEnumerated.h"
#include "constr_SEQUENCE.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Dependencies */
typedef enum E2SM_RC_EventTrigger_Format5__onDemand {
	E2SM_RC_EventTrigger_Format5__onDemand_true	= 0
	/*
	 * Enumeration is extensible
	 */
} e_E2SM_RC_EventTrigger_Format5__onDemand;

/* Forward declarations */
struct EventTrigger_UE_Info;
struct EventTrigger_Cell_Info;

/* E2SM-RC-EventTrigger-Format5 */
typedef struct E2SM_RC_EventTrigger_Format5 {
	long	 onDemand;
	struct EventTrigger_UE_Info	*associatedUEInfo;	/* OPTIONAL */
	struct EventTrigger_Cell_Info	*associatedCellInfo;	/* OPTIONAL */
	/*
	 * This type is extensible,
	 * possible extensions are below.
	 */
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} E2SM_RC_EventTrigger_Format5_t;

/* Implementation */
/* extern asn_TYPE_descriptor_t asn_DEF_onDemand_2;	// (Use -fall-defs-global to expose) */
extern asn_TYPE_descriptor_t asn_DEF_E2SM_RC_EventTrigger_Format5;
extern asn_SEQUENCE_specifics_t asn_SPC_E2SM_RC_EventTrigger_Format5_specs_1;
extern asn_TYPE_member_t asn_MBR_E2SM_RC_EventTrigger_Format5_1[3];

#ifdef __cplusplus
}
#endif

#endif	/* _E2SM_RC_EventTrigger_Format5_H_ */
#include "asn_internal.h"
