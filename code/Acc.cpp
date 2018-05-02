////////////////////////////////////////////////////////////
//
// MIT License
//
// Copyright(c) 2018 Dustin Hauptman - dhauptman.dh@gmail.com
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// The origin of this software must not be misrepresented; you must not claim
// that you wrote the original software.If you use this software in a product,
// an acknowledgment of the software used is required.
//
////////////////////////////////////////////////////////////

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

  // Probably should grab this from an input...nah CTRL-C + CTRL-V
  percentage_file.open("Percentage_out.txt");
  statistics_file.open("Statistics_out.txt");

  delta_exec.open("out/delta_exec");
  if(!delta_exec) {
    std::cerr << "Unable to open delta_exec\n";
    return(-1);
  }

  weight.open("out/Weight");
  if(!weight) {
    std::cerr << "Unable to open Weight\n";
    return(-1);
  }

  lw_weight.open("out/Load\ Weight");
  if(!lw_weight) {
    std::cerr << "Unable to open lw_weight\n";
    return(-1);
  }

  lw_inv_weight.open("out/Inverted\ Weight");
  if(!lw_inv_weight) {
    std::cerr << "Unable to open lw_inv_weight\n";
    return(-1);
  }

  uint64_t delta_exec_val;
  uint32_t weight_val;
  load_weight lw;


  //accumulators
  accumulator_set<double, features<tag::mean, tag::variance, tag::max, tag::min > > acc_new;
  accumulator_set<double, features<tag::mean, tag::variance, tag::max, tag::min > > acc_old;

  // All the files should be the same legnth so just read from 1 until the end of the file
  while (delta_exec >> delta_exec_val) {
    weight >> weight_val;
    lw_weight >> lw.weight;
    lw_inv_weight >> lw.inv_weight;

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


  }
  //Close da files
  delta_exec.close();
  weight.close();
  lw_weight.close();
  lw_inv_weight.close();
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
  return 0;
}
