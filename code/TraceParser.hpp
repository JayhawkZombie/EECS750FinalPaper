#include <iostream>
#include <algorithm>
#include <string>

#include <boost/xpressive/xpressive.hpp>
#include <boost/xpressive/xpressive_fwd.hpp>

namespace xp = boost::xpressive;

struct output_nested_results
{
    int tabs_;

    output_nested_results( int tabs = 0 )
        : tabs_( tabs )
    {
    }

    template< typename BidiIterT >
    void operator ()( xp::match_results< BidiIterT > const &what ) const
    {
        // first, do some indenting
        typedef typename std::iterator_traits< BidiIterT >::value_type char_type;
        char_type space_ch = char_type(' ');
        std::fill_n( std::ostream_iterator<char_type>( std::cout ), tabs_ * 4, space_ch );

        // output the match
        std::cout << what[0] << '\n';

        // output any nested matches
        std::for_each(
            what.nested_results().begin(),
            what.nested_results().end(),
            output_nested_results( tabs_ + 1 ) );
    }
};

class TraceParser
{
public:

  TraceParser()
  {
    any_ws = xp::as_xpr('\t') | xp::as_xpr(' ');
    sp     = xp::as_xpr(' ');
    col    = xp::as_xpr(':');
    string = -+xp::_w
             [ std::cout << "string " << xp::_ ];
    trace_line = string
             [ std::cout << "Matched: " << xp::_ ];
  }
    
    /*
    number = -+xp::digit;
    comma  = xp::as_xpr(',');
    core_number = '[' >> number >> ']';
    
    time_seconds      = -+xp::digit;
    time_microseconds = -+xp::digit;

    curr_sys_time = time_seconds >> '.' >> time_microseconds;
    curr_function = string       >> '.' >> string 
                                 >> '.' >> number >> col;

    variable_value_pair = ~(xp::set=col) >> col >> any_ws >> number;
    */

    void TryParse(const std::string &line)
    {
      xp::smatch match;
      if (xp::regex_match(line, match, trace_line))
      {
        std::cout << "Successfully matched some stuff!" << "\n";

        std::for_each
        (
          match.nested_results().begin(),
          match.nested_results().end(),
          output_nested_results()
        );

      }
    }

private:

  xp::sregex any_ws, sp, col, comma;
  xp::sregex time_seconds, time_microseconds;
  xp::sregex string, number, core_number, curr_sys_time, curr_function, which_trace;
  xp::sregex variable_value_pair;
  xp::sregex trace_line;



};
