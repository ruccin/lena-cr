/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
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
 * Author: Jaume Nin <jaume.nin@cttc.cat>
 */

#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
#include "ns3/trace-helper.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/lte-enb-net-device.h"
#include "ns3/lte-ue-net-device.h"
//#include "ns3/gtk-config-store.h"

using namespace ns3;

/**
 * Sample simulation script for LTE+EPC. It instantiates several eNodeB,
 * attaches one UE per eNodeB starts a flow for each UE to  and from a remote host.
 * It also  starts yet another flow between each UE pair.
 */

NS_LOG_COMPONENT_DEFINE ("EpcFirstExample");

int
main (int argc, char *argv[])
{

  uint16_t numberOfEnbNodes = 1;
  uint16_t numberOfUeNodes = 2;
  double simTime = 120;
  double distance = 4000.0;
  double interPacketInterval = 100;

  // Command line arguments
  CommandLine cmd;
  cmd.AddValue("numberOfNodes", "Number of eNodeBs", numberOfEnbNodes);
  cmd.AddValue("numberOfNodes", "Number of Ues", numberOfUeNodes);
  cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
  cmd.AddValue("distance", "Distance between eNBs [m]", distance);
  cmd.AddValue("interPacketInterval", "Inter packet interval [ms])", interPacketInterval);
  cmd.Parse(argc, argv);

  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);

  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults();

  // parse again so you can override default values from the command line
  cmd.Parse(argc, argv);

  Ptr<Node> pgw = epcHelper->GetPgwNode ();

   // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  // interface 0 is localhost, 1 is the p2p device
  //Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  NodeContainer ueNodes;
  NodeContainer enbNodes;
  enbNodes.Create(numberOfEnbNodes);
  ueNodes.Create(numberOfUeNodes.Get (0));
  staNodes.Create(numberOfUeNodes.Get (1));

  // Install Mobility Model
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (distance, 0.0, 0.0));
  positionAlloc->Add (Vector (distance * 0.161, 0.0, 0.0));
  positionAlloc->Add (Vector (distance * 0.334, 0.0, 0.0));

  MobilityHelper mobility;
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator(positionAlloc);
  mobility.Install(enbNodes);
  mobility.Install(ueNodes);
  mobility.Install(staNodes);

  // Install LTE Devices to the nodes
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);

  //Config::SetDefault ("ns3::LteEnbNetDevice::DlEarfcn", UintegerValue (255444));
  //Config::SetDefault ("ns3::LteUeNetDevice::DlEarfcn", UintegerValue (255444));
  Config::SetDefault ("ns3::LteEnbNetDevice::DlBandwidth", UintegerValue (5));
  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (35));
  Config::SetDefault ("ns3::LteUePhy::TxPower", DoubleValue (20));
  Config::SetDefault ("ns3::LteEnbPhy::NoiseFigure", DoubleValue (5.0));
  Config::SetDefault ("ns3::LteUePhy::NoiseFigure", DoubleValue (5.0));
  
  lteHelper->SetEnbAntennaModelType("ns3::IsotropicAntennaModel");
  lteHelper->SetEnbAntennaModelAttribute("Gain", DoubleValue(5));

  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
  
  // Assign IP address to UEs, and install applications
  Ptr<Node> ueNode = ueNodes.Get (0);
  // Set the default gateway for the UE
  Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
  ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
  
  // Attach one UE per eNodeB
  lteHelper->Attach (ueLteDevs.Get(0), enbLteDevs.Get(0));
  Ptr<NetDevice> ueDevice = ueLteDevs.Get (0);
  enum EpsBearer::Qci q = EpsBearer::GBR_NON_CONV_VIDEO;
  EpsBearer bearer (q);
  lteHelper->ActivateDedicatedEpsBearer (ueDevice, bearer, EpcTft::Default ());

  // Install and start applications on UEs and remote host
  uint16_t dlPort = 1234;

  ApplicationContainer clientApps;
  ApplicationContainer serverApps;

  BulkSendHelper clientbulk ("ns3::TcpSocketFactory", InetSocketAddress (ueIpIface.GetAddress (), dlPort));
  PacketSinkHelper serverbulk ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
      
  clientApps.Add (clientbulk.Install (remoteHost));
  clientbulk.SetAttribute ("SendSize", UintegerValue (2000));
  clientbulk.SetAttribute ("MaxBytes", UintegerValue (1000000000));

  serverApps.Add (serverbulk.Install (ueNodes.Get(0))); 
  
  serverApps.Start (Seconds (0.01));
  clientApps.Start (Seconds (0.01));

  lteHelper->EnableTraces ();

  // Uncomment to enable PCAP tracing
  //p2ph.EnablePcapAll("lena-epc-first");

  Simulator::Stop(Seconds(simTime));
  Simulator::Run();

  /*GtkConfigStore config;
  config.ConfigureAttributes();*/

  FlowMonitorHelper flowmo;
  Ptr<FlowMonitor> monitor;

  monitor = flowmo.Install (remoteHost);
  monitor = flowmo.Install (ueNodes);
  monitor->SerializeToXmlFile ("flowmon.xml", true, true);

  Simulator::Destroy();
  return 0;

}

