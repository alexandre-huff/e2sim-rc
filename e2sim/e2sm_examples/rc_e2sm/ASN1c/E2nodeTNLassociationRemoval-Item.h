/*
 * Generated by asn1c-0.9.29 (http://lionet.info/asn1c)
 * From ASN.1 module "E2AP-PDU-Contents"
 * 	found in "e2ap-v02.02.03.asn1"
 * 	`asn1c -fcompound-names -findirect-choice -fincludes-quoted -fno-include-deps -gen-PER -no-gen-OER -no-gen-example`
 */

#ifndef	_E2nodeTNLassociationRemoval_Item_H_
#define	_E2nodeTNLassociationRemoval_Item_H_


#include "asn_application.h"

/* Including external dependencies */
#include "TNLinformation.h"
#include "constr_SEQUENCE.h"

#ifdef __cplusplus
extern "C" {
#endif

/* E2nodeTNLassociationRemoval-Item */
typedef struct E2nodeTNLassociationRemoval_Item {
	TNLinformation_t	 tnlInformation;
	TNLinformation_t	 tnlInformationRIC;
	/*
	 * This type is extensible,
	 * possible extensions are below.
	 */
	
	/* Context for parsing across buffer boundaries */
	asn_struct_ctx_t _asn_ctx;
} E2nodeTNLassociationRemoval_Item_t;

/* Implementation */
extern asn_TYPE_descriptor_t asn_DEF_E2nodeTNLassociationRemoval_Item;

#ifdef __cplusplus
}
#endif

#endif	/* _E2nodeTNLassociationRemoval_Item_H_ */
#include "asn_internal.h"
