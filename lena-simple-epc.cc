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
#include "ns3/flow-monitor-module.h"
#include "ns3/wifi-module.h"
#include "ns3/csma-module.h"
#include "ns3/olsr-helper.h"
#include "ns3/olsr-routing-protocol.h"
#include "ns3/bridge-module.h"

using namespace ns3;

/**
 * Sample simulation script for LTE+EPC. It instantiates several eNodeB,
 * attaches one UE per eNodeB starts a flow for each UE to  and from a remote host.
 * It also  starts yet another flow between each UE pair.
 */

NS_LOG_COMPONENT_DEFINE ("EpcFirstExample");

Ptr<PacketSink> sink;

void
GetTotalRx ()
{
  Time now = Simulator::Now ();
  double totalrx = sink->GetTotalRx () * 8;
  double totalthroughput = totalrx / now.GetSeconds ();
  std::cout << now.GetSeconds () << "s: \t" << "total RX :" << totalrx << " " << "total Throughput :" << totalthroughput << " Mbit/s" << std::endl;
  Simulator::Schedule (MilliSeconds (100), &GetTotalRx);
}

int
main (int argc, char *argv[])
{
  uint32_t payloadSize = 1000;                       /* Transport layer payload size in bytes. */
  std::string dataRate = "100Mbps"; 
  std::string tcpVariant = "TcpNewReno";             /* TCP variant type. */
  uint16_t numberOfenbNodes = 1;
  uint16_t numberOfueNodes = 1;
  double simTime = 10;
  double interPacketInterval = 100;

  // Command line arguments
  CommandLine cmd;
  cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
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
  p2ph.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (100)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  // interface 0 is localhost, 1 is the p2p device
  Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  // LTE configuration parametes
  //lteHelper->SetSchedulerType ("ns3::PfFfMacScheduler");
  //lteHelper->SetSchedulerAttribute ("UlCqiFilter", EnumValue (FfMacScheduler::PUSCH_UL_CQI));
  // LTE-U DL transmission @5180 MHz
  //lteHelper->SetEnbDeviceAttribute ("DlEarfcn", UintegerValue (255444));
  // needed for initial cell search
  //lteHelper->SetUeDeviceAttribute ("DlEarfcn", UintegerValue (255444));
  // LTE calibration
  lteHelper->SetEnbAntennaModelType ("ns3::IsotropicAntennaModel");
  //lteHelper->SetEnbAntennaModelAttribute ("Gain", DoubleValue (5.0));
  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (35.0));
  Config::SetDefault ("ns3::LteUePhy::TxPower", DoubleValue (20.0));

  // Set Path loss model
  lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::FriisPropagationLossModel"));
  //lteHelper->SetPathlossModelAttribute ("ReferenceDistance", DoubleValue (400));
  //lteHelper->SetPathlossModelAttribute ("Frequency", DoubleValue (5.));

  NodeContainer ueNodes;
  NodeContainer enbNodes;
  enbNodes.Create(numberOfenbNodes);
  ueNodes.Create(numberOfueNodes);

  // Install Mobility Model
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector(4010, 0, 0));
  positionAlloc->Add (Vector(10, 0, 0));

  MobilityHelper mobility;
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator(positionAlloc);
  mobility.Install(enbNodes);
  mobility.Install(ueNodes);

  // Install LTE Devices to the nodes
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);

  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
  // Assign IP address to UEs, and install applications
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  // Attach one UE per eNodeB
  lteHelper->Attach (ueLteDevs.Get(0), enbLteDevs.Get(0));
        // side effect: the default EPS bearer will be activated

//
//
//
//
//
  NodeContainer wifiNodes;
  wifiNodes.Create (2);
  Ptr<Node> wifiNode = wifiNodes.Get ();

  OlsrHelper olsr;
  Ipv4StaticRoutingHelper staticRouting;
  Ipv4ListRoutingHelper list;
  list.Add (staticRouting, 0);
  list.Add (olsr, 10);

  InternetStackHelper internet_olsr;
  internet_olsr.SetRoutingHelper (list);
  internet_olsr.Install (wifiNode.Get (0));
  internet_olsr.Install (wifiNode.Get (1));
  internet_olsr.Install (pgw);
  internet_olsr.Install (remoteHostContainer);

  CsmaHelper csmaHelper;
  csmaHelper.SetChannelAttribute ("DataRate", DataRateValue (DataRate ("100Mbps")));
  csmaHelper.SetChannelAttribute ("Delay", TimeValue (MicroSeconds (100)));
  //csmaHelper.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  csmaHelper.SetDeviceAttribute ("EncapsulationMode", StringValue ("Mac"));

  NodeContainer csmaContainer;
  csmaContainer.Add (wifiNode.Get (1));
  csmaContainer.Add (pgw);

  NetDeviceContainer csmaDevs = csmaHelper.Install (csmaContainer);
  ipv4h.SetBase ("2.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer csmaIpIface = ipv4h.Assign (csmaDevs);

  WifiMacHelper wifiMac;
  WifiHelper wifiHelper;
  wifiHelper.SetStandard (WIFI_PHY_STANDARD_80211ac);

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel");

  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  wifiPhy.Set ("TxPowerStart", DoubleValue (23.0));
  wifiPhy.Set ("TxPowerEnd", DoubleValue (23.0));
  wifiPhy.Set ("TxPowerLevels", UintegerValue (2));
  wifiPhy.Set ("TxGain", DoubleValue (0));
  wifiPhy.Set ("RxGain", DoubleValue (0));
  wifiPhy.Set ("RxNoiseFigure", DoubleValue (10));
  wifiPhy.Set ("CcaMode1Threshold", DoubleValue (-64.8));
  wifiPhy.Set ("EnergyDetectionThreshold", DoubleValue (-61.8));
  wifiPhy.SetErrorRateModel ("ns3::YansErrorRateModel");
  wifiHelper.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                      "DataMode", StringValue ("VhtMcs9"),
                                      "ControlMode", StringValue ("VhtMcs0"));
  
  /* Configure AP */
  Ssid ssid = Ssid ("network");
  wifiMac.SetType ("ns3::ApWifiMac",
                   "Ssid", SsidValue (ssid));

  Ptr<NetDevice> apDevice = (wifiHelper.Install (wifiPhy, wifiMac, wifiNode.Get (1))).Get (0);

  /* Configure STA */
  wifiMac.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (ssid));

  Ptr<NetDevice> staDevices = (wifiHelper.Install (wifiPhy, wifiMac, wifiNode.Get (0))).Get (0);

  BridgeHelper bridgeHelper;
  Ptr<NetDevice> wifiApBrDev = (bridgeHelper.Install (wifiNode.Get (1), NetDeviceContainer (apDevice, csmaDevs.Get (1)))).Get (0);

  MobilityHelper mobility2;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (2010.0, 0.0, 0.0));
  positionAlloc->Add (Vector (10.0, 10.0, 0.0));

  mobility2.SetPositionAllocator (positionAlloc);
  mobility2.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility2.Install (wifiNode.Get (1));  
  mobility2.Install (wifiNode.Get (0));

 /* Internet stack */
  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer apInterface;
  apInterface = address.Assign (apDevice);
  Ipv4InterfaceContainer staInterface;
  staInterface = address.Assign (staDevices);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> staStaticRouting = ipv4RoutingHelper.GetStaticRouting (wifiNode.Get (1)->GetObject<Ipv4> ());
  staStaticRouting->AddHostRouteTo (internetDevices.GetAddress (1), Ipv4Address ("10.0.0.1"), 1);

  Ptr<Ipv4StaticRouting> apStaticRouting = ipv4RoutingHelper.GetStaticRouting (wifiNode.Get (0)->GetObject<Ipv4> ());
  apStaticRouting->AddHostRouteTo (internetDevices.GetAddress (1), Ipv4Address("2.0.0.1"), 1);

  Ptr<Ipv4StaticRouting> PgwStaticRouting = ipv4RoutingHelper.GetStaticRouting (pgw->GetObject<Ipv4> ());
  PgwStaticRouting->AddHostRouteTo (internetDevices.GetAddress (1), internetDevices.GetAddress (0), 1);


  uint16_t dlPort = 1234;
  ApplicationContainer serverApps;
  ApplicationContainer clientApps;


  PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
  serverApps.Add (dlPacketSinkHelper.Install (remoteHost));
  sink = StaticCast<PacketSink> (serverApps.Get (0));

  OnOffHelper client1 ("ns3::UdpSocketFactory", (InetSocketAddress (remoteHostAddr, dlPort)));
  client1.SetAttribute ("PacketSize", UintegerValue (payloadSize));
  client1.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  client1.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  client1.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
  clientApps.Add (client1.Install (ueNodes));

  OnOffHelper client2 ("ns3::UdpSocketFactory", (InetSocketAddress (remoteHostAddr, dlPort)));
  client2.SetAttribute ("PacketSize", UintegerValue (payloadSize));
  client2.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  client2.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  client2.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
  clientApps.Add (client2.Install (wifiNode.Get (0)));

  serverApps.Start (Seconds (0.0));
  clientApps.Start (Seconds (0.0));
  //Simulator::Schedule (MilliSeconds (100), &GetTotalRx);

  lteHelper->EnableTraces ();

  // Uncomment to enable PCAP tracing
  //p2ph.EnablePcapAll("lena-epc-first");

  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

  Simulator::Stop(Seconds(simTime + 1));
  Simulator::Run();

  /*GtkConfigStore config;
  config.ConfigureAttributes();*/

  monitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
      // first 2 FlowIds are for ECHO apps, we don't want to display them
      //
      // Duration for throughput measurement is 9.0 seconds, since
      //   StartTime of the OnOffApplication is at about "second 1"
      // and
      //   Simulator::Stops at "second 10".
          Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
          std::cout << "Flow " << i->first - 2 << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
          std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
          std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
          std::cout << "  Throughput:  " << i->second.txBytes * 8.0 / 9.0 / 1000 / 1000  << " Mbps\n";
          std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
    }


  Simulator::Destroy();
  return 0;

}

