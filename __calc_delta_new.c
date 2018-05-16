static u64 __calc_delta(u64 delta_exec,
unsigned long weight, struct load_weight *lw)
{
	u64 val;
	
	val = (!lw->weight) 
	    ? (delta_exec * (u64)weight * (u64)WMULT_CONST)
	    : (delta_exec * (u64)weight / (u64)lw->weight);
	    	
	return val;
}