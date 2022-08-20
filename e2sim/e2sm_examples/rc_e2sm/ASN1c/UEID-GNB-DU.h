/*
 * Generated by asn1c-0.9.29 (http://lionet.info/asn1c)
 * From ASN.1 module "E2SM-COMMON-IEs"
 * 	found in "e2sm-v02.01.asn1"
 * 	`asn1c -fcompound-names -findirect-choice -fincludes-quoted -fno-include-deps -gen-PER -no-gen-OER -no-gen-example`
 */

#ifndef	_UEID_GNB_DU_H_
#define	_UEID_GNB_DU_H_


#include "asn_application.h"

/* Including external dependencies */
#include "GNB-CU-UE-F1AP-ID.h"
#include "RANUEID.h"
#include "constr_SEQUENCE.h"

#ifdef __cplusplus
extern "C" {
#endif

/* UEID-GNB-DU */
typedef struct UEID_GNB_DU {
	GNB_CU_UE_F1AP_ID_t	 gNB_CU_UE_F1AP_ID;
	RANUEID_t	*ran_UEID;	/* OPTIONAL */
	/*
	 * This type is extensible,
	 * possible extensions are below.
	 */
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} UEID_GNB_DU_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_UEID_GNB_DU;
extern asn_SEQUENCE_specifics_t asn_SPC_UEID_GNB_DU_specs_1;
extern asn_TYPE_member_t asn_MBR_UEID_GNB_DU_1[2];

#ifdef __cplusplus
}
#endif

#endif	/* _UEID_GNB_DU_H_ */
#include "asn_internal.h"
