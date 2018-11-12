/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/netanim-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-layout-module.h"
#include <crypto++/des.h>
#include <crypto++/modes.h>
#include <crypto++/osrng.h>
#include <crypto++/hex.h>

// Network topology (default)
//
//        n2 n3 n4              .
//         \ | /                .
//          \|/                 .
//     n1--- n0---n5            .
//          /|\                 .
//         / | \                .
//        n8 n7 n6              .
//


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Star");

int 
main (int argc, char *argv[])
{

  //
  // Set up some default values for the simulation.
  //
  Config::SetDefault ("ns3::OnOffApplication::PacketSize", UintegerValue (137));

  // ??? try and stick 15kb/s into the data rate
  Config::SetDefault ("ns3::OnOffApplication::DataRate", StringValue ("14kb/s"));

  //
  // Default number of nodes in the star.  Overridable by command line argument.
  //
  uint32_t nSpokes = 8;

  CommandLine cmd;
  cmd.AddValue ("nSpokes", "Number of nodes to place in the star", nSpokes);
  cmd.Parse (argc, argv);

  NS_LOG_INFO ("Build star topology.");
  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
  PointToPointStarHelper star (nSpokes, pointToPoint);

  NS_LOG_INFO ("Install internet stack on all nodes.");
  InternetStackHelper internet;
  star.InstallStack (internet);

  NS_LOG_INFO ("Assign IP Addresses.");
  star.AssignIpv4Addresses (Ipv4AddressHelper ("10.1.1.0", "255.255.255.0"));

  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps = echoServer.Install (star.GetHub());

  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  UdpEchoClientHelper echoClient (star.GetHubIpv4Address (0), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer spokeApps;
  for (uint32_t i = 0; i < star.SpokeCount (); ++i)
  {
      spokeApps.Add (echoClient.Install (star.GetSpokeNode (i)));
  }
  spokeApps.Start (Seconds (1.0));
  spokeApps.Stop (Seconds (10.0));

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  CryptoPP::AutoSeededRandomPool prng;

  CryptoPP::SecByteBlock key(0x00, CryptoPP::DES_EDE2::DEFAULT_KEYLENGTH);
  prng.GenerateBlock(key, key.size());

  byte iv[CryptoPP::DES_EDE2::BLOCKSIZE];
  prng.GenerateBlock(iv, sizeof(iv));

  std::string plain = "Hello World";
  std::string cipher, encoded;
  
  CryptoPP::CBC_Mode<CryptoPP::DES_EDE2 >::Encryption e;
  e.SetKeyWithIV(key, key.size(), iv);

  CryptoPP::StringSource ss1(plain, true, new CryptoPP::StreamTransformationFilter(e, new CryptoPP::StringSink(cipher)));
  CryptoPP::StringSource ss2(cipher, true, new CryptoPP::HexEncoder(new CryptoPP::StringSink(encoded)));

  uint32_t nApplications = spokeApps.GetN ();
  for (uint32_t i = 0;i < nApplications; ++i)
  {
    Ptr<Application> p = spokeApps.Get (i);
    echoClient.SetFill (p, encoded);
  }

  pointToPoint.EnablePcapAll ("3des-star");
  
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
