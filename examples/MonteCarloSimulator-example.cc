#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/log.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/mobility-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/on-off-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/packet-sink.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/abort.h"
#include "ns3/mobility-model.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/csma-module.h"
#include "ns3/config-store.h"
#include "iostream"
#include "algorithm"
#include "random"
#include "ns3/MonteCarloSimulator.h"

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("thesis");

NodeContainer wifiApNodes;
NodeContainer wifiStaNodes;
ApplicationContainer sinkApplications;
std::string epsilonType = "sticky";
double epsilonValue = 0.3;
int stickyCounter = 2;
double stickyCounterArray[4];
double (*rewardArray)[500];
double (*throughputArray)[500];
int* roundNum;
int dataRate[2] = {12, 15};
bool stickyUsed[2] = {0, 0};

// Enable or disable network interfaces in order to switch station's association
void StatoAP1(Ptr<Node> sta){
    sta->GetObject<Ipv4>()->SetDown(2);
    sta->GetObject<Ipv4>()->SetUp(1);
}

void StatoAP2(Ptr<Node> sta){
    sta->GetObject<Ipv4>()->SetDown(1);
    sta->GetObject<Ipv4>()->SetUp(2);
}

void RandomizeConnection(Ptr<Node> wifiNode){
    std::random_device randomDevice;
    std::mt19937 gen(randomDevice());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    double randomValue = dis(gen);
    if (randomValue < .5){
        StatoAP1(wifiNode);
    } else {
        StatoAP2(wifiNode);
    }
}

void EpsilonGreedy(Ptr<Node> wifiNode, uint32_t nodeIndex){
    // Generate random values with null-device as seed to obtain different values in short timeframe
    std::random_device randomDevice;
    std::mt19937 gen(randomDevice());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    double randomEpsilon = dis(gen);
    if (randomEpsilon < epsilonValue){
        RandomizeConnection(wifiNode);
    } else
    {
        if (rewardArray[2 * nodeIndex][*roundNum] > rewardArray[2 * nodeIndex + 1][*roundNum])
        {
            StatoAP1(wifiNode);
        }
        else
        {
            StatoAP2(wifiNode);
        }
    }
}

void ChooseAP(){
    if (epsilonType == "sticky")
    {
        int currentAssociation[2] = {0, 0};
        if (throughputArray[0][*roundNum-1] > throughputArray[1][*roundNum-1])
        {
            currentAssociation[0] = 0;
        }
        else
        {
            currentAssociation[0] = 1;
        }
        if (throughputArray[2][*roundNum-1] > throughputArray[3][*roundNum-1])
        {
            currentAssociation[1] = 2;
        }
        else
        {
            currentAssociation[1] = 3;
        }
        for (uint32_t nodeIndex = 0; nodeIndex < wifiStaNodes.GetN(); ++nodeIndex)
        {
            if (stickyUsed[nodeIndex])
            {
                if (throughputArray[currentAssociation[nodeIndex]][*roundNum-1] >
                    dataRate[nodeIndex] * .996)
                {
                    stickyCounterArray[currentAssociation[nodeIndex]] = stickyCounter;
                }
                else
                {
                    stickyCounterArray[currentAssociation[nodeIndex]] =
                        stickyCounterArray[currentAssociation[nodeIndex]] - 1;
                }
            }
            else
            {
                if (throughputArray[currentAssociation[nodeIndex]][*roundNum-1] >
                    dataRate[nodeIndex] * .996)
                {
                    stickyCounterArray[currentAssociation[nodeIndex]] = stickyCounter;
                }
            }
            if (stickyCounterArray[currentAssociation[nodeIndex]] > 0)
            {
                stickyUsed[nodeIndex] = 1;
            }
            else
            {
                EpsilonGreedy(wifiStaNodes.Get(nodeIndex), nodeIndex);
            }
        }
    }
    else if (epsilonType == "greedy")
    {
        for (uint32_t nodeIndex = 0; nodeIndex < wifiStaNodes.GetN(); ++nodeIndex)
        {
            EpsilonGreedy(wifiStaNodes.Get(nodeIndex), nodeIndex);
        }
    }
    else
    {
        for (uint32_t nodeIndex = 0; nodeIndex < wifiStaNodes.GetN(); ++nodeIndex)
        {
            RandomizeConnection(wifiStaNodes.Get(nodeIndex));
        }
    }
}


int main (int argc, char *argv[]){
    double roundTime = 2;
    double numRounds = 10;
    double roundWarmup = 1;
    uint32_t printing = 0;
    std::string outputName = "example-algorithms";

    CommandLine cmd;
    cmd.AddValue("roundTime", "Duration of single round", roundTime);
    cmd.AddValue("numRounds", "Number of rounds", numRounds);
    cmd.AddValue("printing", "Number of stage from which results will be printed", printing);
    cmd.AddValue("roundWarmup", "Warmup time for each round", roundWarmup);
    cmd.AddValue("epsilonType", "Type of epsilon algorithm. Available types: none, "
                                "greedy, sticky", epsilonType);
    cmd.AddValue("epsilonValue", "Value of epsilon parameter", epsilonValue);
    cmd.AddValue("stickyCounter", "Sticky counter, used in epsilon sticky algorithm",
                 stickyCounter);
    cmd.AddValue("outputName", "Name of the output file with results",outputName);
    cmd.Parse (argc,argv);

    if (roundWarmup >= roundTime){
        std::cout << "Warmup time cannot exceed time of a single round" << std::endl;
        return 1;
    }

    if (!(epsilonType == "none" || epsilonType == "greedy" || epsilonType == "sticky")){
        std::cout << "Unsupported epsilon algorithm" << std::endl;
        return 1;
    }

    if (epsilonValue < 0 || epsilonValue > 1){
        std::cout << "Epsilon parameter should be in range <0, 1>" << std::endl;
        return 1;
    }

    // Zero values in used arrays
    for (int i = 0; i < 4; ++i){
        stickyCounterArray[i] = 0;
    }


    // Create AP and stations
    wifiApNodes.Create(2);
    wifiStaNodes.Create(2);

    // Configure mobility
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();

    positionAlloc->Add (Vector (0.0, 2.0, 0.0)); // position of AP1
    positionAlloc->Add (Vector (6.0, 2.0, 0.0)); // position of AP2
    positionAlloc->Add (Vector (2.0, 5.0, 0.0)); // position of station 1
    positionAlloc->Add (Vector (2.5, 0.0, 0.0)); // position of station 2

    mobility.SetPositionAllocator (positionAlloc);

    mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

    mobility.Install (wifiApNodes);
    mobility.Install (wifiStaNodes);

    // Configure wireless channel
    Ptr<MatrixPropagationLossModel> lossModel1;
    Ptr<MatrixPropagationLossModel> lossModel2;
    Ptr<YansWifiChannel> wifiChannel1 = CreateObject<YansWifiChannel>();
    Ptr<YansWifiChannel> wifiChannel2 = CreateObject<YansWifiChannel>();
    WifiPhyStateHelper phyStateHelper;
    lossModel1 = CreateObject<MatrixPropagationLossModel>();
    lossModel1->SetDefaultLoss(0);
    lossModel2 = CreateObject<MatrixPropagationLossModel>();
    lossModel2->SetDefaultLoss(0);

    // Create & setup wifi channel
    wifiChannel1->SetPropagationLossModel(lossModel1);
    wifiChannel1->SetPropagationDelayModel(CreateObject<ConstantSpeedPropagationDelayModel>());

    wifiChannel2->SetPropagationLossModel(lossModel2);
    wifiChannel2->SetPropagationDelayModel(CreateObject<ConstantSpeedPropagationDelayModel>());

    YansWifiPhyHelper phy;
    phy.SetChannel (wifiChannel1);

    // Set channel width for given PHY
    phy.Set("ChannelWidth", UintegerValue(20));

    // Set channel number for AP1
    phy.Set("ChannelNumber", UintegerValue(36));

    // Prepare MCS values, which will be used for Data and Control Modes in Wi-Fi network
    WifiMacHelper mac;
    WifiHelper wifi;
    wifi.SetStandard (WIFI_STANDARD_80211ax);

    std::string oss3 = "HeMcs3";
    std::string oss4 = "HeMcs4";
    std::string oss5 = "HeMcs5";
    std::string oss6 = "HeMcs6";
    wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager","DataMode", StringValue (oss3),
                                 "ControlMode", StringValue (oss3)); //Set MCS

    Ssid ssid = Ssid ("toy-scenario-AP1"); //Set SSID
    mac.SetType ("ns3::AdhocWifiMac");

    // Connect nodes to Wi-Fi network
    NetDeviceContainer apDevices1;
    apDevices1 = wifi.Install (phy, mac, wifiApNodes.Get(0));

    //    mac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssid));
    NetDeviceContainer staDevices1;
    staDevices1 = wifi.Install (phy, mac, wifiStaNodes.Get(0));

    wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager","DataMode", StringValue (oss6),
                                 "ControlMode", StringValue (oss6)); //Set MCS
    staDevices1.Add(wifi.Install(phy, mac, wifiStaNodes.Get(1)));

    phy.Set("ChannelNumber", UintegerValue(44));
    phy.SetChannel (wifiChannel2);

    NetDeviceContainer apDevices2;
    apDevices2 = wifi.Install (phy, mac, wifiApNodes.Get(1));

    wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager","DataMode", StringValue (oss4),
                                 "ControlMode", StringValue (oss4)); //Set MCS
    NetDeviceContainer staDevices2;
    staDevices2 = wifi.Install (phy, mac, wifiStaNodes.Get(0));

    wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager","DataMode", StringValue (oss5),
                                 "ControlMode", StringValue (oss5)); //Set MCS
    staDevices2.Add(wifi.Install(phy, mac, wifiStaNodes.Get(1)));

    // Set guard interval on all interfaces of all nodes
    Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/HeConfiguration/GuardInterval", TimeValue (NanoSeconds (800)));

    // Install an Internet stack
    InternetStackHelper stack;
    stack.Install (wifiApNodes);
    stack.Install (wifiStaNodes);

    // Configure IP addressing for Wi-Fi
    Ipv4AddressHelper address;
    address.SetBase ("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer staNodeInterface;
    Ipv4InterfaceContainer apNodeInterface;

    staNodeInterface = address.Assign (staDevices1);
    apNodeInterface = address.Assign (apDevices1);

    address.SetBase ("192.168.2.0", "255.255.255.0");

    staNodeInterface.Add(address.Assign (staDevices2));
    apNodeInterface.Add(address.Assign (apDevices2));

    // Set transmitted power for all Station nodes in the network
    Config::Set ("/$ns3::NodeListPriv/NodeList/2/$ns3::Node/DeviceList/0/$ns3::WifiNetDevice/Phy/$ns3::YansWifiPhy/TxPowerEnd",
                DoubleValue(-79.5)); //-76.68
    Config::Set ("/$ns3::NodeListPriv/NodeList/2/$ns3::Node/DeviceList/0/$ns3::WifiNetDevice/Phy/$ns3::YansWifiPhy/TxPowerStart",
                DoubleValue(-79.5));
    Config::Set ("/$ns3::NodeListPriv/NodeList/2/$ns3::Node/DeviceList/1/$ns3::WifiNetDevice/Phy/$ns3::YansWifiPhy/TxPowerEnd",
                DoubleValue(-76.45)); //-79.85
    Config::Set ("/$ns3::NodeListPriv/NodeList/2/$ns3::Node/DeviceList/1/$ns3::WifiNetDevice/Phy/$ns3::YansWifiPhy/TxPowerStart",
                DoubleValue(-76.45));
    Config::Set ("/$ns3::NodeListPriv/NodeList/3/$ns3::Node/DeviceList/0/$ns3::WifiNetDevice/Phy/$ns3::YansWifiPhy/TxPowerEnd",
                DoubleValue(-71.1)); //-72.52
    Config::Set ("/$ns3::NodeListPriv/NodeList/3/$ns3::Node/DeviceList/0/$ns3::WifiNetDevice/Phy/$ns3::YansWifiPhy/TxPowerStart",
                DoubleValue(-71.1));
    Config::Set ("/$ns3::NodeListPriv/NodeList/3/$ns3::Node/DeviceList/1/$ns3::WifiNetDevice/Phy/$ns3::YansWifiPhy/TxPowerEnd",
                DoubleValue(-72.3)); //-76.53
    Config::Set ("/$ns3::NodeListPriv/NodeList/3/$ns3::Node/DeviceList/1/$ns3::WifiNetDevice/Phy/$ns3::YansWifiPhy/TxPowerStart",
                DoubleValue(-72.3));

    // Install applications (traffic generators)
    ApplicationContainer sourceApplications;
    uint32_t portNumber = 9;
    for (uint32_t staIndex = 0; staIndex < 2; ++staIndex){
        for (uint32_t APindex = 0; APindex < 2; ++APindex)
        {
            auto ipv4 = wifiApNodes.Get(APindex)->GetObject<Ipv4>();
            const auto socketAddress = ipv4->GetAddress (1, 0).GetLocal ();
            InetSocketAddress sinkSocket (socketAddress, portNumber++);
            OnOffHelper onOffHelper ("ns3::UdpSocketFactory", sinkSocket);
            onOffHelper.SetConstantRate(DataRate(dataRate[staIndex] * 10e6),1472);
            sourceApplications.Add (onOffHelper.Install (wifiStaNodes.Get (staIndex)));
            PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", sinkSocket);
            sinkApplications.Add (packetSinkHelper.Install (wifiApNodes.Get (APindex)));
        }
    }

    // Initialize simulation at random
    RandomizeConnection(wifiStaNodes.Get(0));
    RandomizeConnection(wifiStaNodes.Get(1));

    MonteCarloSimulator monteCarloSimulator = MonteCarloSimulator(
        &sinkApplications, numRounds, roundTime, roundWarmup,
        outputName, printing,true,
        &ChooseAP);
    rewardArray = monteCarloSimulator.GetRewardArray();
    throughputArray = monteCarloSimulator.GetThroughputArray();
    roundNum = monteCarloSimulator.GetCurrentRound();

    sinkApplications.Start (Seconds (0.0));
    sinkApplications.Stop (Seconds ((numRounds + 1) * roundTime));
    sourceApplications.Start (Seconds (0.0));
    sourceApplications.Stop (Seconds ((numRounds + 1) * roundTime));

    // Define simulation stop time
    Simulator::Stop (Seconds ((numRounds + 1) * roundTime));

    // Print information that the simulation will be executed
    std::clog << std::endl << "Starting simulation... " << std::endl;

    // Run the simulation!
    Simulator::Run ();

    //Clean-up
    Simulator::Destroy ();

    return 0;
}