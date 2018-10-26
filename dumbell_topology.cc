#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-dumbbell.h"
#include "ns3/ipv4-global-routing-helper.h"

#include<iostream>
using namespace ns3;

int main(int argc, char *argv[])
{
	CommandLine cmd;
  	cmd.Parse (argc, argv);

  	uint32_t left,right;

  	//Setting the properties of the point to point links in the bottleneck network
  	PointToPointHelper leftHelper,rightHelper,bottleneckHelper;
  	leftHelper.SetDeviceAttribute("DataRate",StringValue("5Mbps"));
  	leftHelper.SetChannelAttribute("Delay",StringValue("2ms"));

  	rightHelper.SetDeviceAttribute("DataRate",StringValue("3Mbps"));
  	rightHelper.SetChannelAttribute("Delay",StringValue("2ms"));

  	bottleneckHelper.SetDeviceAttribute("DataRate",StringValue("1Mbps"));
  	bottleneckHelper.SetChannelAttribute("Delay",StringValue("5ms"));

  	std::cout<<"Enter the number of nodes on left and right:";
  	std::cin>>left>>right;

  	PointToPointDumbbellHelper network1(left,leftHelper,right,rightHelper,bottleneckHelper);

  	//Installing stack on all the nodes
  	InternetStackHelper stack;
  	network1.InstallStack(stack);

  	//Assigining IPv4 addresses to all nodes
	Ipv4AddressHelper address1,address2,routerAddress;
	address1.SetBase ("10.1.1.0", "255.255.255.252");
  	address2.SetBase ("10.1.2.0", "255.255.255.252");
  	routerAddress.SetBase("10.1.3.0","255.255.255.252");

  	network1.AssignIpv4Addresses (address1,address2,routerAddress);

  	Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  	//Sending TCP packets in bulk

  	//Assigning the bulk sending source
  	std::cout<<network1.GetRightIpv4Address(1)<<std::endl;
  	BulkSendHelper source("ns3::TcpSocketFactory",InetSocketAddress (network1.GetRightIpv4Address(1), 9));
  	source.SetAttribute ("MaxBytes", UintegerValue (0));

  	//Installing the application on the source and setting its start and finish time
  	ApplicationContainer sourceApps = source.Install(network1.GetLeft(1));
  	sourceApps.Start (Seconds (1.0));
    sourceApps.Stop (Seconds (10.0));

    //Receiving the TCP packets
    PacketSinkHelper sink ("ns3::TcpSocketFactory",InetSocketAddress (Ipv4Address::GetAny (),9));
  	ApplicationContainer sinkApps = sink.Install (network1.GetRight(1));
  	sinkApps.Start (Seconds (0.0));
  	sinkApps.Stop (Seconds (10.0));

 
  	Simulator::Run();
  	Simulator::Destroy();
  	return 0;
}