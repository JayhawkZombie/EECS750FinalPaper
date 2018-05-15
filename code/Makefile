all: driver acc

driver:
	g++ -std=c++17 -O2 driver.cpp -o driver

stats:
	@./driver -n data/new_kernel/trace_non_cgroup_new.txt -o data/old_kernel/trace_non_cgroup_old.txt --out-file non_cgroup.txt --out-folder out/non_cgroup -mvcsa
	@./driver -n data/new_kernel/trace_cgroup_new.txt -o data/old_kernel/trace_cgroup_old.txt --out-file cgroup.txt --out-folder out/cgroup -mvcsa

acc:
	g++ -std=c++17 -O2 Acc.cpp -o acc

accuracy:
	@./acc

clean:
	rm driver
	rm acc