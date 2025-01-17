/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <gtest/gtest.h>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/PortStats.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/packet/IPv4Hdr.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/UDPHeader.h"

using namespace facebook::fboss;
using folly::IOBuf;
using folly::IPAddressV4;
using folly::IPAddressV6;
using folly::io::Cursor;

TEST(UDP, IPv4Checksum) {
  uint8_t body[] = {
      0x24, 0x02, 0x07, 0xe8, 0x00, 0x00, 0x00, 0x11, 0x00, 0x00, 0x07, 0x18,
      0x0a, 0x80, 0xe3, 0x2d, 0xd6, 0xa8, 0x0a, 0x66, 0x81, 0xdc, 0x83, 0x00,
      0xd6, 0xa8, 0x0c, 0xac, 0x8b, 0x63, 0x48, 0x4c, 0xd6, 0xa8, 0x0c, 0xac,
      0x94, 0x9a, 0x88, 0x95, 0xd6, 0xa8, 0x0c, 0xac, 0x94, 0x9b, 0xd1, 0xcf,
  };
  uint32_t bodyLength = sizeof(body) + UDPHeader::size();

  IPv4Hdr ip(
      IPAddressV4("10.78.75.41"),
      IPAddressV4("10.212.50.21"),
      static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP),
      bodyLength);
  ip.dscp = 0x10;
  ip.dontFragment = true;
  ip.ttl = 88;
  ip.computeChecksum();

  UDPHeader udp;
  udp.srcPort = 123;
  udp.dstPort = 123;
  udp.length = bodyLength;

  IOBuf bodyBuf(IOBuf::WRAP_BUFFER, body, sizeof(body));
  Cursor cursor(&bodyBuf);

  EXPECT_EQ(0x9001, ip.csum);
  EXPECT_EQ(0x6431, udp.computeChecksum(ip, cursor));
}

TEST(UDP, IPv4ChecksumZero) {
  uint8_t body[] = {
      0xa5, 0x7c, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
      0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
      0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
      0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b,
      0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36,
      0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
  };
  uint32_t bodyLength = sizeof(body) + UDPHeader::size();

  IPv4Hdr ip(
      IPAddressV4("10.5.172.23"),
      IPAddressV4("10.5.172.25"),
      static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP),
      bodyLength);
  ip.id = 0x3538;
  ip.dontFragment = true;
  ip.ttl = 96;
  ip.computeChecksum();

  UDPHeader udp;
  udp.srcPort = 1234;
  udp.dstPort = 1234;
  udp.length = bodyLength;

  IOBuf bodyBuf(IOBuf::WRAP_BUFFER, body, sizeof(body));
  Cursor cursor(&bodyBuf);

  EXPECT_EQ(0x791e, ip.csum);
  EXPECT_EQ(0xffff, udp.computeChecksum(ip, cursor));
}

TEST(UDP, IPv6Checksum) {
  uint8_t body[] = {
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
      0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
      0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
      0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b,
      0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36,
      0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
  };
  uint32_t bodyLength = sizeof(body) + UDPHeader::size();

  IPv6Hdr ip(
      6,
      0,
      0,
      72,
      static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP),
      64,
      IPAddressV6("2401:db00:20:702c:face:0:1:0"),
      IPAddressV6("2401:db00:20:702c:face:0:5:0"));

  UDPHeader udp;
  udp.srcPort = 1234;
  udp.dstPort = 1234;
  udp.length = bodyLength;

  IOBuf bodyBuf(IOBuf::WRAP_BUFFER, body, sizeof(body));
  Cursor cursor(&bodyBuf);

  EXPECT_EQ(0x3d76, udp.computeChecksum(ip, cursor));
}

TEST(UDP, IPv6ChecksumZero) {
  uint8_t body[] = {
      0x3d, 0x77, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
      0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
      0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
      0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b,
      0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36,
      0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
  };
  uint32_t bodyLength = sizeof(body) + UDPHeader::size();

  IPv6Hdr ip(
      6,
      0,
      0,
      72,
      static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP),
      64,
      IPAddressV6("2401:db00:20:702c:face:0:1:0"),
      IPAddressV6("2401:db00:20:702c:face:0:5:0"));

  UDPHeader udp;
  udp.srcPort = 1234;
  udp.dstPort = 1234;
  udp.length = bodyLength;

  IOBuf bodyBuf(IOBuf::WRAP_BUFFER, body, sizeof(body));
  Cursor cursor(&bodyBuf);

  EXPECT_EQ(0xffff, udp.computeChecksum(ip, cursor));
}

TEST(UDP, writeAndParse) {
  UDPHeader udp;
  udp.srcPort = 1234;
  udp.dstPort = 4321;
  udp.length = 1024;
  udp.csum = 4242;
  uint8_t udpSizeArr[UDPHeader::size()];
  folly::IOBuf buf(IOBuf::WRAP_BUFFER, udpSizeArr, sizeof(udpSizeArr));
  folly::io::RWPrivateCursor cursor(&buf);
  udp.write(&cursor);
  UDPHeader udp2;
  Cursor readCursor(&buf);
  udp2.parse(&readCursor, nullptr);
  EXPECT_EQ(udp, udp2);
}

TEST(UDP, parseError) {
  uint8_t udpSizeSmall[UDPHeader::size() - 1];
  folly::IOBuf buf(IOBuf::WRAP_BUFFER, udpSizeSmall, sizeof(udpSizeSmall));
  Cursor readCursor(&buf);
  SwitchStats swStats;
  PortStats portStats(PortID(1), "port1", &swStats);
  UDPHeader udp;
  EXPECT_THROW(udp.parse(&readCursor, &portStats), FbossError);
}
