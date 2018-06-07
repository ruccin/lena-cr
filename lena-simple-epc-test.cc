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
#include "ns3/ipv4-flow-classifier.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/lte-enb-phy.h"
#include "ns3/lte-enb-mac.h"
#include "ns3/lte-ue-phy.h"
#include "ns3/lte-ue-mac.h"
#include "ns3/lte-enb-net-device.h"
#include "ns3/lte-ue-net-device.h"
#include "ns3/lte-enb-rrc.h"
#include "ns3/lte-ue-rrc.h"
#include "ns3/lte-common.h"
#include "ns3/trace-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/wifi-module.h"

using namespace ns3;

/**
 * Sample simulation script for LTE+EPC. It instantiates several eNodeB,
 * attaches one UE per eNodeB starts a flow for each UE to  and from a remote host.
 * It also  starts yet another flow between each UE pair.
 */

NS_LOG_COMPONENT_DEFINE ("EpcFirstExample");
/*
// FlowMonitor
  void PrintStats(Ptr<FlowMonitor> monitor){
    double rxBD = 0;
    std::ofstream THROUGHPUT;

    FlowMonitorHelper flowmonitor;
    monitor->CheckForLostPackets(Time(0.001));

    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmonitor.GetClassifier());
    std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin(); iter != stats.end(); ++iter){
      
      if (iter->first == 1){

        double Throughput = (iter->second.rxBytes-rxBD) * 8.0 / 1024 / 1024;
        THROUGHPUT<<Throughput<<"\n";
        rxBD = iter->second.rxBytes;

        std::cout<<"THROUGHPUT"<<Throughput<<"\n"<<std::endl;
      }
    }
  Simulator::Schedule(Seconds(1.0), &PrintStats, monitor);
  
  return;
  }
*/

int
main (int argc, char *argv[])
{
  uint16_t dlPort = 1234;
  uint16_t numberOfEnbNodes = 1;
  uint16_t numberOfUeNodes = 1;
  uint16_t numberOfStaNodes = 1;
  uint32_t payloadSize = 1472;
  double simTime = 30;
  double distance = 4000.0;
  std::string phyRate = "HtMcs7";

  // Command line arguments
  CommandLine cmd;
  cmd.AddValue("numberOfNodes", "Number of eNodeBs", numberOfEnbNodes);
  cmd.AddValue("numberOfNodes", "Number of Ues", numberOfUeNodes);
  cmd.AddValue("numberOfNodes", "Number of STAs(Ue)", numberOfStaNodes);
  cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
  cmd.AddValue("distance", "Distance between eNBs [m]", distance);
  cmd.Parse(argc, argv);

  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
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
  Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), Ipv4Address ("3.0.0.0"), 1);

  NodeContainer ueNodes;
  NodeContainer enbNodes;
  NodeContainer staNodes;
  enbNodes.Create(numberOfEnbNodes);
  ueNodes.Create(numberOfUeNodes);
  staNodes.Create(numberOfStaNodes);

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

  // LTE configuration parametes
  lteHelper->SetSchedulerType ("ns3::PfFfMacScheduler");
  lteHelper->SetSchedulerAttribute ("UlCqiFilter", EnumValue (FfMacScheduler::PUSCH_UL_CQI));
  
  // LTE-U DL transmission @5180 MHz
  //lteHelper->SetEnbDeviceAttribute ("DlEarfcn", UintegerValue (255444));
  lteHelper->SetEnbDeviceAttribute ("DlBandwidth", UintegerValue (6));
  
  // needed for initial cell search
  //lteHelper->SetUeDeviceAttribute ("DlEarfcn", UintegerValue (255444));
  
  // LTE calibration
  Ptr<LteEnbPhy> enbPhy = enbLteDevs.Get(0)->GetObject<LteEnbNetDevice>()->GetPhy();
  enbPhy->SetTxPower (35);
  enbPhy->SetAttribute ("NoiseFigure", DoubleValue (5.0));

  Ptr<LteUePhy> uePhy = ueLteDevs.Get(0)->GetObject<LteUeNetDevice>()->GetPhy();
  uePhy->SetTxPower (20);
  uePhy->SetAttribute ("NoiseFigure", DoubleValue (9.0));
  
  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
  Ipv4Address ueAddr = ueIpIface.GetAddress (1);

  // Assign IP address to UEs, and install applications
  Ptr<Node> ueNode = ueNodes.Get (0);
  // Set the default gateway for the UE
  Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
  ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);

  // Attach one UE per eNodeB
  lteHelper->Attach (ueLteDevs.Get(0), enbLteDevs.Get(0));
  Ptr<NetDevice> ueDevice = ueLteDevs.Get (0);
  enum EpsBearer::Qci q = EpsBearer::GBR_GAMING;
  EpsBearer bearer (q);
  lteHelper->ActivateDedicatedEpsBearer (ueDevice, bearer, EpcTft::Default ());

  // WiFi
  WifiHelper wifiHelper;
  wifiHelper.SetStandard (WIFI_PHY_STANDARD_80211n_5GHZ);
  WifiMacHelper wifiMac;

  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel", "Frequency", DoubleValue (5e9));

  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create());
  wifiPhy.Set ("TxPowerStart", DoubleValue (10.0));
  wifiPhy.Set ("TxPowerEnd", DoubleValue (10.0));
  wifiPhy.Set ("TxPowerLevels", UintegerValue (1));
  wifiPhy.Set ("TxGain", DoubleValue (0));
  wifiPhy.Set ("RxGain", DoubleValue (0));
  wifiPhy.Set ("RxNoiseFigure", DoubleValue (10));
  wifiPhy.Set ("CcaMode1Threshold", DoubleValue (-79));
  wifiPhy.Set ("EnergyDetectionThreshold", DoubleValue (-79 + 3));
  wifiPhy.SetErrorRateModel ("ns3::YansErrorRateModel");
  wifiHelper.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                      "DataMode", StringValue (phyRate),
                                      "ControlMode", StringValue ("HtMcs0"));
  // Set of Ap
  Ssid ssid = Ssid ("network");
  wifiMac.SetType ("ns3::ApWifiMac",
                   "Ssid", SsidValue (ssid));
  
  NetDeviceContainer apDevices;
  apDevices = wifiHelper.Install (wifiPhy, wifiMac, ueNodes);

  // Set of Sta
  wifiMac.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (ssid));

  NetDeviceContainer staDevices;
  staDevices = wifiHelper.Install (wifiPhy, wifiMac, staNodes);

  internet.Install (staNodes);
  ipv4h.SetBase ("3.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer staInterface;
  staInterface = ipv4h.Assign (staDevices);  
  Ipv4Address staAddr = staInterface.GetAddress (1);
/*
  Ptr<Packet> packet = Create<Packet> (payloadSize);

  Ptr<EpcSgwPgwApplication> epcsgwpgwapp = CreateObject<EpcSgwPgwApplication> ();
  epcsgwpgwapp.RecvFromTunDevice (packet, Ipv4Address ("3.0.0.1"), Ipv4Address ("1.0.0.2"), 1);
  remoteHost->AddApplication (epcsgwpgwapp);
*/
  Ptr<Node> staNode = staNodes.Get (0);
  Ptr<Ipv4StaticRouting> staStaticRouting = ipv4RoutingHelper.GetStaticRouting (staNode->GetObject<Ipv4> ());
  staStaticRouting->AddHostRouteTo (Ipv4Address ("1.0.0.2"), Ipv4Address ("3.0.0.1"), 1);
 
  Ptr<Ipv4StaticRouting> apStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
  apStaticRouting->AddHostRouteTo (Ipv4Address ("1.0.0.2"), Ipv4Address ("7.0.0.1"), 1);  

  Ptr<Ipv4StaticRouting> rhStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  rhStaticRouting->AddHostRouteTo (Ipv4Address ("1.0.0.2"), Ipv4Address ("1.0.0.2"), 1);

  // Install and start applications on UEs and remote host
  ApplicationContainer clientApps;
  ApplicationContainer clientApps2;
  ApplicationContainer serverApps;

  BulkSendHelper server ("ns3::TcpSocketFactory", (InetSocketAddress (ueAddr, dlPort)));
  serverApps.Add (server.Install (remoteHost));
  server.SetAttribute ("SendSize", UintegerValue (1024));
  server.SetAttribute ("MaxBytes", UintegerValue (1000000000));

  BulkSendHelper client2 ("ns3::TcpSocketFactory", (InetSocketAddress (staAddr, dlPort)));
  clientApps2.Add (client2.Install (ueNodes.Get(0)));
  client2.SetAttribute ("SendSize", UintegerValue (1024));
  client2.SetAttribute ("MaxBytes", UintegerValue (1000000000));

  OnOffHelper client ("ns3::TcpSocketFactory", (InetSocketAddress (ueAddr, dlPort)));
  client.SetAttribute ("PacketSize", UintegerValue (payloadSize));
  client.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  client.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  client.SetAttribute ("PacketSize", UintegerValue (1024));
  client.SetAttribute ("MaxBytes", UintegerValue (1000000000));
  client.SetAttribute ("DataRate", DataRateValue (DataRate ("100Mb/s")));
  clientApps.Add (client.Install (staNodes.Get(0))); 
  
  serverApps.Start (Seconds (0.01));
  clientApps.Start (Seconds (0.01));

  double statsStartTime = 0.04; // need to allow for RRC connection establishment + SRS
  double statsDuration = 1.0;

  //lteHelper->EnableTraces ();
  wifiPhy.EnablePcap("lena-simple-epc-test", staDevices);
  
  lteHelper->EnableRlcTraces ();
  Ptr<RadioBearerStatsCalculator> rlcStats = lteHelper->GetRlcStats ();
  rlcStats->SetAttribute ("StartTime", TimeValue (Seconds (statsStartTime)));
  rlcStats->SetAttribute ("EpochDuration", TimeValue (Seconds (statsDuration)));

  FlowMonitorHelper flowmonitor;
  Ptr<FlowMonitor> monitor;

  monitor = flowmonitor.InstallAll ();
  
  monitor->SerializeToXmlFile ("flowmo.xml", true, true);

  Simulator::Stop(Seconds(simTime));
  //Simulator::Schedule(Seconds(1.0), monitor);
  
  
  Simulator::Run();
  Simulator::Destroy();
  return 0;

}

