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

// #include <stdexcept>

#include "global_data.hpp"
#include "utils.hpp"
#include "logger.h"

extern "C" {
    #include "GlobalE2node-gNB-ID.h"
    #include "PLMN-Identity.h"
    #include "BIT_STRING.h"
    #include "OCTET_STRING.h"
}

/**
 * Throws std::invalid_argument
*/
GlobalE2NodeData::GlobalE2NodeData(std::string mcc, std::string mnc, uint32_t gnb_id) : gnbid(gnb_id) {
    if (mcc.length() != 3) {
        throw std::invalid_argument("MCC requires 3 digits");
    }

    size_t mnc_len = mnc.length();
    if (mnc_len != 2 && mnc_len != 3) {
        throw std::invalid_argument("MNC requires 2 or 3 digits");
    }

    if (gnb_id >= 1<<GNB_ID_LENGTH) {
        std::string error = std::string("maximum gnb_id value is 2^") + std::to_string(GNB_ID_LENGTH) + "-1";
        throw std::invalid_argument(error);
    }

    uint32_t gnbid_copy;
    gnbid_copy = gnb_id;

    // Building the Global E2 Node ID of the simulator
    globalE2NodeId = (GlobalE2node_ID_t *) calloc(1, sizeof(GlobalE2node_ID_t));
    globalE2NodeId->present = GlobalE2node_ID_PR_gNB;
    globalE2NodeId->choice.gNB = (GlobalE2node_gNB_ID_t *) calloc(1, sizeof(GlobalE2node_gNB_ID_t));

    // encoding PLMN identity
    PLMN_Identity_t *plmnid = common::utils::encodePlmnId(mcc.c_str(), mnc.c_str());
    if (plmnid) {
        globalE2NodeId->choice.gNB->global_gNB_ID.plmn_id = *plmnid;
        free(plmnid);
    } else {
        logger_error("Unable to encode PLMN ID based for MCC=%s MNC=%s", mcc.c_str(), mnc.c_str());
    }
    // encoding gNodeB identity
    globalE2NodeId->choice.gNB->global_gNB_ID.gnb_id.present = GNB_ID_Choice_PR_gnb_ID;
    BIT_STRING_t *gnb_bstr = &globalE2NodeId->choice.gNB->global_gNB_ID.gnb_id.choice.gnb_ID;
    memset(gnb_bstr, 0, sizeof(BIT_STRING_t));
    gnb_bstr->buf = (uint8_t *) calloc(1, 4); // maximum size is 32 bits
    gnb_bstr->size = 4;
    gnb_bstr->bits_unused = 3; // we are using 29 bits for gnb_id so that 7 bits (3+4) is left for the NR Cell Identity (128 cells per gNodeB)
    gnb_id = ((gnb_id & 0X1FFFFFFF) << 3);
    gnb_bstr->buf[0] = ((gnb_id & 0XFF000000) >> 24);
    gnb_bstr->buf[1] = ((gnb_id & 0X00FF0000) >> 16);
    gnb_bstr->buf[2] = ((gnb_id & 0X0000FF00) >> 8);
    gnb_bstr->buf[3] = (gnb_id & 0X000000FF);

    if (LOGGER_LEVEL >= LOGGER_DEBUG) {
        logger_debug("Global gNodeB Identity encoded for %u", gnbid_copy);
        xer_fprint(stderr , &asn_DEF_BIT_STRING, gnb_bstr);
    }
}

GlobalE2NodeData::~GlobalE2NodeData() {
    ASN_STRUCT_FREE(asn_DEF_GlobalE2node_ID, globalE2NodeId);
}

/*
  Return a copy of the Global gNodeb ID.
  It is the caller responsibility to free the returned pointer.
*/
GlobalE2node_ID_t *GlobalE2NodeData::getGlobalE2NodeId() {
    GlobalE2node_ID_t *copy = (GlobalE2node_ID_t *) calloc(1, sizeof(GlobalE2node_ID_t));
    copy->present = globalE2NodeId->present;
    copy->choice.gNB = (GlobalE2node_gNB_ID_t *) calloc(1, sizeof(GlobalE2node_gNB_ID_t));

    OCTET_STRING_fromBuf(&copy->choice.gNB->global_gNB_ID.plmn_id,
                        (char *)globalE2NodeId->choice.gNB->global_gNB_ID.plmn_id.buf,
                        globalE2NodeId->choice.gNB->global_gNB_ID.plmn_id.size);

    copy->choice.gNB->global_gNB_ID.gnb_id.present = globalE2NodeId->choice.gNB->global_gNB_ID.gnb_id.present;
    copy->choice.gNB->global_gNB_ID.gnb_id.choice.gnb_ID.buf =
            (uint8_t *) calloc(globalE2NodeId->choice.gNB->global_gNB_ID.gnb_id.choice.gnb_ID.size, sizeof(uint8_t));

    copy->choice.gNB->global_gNB_ID.gnb_id.choice.gnb_ID.size = globalE2NodeId->choice.gNB->global_gNB_ID.gnb_id.choice.gnb_ID.size;
    copy->choice.gNB->global_gNB_ID.gnb_id.choice.gnb_ID.bits_unused = globalE2NodeId->choice.gNB->global_gNB_ID.gnb_id.choice.gnb_ID.bits_unused;
    mempcpy(copy->choice.gNB->global_gNB_ID.gnb_id.choice.gnb_ID.buf,
            globalE2NodeId->choice.gNB->global_gNB_ID.gnb_id.choice.gnb_ID.buf,
            copy->choice.gNB->global_gNB_ID.gnb_id.choice.gnb_ID.size);

    return copy;
}

/*
    Return a copy of the PLMN Identity.
    NOTE: PLMN_Identity_t is a redefined OCTET_STRING_t type

    It is the caller responsibility to free the returned pointer.
*/
PLMN_Identity_t *GlobalE2NodeData::getGlobalE2NodePlmnId() {
    PLMN_Identity_t *plmnid = (PLMN_Identity_t *) calloc(1, sizeof(PLMN_Identity_t));
    plmnid->size = globalE2NodeId->choice.gNB->global_gNB_ID.plmn_id.size;
    plmnid->buf = (uint8_t *) calloc(plmnid->size, sizeof(uint8_t));
    memcpy(plmnid->buf, globalE2NodeId->choice.gNB->global_gNB_ID.plmn_id.buf, plmnid->size);

    return plmnid;
}

/*
  Return a copy of the Global E2 Node gNodeB Id encoded in bit string.
  It is the caller responsibility to free the returned pointer.
*/
BIT_STRING_t *GlobalE2NodeData::getGlobalE2Node_gNBId() {
    BIT_STRING_t *gnbid = (BIT_STRING_t *) calloc(1, sizeof(BIT_STRING_t));
    gnbid->size = globalE2NodeId->choice.gNB->global_gNB_ID.gnb_id.choice.gnb_ID.size;
    gnbid->buf = (uint8_t *) calloc(gnbid->size, sizeof(uint8_t));
    memcpy(gnbid->buf, globalE2NodeId->choice.gNB->global_gNB_ID.gnb_id.choice.gnb_ID.buf, gnbid->size);
    gnbid->bits_unused = globalE2NodeId->choice.gNB->global_gNB_ID.gnb_id.choice.gnb_ID.bits_unused;

    return gnbid;
}

void GlobalE2NodeData::updateCellTxReferenceLevel(uint16_t pci, double gain) {
    std::lock_guard<std::mutex> guard(cellsLock);
    auto it = cells.find(pci);
    if (it != cells.end()) {
        it->second->setGain(gain);
    } else {
        std::shared_ptr<Cell> cell = std::make_shared<Cell>(pci, gain);
        addCell(cell);
    }
}

bool GlobalE2NodeData::addCell(std::shared_ptr<Cell> cell) {
    std::lock_guard<std::mutex> guard(cellsLock);
    uint16_t pci = cell->getPci();
    auto it = cells.find(pci);
    if (it == cells.end()) {
        cells[pci] = cell;
        return true;
    }
    return false;
}

void GlobalE2NodeData::deleteCell(uint16_t pci) {
    std::lock_guard<std::mutex> guard(cellsLock);
    cells.erase(pci);
}

std::shared_ptr<Cell> GlobalE2NodeData::getCell(uint16_t pci) {
    auto it = cells.find(pci);
    if (it != cells.end()) {
        return it->second;
    }
    return std::shared_ptr<Cell>();
}

/*
    Retrieve a list of all cells and their corresponding data
*/
std::vector<std::shared_ptr<Cell>> GlobalE2NodeData::getCells() {
    std::lock_guard<std::mutex> guard(cellsLock);
    std::vector<std::shared_ptr<Cell>> list;
    for (auto cell : cells) {
        list.emplace_back(cell.second);
    }
     return std::move(list);
}

void GlobalE2NodeData::setPendingTxReferenceLevel(uint16_t pci, double gain) {
    std::lock_guard<std::mutex> guard(pendingLock);
    pending_tx_gain[pci] = gain;
}

bool GlobalE2NodeData::hasPendingTxReferenceLevel(uint16_t pci) {
    std::lock_guard<std::mutex> guard(pendingLock);
    return pending_tx_gain.find(pci) != pending_tx_gain.end();
}

bool GlobalE2NodeData::getPendingTxReferenceLevel(uint16_t pci, double &outGain) {
    std::lock_guard<std::mutex> guard(pendingLock);
    auto it = pending_tx_gain.find(pci);
    if (it == pending_tx_gain.end()) return false;
    outGain = it->second;
    return true;
}

void GlobalE2NodeData::clearPendingTxReferenceLevel(uint16_t pci) {
    std::lock_guard<std::mutex> guard(pendingLock);
    pending_tx_gain.erase(pci);
}

/*
     Add a new UEInfo in the list of connected UEs
     UEInfo is only added if it is not already present in the UEList.
 */
void UEList::addUE(std::shared_ptr<e2sim::ue::UEInfo> ue) {
    std::lock_guard<std::mutex> guard(ue_lock);
    ue_map.emplace(ue->imsi, ue);
}

void UEList::removeUE(std::string imsi) {
    std::lock_guard<std::mutex> guard(ue_lock);
    ue_map.erase(imsi);
}

std::shared_ptr<e2sim::ue::UEInfo> UEList::getUEInfo(std::string imsi) {
    std::lock_guard<std::mutex> guard(ue_lock);

    auto it = ue_map.find(imsi);
    if (it == ue_map.end()) {
        return std::shared_ptr<e2sim::ue::UEInfo>();
    }

    return it->second;
}

std::vector<std::shared_ptr<e2sim::ue::UEInfo>> UEList::getUEs() {
    std::vector<std::shared_ptr<e2sim::ue::UEInfo>> ues;
    std::lock_guard<std::mutex> guard(ue_lock);
    ues.reserve(ue_map.size());
    for (const auto &kv : ue_map) {
        ues.emplace_back(kv.second);
    }
    return ues;
}
