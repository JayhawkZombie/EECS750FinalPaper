all:
	driver
	acc

driver:
	g++ -std=c++17 -O2 driver.cpp -o driver

compute_stats:
	@./driver -n data/tracing_data_new.txt -o data/tracing_data_old.txt --out-file out/tracing_results --out-folder out -mvcsa
	@./driver -n data/cgroup_data_new.txt -o data/cgroup_data_old.txt --out-folder cgroups --out-file cgroups/tracing_results -mvcsa

acc:
	g++ -std=c++17 -O2 Acc.cpp -o acc

accuracy:
	@./acc