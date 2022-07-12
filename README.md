This tool finds the minimum number of comparisons required to sort N elements, by searching for suitable comparison algorithms, as used in "Lower Bounds for Sorting 16, 17, and 18 Elements" https://arxiv.org/abs/2206.05597.

Building:
---------

When building the program, you need to specify the number of elements N.
The following example builds the program for `N=13`.
```sh
mkdir build
cd build
cmake .. -DNUMEL=13
make -j 6
```

The following options are available when building the program.

| Option                       | Description                                                                         |
|------------------------------|-------------------------------------------------------------------------------------|
| `-DCMAKE_BUILD_TYPE=Debug`   | Build in debug mode (default)                                                       |
| `-DCMAKE_BUILD_TYPE=Release` | Build in release mode. This is faster                                               |
| `-DNUMEL=<number>`           | Set number of elements N                                                            |
| `-DVARIABLE_N=<True/False>`  | If enabled, N can be set when executing the program, `NUMEL` specifies the maximum. |

Usage:
------

Start the program by running the generated executable.
```sh
./sortinglowerbounds
```

### Command Line Options


| Option                                    | Description                                               |
|-------------------------------------------|-----------------------------------------------------------|
| `--help`                                  | produce help message                                      |
| `-i` ( `--interactive` )                  | run in interactive mode - enables tui                     |
| `--forward-search`                        | run forward search, non-interactive                       |
| `--backward-search`                       | run backward search, non-interactive                      |
| `--bidir-search`                          | run bidirectional search, non-interactive                 |
| `-N` ( `--num-elements` ) arg (=13)       | set number of elements N                                  |
| `-C` ( `--num-comparisons` ) arg          | set number of comparisons C                               |
| `-t` ( `--threads` ) arg (=8)             | set number of threads                                     |
| `--eff-bandwidth` arg (=0.125)            | set efficiency bandwidth for bidir search                 |
| `--full-layers` arg (=10)                 | set full bw layers for bidir search                       |
| `--reuse-bw` arg (=1)                     | set whether to reuse bw search results from previous runs |
| `--log-path` arg (=./outputs)             | set directory for log files                               |
| `--bw-path` arg (=./storageBw)            | set directory for backward search storage                 |
| `--tempfile-fast` arg (=./temp_fast.mmap) | fast temp storage file (ssd), fw search only              |
| `--tempfile-slow` arg (=./temp_slow.mmap) | slow temp storage file (hdd), fw search only              |
| `--active-poset-mem` arg (=0.25)          | Memory (RAM) for active posets in Gb                      |
| `--old-poset-mem` arg (=0.25)             | Memory (RAM) for old posets in Gb                         |

Experiments
-----------

We used the following commands to tun our experiments.

```sh
cmake .. -DCMAKE_BUILD_TYPE=Release -DNUMEL=11
make -j 48
./sortinglowerbounds --bidir-search --log-path ../outputs --tempfile-fast /scratch/usfs/storage_fast.mmap --tempfile-slow /scratch_medium/usfs/storage_slow.mmap --active-poset-mem 0.1 --old-poset-mem 0.01 --eff-bandwidth 0.05 --full-layers 4 --reuse-bw false

cmake .. -DCMAKE_BUILD_TYPE=Release -DNUMEL=12
make -j 48
./sortinglowerbounds --bidir-search --log-path ../outputs --tempfile-fast /scratch/usfs/storage_fast.mmap --tempfile-slow /scratch_medium/usfs/storage_slow.mmap --active-poset-mem 0.1 --old-poset-mem 0.01 --eff-bandwidth 0.05 --full-layers 5 --reuse-bw false

cmake .. -DCMAKE_BUILD_TYPE=Release -DNUMEL=13
make -j 48
./sortinglowerbounds --bidir-search --log-path ../outputs --bw-path=../storageBw --tempfile-fast /scratch/usfs/storage_fast.mmap --tempfile-slow /scratch_medium/usfs/storage_slow.mmap --active-poset-mem 0.5 --old-poset-mem 0.1 --eff-bandwidth 0.05 --full-layers 6 --reuse-bw false
./sortinglowerbounds --bidir-search --log-path ../outputs --bw-path=../storageBw --tempfile-fast /scratch/usfs/storage_fast.mmap --tempfile-slow /scratch_medium/usfs/storage_slow.mmap --active-poset-mem 0.5 --old-poset-mem 0.1 --eff-bandwidth 0.08 --full-layers 6 --reuse-bw false
./sortinglowerbounds --forward-search --log-path ../outputs --bw-path=../storageBw --tempfile-fast /scratch/usfs/storage_fast.mmap --tempfile-slow /scratch_medium/usfs/storage_slow.mmap --active-poset-mem 0.5 --old-poset-mem 0.1 --eff-bandwidth 0.05 --full-layers 6 --reuse-bw false
./sortinglowerbounds --backward-search --log-path ../outputs --bw-path=../storageBw --tempfile-fast /scratch/usfs/storage_fast.mmap --tempfile-slow /scratch_medium/usfs/storage_slow.mmap --active-poset-mem 0.5 --old-poset-mem 0.1 --eff-bandwidth 0.05 --full-layers 6 --reuse-bw false

cmake .. -DCMAKE_BUILD_TYPE=Release -DNUMEL=14
make -j 48
./sortinglowerbounds --log-path ../outputs --tempfile-fast /scratch/usfs/storage_fast.mmap --tempfile-slow /scratch_medium/usfs/storage_slow.mmap --active-poset-mem 10 --old-poset-mem 1 --bidir-search --eff-bandwidth 0.12 --full-layers 9 --reuse-bw false

cmake .. -DCMAKE_BUILD_TYPE=Release -DNUMEL=15
make -j 48
./sortinglowerbounds --bidir-search --log-path ../outputs --tempfile-fast /scratch/usfs/storage_fast.mmap --tempfile-slow /scratch_medium/usfs/storage_slow.mmap --active-poset-mem 50 --old-poset-mem 10 --eff-bandwidth 0.15 --full-layers 10 --reuse-bw false

cmake .. -DCMAKE_BUILD_TYPE=Release -DNUMEL=16
make -j 48
./sortinglowerbounds --bidir-search --log-path ../outputs --tempfile-fast /scratch/usfs/storage_fast.mmap --tempfile-slow /scratch_medium/usfs/storage_slow.mmap --active-poset-mem 100 --old-poset-mem 20 --eff-bandwidth 0.20 --full-layers 11 --reuse-bw false

cmake .. -DCMAKE_BUILD_TYPE=Release -DNUMEL=17
make -j 48
./sortinglowerbounds --bd --log-path ../outputs --tempfile-fast /scratch/usfs/storage_fast.mmap --tempfile-slow /scratch_medium/usfs/storage_slow.mmap --active-poset-mem 250 --old-poset-mem 200 --eff-bandwidth 0.24 --full-layers 12 --reuse-bw false

cmake .. -DCMAKE_BUILD_TYPE=Release -DNUMEL=18
make -j 48
./sortinglowerbounds -i --bw-path ../storageBw --log-path ../outputs --tempfile-fast /scratch/usfs/storage_fast.mmap --tempfile-slow /scratch_medium/usfs/storage_slow.mmap --active-poset-mem 250 --old-poset-mem 150 --eff-bandwidth 0.2 --full-layers 13 --reuse-bw false

cmake .. -DCMAKE_BUILD_TYPE=Release -DNUMEL=19
make -j 48
./sortinglowerbounds --bidir-search --log-path ../outputs --tempfile-fast /scratch/usfs/storage_fast.mmap --tempfile-slow /scratch_medium/usfs/storage_slow.mmap --active-poset-mem 50 --old-poset-mem 5 --eff-bandwidth 0.01 --full-layers 8 --reuse-bw false

```

Contributors
------------

The code in this repository is based on an implementation of Peczarski's algorithm by Julian Obst.
It has been developed by Florian Stober and Armin Weiß.
