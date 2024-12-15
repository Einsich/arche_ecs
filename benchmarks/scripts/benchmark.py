import os
print("Running benchmark")
Tests = [
  (1, "cold_cache", "Perfromance test with cold cache, 1 run"),
  (2, "warm_cache", "Perfromance test with warm cache, 2 runs"),
  (4, "warmer_cache", "Perfromance test with warmer cache, 4 runs"),
  (8, "hot_cache", "Perfromance test with hot cache, 8 runs"),
]
for test in Tests:
  os.system(f"{os.getcwd()}/bin/rel/sources/tests/benchmark.exe --repeat_count={test[0]} --output=benchmarks/{test[1]}.csv")
  os.system(f"python3 benchmarks/scripts/build_plot.py {test[1]} \"{test[2]}\"")
