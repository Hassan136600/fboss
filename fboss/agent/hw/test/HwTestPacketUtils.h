/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <memory>
#include <optional>
#include <random>
#include <type_traits>
#include <vector>

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include <folly/MacAddress.h>
#include <folly/io/Cursor.h>

#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {
class HwSwitch;
class SwitchState;
} // namespace facebook::fboss

namespace facebook::fboss::utility {

folly::MacAddress getInterfaceMac(
    const std::shared_ptr<SwitchState>& state,
    VlanID vlan);
folly::MacAddress getInterfaceMac(
    const std::shared_ptr<SwitchState>& state,
    InterfaceID intf);
folly::MacAddress getFirstInterfaceMac(const cfg::SwitchConfig& cfg);
folly::MacAddress getFirstInterfaceMac(
    const std::shared_ptr<SwitchState>& state);
std::optional<VlanID> firstVlanID(const cfg::SwitchConfig& cfg);
std::optional<VlanID> firstVlanID(const std::shared_ptr<SwitchState>& state);
VlanID getIngressVlan(const std::shared_ptr<SwitchState>& state, PortID port);

std::unique_ptr<facebook::fboss::TxPacket> makeLLDPPacket(
    const HwSwitch* hw,
    const folly::MacAddress srcMac,
    std::optional<VlanID> vlanid,
    const std::string& hostname,
    const std::string& portname,
    const std::string& portdesc,
    const uint16_t ttl,
    const uint16_t capabilities);

void sendTcpPkts(
    facebook::fboss::HwSwitch* hwSwitch,
    int numPktsToSend,
    std::optional<VlanID> vlanId,
    folly::MacAddress dstMac,
    const folly::IPAddress& dstIpAddress,
    int l4SrcPort,
    int l4DstPort,
    PortID outPort,
    uint8_t trafficClass = 0,
    std::optional<std::vector<uint8_t>> payload = std::nullopt);

bool isPtpEventPacket(folly::io::Cursor& cursor);
uint8_t getIpHopLimit(folly::io::Cursor& cursor);

struct RandomNumberGenerator {
  RandomNumberGenerator(int seed, uint64_t begin, uint64_t end)
      : generator(seed), distibution(begin, end) {}
  auto operator()() {
    return distibution(generator);
  }

  std::mt19937_64 generator;
  std::uniform_int_distribution<> distibution;
};

} // namespace facebook::fboss::utility
