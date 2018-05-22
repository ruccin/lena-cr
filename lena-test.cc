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
#include "ns3/laa-wifi-coexistence-helper.h"
#include "ns3/duty-cycle-access-manager.h"
#include "ns3/laa-wifi-coexistence-helper.h"
#include "ns3/scenario-helper.h"

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
  uint16_t bw[6] = {6, 15, 25, 50, 75, 100};
  uint16_t maxbw = 0;
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
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);

  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.2", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);

  NodeContainer ueNodes;
  NodeContainer enbNodes;
  NodeContainer staNodes;
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

  // Set of Pathloss Model
  lteHelper->SetAttribute ("PathlossModel", StringValue ("ns3::LogDistancePropagationLossModel"));

  std::cout << "Set of Pathloss Model" << std::endl;

/*
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
  
  // Set of Scheduler
  lteHelper->SetSchedulerAttribute ("UlCqiFilter", EnumValue (FfMacScheduler::PUSCH_UL_CQI));
  
  std::cout << "Set of Scheduler" << std::endl;
*/

  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);

  for (uint16_t i = 0; i < 6; i++)
  {
    if (bw[i] == max)
      maxbw = bw[i];
  }

  Config::SetDefault ("ns3::LteEnbNetDevice::DlBandwidth", UintegerValue (maxbw));

  //Ptr<LteEnbNetDevice> lteEnbNetDevice = enbLteDevs->GetObject<LteEnbNetDevice> (); 
  //Ptr<SpectrumChannel> downlinkSpectrumChannel = lteEnbNetDevice->GetPhy ()->GetDownlinkSpectrumPhy ()->GetChannel ();

  downlinkSpectrumChannel = lteHelper.GetDownlinkSpectrumChannel(enbLteDevs);

  SpectrumWifiPhyHelper spectrumPhy = SpectrumWifiPhyHelper::Default ();
  spectrumPhy.SetChannel (downlinkSpectrumChannel);

  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211n_5GHZ);
  WifiMacHelper mac;
  wifi.SetRemoteStationManager ("ns3::IdealWifiManager");

  mac.SetType ("ns3::StaWifiMac",
               "ActiveProbing", BooleanValue (false));

  Ptr<NetDeviceContainer> UEDevices = wifi.Install (spectrumPhy, mac, ueNodes.Get (0));

  mac.SetType ("ns3::APWifiMac")

  Ptr<NetDeviceContainer> APDevices = wifi.Install (spectrumPhy, mac, apNodes.Get (0));
    
  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  internet.Install (apNodes);

  ipv4h.SetBase ("3.0.0.2", "255.0.0.0");
  ipv4h.Assign (APDevices);
  ipv4h.Assign (UEDevices);

  // Assign IP address to AP, and install applications
  
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Node> ueNode = ueNodes.Get (0);
  Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4>());
  ueStaticRouting->AddHostRouteTo (Ipv4Address ("1.0.0.2"), Ipv4Address ("3.0.0.2"), 1);

  Ptr<Node> apNode = apNodes.Get (0);
  Ptr<Ipv4StaticRouting> apStaticRouting = ipv4RoutingHelper.GetStaticRouting (apNode->GetObject<Ipv4>());
  apStaticRouting->AddHostRouteTo (Ipv4Address ("1.0.0.2"), Ipv4Address ("1.0.0.2"), 1);

  std::cout << "Install the IP Stack and Assign IP address to UEs" << std::endl;

  // Install and start applications on UE and remote host
  
  uint16_t dlPort = 20;
  ApplicationContainer clientApps;
  ApplicationContainer serverApps;
  serverApps.Start (Seconds (0.01));
  clientApps.Start (Seconds (0.01));

  BulkSendHelper dlClientHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
  dlClientHelper.SetAttribute ("MaxBytes", UintegerValue (10000000000));
  dlClientHelper.SetAttribute ("PacketSize", UintegerValue (1024));

  clientApps.Add (dlClientHelper.Install (remoteHost));
     
  PacketSinkHelper dlPacketSinkHelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
  serverApps.Add (dlPacketSinkHelper.Install (ueNodes));
  
 /*
  uint16_t wifiPort=3000;
  ApplicationContainer wifiClientApps;
  ApplicationContainer wifiServerApps;
  PacketSinkHelper wifiPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny(),wifiPort));
  wifiServerApps.Add (wifiPacketSinkHelper.Install(remoteHost.Get (0));
  OnOffHelper wifiClient ("ns3::UdpSocketFactory",Address(InetSocketAddress(Ipv4Address("1.0.0.2"),wifiPort)));
  wifiClient.SetAttribute("OnTime",StringValue("ns3::ExponentialRandomVariable[Mean=0.352]"));
  wifiClient.SetAttribute("OffTime",StringValue("ns3::ExponentialRandomVariable[Mean=0.652]"));
  wifiClient.SetAttribute("DataRate",StringValue("320kb/s"));
  wifiClient.SetAttribute("PacketSize",UintegerValue(1024));
  wifiClient.Install(ueNodes.Get(0));
  wifiServerApps.Start(Seconds(.01));
  wifiClientApps.Start(Seconds(.1));
*/
  std::cout << "Install and start applications on UE and remote host" << std::endl;

  // Simulation Start
  std::cout << "Simulation running" << std::endl;
                  
                   
  Simulator::Stop(Seconds(simTime));

  Simulator::Run();

  //monitor->CheckForLostPackets ();

  //monitor->SerializeToXmlFile ("lena-cr-result.xml" , true, true );
  
  Simulator::Destroy();
  return 0;

}
