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
#include "ns3/lte-enb-net-device.h"
#include "ns3/lte-ue-net-device.h"
#include "ns3/lte-enb-rrc.h"
#include "ns3/lte-ue-rrc.h"
#include "ns3/lte-common.h"
#include "ns3/trace-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/spectrum-wifi-helper.h"

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("lena-cr-test");

int
main (int argc, char *argv[])
{

  uint16_t numberOfUENodes = 1;
  uint16_t numberOfeNBNodes = 1;
  uint16_t numberOfAPNodes = 1;
  double simTime = 60;
  double distance = 4000;
  double interPacketInterval = 100;
  uint16_t bw[6] = {6, 15, 25, 50, 75, 100};
  uint8_t maxbw = 0;
  uint16_t max = 100;

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
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);

  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

  NodeContainer ueNodes;
  NodeContainer enbNodes;
  NodeContainer apNodes;
  enbNodes.Create(numberOfeNBNodes);
  ueNodes.Create(numberOfUENodes);
  apNodes.Create(numberOfAPNodes);

  // Position of All
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (distance, 0.0, 0.0));
  positionAlloc->Add (Vector (distance * 0.161, 0.0, 0.0));
  positionAlloc->Add (Vector (distance * 0.334, 0.0, 0.0));

  MobilityHelper Mobility;
  Mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  Mobility.SetPositionAllocator (positionAlloc);
  Mobility.Install (enbNodes);
  Mobility.Install (apNodes);
  Mobility.Install (ueNodes);
  
  std::cout << "Set of Position of All" << std::endl;

/*
  // Configuration MIMO
  Config::SetDefault("ns3::LteEnbRrc::DefaultTransmissionMode", UintegerValue(mimo));
  Config::SetDefault("ns3::LteAmc::AmcModel", EnumValue(LteAmc::PiroEW2010));
  Config::SetDefault("ns3::LteAmc::Ber", DoubleValue(0.00005));

  std::cout << "Configuration MIMO" << std::endl;
  
  // Set of Scheduler
  lteHelper->SetSchedulerAttribute ("UlCqiFilter", EnumValue (FfMacScheduler::PUSCH_UL_CQI));
  
  std::cout << "Set of Scheduler" << std::endl;
*/
  for (uint8_t i = 0; i < 6; i++)
  {
    if (bw[i] == max)
      maxbw = bw[i];
  }

  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);

  Config::SetDefault ("ns3::LteEnbNetDevice::DlEarfcn", UintegerValue (100));
  Config::SetDefault ("ns3::LteEnbNetDevice::UlEarfcn", UintegerValue (100 + 18000));
  Config::SetDefault ("ns3::LteEnbNetDevice::DlBandwidth", UintegerValue (maxbw));
  Config::SetDefault ("ns3::LteEnbNetDevice::UlBandwidth", UintegerValue (maxbw));
  Config::SetDefault ("ns3::LteEnbPhy::TxPower", DoubleValue (35));
  Config::SetDefault ("ns3::LteEnbPhy::NoiseFigure", DoubleValue (5.0));

  lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::LogDistancePropagationLossModel"));

  lteHelper->SetEnbAntennaModelType("ns3::IsotropicAntennaModel");
  lteHelper->SetUeAntennaModelType("ns3::IsotropicAntennaModel");

  Ptr<LteEnbNetDevice> lteEnbNetDevice = enbLteDevs.Get (0)->GetObject<LteEnbNetDevice> ();
  Ptr<SpectrumChannel> downlinkSpectrumChannel = lteEnbNetDevice->GetPhy ()->GetDownlinkSpectrumPhy ()->GetChannel ();

  SpectrumWifiPhyHelper spectrumPhy = SpectrumWifiPhyHelper::Default ();
  spectrumPhy.SetChannel (downlinkSpectrumChannel);

  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211n_5GHZ);
  WifiMacHelper mac;
  wifi.SetRemoteStationManager ("ns3::IdealWifiManager");

  mac.SetType ("ns3::StaWifiMac",
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer UEDevices;
  NetDeviceContainer APDevices;
  
  UEDevices = wifi.Install (spectrumPhy, mac, ueNodes.Get (0));

  mac.SetType ("ns3::ApWifiMac");

  APDevices = wifi.Install (spectrumPhy, mac, apNodes.Get (0));
 
  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  internet.Install (apNodes);

  ipv4h.SetBase ("2.0.0.0", "255.0.0.0");
  ipv4h.Assign (UEDevices);
  ipv4h.Assign (APDevices);

  Ptr<Node> ueNode = ueNodes.Get (0);
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
  ueStaticRouting->AddHostRouteTo (Ipv4Address ("1.0.0.2"), 1);

  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddHostRouteTo (Ipv4Address ("1.0.0.2"), Ipv4Address ("1.0.0.2"), 1);

  // Install and start applications on UE and remote host
  uint16_t dlPort = 1234;

  ApplicationContainer clientApps;
  ApplicationContainer serverApps;

  BulkSendHelper dlClientHelper ("ns3::TcpSocketFactory", Address (InetSocketAddress (Ipv4Address ("1.0.0.2"), dlPort)));
  dlClientHelper.SetAttribute ("MaxBytes", UintegerValue (1000000000));
  dlClientHelper.SetAttribute ("SendSize", UintegerValue (1024));
  clientApps.Add (dlClientHelper.Install (ueNodes.Get (0));

  PacketSinkHelper dlPacketSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
  serverApps.Add (dlPacketSinkHelper.Install (remoteHost));

  std::cout << "Install and start applications on UE and remote host" << std::endl;

  // Simulation Start
  std::cout << "Simulation running" << std::endl;

  lteHelper->EnableMacTraces;
  lteHelper->EnableRlcTraces;
  lteHelper->EnablePdcpTraces;

  Simulator::Stop(Seconds(simTime));

  Simulator::Run();
  
  FlowMonitorHelper flowMo;
  Ptr<FlowMonitor> monitor;

  monitor = flowmo.Install (ueNodes);
  monitor = flowmo.Install (remoteHost);
  monitor->SerializeToXmlFile ("lena-cr.xml", true, true);
  
  Simulator::Destroy();
  return 0;

}
