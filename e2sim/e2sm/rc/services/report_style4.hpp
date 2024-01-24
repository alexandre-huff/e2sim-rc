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

#ifndef RRC_STATE_OBSERVER_HPP
#define RRC_STATE_OBSERVER_HPP

#include <string>
#include <any>

#include <envman/environment_manager.h>

#include "ric_indication.hpp"
#include "subaction.hpp"
#include "types_rc.hpp"
#include "ric_subscription.hpp"
#include "encode_rc.hpp"
#include "global_data.hpp"

extern "C" {
    #include "UEID.h"
    #include "E2SM-RC-IndicationMessage-Format2-RANParameter-Item.h"
}

/**
 * This class implements the E2SM-RC REPORT Service Style 4 feature
*/
class RRCStateObserver : public EnvironmentManagerObserver, public SubscriptionAction {
public:
    RRCStateObserver(ric_subscription_info_t info, std::any style4_data,
            std::shared_ptr<RICIndicationProcedure> ric_indication, EnvironmentManager *manager, const std::shared_ptr<GlobalE2NodeData> &global_data) :
            subscriptionInfo(info), style4Data(std::any_cast<common::rc::report_style4_data>(style4_data)),
            ricIndication(ric_indication), envManager(manager), globalE2NodeData(global_data) { /* pass */ }
    ~RRCStateObserver() { /* pass */ }

    /**
     * Notifies the observer about a new ANR update.
     * @iMSI UE unique identifier
     * @entries ANR entries
     */
    virtual void anrUpdate(const std::string iMSI, const std::map<std::string, std::shared_ptr<anr_entry>> &entries) override;

    /**
     * Notifies the observer about a new Flow update.
     * @iMSI UE unique identifier
     * @entry Flow descriptor
     */
    virtual void flowUpdate(const std::string iMSI, const flow_entry &entry) override;

    /**
     * Notifies the observer about a new UE requessting association.
     * @ue The UE data
     */
    virtual void associationRequest(const std::shared_ptr<ue_data> ue) override;

    /**
     * Notifiess the observer about a new UE requesting disassociation.
     * @ue The UE description
     */
    virtual void disassociationRequest(const std::shared_ptr<ue_data> ue) override;

    /**
     * Starts the RRCStateObserver subscription action
     *
     * IMPORTANT: first set the shared_ptr that owns this object
    */
    virtual bool start() override;

    /**
     * Stops the RRCStateObserver subscription action
    */
    virtual bool stop() override;

    void setMySharedPtr(std::shared_ptr<RRCStateObserver> my_shared_ptr); // unfortunately this is required due to current EnvironmentManager design

    bool generate_ueid_report_info(UEID_t &ueid, const std::string &imsi);

    bool generate_ran_params_report_info(const e_RRC_State changed_to, const int rsrp, const int rsrq, const int sinr,
            std::vector<E2SM_RC_IndicationMessage_Format2_RANParameter_Item_t *> &params);

    void encode_and_send_report_msg(std::vector<common::rc::indication_msg_format2_ueid_t> &ue_ids);

private:
    bool isStarted = false;
    uint16_t counter = 0; // Indication sequence number counter
    ric_subscription_info_t subscriptionInfo;
    common::rc::report_style4_data style4Data;
    std::shared_ptr<RICIndicationProcedure> ricIndication;
    EnvironmentManager *envManager;
    std::shared_ptr<RRCStateObserver> mySharedPtr;  // unfortunately this is required due to current EnvironmentManager design
    std::shared_ptr<GlobalE2NodeData> globalE2NodeData;
};

#endif
