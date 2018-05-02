////////////////////////////////////////////////////////////
//
// MIT License
//
// Copyright(c) 2018 Kurt Slagle - kurt_slagle@yahoo.com
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

#include "TraceParser.hpp"

#include <iostream>
#include <algorithm>
#include <string>
#include <fstream>
#include <map>
#include <iomanip>
#include <sstream>

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/moment.hpp>
#include <boost/accumulators/statistics/median.hpp>
#include <boost/accumulators/statistics/min.hpp>
#include <boost/accumulators/statistics/max.hpp>
#include <boost/accumulators/statistics/variance.hpp>
#include <boost/accumulators/statistics/density.hpp>

#include <boost/xpressive/xpressive.hpp>
#include <boost/xpressive/xpressive_fwd.hpp>
#include <boost/xpressive/regex_actions.hpp>

#include <sstream>
#include <iostream>
#include <queue>

#include "clara.hpp"
#include "Grammar.h"

using namespace boost::xpressive;

std::queue<std::string> var_stack;

int main(int argc, char **argv)
{

  std::string input_file_new, input_file_old, output_file;
  std::string output_folder = "out";
  bool report_min_max = false;
  bool report_max = false;
  bool report_count = false;
  bool report_mean = false;
  bool report_variance = false;
  bool report_stddev = false;
  bool report_density = false;

  using namespace clara;
  auto cli = Opt(input_file_new, "old input file")
             ["-n"]["--new-input-file"]
             ("File to read new kernel's trace data from")
             .required()
           | Opt(input_file_old, "old input file")
             ["-o"]["--old-input-file"]
             ("File to read old kernel's trace data from")
             .required()
           | Opt(output_file, "output file")
             ["--out-file"]
             ("File to write results to")
           | Opt(report_min_max)
             ["-m"]["--minmax"]
             ("Report minimum and maximum of value per variable")
           | Opt(report_count)
             ["-c"]["--count"]
             ("Report count per variable")
           | Opt(report_density)
             ["-d"]["--density"]
             ("Report density per variable")
           | Opt(report_variance)
             ["-v"]["--variance"]
             ("Report variance per variable")
           | Opt(report_stddev)
             ["-s"]["--std-dev"]
             ("Report standard deviation per variable")
           | Opt(report_mean)
             ["-a"]["--average"]
             ("Report average of value per variable")
           | Opt(output_folder, "outfolder")
             ["--out-folder"]
             ("Folder to place output files in")
           ;

  auto result = cli.parse(Args(argc, argv));
  if (!result)
  {
    std::cerr << "Command line error: " << result.errorMessage() << "\n";
    return 1;
  }

  try
  {
    std::string str;

    comma  = as_xpr(',');
    any_ws = as_xpr('\t') | as_xpr(' ');
    sp     = as_xpr(' ');
    col    = as_xpr(':');
    number = +digit;
    string = -+_w;

    vname    = -+~_n;
    nonsense = *~_d;

    sregex var_name_pushed = (vname)
                          [push_(boost::xpressive::ref(var_stack), as<std::string>(_))];
    sregex var_value_pushed = (number)
                          [push_(boost::xpressive::ref(var_stack), as<std::string>(_))];

    var_list   =  var_name_pushed
              >> col
              >> any_ws
              >> var_value_pushed
              ;

    trace_line = -*any_ws 
              >> (process_name= string) 
              >> as_xpr('-') 
              >> (process_id= number)
              >> *any_ws
              >> '[' >> (running_core= number) >> ']'
              >> nonsense
              >> (time_seconds= number) >> '.' >> (time_microseconds= number) >> col
              >> -+_ >> col
              >> ( as_xpr(" Trace New") | as_xpr(" Trace Old" ) )
              >> +(+*any_ws >> var_list >> comma) >> +*any_ws >> var_list
              ;

    smatch match;

    std::map<std::string, boost::accumulators::accumulator_set<uint64_t,
                                       boost::accumulators::stats
                                       <
                                        boost::accumulators::tag::min,
                                        boost::accumulators::tag::max,
                                        boost::accumulators::tag::mean,
                                        boost::accumulators::tag::immediate_mean,
                                        boost::accumulators::tag::median,
                                        boost::accumulators::tag::variance(boost::accumulators::immediate)
                                       >
                                      >> OldStats, NewStats;

    std::ifstream ifile_new(input_file_new);
    std::ifstream ifile_old(input_file_old);

    std::stringstream input_stream_new, input_stream_old;
    std::stringstream output_stream;

    std::ostream& output = output_stream;
    
    if (!ifile_new)
    {
      std::cerr << "Failed to open input file for new kernel's data: " << input_file_new << "\n";
      return 2;
    }

    if (!ifile_old)
    {
      std::cerr << "Failed to open input file for old kernel's data: " << input_file_old << "\n";
    }

    input_stream_new << ifile_new.rdbuf();
    input_stream_old << ifile_old.rdbuf();

    ifile_new.close();
    ifile_old.close();

    std::string str_old, str_new;

    std::map<std::string, std::stringstream> output_streams;

    auto add_vars_old = [&output_streams](auto & Stats) 
    { 
      auto var = var_stack.front();
      var_stack.pop();
      auto val = var_stack.front();
      var_stack.pop();

      auto & stream = output_streams[var];

      uint64_t uval = std::stoul(val);
      Stats[var](uval);
      stream << val << "\n";
    };

    auto add_vars_new = [](auto & Stats) 
    { 
      auto var = var_stack.front();
      var_stack.pop();
      auto val = var_stack.front();
      var_stack.pop();

      uint64_t uval = std::stoul(val);
      Stats[var](uval);
    };

    while (input_stream_new && input_stream_old && std::getline(input_stream_new, str_new) && std::getline(input_stream_old, str_old))
    {
      if (regex_search(str_new, match, trace_line))
      {
        while (!var_stack.empty())
        {
          add_vars_new(NewStats);
        }
      }

      if (regex_search(str_old, match, trace_line))
      {
        while (!var_stack.empty())
        {
          add_vars_old(OldStats);
        }
      }
    }

    for (const auto & vars : NewStats)
    {
      output << vars.first << "\n";
      auto &accum_new = vars.second;
      auto &accum_old = OldStats[vars.first];
      auto &stream = output_streams[vars.first];

      if (report_count)
      {
        const auto & oldval = boost::accumulators::count(accum_old);
        const auto & newval = boost::accumulators::count(accum_new);
        const auto & maxval = std::max(oldval, newval);
        const auto & minval = std::min(oldval, newval);
        output << std::setw(20) << std::left << "\tCount: " << std::setw(20) << std::right << newval << "    " << std::setw(20) << std::right << oldval << "  (" << std::setw(20) << std::right << maxval - minval << ")\n";
      }
      if (report_min_max)
      {
        const auto & oldmin = boost::accumulators::min(accum_old);
        const auto & newmin = boost::accumulators::min(accum_new);
        const auto & oldmax = boost::accumulators::max(accum_old);
        const auto & newmax = boost::accumulators::max(accum_new);

        const auto & minmin = std::min(oldmin, newmin);
        const auto & maxmin = std::max(oldmin, newmin);
        const auto & minmax = std::min(oldmax, newmax);
        const auto & maxmax = std::max(oldmax, newmax);

        output << std::setw(20) << std::left << "\tMin: "   << std::setw(20) << std::right << newmin   << "    " << std::setw(20) << std::right << oldmin << "  (" << std::setw(20) << std::right << maxmin - minmin << ")\n";
        output << std::setw(20) << std::left << "\tMax: "   << std::setw(20) << std::right << newmax   << "    " << std::setw(20) << std::right << oldmax << "  (" << std::setw(20) << std::right << maxmax - minmax << ")\n";
      }
      if (report_variance)
      {
        const auto & oldval = boost::accumulators::variance(accum_old);
        const auto & newval = boost::accumulators::variance(accum_new);
        const auto & maxval = std::max(oldval, newval);
        const auto & minval = std::min(oldval, newval);
        output << std::setw(20) << std::left << "\tVariance: " << std::setw(20) << std::right << newval << "    " << std::setw(20) << std::right << oldval << "  (" << std::setw(20) << std::right << maxval - minval << ")\n";
      }
      if (report_stddev)
      {
        const auto & oldval = std::sqrt(boost::accumulators::variance(accum_old));
        const auto & newval = std::sqrt(boost::accumulators::variance(accum_new));
        const auto & maxval = std::max(oldval, newval);
        const auto & minval = std::min(oldval, newval);
        output << std::setw(20) << std::left << "\tStddev:"    << std::setw(20) << std::right << newval << "    " << std::setw(20) << std::right << oldval << "  (" << std::setw(20) << std::right << maxval - minval << ")\n";
      }
      if (report_mean)
      {
        const auto & oldval = boost::accumulators::mean(accum_old);
        const auto & newval = boost::accumulators::mean(accum_new);
        const auto & maxval = std::max(oldval, newval);
        const auto & minval = std::min(oldval, newval);
        output << std::setw(20) << std::left << "\tMean:"      << std::setw(20) << std::right << newval << "    " << std::setw(20) << std::right << oldval << "  (" << std::setw(20) << std::right << maxval - minval << ")\n";
      }

      output << "\n\n";
    }

    for (auto & out_stream : output_streams)
    {
      std::cout << "Creating output file: " << output_folder << "/" << out_stream.first << "\n";
      std::ofstream ofile(output_folder + "/" + out_stream.first);
      ofile << out_stream.second.str();
      ofile.close();
    }

    if (!output_file.empty())
    {
      std::ofstream outfile(output_file);
      outfile << output_stream.str();
      outfile.close();
    }
    else
    {
      std::cout << output_stream.str() << "\n";
    }
  }
  catch (const std::exception &exc)
  {
    std::cerr << "Exception: " << exc.what() << "\n";
  }

  

  return 0;
}
