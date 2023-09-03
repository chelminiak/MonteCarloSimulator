#include "MonteCarloSimulator.h"

#include "ns3/packet-sink.h"
#include "ns3/log.h"
#include "ns3/names.h"
#include "ns3/simulator.h"
#include "functional"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("MonteCarloSimulator");

MonteCarloSimulator::MonteCarloSimulator(ApplicationContainer* sinkApplications,
                                         double numberOfRounds,
                                         double roundTime,
                                         double roundWarmup,
                                         std::string outputName,
                                         uint32_t resultsPrinting,
                                         bool useDefaultRewardCalculation,
                                         std::function<void()> BehaviourFunction)
{
    sinks = sinkApplications;
    rounds = numberOfRounds;
    time = roundTime;
    warmup = roundWarmup;
    outputFileName = outputName + ".csv";
    printing = resultsPrinting;
    useDefaultCalculation = useDefaultRewardCalculation;
    if (useDefaultRewardCalculation)
    {
        Simulator::Schedule(Seconds(warmup),
                            &MonteCarloSimulator::GetWarmupStatistics,this);
    }
    for (int round = 0; round < rounds + 1; ++round)
    {
        if (useDefaultRewardCalculation){
            Simulator::Schedule(Seconds((round + 1) * time),
                                &MonteCarloSimulator::DefaultRewardCalculation, this);
            Simulator::Schedule(Seconds((round + 1) * time + warmup),
                                &MonteCarloSimulator::GetWarmupStatistics, this);
            Simulator::Schedule(Seconds((round + 1) * time),
                                &MonteCarloSimulator::HandleResults, this);
        }
        Simulator::Schedule(Seconds((round + 1) * time),
                            BehaviourFunction);
    }
    for (int i = 0; i < 200; ++i)
    {
        for (int j = 0; j < 500; ++j)
        {
            throughputArray[i][j] = 0;
            throughputSumArray[i] = 0;
            totalBytes[i] = 0;
            chooseArray[i] = 0;
            rewardArray[i][j] = 0;
        }
    }
}

bool
MonteCarloSimulator::FileExists(const std::string& filename)
{
    std::ifstream f(filename.c_str());
    return f.good();
}

void
MonteCarloSimulator::GetWarmupStatistics()
{
    for (uint32_t applicationIndex = 0; applicationIndex < sinks->GetN(); ++applicationIndex)
    {
        totalBytes[applicationIndex] =
            DynamicCast<PacketSink>(sinks->Get(applicationIndex))->GetTotalRx();
    }
}

void
MonteCarloSimulator::DefaultRewardCalculation()
{
    uint64_t totalBytesThroughput[sinks->GetN()];
    for (uint32_t applicationIndex = 0; applicationIndex < sinks->GetN(); ++applicationIndex)
    {
        totalBytesThroughput[applicationIndex] =
            DynamicCast<PacketSink>(sinks->Get(applicationIndex))->GetTotalRx();
        throughputArray[applicationIndex][currentRound] =
            (totalBytesThroughput[applicationIndex] - totalBytes[applicationIndex]) * 8 /
            ((time - warmup) * 1000000.0);
        totalBytes[applicationIndex] = totalBytesThroughput[applicationIndex];
    }
    for (uint32_t applicationIndex = 0; applicationIndex < sinks->GetN(); ++applicationIndex)
    {
        if (throughputArray[applicationIndex][currentRound] > 0)
        {
            chooseArray[applicationIndex] += 1;
            throughputSumArray[applicationIndex] += throughputArray[applicationIndex][currentRound];
            rewardArray[applicationIndex][currentRound] =
                throughputSumArray[applicationIndex] / chooseArray[applicationIndex];
        }
    }
}

void
MonteCarloSimulator::HandleResults()
{
    if (currentRound >= printing)
    {
        std::cout << "Results for round " << currentRound << ": " << std::endl;
        for (uint32_t applicationIndex = 0; applicationIndex < sinks->GetN();
             ++applicationIndex)
        {
            std::cout << "Reward for application number " << applicationIndex << ": "
                      << rewardArray[applicationIndex][currentRound] << std::endl;
        }
    }
    std::ofstream outputFile;

    std::string outputCsv = outputFileName;
    if (FileExists(outputCsv))
    {
        // If the file exists, append to it
        outputFile.open(outputCsv, std::ios::app);
    }
    else
    {
        // If the file does not exist, create it and set the header line
        outputFile.open(outputCsv, std::ios::app);
        outputFile << "StageNumber";
        for (uint32_t applicationIndex = 0; applicationIndex < sinks->GetN();
             ++applicationIndex)
        {
            outputFile << ",Reward" << applicationIndex;
        }
        outputFile << std::endl;
    }

    outputFile << currentRound;
    for (uint32_t applicationIndex = 0; applicationIndex < sinks->GetN();
         ++applicationIndex)
    {
        outputFile << "," << rewardArray[applicationIndex][currentRound];
    }
    outputFile << std::endl;

    outputFile.close();
    currentRound += 1;
}

int*
MonteCarloSimulator::GetCurrentRound()
{
    return &currentRound;
}

double (*MonteCarloSimulator::GetRewardArray())[500]
{
    return rewardArray;
}

double *MonteCarloSimulator::GetChooseArray()
{
    return chooseArray;
}

u_int32_t *MonteCarloSimulator::GetTotalBytes()
{
    return totalBytes;
}

double (*MonteCarloSimulator::GetThroughputArray())[500]
{
    return throughputArray;
}

double *MonteCarloSimulator::GetThroughputSumArray()
{
    return throughputSumArray;
}

void
MonteCarloSimulator::SetRewardCalculationFunction(std::function<void()> RewardCalculationFunction)
{
    if (!useDefaultCalculation)
    {
        for (int round = 0; round < rounds + 1; ++round)
        {
            Simulator::Schedule(Seconds((round + 1) * time), RewardCalculationFunction);
            Simulator::Schedule(Seconds((round + 1) * time),
                                &MonteCarloSimulator::HandleResults,
                                this);
        }
    }
}

void
MonteCarloSimulator::SetWarmupStatisticsCollectionFunction(
    std::function<void()> WarmupStatisticsFunction)
{
    if (!useDefaultCalculation)
    {
        for (int round = 0; round < rounds + 2; ++round)
        {
            Simulator::Schedule(Seconds((round)*time + warmup), WarmupStatisticsFunction);
        }
    }
}

void
MonteCarloSimulator::SetEndConditionFunction(std::function<bool()> EndConditionFunction)
{
    for (int round = 0; round < rounds + 1; ++round)
    {
        Simulator::Schedule(Seconds((round + 1) * time), [EndConditionFunction]()
                            {if (EndConditionFunction()){
                Simulator::Stop();
            }});
    }
}


}