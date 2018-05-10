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
#include "ns3/flow-monitor-helper.h"
#include "ns3/ipv4-flow-classifier.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/lte-enb-phy.h"
#include "ns3/lte-enb-mac.h"
#include "ns3/lte-ue-phy.h"
#include "ns3/lte-ue-mac.h"
#include "ns3/ff-mac-scheduler.h"
#include "ns3/pf-ff-mac-scheduler.h"
#include "ns3/lte-enb-net-device.h"
#include "ns3/lte-ue-net-device.h"
#include "ns3/friis-spectrum-propagation-loss.h"
#include "ns3/lte-enb-rrc.h"
#include "ns3/lte-ue-rrc.h"
#include "ns3/lte-common.h"
#include "ns3/trace-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/wifi-module.h"
#include "ns3/scenario-helper.h"

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("lena-cr-test");

int
main (int argc, char *argv[])
{

  uint16_t numberOfUENodes = 1;
  uint16_t numberOfeNBNodes = 1;
  //uint16_t wifi_ap = 1;
  //uint16_t wifi_sta = 1;
  double simTime = 60;
  double distance = 4000;
  double PoweNB = 35;
  //double Powuav = 25;
  double Powue = 20;
  uint8_t mimo = 2;

  // Command line arguments
  CommandLine cmd;
  cmd.AddValue("numberOfUENodes", "Number of UE Nodes", numberOfUENodes);
  cmd.AddValue("numberOfeNBNodes", "Number of eNB Nodes", numberOfeNBNodes);
  cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
  cmd.AddValue("distance", "Distance between eNB and UE [m]", distance);
  cmd.Parse(argc, argv);

  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults();

  // parse again so you can override default values from the command line
  cmd.Parse(argc, argv);

  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);
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
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);

  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);
 
  NodeContainer ueNodes;
  NodeContainer enbNodes;
  enbNodes.Create(numberOfeNBNodes);
  ueNodes.Create(numberOfUENodes);
/*
  // Set of WiFi
  NodeContainer wifiApNode;
  wifiApNode.Create (wifi_sta);

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create());

  WifiHelper wifi = WifiHelper::Default ();
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  NqosWaveMacHelper mac = NqosWaveMacHelper::Default ();

  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
              "Ssid", SsidValue (ssid),
              "ActiveProbing", BooleanValue (false));
*/
/*
  // Position of eNB
  Ptr<ListPositionAllocator> enbpositionAlloc = CreateObject<ListPositionAllocator> ();
  enbpositionAlloc->Add (Vector (distance, 0.0, 0.0));
  
  MobilityHelper enbMobility;
  enbMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  enbMobility.SetPositionAllocator (enbpositionAlloc);
  enbMobility.Install (enbNodes);

  std::cout << "Set of eNB Position" << std::endl;

  // Position of UE
  Ptr<ListPositionAllocator> uepositionAlloc = CreateObject<ListPositionAllocator> ();
  uepositionAlloc->Add (Vector (distance * 0.161, 0.0, 0.0));

  MobilityHelper uemobility;
  uemobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  uemobility.SetPositionAllocator (uepositionAlloc);
  uemobility.Install (ueNodes);
*/
  MobilityHeelper mobilityBs;

  mobilityBs.SetPositionAllocator ("ns3::GridPositionAllocator",
                                   "MinX", DoubleValue (20),
                                   "MinY", DoubleValue (5),
                                   "DeltaX", DoubleValue (25),
                                   "LayoutType", StringValue ("RowFirst"));
  mobilityBs.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityBs.Install (enbNodes);

  mobilityBs.SetPositionAllocator ("ns3::GridPositionAllocator",
                                   "MinX", DoubleValue (100),
                                   "MinY", DoubleValue (10),
                                   "DeltaX", DoubleValue (25),
                                   "LayoutType", StringValue ("RowFirst"));
  mobilityBs.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobilityBs.Install (ueNodes);

  std::cout << "Set of UE Position" << std::endl;
/*
  // Position of WiFi
  Ptr<ListPositionAllocator> uavpositionAlloc = CreateObject<ListPositionAllocator> ();

  for (uint16_t j = 0; j < numberOfUAVNodes; j++)
  {
    uavpositionAlloc->Add (Vector (distance * 0.334, 0.0, 0.0));
  }

  MobilityHelper apmobility;
  apmobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  apmobility.SetPositionAllocator (uavpositionAlloc);
  apmobility.Install (wifi_sta);

  std::cout << "Set of UE Position" << std::endl;
*/
  // Set of Antenna and Bandwidth
  lteHelper->SetEnbAntennaModelType("ns3::IsotropicAntennaModel");
  lteHelper->SetEnbDeviceAttribute("DlBandwidth", UintegerValue(50));
  lteHelper->SetEnbDeviceAttribute("UlBandwidth", UintegerValue(50));
  lteHelper->SetUeAntennaModelType("ns3::IsotropicAntennaModel");

  std::cout << "Set of Antenna and Bandwidth" << std::endl;

  // Configuration MIMO
  Config::SetDefault("ns3::LteEnbRrc::DefaultTransmissionMode", UintegerValue(mimo));
  Config::SetDefault("ns3::LteAmc::AmcModel", EnumValue(LteAmc::PiroEW2010));
  Config::SetDefault("ns3::LteAmc::Ber", DoubleValue(0.00005));

  std::cout << "Configuration MIMO" << std::endl;

  // Install LTE Devices to the nodes
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);
/*
  // Install WiFi Devices to the nodes
  NetDeviceContainer wifiApDevs;

  wifiApDevs = wifi.Install (phy, mac, wifi_sta);
*/ 

/*  
  // Set of Scheduler
  lteHelper->SetSchedulerAttribute ("UlCqiFilter", EnumValue (FfMacScheduler::PUSCH_UL_CQI));
  
  std::cout << "Set of Scheduler" << std::endl;

  // Set of Pathloss Model
  lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::LogDistancePropagationLossModel"));

  std::cout << "Set of Pathloss Model" << std::endl;
*/
  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
 
  // Assign IP address to UEs, and install applications
  Ptr<Node> ueNode = ueNodes.Get (0);
  Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4>());
  ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);

  std::cout << "Install the IP Stack and Assign IP address to UEs" << std::endl;

  // Attach one UE per eNodeB
  lteHelper->Attach (ueLteDevs.Get(0), enbLteDevs.Get(0));

  // Activate EpsBearer
  lteHelper->ActivateDedicatedEpsBearer (ueLteDevs, EpsBearer::NGBR_VIDEO_TCP_OPERATOR, EpcTft::Default());

  std::cout << "Attach UE per eNB and Activate EpsBearer" << std::endl;

  // Install and start applications on UE and remote host
  uint16_t dlPort = 20;
  ApplicationContainer clientApps;
  ApplicationContainer serverApps;
  serverApps.Start (Seconds (0.01));
  clientApps.Start (Seconds (0.01));

  BulkSendHelper dlClientHelper ("ns3::TcpSocketFactory", InetSocketAddress (ueIpIface.GetAddress (0), dlPort));
  dlClientHelper.SetAttribute ("MaxBytes", UintegerValue (10000000000));
  dlClientHelper.SetAttribute ("SendSize", UintegerValue (2000));

  clientApps.Add (dlClientHelper.Install (remoteHost));
     
  PacketSinkHelper dlPacketSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
  serverApps.Add (dlPacketSinkHelper.Install (ueNodes));
  
  std::cout << "Install and start applications on UE and remote host" << std::endl;

  // REM settings tuned to get a nice figure for this specific scenario
  Config::SetDefault ("ns3::RadioEnvironmentMapHelper::OutputFile", StringValue ("laa-wifi-indoor.rem"));
  Config::SetDefault ("ns3::RadioEnvironmentMapHelper::XMin", DoubleValue (0));
  Config::SetDefault ("ns3::RadioEnvironmentMapHelper::XMax", DoubleValue (120));
  Config::SetDefault ("ns3::RadioEnvironmentMapHelper::YMin", DoubleValue (0));
  Config::SetDefault ("ns3::RadioEnvironmentMapHelper::YMax", DoubleValue (50));
  Config::SetDefault ("ns3::RadioEnvironmentMapHelper::XRes", UintegerValue (1200));
  Config::SetDefault ("ns3::RadioEnvironmentMapHelper::YRes", UintegerValue (500));
  Config::SetDefault ("ns3::RadioEnvironmentMapHelper::Z", DoubleValue (1.5));

  // Tracing
  lteHelper->EnableMacTraces ();
  lteHelper->EnableRlcTraces ();
  lteHelper->EnablePdcpTraces ();

  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.InstallAll();

  // Simulation Start
  std::cout << "Simulation running" << std::endl;

  Simulator::Stop(Seconds(simTime));

  Simulator::Run();

  monitor->CheckForLostPackets ();

  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();

  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin (); iter != stats.end (); ++iter)
  {
  Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (iter->first);

          NS_LOG_UNCOND("Flow ID: " << iter->first << " Src Addr " << t.sourceAddress << " Dst Addr " << t.destinationAddress);
          NS_LOG_UNCOND("Tx Packets = " << iter->second.txPackets);
          NS_LOG_UNCOND("Rx Packets = " << iter->second.rxPackets);
          NS_LOG_UNCOND("Throughput: " << iter->second.rxBytes * 8.0 / (iter->second.timeLastRxPacket.GetSeconds()-iter->second.timeFirstTxPacket.GetSeconds()) / 1024  << " Kbps");
  }

  monitor->SerializeToXmlFile ("result-test.xml" , true, true );
  
  Simulator::Destroy();
  return 0;

}
