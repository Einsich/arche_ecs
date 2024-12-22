import os

if False:
  print("Running benchmark for iterations")
  Tests = [
    (1, "cold_cache", "Perfromance test with cold cache, 1 run"),
    (2, "warm_cache", "Perfromance test with warm cache, 2 runs"),
    (4, "warmer_cache", "Perfromance test with warmer cache, 4 runs"),
    (8, "hot_cache", "Perfromance test with hot cache, 8 runs"),
  ]
  for test in Tests:
    os.system(f"{os.getcwd()}/bin/rel/sources/tests/benchmark.exe --repeat_count={test[0]} --output=benchmarks/{test[1]}.csv")
    os.system(f"python3 benchmarks/scripts/build_plot.py benchmarks/{test[1]} \"{test[2]}\" \"(3, 6, 9, 10, 11, 14, 17)\"")

if True:
  print("Running benchmark for entities creation")
  Tests = [
    ("entity_creation", "Compare creation of entities one by one or by group"),
  ]
  for test in Tests:
    os.system(f"{os.getcwd()}/bin/rel/sources/tests/benchmark_creation.exe --output=benchmarks/{test[0]}.csv")
    os.system(f"python3 benchmarks/scripts/build_plot.py benchmarks/{test[0]} \"{test[1]}\" \"(1, 2, 3, 6, 4, 5)\"")
