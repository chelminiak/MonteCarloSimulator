/*
 * Copyright (c) 2023 AGH University of Science and Technology
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
 */


#ifndef MONTECARLOSIMULATOR_H
#define MONTECARLOSIMULATOR_H

// Add a doxygen group for this module.
// If you have more than one file, this should be in only one of them.
/**
 * \defgroup MonteCarloSimulator Description of the MonteCarloSimulator
 */

#include "ns3/application-container.h"

namespace ns3
{
/**
 * This object implements the Monte Carlo simulator object, capable of conducting Monte Carlo
 * simulations
 */
class MonteCarloSimulator
{
  public:
    /**
     * The constructor of the MonteCarloSimulator library. This method copies a reference to
     * ApplicationContainer with sinkApplications and schedules appropriate number of rounds,
     * based on the provided arguments
     * @param sinkApplications a reference to ApplicationContainer with Application Sinks,
     * which are used in default per-flow reward calculation
     * @param numberOfRounds number of scheduled rounds (starting from round 1; simulator allows
     * the user to run "zero round" with the desired pre-conditions (configured outside this
     * simulator). The MonteCarloSimulator will gather statistics from that round, but scheduling
     * occurs from round 1 to round number numberOfRounds
     * @param roundTime time of a single round
     * @param roundWarmup time of the warmup period (period from which statistics are not included
     * in reward calculation)
     * @param outputName name for the output .csv file with per-flow rewards from each round;
     * the reward from each round is a average throughput obtained during rounds in which the flow
     * was active (its throughput was higher then 0)
     * @param resultsPrinting number of the first round from which results are printed in the
     * console; creation of output file is enabled from round 0
     * @param useDefaultRewardCalculation boolean value used for either scheduling the default
     * warmup statistics collector and reward calculator or to set these methods later using
     * separate methods
     * @param BehaviourFunction function with the behaviour of nodes in the network; this function
     * must be of void() type and cannot take any arguments
     */
    MonteCarloSimulator(ApplicationContainer* sinkApplications, double numberOfRounds,
                        double roundTime, double roundWarmup, std::string outputName,
                        uint32_t resultsPrinting, bool useDefaultRewardCalculation,
                        std::function<void()> BehaviourFunction);
    /**
     * Return the pointer to integer with current round number
     * @return the pointer to integer with current round number
     */
    int* GetCurrentRound();
    /**
     * Return the pointer to array with the number of times the flow was used; index in array
     * corresponds with a number of the flow
     * @return the pointer to array with the number of times the flow was used
     */
    double *GetChooseArray();
    /**
     * Return the pointer to per-flow number of bytes transmitted during the course of the whole
     * simulation; index in array corresponds with a number of the flow
     * @return the pointer to the per-flow number of bytes transmitted during the course of the
     * whole simulation
     */
    u_int32_t *GetTotalBytes();
    /**
     * Return the pointer to the array with throughputs obtained by each flow in consecutive rounds;
     * first is the number of flow and the second is number of the round
     * @return the pointer to the array with throughputs obtained by each flow in consecutive rounds
     */
    double (*GetThroughputArray())[500];
    /**
     * Return the array with sum of the throughputs obtained by each flow in consecutive rounds;
     * first index is the number of flow and the second is number of the round
     * @return the pointer to the array with sum of the throughputs obtained by each flow in
     * consecutive rounds
     */
    double *GetThroughputSumArray();
    /**
     * Return the pointer to the array with per-flow rewards obtained in consecutive rounds;
     * first index is the number of flow and the second is number of the round
     * @return the pointer to the array with per-flow rewards obtained in consecutive rounds
     */
    double (*GetRewardArray())[500];
    /**
     * Set the custom reward calculation function; this method should be of void() type, take
     * no input parameters and store the values of the rewards in rewardArray
     * @param RewardCalculationFunction tthe custom reward calculation function
     */
    void SetRewardCalculationFunction(std::function<void()> RewardCalculationFunction);
    /**
     * Set the custom statistics collector, invoked in each round after roundWarmup time;
     * this method should be of void() type and take no input parameters
     * @param WarmupStatisticsFunction the custom statistics collector, invoked in each round after
     * roundWarmup time
     */
    void SetWarmupStatisticsCollectionFunction(std::function<void()> WarmupStatisticsFunction);
    /**
     * Set the bool() function with no input parameters, which after returning true value ends
     * whole simulation, regardless of the number of remaining rounds
     * @param EndConditionFunction the bool() function with no input parameters, which after
     * returning true value ends whole simulation, regardless of the number of remaining rounds
     */
    void SetEndConditionFunction(std::function<bool()> EndConditionFunction);

  private:
    ApplicationContainer* sinks;
    double rounds;
    double time;
    double warmup;
    int printing;
    std::string outputFileName;
    double chooseArray[200];
    uint32_t totalBytes[200];
    double throughputArray[200][500];
    double throughputSumArray[200];
    double rewardArray[200][500];
    int currentRound = 0;
    bool useDefaultCalculation;
    /*
     * Function gathering the total number of bytes transmitted before the warm-up period after
     * the "warmup" time; the previous periods are not included during reward calculation
     */
    void GetWarmupStatistics();
    /**
     * Results handler; by default this function stores all results in filename.csv and prints
     * the results in the console starting from the "printing" round
     */
    void HandleResults();
    /*
     * Function with the default reward calculation method: average throughput granted to a flow
     * in a rounds when the flow was active (its throughput was higher than 0)
     */
    void DefaultRewardCalculation();
    /**
     * Function to check whether the file with the filename name exists in the output location;
     * of the file exists the results are added to the bottom of the file
     * @param filename name of the .csv file (passed without the ".csv") to be checked
     * @return true if the file exists or false in the other case
     */
    bool FileExists(const std::string& filename);
};

}

#endif /* MONTECARLOSIMULATOR_H */
