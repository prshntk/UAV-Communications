/**
  c++ program for adhoc routing protocol comparison
  in uav scenario

 */
#include <iostream>
#include <cmath>
#include <fstream>
#include "ns3/aodv-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/v4ping-helper.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/olsr-module.h"
#include "ns3/dsdv-module.h"
#include "ns3/dsr-module.h"
#include "ns3/mesh-helper.h"
#include "ns3/mesh-module.h"
#include "ns3/applications-module.h"
#include <fstream>
using std::ofstream;


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("UdpTraceClientServerExample");

/**
  enabling logging 
 */
class AodvExample 
{
public:
  AodvExample ();
  /**
   * \brief Configure script parameters
   * \param argc is the command line argument count
   * \param argv is the command line arguments
   * \return true on successful configuration
  */
  bool Configure (int argc, char **argv);
  /// Run simulation
  void Run ();
  /**
   * Report results
   * \param os the output stream
   */
  void Report (std::ostream & os);

private:

  // parameters
  /// Number of nodes
  uint32_t size;
  /// Distance between nodes, meters
  double step;
  /// Simulation time, seconds
  double totalTime;
  /// Write per-device PCAP traces if true
  bool pcap;
  /// Print routes if true
  bool printRoutes;


  // network
  /// nodes used in the example
  NodeContainer nodes;
  /// devices used in the example
  NetDeviceContainer devices;
  /// interfaces used in the example
  Ipv4InterfaceContainer interfaces;

private:
  /// Create the nodes
  void CreateNodes ();
  /// Create the devices
  void CreateDevices ();
  /// Create the network
  void InstallInternetStack ();
  /// Create the simulation applications
  void InstallApplications ();
 
};

/**
 helper functions declaration 
 */

void maintainleaderfollower (Ptr<Node> node1,Ptr<Node> node2);
double pkr (Ptr<MobilityModel> mob);
double pks (Ptr<MobilityModel> mob);
double pkt (Ptr<MobilityModel> mob);


/**
main function

 */

int main (int argc, char **argv)
{
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_ALL);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_ALL);
  AodvExample test;
  if (!test.Configure (argc, argv))
    NS_FATAL_ERROR ("Configuration failed. Aborted.");
  test.Run ();
  test.Report (std::cout);
  return 0;
}

//-----------------------------------------------------------------------------
AodvExample::AodvExample () :
  size (4),
  step (5),
  totalTime (10),
  pcap (true),
  printRoutes (true)
{
}

bool
AodvExample::Configure (int argc, char **argv)
{
  // Enable AODV logs by default. Comment this if too noisy
  // LogComponentEnable("AodvRoutingProtocol", LOG_LEVEL_ALL);

  SeedManager::SetSeed (12345);
  CommandLine cmd (__FILE__);

  cmd.AddValue ("pcap", "Write PCAP traces.", pcap);
  cmd.AddValue ("printRoutes", "Print routing table dumps.", printRoutes);
  cmd.AddValue ("size", "Number of nodes.", size);
  cmd.AddValue ("time", "Simulation time, s.", totalTime);
  cmd.AddValue ("step", "Grid step, m", step);

  cmd.Parse (argc, argv);
  return true;
}

/**

Run function to final run the simulation

 */
void
AodvExample::Run ()
{
//  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", UintegerValue (1)); // enable rts cts all the time.
  CreateNodes ();
  CreateDevices ();
  InstallInternetStack ();
  InstallApplications ();

  std::cout << "Starting simulation for " << totalTime << " s ...\n";
  
  AnimationInterface anim ("SimpleNS3Simulation_NetAnimationOutput.xml"); /** creating animation to netanim*/
  for (uint n=1 ; n < 11 ; n++)
  {
  Simulator::Schedule (Seconds (n), &maintainleaderfollower,nodes.Get(0),nodes.Get(1));         /** function call for maintaining leader follower */
  Simulator::Schedule (Seconds (n), &maintainleaderfollower,nodes.Get(0),nodes.Get(2));
  Simulator::Schedule (Seconds (n), &maintainleaderfollower,nodes.Get(0),nodes.Get(3));
  }
  Simulator::Stop (Seconds (totalTime));
  Simulator::Run ();
  Simulator::Destroy ();
}
/**

create output function

 */
void
AodvExample::Report (std::ostream &)
{ 
}

/**

creating nodes and initial topology, giving intial velocity

 */

void
AodvExample::CreateNodes ()
{
  std::cout << "Creating " << (unsigned)size << " nodes " << step << " m apart.\n";
  nodes.Create (size);
  // Name nodes
  for (uint32_t i = 0; i < size; ++i)
    {
      std::ostringstream os;
      os << "node-" << i;
      Names::Add (os.str (), nodes.Get (i));
    }
  // Create static grid
  MobilityHelper mobility;
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",

"MinX", DoubleValue (0.0),

"MinY", DoubleValue (0.0),

"DeltaX", DoubleValue (5.0),

"DeltaY", DoubleValue (10.0),

"GridWidth", UintegerValue (3),

"LayoutType", StringValue ("RowFirst"));
  mobility.SetMobilityModel ("ns3::ConstantVelocityMobilityModel");
  

  mobility.Install (nodes);
  for (uint n=1 ; n < size ; n++)
 {
    Ptr<ConstantVelocityMobilityModel> mob = nodes.Get(n)->GetObject<ConstantVelocityMobilityModel>();
    mob->SetVelocity(Vector(2, 2, 2));        
 }
} 


void
AodvExample::CreateDevices ()
{
  WifiMacHelper wifiMac;
  wifiMac.SetType ("ns3::AdhocWifiMac");
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  wifiPhy.Set ("TxPowerStart", DoubleValue (10) );
  wifiPhy.Set ("TxPowerLevels", UintegerValue (10) );
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue ("OfdmRate6Mbps"), "RtsCtsThreshold", UintegerValue (0));
  devices = wifi.Install (wifiPhy, wifiMac, nodes); 

  if (pcap)
    {
      wifiPhy.EnablePcapAll (std::string ("aodv"));
    }
}
/**
implementing protocol and ip addressing

 */
void
AodvExample::InstallInternetStack ()
{
  AodvHelper aodv;
  OlsrHelper olsr;
  DsdvHelper dsdv;
  DsrHelper dsr;
  // you can configure AODV attributes here using aodv.Set(name, value)
  InternetStackHelper stack;
  stack.SetRoutingHelper (dsdv); // has effect on the next Install ()*/
  stack.Install (nodes);
  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.0.0.0");
  interfaces = address.Assign (devices);

  if (printRoutes)
    {
      Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("aodv.routes", std::ios::out);
      aodv.PrintRoutingTableAllAt (Seconds (8), routingStream);
    }
}

/**

creating application for sending an array containing x,y,z components of velocity 

 */

void
AodvExample::InstallApplications ()
{
  Address  serverAddress ;
  uint16_t port = 9;  // well-known echo port number
  UdpEchoServerHelper server (port);
  ApplicationContainer apps = server.Install (nodes.Get (1));
  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (10.0));
  serverAddress = Address (InetSocketAddress (Ipv4Address ("255.255.255.255"), port));
  uint32_t packetSize = 1024;
  uint32_t maxPacketCount = 320;
  Time interPacketInterval = Seconds (0.2);
  UdpEchoClientHelper client (serverAddress, port);
  client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
  client.SetAttribute ("Interval", TimeValue (interPacketInterval));
  client.SetAttribute ("PacketSize", UintegerValue (packetSize));
  apps = client.Install (nodes.Get (0));
  apps.Start (Seconds (2.0));
  apps.Stop (Seconds (10.0));
  double qw=pkr(nodes.Get(0)->GetObject<MobilityModel>());
  double qy=pks(nodes.Get(0)->GetObject<MobilityModel>());
  double qz=pkt(nodes.Get(0)->GetObject<MobilityModel>());
  client.SetFill (apps.Get (0), "hi");
  uint8_t fill[] = {0,1,2};
  fill[0]=uint8_t(qw);
  fill[1]=uint8_t(qy);
  fill[2]=uint8_t(qz);
  client.SetFill (apps.Get (0), fill, sizeof(fill), 1024);

}
void maintainleaderfollower (Ptr<Node> node1,Ptr<Node> node2)
{
    Ptr<ConstantVelocityMobilityModel> mob = node1->GetObject<ConstantVelocityMobilityModel>();
    Vector v=mob->GetVelocity(); 
    Ptr<ConstantVelocityMobilityModel> mobi = node1->GetObject<ConstantVelocityMobilityModel>();
    mobi->SetVelocity(v);          
}  

/**
helper functions to return velocity functions 

 */
double pkr (Ptr<MobilityModel> mob)
{
  double x=mob->GetVelocity().x;
  return x;
}
double pks (Ptr<MobilityModel> mob)
{
  double x=mob->GetVelocity().y;
  return x;
}
double pkt (Ptr<MobilityModel> mob)
{
  double x=mob->GetVelocity().z;
  return x;
}  
