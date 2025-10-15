////////////////////////////////////////////////////////////////////////////////////
// ns-3 script for the REPLICA project
////////////////////////////////////////////////////////////////////////////////////

#include "dataset-csv.h"

#include "ns3/antenna-module.h"
#include "ns3/applications-module.h"
#include "ns3/config-store-module.h"
#include "ns3/config-store.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/internet-module.h"
#include "ns3/ml-propagation-loss-model.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/node-list.h"
#include "ns3/nr-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/propagation-module.h"
#include "ns3/trace-based-propagation-loss-model.h"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <sstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ReplicaExample");

///////////////////////////////////////////////////////////
// PARAMETERS AND GLOBAL VARIABLES
///////////////////////////////////////////////////////////

/// Monitoring time per trace
const Time MONITORING_TIME_PER_TRACE = Seconds(1.0);

/// Warm-up time for each before starting monitoring
const Time WARMUP_TIME_PER_TRACE = Seconds(0.0);

/// Total time per position
const Time TOTAL_TIME_PER_TRACE = WARMUP_TIME_PER_TRACE + MONITORING_TIME_PER_TRACE;

DatasetCsv g_datasetCsv; //!< Dataset CSV

Time g_monitoringStartTime;     //!< Time when monitoring started
uint32_t g_rxBytesUplink = 0;   //!< Rx total bytes
uint32_t g_rxBytesDownlink = 0; //!< Tx total bytes
uint32_t g_txBytesUplink = 0;
uint32_t g_txBytesDownlink = 0;

std::ofstream g_resultsFileStream; //!< Results file stream
std::string g_outputDir = "scratch/replica/simulations/";
uint32_t g_simulationTime = 10; //!< Simulation time

///////////////////////////////////////////////////////////
// PATHS
///////////////////////////////////////////////////////////

/**
 * Get the directory of a given dataset.
 *
 * @param dataset Dataset name.
 * @return Dataset directory.
 */
inline std::string
GetDatasetDirectory(const std::string &dataset)
{
    return "./scratch/replica/"
           "datasets/" +
           dataset + "/";
}

/**
 * Get the path to the dataset file.
 *
 * @param dataset Dataset name.
 * @return Dataset file path.
 */
inline std::string
GetDatasetPositionsPath(const std::string &dataset)
{
    return GetDatasetDirectory(dataset) + "dataset-unique/"
                                          "propagation-loss-unique-dataset.csv";
}

/**
 * Get the path of the fading CDF CSV file for a given ML training algorithm.
 *
 * @param dataset Dataset name.
 * @param mlTrainingAlgorithm ML training algorithm (e.g., xgb, svr).
 * @return Fading CDF CSV file path.
 */
inline std::string
GetFadingCdfPath(const std::string &dataset, const std::string &mlTrainingAlgorithm)
{
    // clang-format off
    return GetDatasetDirectory(dataset) +
           "ml-model/" +
           "position/" +
           mlTrainingAlgorithm + "/"
           "fading-ecdf.csv";
    // clang-format on
}

/**
 * @param dataset Dataset name.
 * @param lossModelStripped Propagation loss model, stripped of prefixes
 *                          (e.g., xgb, friis, 3gpp).
 * @param nRun Simulation run.
 * @return Path of the results CSV file.
 */
inline std::string
ResultsFileNameStructure(const std::string &lossModelStripped,
                         uint32_t nRun,
                         const std::string &protocol,
                         const std::string &mode,
                         const int distance)
{
    // ensure g_outputDir exists
    std::filesystem::create_directories(g_outputDir);

    // clang-format off
    return g_outputDir + lossModelStripped + "-dist" + std::to_string(distance) + "m-" + protocol + "-" + mode + "-nRun" + std::to_string(nRun) + "-simTime" + std::to_string(g_simulationTime);
    // clang-format on
}

///////////////////////////////////////////////////////////
// AUXILIARY FUNCTIONS
///////////////////////////////////////////////////////////

/**
 * Get the position of a given node.
 *
 * @param nodeId Node ID.
 * @return Node position.
 */
Vector
GetNodePosition(uint32_t nodeId)
{
    Ptr<ConstantPositionMobilityModel> nodeMobility =
        NodeList::GetNode(nodeId)->GetObject<ConstantPositionMobilityModel>();

    return nodeMobility->GetPosition();
}

/**
 * Set the position of a given node.
 *
 * @param nodeId Node ID.
 * @param position Node position.
 */
void SetNodePosition(uint32_t nodeId, const Vector &position)
{
    Ptr<ConstantPositionMobilityModel> nodeMobility =
        NodeList::GetNode(nodeId)->GetObject<ConstantPositionMobilityModel>();

    return nodeMobility->SetPosition(position);
}

/**
 * Reset the throughput counters and start a new monitoring session for a given position.
 */
void ResetCounters()
{
    g_rxBytesUplink = 0;
    g_txBytesUplink = 0;
    g_rxBytesDownlink = 0;
    g_txBytesDownlink = 0;

    g_monitoringStartTime = Simulator::Now();
}

/**
 * Calculate the current throughput and update the results file.
 */
void UpdateThroughputResultsFile()
{
    Vector gnbPosition = GetNodePosition(0);
    Vector uePosition = GetNodePosition(1);

    // Calculate the throughput
    Time duration = Simulator::Now() - g_monitoringStartTime;
    double throughputKbpsUplink = ((g_rxBytesUplink * 8.0) / duration.GetSeconds()) / 1e3;
    double throughputKbpsDownlink = ((g_rxBytesDownlink * 8.0) / duration.GetSeconds()) / 1e3;

    // Update the results file
    std::stringstream ss;

    // Write CSV row
    ss << Simulator::Now().GetMilliSeconds() << "," << gnbPosition.x << "," << gnbPosition.y << ","
       << gnbPosition.z << "," << uePosition.x << "," << uePosition.y << "," << uePosition.z << ","
       << throughputKbpsUplink << "," << throughputKbpsDownlink;

    NS_LOG_INFO(ss.str());

    g_resultsFileStream << ss.str() << std::endl;
}

void RxPacketCallbackUplink(Ptr<const Packet> packet, Ptr<Ipv4> ipv4, uint32_t interface)
{
    g_rxBytesUplink += packet->GetSize();
}

void RxPacketCallbackDownlink(Ptr<const Packet> packet, Ptr<Ipv4> ipv4, uint32_t interface)
{
    g_rxBytesDownlink += packet->GetSize();
}

void TxPacketCallbackUplink(Ptr<const Packet> packet, Ptr<Ipv4> ipv4, uint32_t interface)
{
    g_txBytesUplink += packet->GetSize();
}

void TxPacketCallbackDownlink(Ptr<const Packet> packet, Ptr<Ipv4> ipv4, uint32_t interface)
{
    g_txBytesDownlink += packet->GetSize();
}

void StartThroughputMonitoring(uint32_t iteration)

{
    // Schedule the start of a new monitoring session (after Minstrel stabilizes)
    Simulator::Schedule(WARMUP_TIME_PER_TRACE, &ResetCounters);

    // Schedule the calculation and update of the throughput
    Simulator::Schedule(WARMUP_TIME_PER_TRACE + MONITORING_TIME_PER_TRACE,
                        &UpdateThroughputResultsFile);

    // Schedule the next monitoring session
    if (iteration + 1 < g_simulationTime)
    {
        Simulator::Schedule(WARMUP_TIME_PER_TRACE + MONITORING_TIME_PER_TRACE,
                            &StartThroughputMonitoring,
                            iteration + 1);
    }
}

double
CalculateFriisLossDb(double logDistD0, double frequencyMhz)
{
    constexpr double C = 3e8;
    double lossDb = -20.0 * std::log10(C / (4.0 * M_PI * frequencyMhz * 1e6 * logDistD0));

    return lossDb;
}

void PrintAppsOnEachNode();

///////////////////////////////////////////////////////////
// MAIN
///////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
    std::string lossModel;
    std::string mode;
    std::string protocol;

    uint32_t nRun = 1;
    bool verbose = false;
    bool printApps = false;
    bool pcap = false;

    //* nr params *//
    // set simulation time and mobility
    int64_t randomStream = 1;
    double centralFrequency = 3.5e9; // Hz
    double bandwidth = 100e6;        // Hz
    uint32_t numUes = 1;             // Number of UEs
    uint32_t numGnbs = 1;            // Number of gNBs
    uint16_t numerology = 1;         // Numerology
    double distance = 100.0;         // meters
    double rss = -30.0;              // dBm
    std::string scenario = "RMa";
    std::string dataRateStr = "1Gbps";

    bool mimo = false;
    NrHelper::MimoPmiParams mimoPmiParams;
    mimoPmiParams.subbandSize = 16;
    mimoPmiParams.rankLimit = 4;

    bool configBearer = false;

    /**
     * Default channel condition model: This model varies based on the selected scenario.
     * For instance, in the Urban Macro scenario, the default channel condition model is
     * the ThreeGppUMaChannelConditionModel.
     */
    std::string channelConditionModel = "LOS";

    CommandLine cmd;
    cmd.AddValue("lossModel",
                 "Propagation loss model {mlpl-xgb, mlpl-svr, friis, fixed-rss}",
                 lossModel);
    cmd.AddValue("mode", "uplink, downlink or bidir}", mode);
    cmd.AddValue("protocol",
                 "The transport protocol used for the simulation (udp or tcp).",
                 protocol);
    cmd.AddValue("nRun", "Simulation run seed (for confidence interval)", nRun);
    cmd.AddValue("simulationTime", "Dictates the time of the simulation", g_simulationTime);
    cmd.AddValue("pcap", "Enable pcap", pcap);
    cmd.AddValue("printApps", "Print the applications on each node", printApps);
    cmd.AddValue("verbose", "Enable verbose output", verbose);

    //* nr params *//
    cmd.AddValue("channelConditionModel",
                 "The channel condition model for the simulation. Choose among 'Default', 'LOS',"
                 "'NLOS', 'Buildings'.",
                 channelConditionModel);
    cmd.AddValue("scenario",
                 "The scenario used in the simulation:  InF, InH, UMa, UMi, RMa, InH-OfficeMixed, "
                 "InH-OfficeOpen, V2V-Highway, V2V-Urban, NTN-DenseUrban, NTN-Urban, NTN-Suburban, "
                 "NTN-Rural, Custom",
                 scenario);
    cmd.AddValue("mimo", "Enable MIMO", mimo);
    cmd.AddValue("bearer", "Enable bearer", configBearer);
    cmd.AddValue("numerology", "The numerology used in the simulation", numerology);
    cmd.AddValue("dataRate", "The data rate used in the simulation", dataRateStr);
    cmd.AddValue("frequency", "The central carrier frequency in Hz.", centralFrequency);
    cmd.AddValue("bandwidth", "The system bandwidth in Hz.", bandwidth);
    cmd.AddValue("distance", "The distance between the UE and the gNB", distance);
    cmd.AddValue("rss", "The received signal strength in dBm", rss);
    cmd.Parse(argc, argv);

    // Validate arguments
    NS_ABORT_MSG_IF(lossModel.empty(), "--lossModel argument is mandatory");
    NS_ABORT_MSG_IF(protocol.empty(), "--protocol argument is mandatory (udp or tcp)");
    NS_ABORT_MSG_IF(mode.empty() && mode != "downlink" && mode != "uplink" && mode != "bidir",
                    "Invalid mode. Use 'uplink', 'downlink' or 'bidir'");

    // Define RNG seeds
    SeedManager::SetSeed(1);
    SeedManager::SetRun(nRun);

    if (verbose)
    {
        LogComponentEnable("ReplicaExample", LOG_LEVEL_INFO);
        LogComponentEnable("MlPropagationLossModel", LOG_INFO);
        LogComponentEnable("TraceBasedPropagationLossModel", LOG_INFO);
        // LogComponentEnable("OnOffApplication", LOG_INFO);
        // LogComponentEnable("PacketSink", LOG_INFO);
        // LogComponentEnable("BulkSendApplication", LOG_INFO);
        // LogComponentEnable("CsvReader", LOG_LEVEL_ALL);
    }

    // Create the simulated scenario
    HexagonalGridScenarioHelper hexGrid;
    /**
     * Set the scenario parameters for the simulation, considering the UMa scenario.
     * Following the TR 38.901 specification - Table 7.4.1-1 pathloss models.
     * hBS = 25m for UMa scenario.
     * hUT = 1.5m for UMa scenario.
     */
    double ueHeight = 6;
    double gnbHeight = 12.87;

    hexGrid.SetUtHeight(ueHeight);  // Height of the UE in meters
    hexGrid.SetBsHeight(gnbHeight); // Height of the gNB in meters
    hexGrid.SetSectorization(1);    // Number of sectors
    hexGrid.m_isd = 200;            // Inter-site distance in meters
    uint32_t ueTxPower = 23;        // UE transmission power in dBm
    uint32_t bsTxPower = 40;        // gNB transmission power in dBm
    // double ueSpeed = 0;          // 0.8333;  // in m/s (3 km/h)
    // Antenna parameters
    uint32_t ueNumRows = 1;  // Number of rows for the UE antenna
    uint32_t ueNumCols = 1;  // Number of columns for the UE antenna
    uint32_t gnbNumRows = 1; // Number of rows for the gNB antenna
    uint32_t gnbNumCols = 1; // Number of columns for the gNB antenna
    // Set the number of UEs and gNBs nodes in the scenario
    hexGrid.SetUtNumber(numUes);  // Number of UEs
    hexGrid.SetBsNumber(numGnbs); // Number of gNBs
    // Create a scenario with mobility
    hexGrid.CreateScenario();

    auto ueNodes = hexGrid.GetUserTerminals();
    auto gNbNodes = hexGrid.GetBaseStations();

    double ue_x_coordinate =
        std::sqrt(distance * distance - (gnbHeight - ueHeight) * (gnbHeight - ueHeight));
    ueNodes.Get(0)->GetObject<MobilityModel>()->SetPosition(Vector(ue_x_coordinate, 0, 1.5));
    gNbNodes.Get(0)->GetObject<MobilityModel>()->SetPosition(Vector(0, 0, 25));

    /*
     * Setup the NR module:
     * - NrHelper, which takes care of creating and connecting the various
     * part of the NR stack
     * - NrChannelHelper, which takes care of the spectrum channel
     */
    Ptr<NrPointToPointEpcHelper> epcHelper = CreateObject<NrPointToPointEpcHelper>();
    Ptr<NrHelper> nrHelper = CreateObject<NrHelper>();
    Ptr<NrChannelHelper> channelHelper = CreateObject<NrChannelHelper>();
    nrHelper->SetEpcHelper(epcHelper);

    uint8_t numCc = 1; // Number of component carriers
    CcBwpCreator ccBwpCreator;
    auto band = ccBwpCreator.CreateOperationBandContiguousCc({centralFrequency, bandwidth, numCc});

    // Propagation loss model
    std::string lossModelStripped;

    if (lossModel == "mlpl-xgb" || lossModel == "mlpl-svr")
    {
        g_datasetCsv.LoadMlplDatasetCsv(GetDatasetPositionsPath("replica-dataset"));

        lossModelStripped = lossModel.substr(lossModel.find('-') + 1);
        nrHelper->SetUeAntennaTypeId(ParabolicAntennaModel::GetTypeId().GetName());
        nrHelper->SetGnbAntennaTypeId(ParabolicAntennaModel::GetTypeId().GetName());

        channelHelper->ConfigurePropagationFactory(MlPropagationLossModel::GetTypeId());
        channelHelper->SetPathlossAttribute(
            "FadingCdfPath",
            StringValue(GetFadingCdfPath("replica-dataset", lossModelStripped)));
        channelHelper->SetPathlossAttribute("PathLossCache", BooleanValue(true));
    }
    else if (lossModel == "trace-based")
    {
        lossModelStripped = "traceBased";

        std::string datasetDir;
        if (distance < 100)
        {
            datasetDir =
                "./scratch/replica/datasets/" + lossModel + "-" + protocol + "-" + mode + ".csv";
        }
        else
        {
            datasetDir = "./scratch/replica/datasets/" + lossModel + "-" + protocol + "-" + mode +
                         "-attenuated.csv";
        }

        g_datasetCsv.LoadTraceBasedDatasetCsv(datasetDir);
        g_simulationTime = g_datasetCsv.GetMaxTimeS();

        lossModelStripped = lossModel;
        channelHelper->ConfigurePropagationFactory(TraceBasedPropagationLossModel::GetTypeId());
        channelHelper->SetPathlossAttribute("NetworkTracePath", StringValue(datasetDir));

        std::cout << "dataset path: " + datasetDir << std::endl;
    }
    else if (lossModel == "ThreeGpp" || lossModel == "3gpp")
    {
        lossModelStripped = "3gpp";
        lossModel = "ThreeGpp";
        // Create the ideal beamforming helper in case of a non-phased array model
        Ptr<IdealBeamformingHelper> idealBeamformingHelper = CreateObject<IdealBeamformingHelper>();
        nrHelper->SetBeamformingHelper(idealBeamformingHelper);
        // First configure the channel helper object factories
        channelHelper->ConfigureFactories(scenario, channelConditionModel, lossModel);
        // channelHelper->SetChannelConditionModelAttribute("UpdatePeriod",
        //                                                  TimeValue(Seconds(1)));

        // Beamforming method
        idealBeamformingHelper->SetAttribute("BeamformingMethod",
                                             TypeIdValue(DirectPathBeamforming::GetTypeId()));

        // Antennas for all the UEs
        nrHelper->SetUeAntennaAttribute("NumRows", UintegerValue(ueNumRows));
        nrHelper->SetUeAntennaAttribute("NumColumns", UintegerValue(ueNumCols));
        nrHelper->SetUeAntennaAttribute("AntennaElement",
                                        PointerValue(CreateObject<IsotropicAntennaModel>()));

        // Antennas for all the gNbs
        nrHelper->SetGnbAntennaAttribute("NumRows", UintegerValue(gnbNumRows));
        nrHelper->SetGnbAntennaAttribute("NumColumns", UintegerValue(gnbNumCols));
        nrHelper->SetGnbAntennaAttribute("AntennaElement",
                                         PointerValue(CreateObject<IsotropicAntennaModel>()));

        if (mimo)
        {
            nrHelper->SetupMimoPmi(mimoPmiParams);
        }
    }
    else if (lossModel == "friis")
    {
        lossModelStripped = lossModel;
        // Override the default antenna model with ParabolicAntennaModel
        nrHelper->SetUeAntennaTypeId(ParabolicAntennaModel::GetTypeId().GetName());
        nrHelper->SetGnbAntennaTypeId(ParabolicAntennaModel::GetTypeId().GetName());
        // Configure Friis propagation loss model before assign it to band
        channelHelper->ConfigurePropagationFactory(FriisPropagationLossModel::GetTypeId());
    }
    else if (lossModel == "fixed-rss")
    {
        lossModelStripped = "friis"; // lossModel;
        // Override the default antenna model with ParabolicAntennaModel
        nrHelper->SetUeAntennaTypeId(ParabolicAntennaModel::GetTypeId().GetName());
        nrHelper->SetGnbAntennaTypeId(ParabolicAntennaModel::GetTypeId().GetName());
        // Configure the FixedRss propagation loss model before assign it to band
        channelHelper->ConfigurePropagationFactory(FixedRssLossModel::GetTypeId());

        channelHelper->SetPathlossAttribute("Rss", DoubleValue(rss));
    }
    else
    {
        NS_ABORT_MSG("Unsupported propagation loss model: " << lossModel);
    }

    // After configuring the factories, create and assign the spectrum channels to the bands
    channelHelper->AssignChannelsToBands({band});

    if (configBearer)
    {
        // gNb routing between Bearer and bandwidh part
        nrHelper->SetGnbBwpManagerAlgorithmAttribute("GBR_CONV_VOICE", UintegerValue(0));
        // UE routing between bearer type and bandwidth part
        nrHelper->SetUeBwpManagerAlgorithmAttribute("NGBR_LOW_LAT_EMBB", UintegerValue(0));
    }

    // Get all the BWPs
    auto allBwps = CcBwpCreator::GetAllBwps({band});
    // Set the numerology and transmission powers attributes to all the gNBs and UEs
    nrHelper->SetGnbPhyAttribute("TxPower", DoubleValue(bsTxPower));
    nrHelper->SetGnbPhyAttribute("Numerology", UintegerValue(numerology));
    nrHelper->SetUePhyAttribute("TxPower", DoubleValue(ueTxPower));

    nrHelper->SetSchedulerTypeId(TypeId::LookupByName("ns3::NrMacSchedulerTdmaRR"));
    // nrHelper->SetSchedulerAttribute("FixedMcsDl", BooleanValue(false));
    // nrHelper->SetSchedulerAttribute("FixedMcsUl", BooleanValue(false));

    Config::SetDefault("ns3::NrRlcUm::MaxTxBufferSize", UintegerValue(999999999));

    // Error Model: UE and GNB with same spectrum error model.
    nrHelper->SetUlErrorModel("ns3::NrEesmIrT2");
    nrHelper->SetDlErrorModel("ns3::NrEesmIrT2");

    // Both DL and UL AMC will have the same model behind.
    nrHelper->SetGnbDlAmcAttribute(
        "AmcModel",
        EnumValue(NrAmc::ErrorModel)); // NrAmc::ShannonModel or NrAmc::ErrorModel
    nrHelper->SetGnbUlAmcAttribute(
        "AmcModel",
        EnumValue(NrAmc::ErrorModel)); // NrAmc::ShannonModel or NrAmc::ErrorModel

    // Install and get the pointers to the NetDevices
    NetDeviceContainer gNbNetDev = nrHelper->InstallGnbDevice(gNbNodes, allBwps);
    NetDeviceContainer ueNetDev = nrHelper->InstallUeDevice(ueNodes, allBwps);

    randomStream += nrHelper->AssignStreams(gNbNetDev, randomStream);
    randomStream += nrHelper->AssignStreams(ueNetDev, randomStream);

    if (lossModelStripped == "3gpp")
    {
        nrHelper->GetGnbPhy(gNbNetDev.Get(0), 0)
            ->SetAttribute("Pattern", StringValue("DL|DL|DL|DL|DL|DL|DL|S|UL|UL|"));
    }

    // create the internet and install the IP stack on the UEs
    // get SGW/PGW and create a single RemoteHost
    Ptr<Node> pgw = epcHelper->GetPgwNode();
    Ptr<Node> remoteHost = CreateObject<Node>();
    InternetStackHelper internet;
    internet.Install(remoteHost);

    // connect a remoteHost to pgw. Setup routing too
    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gbps")));
    p2ph.SetDeviceAttribute("Mtu", UintegerValue(2500));
    p2ph.SetChannelAttribute("Delay", TimeValue(Seconds(0.001)));
    NetDeviceContainer internetDevices = p2ph.Install(pgw, remoteHost);

    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);
    Ipv4StaticRoutingHelper ipv4RoutingHelper;

    Ptr<Ipv4StaticRouting> remoteHostStaticRouting =
        ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
    remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);
    internet.Install(ueNodes);

    Ipv4InterfaceContainer ueIpIface;
    ueIpIface = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueNetDev));

    Ptr<Ipv4StaticRouting> ueStaticRouting =
        ipv4RoutingHelper.GetStaticRouting(ueNodes.Get(0)->GetObject<Ipv4>());
    ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);

    // attach UEs to the closest eNB
    nrHelper->AttachToClosestGnb(ueNetDev, gNbNetDev);

    // The bearer that will carry low latency traffic
    NrEpsBearer bearer(NrEpsBearer::NGBR_LOW_LAT_EMBB);

    // Applications
    const Time RX_APP_START_TIME = Seconds(0.5);
    const Time TX_APP_START_TIME = Seconds(1.0);
    // assign IP address to UEs, and install UDP downlink applications
    uint16_t dlPort = 1234;

    Ptr<NrEpcTft> tft = Create<NrEpcTft>();
    NrEpcTft::PacketFilter dlpf;
    dlpf.localPortStart = dlPort;
    dlpf.localPortEnd = dlPort;
    tft->Add(dlpf);

    Ipv4Address remoteHostIp = remoteHost->GetObject<Ipv4>()->GetAddress(1, 0).GetLocal();
    Ipv4Address ueIp = ueIpIface.GetAddress(0);
    DataRate dataRate(dataRateStr); // Common data rate for all applications
    uint32_t packetSize = 1400;     // Common packet size for all applications

    if (protocol == "udp")
    {
        // ON OFF APPLICATION
        if (mode == "downlink" || mode == "bidir")
        {
            OnOffHelper onOffHelper("ns3::UdpSocketFactory", InetSocketAddress(ueIp, dlPort));

            // Saturate the wireless link
            onOffHelper.SetConstantRate(dataRate, packetSize);
            onOffHelper.SetAttribute("StartTime", TimeValue(TX_APP_START_TIME));
            onOffHelper.Install(remoteHost);

            PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory",
                                              InetSocketAddress(Ipv4Address::GetAny(), dlPort));
            packetSinkHelper.SetAttribute("StartTime", TimeValue(RX_APP_START_TIME));
            packetSinkHelper.Install(ueNodes.Get(0));
        }
        if (mode == "uplink" || mode == "bidir")
        {
            OnOffHelper onOffHelper("ns3::UdpSocketFactory",
                                    InetSocketAddress(remoteHostIp, dlPort));

            // Saturate the wireless link
            onOffHelper.SetConstantRate(dataRate, packetSize);
            onOffHelper.SetAttribute("StartTime", TimeValue(TX_APP_START_TIME));
            onOffHelper.Install(ueNodes.Get(0));

            PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory",
                                              InetSocketAddress(Ipv4Address::GetAny(), dlPort));
            packetSinkHelper.SetAttribute("StartTime", TimeValue(RX_APP_START_TIME));
            packetSinkHelper.Install(remoteHost);
        }
    }
    else if (protocol == "tcp")
    {
        // BULK SEND APPLICATION
        if (mode == "downlink" || mode == "bidir")
        {
            BulkSendHelper source("ns3::TcpSocketFactory", InetSocketAddress(ueIp, dlPort));

            source.SetAttribute("MaxBytes", UintegerValue(0));
            source.SetAttribute("StartTime", TimeValue(TX_APP_START_TIME));
            source.SetAttribute("SendSize", UintegerValue(packetSize));
            source.Install(remoteHost);

            PacketSinkHelper sink("ns3::TcpSocketFactory",
                                  InetSocketAddress(Ipv4Address::GetAny(), dlPort));

            sink.SetAttribute("StartTime", TimeValue(RX_APP_START_TIME));
            sink.Install(ueNodes.Get(0));
        }
        if (mode == "uplink" || mode == "bidir")
        {
            BulkSendHelper source("ns3::TcpSocketFactory", InetSocketAddress(remoteHostIp, dlPort));

            source.SetAttribute("MaxBytes", UintegerValue(0));
            source.SetAttribute("StartTime", TimeValue(TX_APP_START_TIME));
            source.SetAttribute("SendSize", UintegerValue(packetSize));
            source.Install(ueNodes.Get(0));

            PacketSinkHelper sink("ns3::TcpSocketFactory",
                                  InetSocketAddress(Ipv4Address::GetAny(), dlPort));

            sink.SetAttribute("StartTime", TimeValue(RX_APP_START_TIME));
            sink.Install(remoteHost);
        }
        Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(9999999));
        Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(9999999));
    }
    else
    {
        NS_ABORT_MSG("Unsupported protocol: " << protocol << ". Supported protocols: udp, tcp");
    }

    if (configBearer)
    {
        nrHelper->ActivateDedicatedEpsBearer(ueNetDev.Get(0), bearer, tft);
    }

    // Create the results file
    const std::string RESULTS_CSV_PATH =
        ResultsFileNameStructure(lossModelStripped, nRun, protocol, mode, distance) + ".csv";

    g_resultsFileStream.open(RESULTS_CSV_PATH, std::ios_base::out);

    if (!g_resultsFileStream.is_open())
    {
        NS_ABORT_MSG("Error opening results file: " << RESULTS_CSV_PATH);
    }

    // Print applications installed on each node
    if (printApps)
    {
        PrintAppsOnEachNode();
    }

    // Write CSV header
    constexpr auto csvHeader =
        "time_ms,x_gNb,y_gNb,z_gNb,x_ue,y_ue,z_ue,throughput_kbps_uplink,throughput_kbps_downlink";
    g_resultsFileStream << csvHeader << std::endl;

    // Schedule throughput monitoring´
    // Setup UE Traces
    Config::ConnectWithoutContext("/NodeList/1/$ns3::Ipv4L3Protocol/Tx",
                                  MakeCallback(&TxPacketCallbackUplink));
    Config::ConnectWithoutContext("/NodeList/1/$ns3::Ipv4L3Protocol/Rx",
                                  MakeCallback(&RxPacketCallbackDownlink));

    // Setup remoteHost Traces
    Config::ConnectWithoutContext("/NodeList/5/$ns3::Ipv4L3Protocol/Tx",
                                  MakeCallback(&TxPacketCallbackDownlink));
    Config::ConnectWithoutContext("/NodeList/5/$ns3::Ipv4L3Protocol/Rx",
                                  MakeCallback(&RxPacketCallbackUplink));

    // Start simulation
    Simulator::Schedule(TX_APP_START_TIME, &StartThroughputMonitoring, 0);
    const Time simulationStopTime =
        TX_APP_START_TIME + (g_simulationTime * TOTAL_TIME_PER_TRACE) + Seconds(1.0);

    NS_LOG_INFO("Simulation stop time: " << simulationStopTime.GetSeconds() << " seconds");
    NS_LOG_INFO(csvHeader);

    // Check pathloss traces
    std::ofstream flowmonOutputFile(
        ResultsFileNameStructure(lossModelStripped, nRun, protocol, mode, distance) +
        "-flowmon.json");

    nrHelper->EnablePathlossTraces();
    FlowMonitorHelper flowmonHelper;
    NodeContainer flowNodes;
    flowNodes.Add(remoteHost);
    flowNodes.Add(ueNodes);

    if (pcap)
    {
        p2ph.EnablePcap(
            ResultsFileNameStructure(lossModelStripped, nRun, protocol, mode, distance) +
                "-remoteHost",
            NodeContainer(remoteHost));
    }

    Ptr<FlowMonitor> monitor = flowmonHelper.Install(flowNodes);
    monitor->SetAttribute("DelayBinWidth", DoubleValue(0.001));
    monitor->SetAttribute("JitterBinWidth", DoubleValue(0.001));
    monitor->SetAttribute("PacketSizeBinWidth", DoubleValue(1));
    Simulator::Stop(simulationStopTime);
    Simulator::Run();

    monitor->CheckForLostPackets(Seconds(1.0));

    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowmonHelper.GetClassifier());

    flowmonOutputFile << "{\n  \"Flows\": [\n";

    bool firstFlow = true;
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();
    for (auto i = stats.begin(); i != stats.end(); ++i)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);

        Time txDuration = i->second.timeLastTxPacket - i->second.timeFirstTxPacket;
        Time rxDuration = i->second.timeLastRxPacket - i->second.timeFirstRxPacket;

        double txOffered = (txDuration.GetSeconds() > 0)
                               ? (i->second.txBytes * 8.0 / txDuration.GetSeconds() / 1000 / 1000)
                               : 0.0;
        double throughput = (rxDuration.GetSeconds() > 0)
                                ? (i->second.rxBytes * 8.0 / rxDuration.GetSeconds() / 1000 / 1000)
                                : 0.0;
        double meanDelay = (i->second.rxPackets > 0)
                               ? (1000 * i->second.delaySum.GetSeconds() / i->second.rxPackets)
                               : 0.0;
        double meanJitter = (i->second.rxPackets > 0)
                                ? (1000 * i->second.jitterSum.GetSeconds() / i->second.rxPackets)
                                : 0.0;
        double packetLossRatio =
            (i->second.txPackets > 0)
                ? ((i->second.txPackets - i->second.rxPackets) / (double)i->second.txPackets * 100)
                : 0.0;

        if (!firstFlow)
        {
            flowmonOutputFile << ",\n";
        }
        firstFlow = false;

        flowmonOutputFile << "    {\n"
                          << "      \"Flow ID\": " << i->first << ",\n"
                          << "      \"Source Address\": \"" << t.sourceAddress << "\",\n"
                          << "      \"Destination Address\": \"" << t.destinationAddress << "\",\n"
                          << "      \"Tx Packets\": " << i->second.txPackets << ",\n"
                          << "      \"Tx Bytes\": " << i->second.txBytes << ",\n"
                          << "      \"Tx Offered (Mbps)\": " << txOffered << ",\n"
                          << "      \"Rx Packets\": " << i->second.rxPackets << ",\n"
                          << "      \"Rx Bytes\": " << i->second.rxBytes << ",\n"
                          << "      \"Mean Delay (ms)\": " << meanDelay << ",\n"
                          << "      \"Mean Jitter (ms)\": " << meanJitter << ",\n"
                          << "      \"Throughput (Mbps)\": " << throughput << ",\n"
                          << "      \"Packet Loss Ratio (%)\": " << packetLossRatio << "\n"
                          << "    }";
    }
    flowmonOutputFile << "\n  ]\n}\n";

    flowmonOutputFile.close();
    Simulator::Destroy();
    g_resultsFileStream.close();

    return 0;
}

void PrintAppsOnEachNode()
{
    for (uint32_t nodeId = 0; nodeId < NodeList::GetNNodes(); nodeId++)
    {
        Ptr<Node> node = NodeList::GetNode(nodeId);
        uint32_t nApps = node->GetNApplications();

        // Get the IP address of the node
        Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
        Ipv4Address ipAddr = ipv4->GetAddress(1, 0).GetLocal();

        std::cout << "Node " << nodeId << " (IP: " << ipAddr << ") has " << nApps
                  << " applications installed:" << std::endl;

        for (uint32_t i = 0; i < nApps; i++)
        {
            Ptr<Application> app = node->GetApplication(i);
            std::cout << "  Application " << i << ": " << app->GetInstanceTypeId().GetName()
                      << std::endl;
        }
    }
}
