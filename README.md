## CPU governor latency benchmark

Produces constant load on a single core and measures the
clock frequency of the CPU. This allows to determine the
amount of time a governor needs to ramp up the frequency,
and the time that a CPU needs to enable its turbo mode.

### Build

Make sure to enable optimizations to reduce overhead and
thus make frequency measurements more accurate:

```
gcc -O3 gov_bench.c
```

### Usage

Make sure to minimize the system load on all cores prior
to running the benchmark, and pin it to one core using
taskset:

```
taskset -c 0 ./gov_bench > data.txt
```

The output will have a gnuplot friendly format, so that
you can easily create a plot doing something like this:

```
plot "data.txt" with lines
```
