#include <boost/xpressive/xpressive.hpp>
#include <boost/xpressive/xpressive_fwd.hpp>
#include <boost/xpressive/regex_actions.hpp>

using namespace boost::xpressive;

struct push_var
{
    // Result type, needed for tr1::result_of
    typedef void result_type;

    template<typename Sequence, typename Value>
    void operator()(Sequence &seq, Value const &val) const
    {
      seq.push(val);
    }
};

function<push_var>::type const push_ = {{}};

mark_tag process_name(1);
mark_tag process_id(2);
mark_tag running_core(3);
mark_tag system_time(4);
mark_tag variable_name(5);
mark_tag variable_value(6);
mark_tag time_seconds(7);
mark_tag time_microseconds(8);

sregex any_ws, sp, col, comma;
sregex string, number, curr_function, which_trace;
sregex variable_value_pair;
sregex trace_line;
sregex vname;
sregex nonsense;
sregex var_list;
