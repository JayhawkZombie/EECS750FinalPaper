#include <stdint.h>
#include <iostream>
#include <fstream>
#include <stdio.h>

//Boost Libraries
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/max.hpp>
#include <boost/accumulators/statistics/min.hpp>
#include <boost/accumulators/statistics/variance.hpp>
using namespace boost::accumulators;

//Math
#include <math.h>


#define BITS_PER_LONG 32
#define WMULT_CONST (~0U)

struct load_weight {
  unsigned long weight;
  uint32_t inv_weight;
};

static inline uint64_t mul_u64_u32_shr(uint64_t a, uint32_t mul, unsigned int shift)
{
  uint32_t ah, al;
  uint64_t ret;

  al = a;
  ah = a >> 32;

  ret = ((uint64_t)al * mul) >> shift;
  if (ah) {
    ret += ((uint64_t)ah * mul) << (32 - shift);
  }
  return ret;
}

static void __update_inv_weight(struct load_weight *lw)
{
  unsigned long w;

  if (lw->inv_weight)
  return;

  w = lw->weight >> 10;

  if (BITS_PER_LONG > 32 && w >= WMULT_CONST)
  lw->inv_weight = 1;
  else if (!w)
  lw->inv_weight = WMULT_CONST;
  else
  lw->inv_weight = WMULT_CONST / w;
}

static uint64_t Old_Calc(uint64_t delta_exec, unsigned long weight, struct load_weight *lw)
{

  uint64_t fact = weight >> 10;
  int shift = 32;
  __update_inv_weight(lw);
  if (fact >> 32) { // Why this and not just the while loop?
    while (fact >> 32) {
      fact >>= 1;
      shift--;
    }
  }
  /* hint to use a 32x32->64 mul */
  fact = (uint64_t)(uint32_t)fact * lw->inv_weight;
  while (fact >> 32) {
    fact >>= 1;
    shift--;
  }
  uint64_t val;
  val =  mul_u64_u32_shr(delta_exec, fact, shift);
  return val;
}

static uint64_t New_calc(uint64_t delta_exec, unsigned long weight, struct load_weight *lw)
{
  uint64_t val;
  val = (!lw->weight) ? (delta_exec * (uint64_t)weight *  (uint64_t)WMULT_CONST) : (delta_exec * (uint64_t)weight /  (uint64_t)lw->weight);
  return val;
}

double floatCalculation(double delta_exec, float weight, float lw_weight)
{
  return delta_exec * weight / lw_weight;
}


int main() {
  /* TODO: DO THE STUFF */
  double calc1;
  uint64_t calc2, calc3;

  std::ifstream delta_exec;
  std::ifstream weight;
  std::ifstream lw_weight;
  std::ifstream lw_inv_weight;

  std::ofstream percentage_file;
  std::ofstream statistics_file;
  std::ofstream PercentOneToFive;
  std::ofstream PercentSixToTen;
  std::ofstream PercentElevenUp;

  // Probably should grab this from an input...nah CTRL-C + CTRL-V
  percentage_file.open("Percentage_out.txt");
  statistics_file.open("Statistics_out.txt");

  delta_exec.open("out/cgroup/delta_exec");
  if(!delta_exec) {
    std::cerr << "Unable to open delta_exec\n";
    return(-1);
  }

  weight.open("out/cgroup/Weight");
  if(!weight) {
    std::cerr << "Unable to open Weight\n";
    return(-1);
  }

  lw_weight.open("out/cgroup/Load\ Weight");
  if(!lw_weight) {
    std::cerr << "Unable to open lw_weight\n";
    return(-1);
  }

  lw_inv_weight.open("out/cgroup/Inverted\ Weight");
  if(!lw_inv_weight) {
    std::cerr << "Unable to open lw_inv_weight\n";
    return(-1);
  }


  PercentOneToFive.open("Percentage_1_5.txt");
  PercentSixToTen.open("Percentage_6_10.txt");
  PercentElevenUp.open("Percentage_11_up.txt");

  uint64_t delta_exec_val;
  uint32_t weight_val;
  load_weight lw;


  //accumulators
  accumulator_set<double, features<tag::mean, tag::variance, tag::max, tag::min > > acc_new;
  accumulator_set<double, features<tag::mean, tag::variance, tag::max, tag::min > > acc_old;

  // All the files should be the same length
  int count1 = 1, count2 = 1, count3 = 1;
  while (delta_exec >> delta_exec_val && weight >> weight_val && lw_weight >> lw.weight && lw_inv_weight >> lw.inv_weight) {

    //Calculations
    calc1 = floatCalculation(double(delta_exec_val), float(weight_val), float(lw.weight));
    calc2 = Old_Calc(delta_exec_val, weight_val, &lw);
    calc3 = New_calc(delta_exec_val, weight_val, &lw);

    // Min-max stuff for the percentage
    double minimum_new = (calc1 < double(calc3)) ? calc1 : double(calc3);
    double maximum_new = (calc1 >= double(calc3)) ? calc1 : double(calc3);
    double percentage_new = minimum_new/maximum_new;

    double minimum_old = (calc1 < double(calc2)) ? calc1 : double(calc2);
    double maximum_old = (calc1 >= double(calc2)) ? calc1 : double(calc2);
    double percentage_old = minimum_old/maximum_old;

    //accumulators
    acc_new(percentage_new);
    acc_old(percentage_old);

    //File output
    percentage_file << percentage_new << '\t' << percentage_old << '\n';

    //Range Check on the Percentages
    double inv_percentage_old = 1.0 - percentage_old;
    if (inv_percentage_old > .01 && inv_percentage_old <= .05)
    {

      PercentOneToFive << "Instance " << count1 << '\n';
      PercentOneToFive << "\tInput Values:\n\tDelta Exec Prior: " << delta_exec_val << "\tWeight: " << weight_val << "\tLoad Weight: " << lw.weight << "\tInverted Weight: " << lw.inv_weight << '\n';
      PercentOneToFive << "\tDelta Execs:\n\tOld: " << calc2 << "\tNew: " << calc3 << "\tFloat: " << calc1 << '\n';
      PercentOneToFive << "\tPercentages:\n\tOld: " << percentage_old << "\tNew: " << percentage_new << "\n\n";
      count1++;
    }

    else if (inv_percentage_old > .05 && inv_percentage_old <= .1)
    {
      PercentSixToTen << "Instance " << count2 << '\n';
      PercentSixToTen << "\tInput Values:\n\t\tDelta Exec Prior: " << delta_exec_val << "\tWeight: " << weight_val << "\tLoad Weight: " << lw.weight << "\tInverted Weight: " << lw.inv_weight << '\n';
      PercentSixToTen << "\tDelta Execs:\n\t\tOld: " << calc2 << "\tNew: " << calc3 << "\tFloat: " << calc1 << '\n';
      PercentSixToTen << "\tPercentages:\n\t\tOld: " << percentage_old << "\tNew: " << percentage_new << "\n\n";
      count2++;
    }

    else if (inv_percentage_old > .1)
    {
      PercentElevenUp << "Instance " << count3 << '\n';
      PercentElevenUp << "\tInput Values:\n\t\tDelta Exec Prior: " << delta_exec_val << "\tWeight: " << weight_val << "\tLoad Weight: " << lw.weight << "\tInverted Weight: " << lw.inv_weight << '\n';
      PercentElevenUp << "\tDelta Execs:\n\t\tOld: " << calc2 << "\tNew: " << calc3 << "\tFloat: " << calc1 << '\n';
      PercentElevenUp << "\tPercentages:\n\t\tOld: " << percentage_old << "\tNew: " << percentage_new << "\n\n";
      count3++;
    }



  }
  std::cout << "Count 1: " << count1 << "\tCount 2: " << count2 << "\tCount 3: " << count3 << "\n";
  //Close da files
  delta_exec.close();
  weight.close();
  lw_weight.close();
  lw_inv_weight.close();
  PercentOneToFive.close();
  PercentSixToTen.close();
  PercentElevenUp.close();
  percentage_file.close();

  //Show da statistics
  statistics_file << "Statistics on New Operation\n";
  statistics_file << "\tMean:\t" << mean(acc_new) << '\n';
  statistics_file << "\tMin:\t" << min(acc_new) << '\n';
  statistics_file << "\tMax:\t" << max(acc_new) << '\n';
  statistics_file << "\tVariance:\t" << variance(acc_new) << '\n';
  statistics_file << "\tSTDDEV:\t" << sqrt(variance(acc_new)) << '\n';
  //More statistics
  statistics_file << "\nStatistics on Old Operation\n";
  statistics_file << "\tMean:\t" << mean(acc_old) << '\n';
  statistics_file << "\tMin:\t" << min(acc_old) << '\n';
  statistics_file << "\tMax:\t" << max(acc_old) << '\n';
  statistics_file << "\tVariance:\t" << variance(acc_old) << '\n';
  statistics_file << "\tSTDDEV:\t" << sqrt(variance(acc_old)) << '\n';
  statistics_file.close();
  return 0;
}
