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
#include "ns3/packet.h"
#include "ns3/socket.h"
#include "ns3/olsr-routing-protocol.h"
#include "ns3/olsr-helper.h"

using namespace ns3;

//
//
// staNode --- (wifi) --- ueNode --- (LTE) --- eNB --- pgw --- (p2p) --- remoteHost
//
//

/*
class TestApplication : public app
{
  public:
  TestApplication ();
  virtual ~TestApplication ();

  void ReceivedAtClient (Ptr<const Packet> p, Ptr<Ipv6> ipv6, uint32_t interface);
  
  void EnbToPgw (Ptr<Packet> p);

  private:
  virtual void DoRun (void);

  std::list<Ptr<Packet> > m_clientRxPkts;
  std::list<uint64_t> m_pgwUidRxFrmEnb;
};

TestApplication::~TestApplication ()
{
}

void ReceivedAtClient (Ptr<const Packet> p, Ptr<Ipv4> ipv4, uint32_t interface)
  {
    Ipv4Header ipv4Header;
    p->PeekHeader (ipv4Header);
    if (ipv4Header.GetNextHeader () == UdpL4Protocol::PROT_NUMBER)
    {
      m_clientRxPkts.push_back (p->Copy ());
    }
  }

void EnbToPgw (Ptr<Packet> p)
  {
    Ipv4Header ipv4Header;
    p->PeekHeader (ipv4Header);
    if (ipv4Header.GetNextHeader () == UdpL4Protocol::PROT_NUMBER)
    {
      m_pgwUidRxFrmEnb.push_back (p->GetUid ());
    }
  }
*/
/**
 * Sample simulation script for LTE+EPC. It instantiates several eNodeB,
 * attaches one UE per eNodeB starts a flow for each UE to  and from a remote host.
 * It also  starts yet another flow between each UE pair.
 */
void ReceivePacket (Ptr<Socket> socket)
{
  NS_LOG_UNCOND ("Received one packet");
}

static void GenerateTraffic (Ptr<Socket> socket, uint32_t pktSize, uint32_t pktCount, Time pktInterval)
{
  if (pktCount > 0)
  {
    socket->Send (Create<Packet> (pktSize));
    Simulator::Schedule (pktInterval, &GenerateTraffic, socket, pktSize, pktCount - 1, pktInterval);
  }
  else
  {
    socket->Close ();
  }
}

NS_LOG_COMPONENT_DEFINE ("EpcFirstExample");

int 
main (int argc, char *argv[])
{
  uint16_t numberOfEnbNodes = 1;
  uint16_t numberOfUeNodes = 1;
  uint16_t numberOfStaNodes = 1;
  uint32_t payloadSize = 1472;
  double simTime = 12;
  double distance = 60;
  double interval = 1.0;
  uint32_t packetSize = 1000; // bytes
  uint32_t numPackets = 1;
  //std::string phyRate = "HtMcs7";

  // Command line arguments
  CommandLine cmd;
  cmd.AddValue("numberOfNodes", "Number of eNodeBs", numberOfEnbNodes);
  cmd.AddValue("numberOfNodes", "Number of Ues", numberOfUeNodes);
  cmd.AddValue("numberOfNodes", "Number of STAs(Ue)", numberOfStaNodes);
  cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
  cmd.AddValue("distance", "Distance between eNBs [m]", distance);
  cmd.Parse(argc, argv);

  Time interPacketInterval = Seconds (interval);

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
  InternetStackHelper internet_olsr;

  // Create a olsr
  OlsrHelper olsr;
  olsr.ExcludeInterface (remoteHost, 2);
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
 
  Ipv4ListRoutingHelper list;
  list.Add (ipv4RoutingHelper, 0);
  list.Add (olsr, 10);

  internet_olsr.SetRoutingHelper (list);
  internet_olsr.Install (remoteHostContainer);
  //internet.Install (pgw);

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);

  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);

  NodeContainer ueNodes;
  NodeContainer enbNodes;
  NodeContainer staNodes;
  enbNodes.Create(numberOfEnbNodes);
  ueNodes.Create(numberOfUeNodes);
  staNodes.Create(numberOfStaNodes);

  // Install Mobility Model
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (distance * 60, 0.0, 0.0));
  positionAlloc->Add (Vector (distance * 20, 0.0, 0.0));
  positionAlloc->Add (Vector (distance, 0.0, 0.0));

  MobilityHelper mobility;
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator(positionAlloc);
  mobility.Install(enbNodes);
  mobility.Install(ueNodes);
  mobility.Install(staNodes);

//Start Set of Lte

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
  InternetStackHelper internet;
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));

  // Attach one UE per eNodeB
  lteHelper->Attach (ueLteDevs.Get (0), enbLteDevs.Get (0));
  Ptr<NetDevice> ueDevice = ueLteDevs.Get (0);
  enum EpsBearer::Qci q = EpsBearer::NGBR_VIDEO_TCP_DEFAULT;
  EpsBearer bearer (q);
  lteHelper->ActivateDedicatedEpsBearer (ueDevice, bearer, EpcTft::Default ());

//End Set of Lte

//Start Set of WiF
  Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",
                      StringValue ("DsssRate1Mbps"));

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
                                      "DataMode", StringValue ("DsssRate1Mbps"),
                                      "ControlMode", StringValue ("DsssRate1Mbps"));
  // Set of Ap
  Ssid ssid = Ssid ("network");
  wifiPhy.Set ("ChannelNumber", UintegerValue (36));
  wifiMac.SetType ("ns3::ApWifiMac",
                   "Ssid", SsidValue (ssid));
  
  NetDeviceContainer apDevices;
  apDevices = wifiHelper.Install (wifiPhy, wifiMac, ueNodes);

  // Set of Sta
  wifiMac.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (ssid),
                   "EnableBeaconJitter", BooleanValue (false));

  NetDeviceContainer staDevices;
  staDevices = wifiHelper.Install (wifiPhy, wifiMac, staNodes);

  internet_olsr.Install (staNodes);

  Ipv4AddressHelper ipv4w;
  ipv4w.SetBase ("3.0.0.0", "255.0.0.0");
  
  Ipv4InterfaceContainer staInterface;
  staInterface = ipv4w.Assign (staDevices);  
//End Set of Wifi

  // Interfaces
  // interface 0 is localhost, 1 is the p2p device
  //Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);
  //Ipv4Address ueAddr = ueIpIface.GetAddress (0);
  //Ipv4Address staAddr = staInterface.GetAddress (0);  

  // Assign IP address to UEs, and install applications
  Ptr<Node> ueNode = ueNodes.Get (0);
  //Ptr<Node> staNode = staNodes.Get (0);

  Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
  ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1, 0);
  //ueStaticRouting->AddNetworkRouteTo (Ipv4Address ("3.0.0.0"), Ipv4Mask ("255.0.0.0"), staAddr, 2);
  
  ApplicationContainer serverApps;
  ApplicationContainer clientApps;

  UdpServerHelper ueserver (80);
  serverApps.Add (ueserver.Install (ueNodes));

  UdpServerHelper rhserver (81);
  serverApps.Add (rhserver.Install (remoteHost));

  serverApps.Start (Seconds (1));
  serverApps.Stop (Seconds (simTime));

  UdpClientHelper wificlient (ueIpIface.GetAddress (0), 80);
  wificlient.SetAttribute ("MaxPackets", UintegerValue (4294967295u));
  wificlient.SetAttribute ("Interval", TimeValue (Time ("0.00002"))); //packets/s
  wificlient.SetAttribute ("PacketSize", UintegerValue (payloadSize));
  clientApps.Add (wificlient.Install (staNodes));

  clientApps.Start (Seconds (1));
  clientApps.Stop (Seconds (6.0));

  UdpClientHelper uclient (internetIpIfaces.GetAddress (1), 81);
  uclient.SetAttribute ("MaxPackets", UintegerValue (4294967295u));
  uclient.SetAttribute ("Interval", TimeValue (Time ("0.00002"))); //packets/s
  uclient.SetAttribute ("PacketSize", UintegerValue (payloadSize));
  clientApps.Add (uclient.Install (ueNodes));

  clientApps.Start (Seconds (6.1));
  clientApps.Stop (Seconds (simTime));

//

  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink = Socket::CreateSocket (ueNodes.Get (0), tid);
  InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink->Bind (local);
  recvSink->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> source = Socket::CreateSocket (staNodes.Get (0), tid);
  InetSocketAddress remote = InetSocketAddress (internetIpIfaces.GetAddress (1), 80);
  source->Connect (remote);

  Ptr<Ipv4> stack = remoteHost->GetObject<Ipv4> ();
  Ptr<Ipv4RoutingProtocol> rp_Gw = (stack->GetRoutingProtocol ());
  Ptr<Ipv4ListRouting> Irp_Gw = DynamicCast<Ipv4ListRouting>(rp_Gw);

  Ptr<olsr::RoutingProtocol> olsrrp_Gw;

  for (uint32_t i = 0; i < Irp_Gw->GetNRoutingProtocols (); i++)
  {
    int16_t priority;
    Ptr<Ipv4RoutingProtocol> temp = Irp_Gw->GetRoutingProtocol (i, priority);
    if (DynamicCast<olsr::RoutingProtocol> (temp))
    {
      olsrrp_Gw = DynamicCast<olsr::RoutingProtocol> (temp);
    }
  }

  Ptr<Ipv4StaticRouting> hnaEntries = Create<Ipv4StaticRouting> ();
  hnaEntries->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);
  olsrrp_Gw->SetRoutingTableAssociation (hnaEntries);
  
  uint64_t totalPacketsThrough = DynamicCast<UdpServer> (serverApps.Get (0))->GetReceived ();
  double throughput = totalPacketsThrough * payloadSize * 8 / (simTime * 1000000.0);
  std::cout << "Throughput: " << throughput << " Mbit/s" << '\n';

/*
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), staAddr, 2);

  Ptr<Ipv4StaticRouting> staStaticRouting = ipv4RoutingHelper.GetStaticRouting (staNode->GetObject<Ipv4> ());
  staStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), remoteHostAddr, 2);

  Ptr<Socket> staSocket = Socket::CreateSocket (staNode, TypeId::LookupByName ("ns3::UdpSocketFactory"));
  Ptr<Packet> stapacket = staSocket->Recv ();
  
  Ptr<EpcSgwPgwApplication> epcSgwPgwApp = EpcSgwPgwApplication::RecvFromTunDevice (stapacket, 
                                                                                    Ipv4Address ("3.0.0.0"), 
                                                                                    Ipv4Address ("1.0.0.0"), 
                                                                                    UdpL4Protocol::PROT_NUMBER);
  
  //Ptr<EpcSgwPgwApplication> epcSgwPgwApp = EpcSgwPgwApplication::RecvFromS1uSocket (staSocket);
  pgw->AddApplication (epcSgwPgwApp);
*/
/*
  ApplicationContainer serverApps;
  ApplicationContainer clientApps;

  PacketSinkHelper dlechoServer ("ns3::UdpSocketFactory", (InetSocketAddress (Ipv4Address::GetAny(), 10)));
  serverApps.Add (dlechoServer.Install (remoteHost));  

  UdpEchoClientHelper dlechoClient (remoteHostAddr, 10);
  dlechoClient.SetAttribute ("MaxPackets", UintegerValue (1000));
  dlechoClient.SetAttribute ("Interval", TimeValue (Seconds (0.2)));
  dlechoClient.SetAttribute ("PacketSize", UintegerValue (1024));
  clientApps.Add (dlechoClient.Install (staNodes.Get (0)));
  

  OnOffHelper dlechoClient ("ns3::UdpSocketFactory", Address(InetSocketAddress (Ipv4Address::GetAny(), 10)));
  dlechoClient.SetAttribute ("PacketSize", UintegerValue (1024));
  dlechoClient.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.352]"));
  dlechoClient.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0.652]"));
  dlechoClient.SetAttribute ("DataRate", DataRateValue (DataRate ("320kb/s")));
  clientApps1 = dlechoClient.Install (staNodes.Get(0));

  PacketSinkHelper ulechoServer ("ns3::UdpSocketFactory", (InetSocketAddress (Ipv4Address::GetAny(), 11)));
  serverApps.Add (ulechoServer.Install (staNodes.Get (0))); 

  UdpEchoClientHelper ulechoClient (staAddr, 11);
  ulechoClient.SetAttribute ("MaxPackets", UintegerValue (1000));
  ulechoClient.SetAttribute ("Interval", TimeValue (Seconds (0.2)));
  ulechoClient.SetAttribute ("PacketSize", UintegerValue (1024));
  clientApps.Add (ulechoClient.Install (remoteHost));

  serverApps.Start (Seconds (0.01));
  clientApps.Start (Seconds (0.01));
  */
/*
  Ptr<Ipv4L3Protocol> ipL3 = (staNodes.Get (0))->GetObject<Ipv4L3Protocol> ();
  ipL3->TraceConnectWithoutContext ("Rx", MakeCallback (&TestApplication::ReceivedAtClient));

  Ptr<Application> appPgw = pgw->GetApplication (0);
  appPgw->TraceConnectWithoutContext ("RxFromS1u", MakeCallback (&TestApplication::EnbToPgw));
*/
  //lteHelper->EnableMacTraces ();
  //lteHelper->EnableRlcTraces ();
  //wifiPhy.EnablePcap("lena-simple-epc-test", staDevices);
/*
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor;

  monitor = flowmon.InstallAll ();
  monitor->SerializeToXmlFile ("flowmo.xml", true, true);

  //Print per flow statistics
  monitor->CheckForLostPackets ();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();

  AsciiTraceHelper asciiTHFlow;
  Ptr<OutputStreamWrapper> flowStream = asciiTHFlow.CreateFileStream ("lte01.txt");
  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    {
          Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
          *flowStream->GetStream () << " Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")" << std::endl;
          *flowStream->GetStream () << " First packet time: " << i->second.timeFirstRxPacket << std::endl;
          *flowStream->GetStream () << " Tx Packets: " << i->second.txPackets << std::endl;
          *flowStream->GetStream () << " Rx Packets: " << i->second.rxPackets << std::endl;
          uint32_t dropes = 0;
           for (uint32_t reasonCode = 0; reasonCode < i->second.packetsDropped.size (); reasonCode++)
           {
        	   dropes+= i->second.packetsDropped[reasonCode];
           }
           *flowStream->GetStream () << " Drop packets: " << dropes << std::endl;
    }

  Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (serverApps.Get (0));
  *flowStream->GetStream () << "Total Bytes Received by sink packet #" << sink1->GetTotalRx () << std::endl;
  std::cout << "Total Bytes Received by sink packet #" << sink1->GetTotalRx () << std::endl;
*/
  wifiPhy.EnablePcap ("olsr-hna-sta", staDevices);
  wifiPhy.EnablePcap ("olsr-hna-ap", apDevices);

  Simulator::ScheduleWithContext (source->GetNode ()->GetId (), Seconds (20.0), &GenerateTraffic, source, packetSize, numPackets, interPacketInterval);
  Simulator::Stop(Seconds(simTime));
  //Simulator::Schedule(Seconds(1.0), monitor);
  Simulator::Run();
  Simulator::Destroy();
  return 0;

}

