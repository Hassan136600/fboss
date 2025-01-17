/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/sai/switch/SaiHandler.h"

#include "fboss/agent/hw/sai/switch/SaiSwitch.h"

#include <folly/logging/xlog.h>
#include "fboss/lib/LogThriftCall.h"

namespace facebook::fboss {

SaiHandler::SaiHandler(const SaiSwitch* hw)
    : hw_(hw), diagShell_(hw), diagCmdServer_(hw, &diagShell_) {}

SaiHandler::~SaiHandler() {}

apache::thrift::ResponseAndServerStream<std::string, std::string>
SaiHandler::startDiagShell() {
  XLOG(DBG2) << "New diag shell session connecting";
  if (!hw_->isFullyInitialized()) {
    throw FbossError("switch is still initializing or is exiting ");
  }
  diagShell_.tryConnect();
  auto streamAndPublisher =
      apache::thrift::ServerStream<std::string>::createPublisher([this]() {
        diagShell_.markResetPublisher();
        XLOG(DBG2) << "Diag shell session disconnected";
      });

  std::string firstPrompt =
      diagShell_.start(std::move(streamAndPublisher.second));
  return {firstPrompt, std::move(streamAndPublisher.first)};
}

void SaiHandler::produceDiagShellInput(
    std::unique_ptr<std::string> input,
    std::unique_ptr<ClientInformation> client) {
  diagShell_.consumeInput(std::move(input), std::move(client));
}

void SaiHandler::diagCmd(
    fbstring& result,
    std::unique_ptr<fbstring> cmd,
    std::unique_ptr<ClientInformation> client,
    int16_t /* unused */,
    bool /* unused */) {
  auto log = LOG_THRIFT_CALL(WARN, *cmd);
  hw_->ensureConfigured(__func__);
  result = diagCmdServer_.diagCmd(std::move(cmd), std::move(client));
}

SwitchRunState SaiHandler::getHwSwitchRunState() {
  return hw_->getRunState();
}

void SaiHandler::getHwFabricReachability(
    std::map<::std::int64_t, ::facebook::fboss::FabricEndpoint>& reachability) {
  hw_->ensureVoqOrFabric(__func__);
  auto reachabilityInfo = hw_->getFabricConnectivity();
  for (auto&& entry : reachabilityInfo) {
    reachability.insert(std::move(entry));
  }
}

void SaiHandler::getHwFabricConnectivity(
    std::map<::std::string, ::facebook::fboss::FabricEndpoint>& connectivity) {
  hw_->ensureVoqOrFabric(__func__);
  auto connectivityInfo = hw_->getFabricConnectivity();
  for (auto& entry : connectivityInfo) {
    auto port = hw_->getProgrammedState()->getPorts()->getNodeIf(entry.first);
    if (port) {
      connectivity.insert({port->getName(), entry.second});
    }
  }
}

void SaiHandler::getHwSwitchReachability(
    std::map<::std::string, std::vector<::std::string>>& reachability,
    std::unique_ptr<::std::vector<::std::string>> switchNames) {
  hw_->ensureVoqOrFabric(__func__);
  if (switchNames->empty()) {
    throw FbossError("Empty switch name list input for getSwitchReachability.");
  }
  std::unordered_set<std::string> switchNameSet{
      switchNames->begin(), switchNames->end()};

  auto state = hw_->getProgrammedState();
  for (const auto& [_, dsfNodes] : std::as_const(*state->getDsfNodes())) {
    for (const auto& [_, node] : std::as_const(*dsfNodes)) {
      if (std::find(
              switchNameSet.begin(), switchNameSet.end(), node->getName()) !=
          switchNameSet.end()) {
        std::vector<std::string> reachablePorts;
        for (const auto& portId :
             hw_->getSwitchReachability(node->getSwitchId())) {
          auto port = state->getPorts()->getNodeIf(portId);
          if (port) {
            reachablePorts.push_back(port->getName());
          } else {
            XLOG(ERR) << "Port " << portId
                      << " in reachability matrix is not found in state";
          }
        }
        reachability.insert({node->getName(), std::move(reachablePorts)});
      }
    }
  }
  return;
}

} // namespace facebook::fboss
