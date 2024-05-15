/*****************************************************************************
#                                                                            *
# Copyright 2023 Alexandre Huff                                              *
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

#ifndef E2AP_MESSAGES_HPP
#define E2AP_MESSAGES_HPP

#include <vector>

extern "C" {
    #include "RICrequestID.h"
    #include "RANfunctionID.h"
    #include "RICsubscriptionDetails.h"
    #include "RICactionID.h"
    #include "RICactionType.h"
    #include "RICactionDefinition.h"
    #include "RICsubsequentAction.h"
    #include "RICsubscriptionResponse.h"
    #include "RICaction-Admitted-Item.h"
    #include "RICaction-NotAdmitted-Item.h"
    #include "Cause.h"
    #include "RICindicationSN.h"
    #include "RICindicationType.h"
    #include "RICindicationHeader.h"
    #include "RICindicationMessage.h"
    #include "RICcallProcessID.h"
    #include "RICcontrolHeader.h"
    #include "RICcontrolMessage.h"
    #include "RICcontrolAckRequest.h"
    #include "RICcontrolOutcome.h"
	#include "CriticalityDiagnostics.h"
}

namespace e2sim {
    namespace messages {

        struct RICActionToBeSetup {
            RICactionID_t ricActionId;
            RICactionType_t ricActionType;
            RICactionDefinition_t *ricActionDefinition = nullptr;
            RICsubsequentAction_t *ricSubsequentAction = nullptr;
        };

        struct RICSubscriptionRequest {
            RICrequestID_t ricRequestId;
            RANfunctionID_t ranFunctionId;
            RICeventTriggerDefinition_t	*ricEventTriggerDefinition = nullptr;
            std::vector<RICActionToBeSetup> sequenceOfActions;
        };

        struct RICSubscriptionResponse {
            bool succeeded;	// true for RIC_SUBSCRIPTION_RESPONSE, and false for RIC_SUBSCRIPTION_FAILURE
            RICrequestID_t ricRequestId;
            RANfunctionID_t ranFunctionId;
            std::vector<RICaction_Admitted_Item_t> admittedList;
            std::vector<RICaction_NotAdmitted_Item_t> notAdmittedList;
            Cause_t cause;	// only valid with RIC_SUBSCRIPTION_FAILURE
			CriticalityDiagnostics_t *diagnostics = nullptr;	// not mandatory and only valid with RIC_SUBSCRIPTION_FAILURE
        };

        struct RICSubscriptionDeleteRequest {
            RICrequestID_t ricRequestId;
            RANfunctionID_t ranFunctionId;
        };

        struct RICSubscriptionDeleteResponse {
            bool succeeded;	// true for RIC_SUBSCRIPTION_DELETE_RESPONSE, and false for RIC_SUBSCRIPTION_DELETE_FAILURE
            RICrequestID_t ricRequestId;
            RANfunctionID_t ranFunctionId;
            Cause_t cause;	// only valid with RIC_SUBSCRIPTION_DELETE_FAILURE
			CriticalityDiagnostics_t *diagnostics = nullptr;	// not mandatory and only valid with RIC_SUBSCRIPTION_DELETE_FAILURE
        };

        struct RICIndication {
            RICrequestID_t ricRequestId;
            RANfunctionID_t ranFunctionId;
            RICactionID_t actionId;
            RICindicationType_t indicationType;
            RICindicationHeader_t header;
            RICindicationMessage_t message;
            RICindicationSN_t *indicationSN = nullptr;		// not mandatory (IMPORTANT range of values (uint16_t) 0..65535)
            RICcallProcessID_t *callProcessId = nullptr;	// not mandatory
        };

        struct RICControlRequest {
            RICrequestID_t ricRequestId;
            RANfunctionID_t ranFunctionId;
            RICcontrolHeader_t header;
            RICcontrolMessage_t message;
            RICcallProcessID_t *callProcessId = nullptr;		// not mandatory
            RICcontrolAckRequest_t *controAckRequest = nullptr;	// not mandatory
        };

        struct RICControlResponse {
			bool succeeded;	// true for RIC_CONTROL_ACKNOWLEDGE, and false for RIC_CONTROL_FAILURE
            RICrequestID_t ricRequestId;
            RANfunctionID_t ranFunctionId;
            RICcallProcessID_t *callProcessId = nullptr;	// not mandatory
            RICcontrolOutcome_t *controlOutcome = nullptr;	// not mandatory
			Cause_t cause;	// only valid with RIC_CONTROL_FAILURE
			CriticalityDiagnostics_t *diagnostics = nullptr;	// not mandatory and only valid with RIC_CONTROL_FAILURE
        };

    }   // namespace
}   // namespace


#endif