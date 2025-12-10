## Machine specs

Apple M4 Pro
24 GB RAM
MacOS 26.1 Tahoe

## Part B results

### No seeds configuration

┌─ process timing ────────────────────────────────────┬─ overall results ────┐
│        run time : 0 days, 0 hrs, 59 min, 58 sec     │  cycles done : 176   │
│   last new find : 0 days, 0 hrs, 0 min, 11 sec      │ corpus count : 38    │
│last saved crash : 0 days, 0 hrs, 49 min, 35 sec     │saved crashes : 24    │
│ last saved hang : none seen yet                     │  saved hangs : 0     │
├─ cycle progress ─────────────────────┬─ map coverage┴──────────────────────┤
│  now processing : 35*1 (92.1%)       │    map density : 5.71% / 6.21%      │
│  runs timed out : 1 (2.63%)          │ count coverage : 1.86 bits/tuple    │
├─ stage progress ─────────────────────┼─ findings in depth ─────────────────┤
│  now trying : trim 32/32             │ favored items : 14 (36.84%)         │
│ stage execs : 768/994 (77.26%)       │  new edges on : 18 (47.37%)         │
│ total execs : 487k                   │ total crashes : 418k (24 saved)     │
│  exec speed : 945.4/sec              │  total tmouts : 98 (0 saved)        │
├─ fuzzing strategy yields ────────────┴─────────────┬─ item geometry ───────┤
│   bit flips : 2/448, 0/447, 0/445                  │    levels : 5         │
│  byte flips : 0/56, 0/55, 1/53                     │   pending : 7         │
│ arithmetics : 0/3878, 0/7416, 0/7100               │  pend fav : 0         │
│  known ints : 0/486, 0/2016, 0/2903                │ own finds : 37        │
│  dictionary : 0/0, 0/0, 0/0, 0/0                   │  imported : 0         │
│havoc/splice : 42/469k, 0/0                         │ stability : 100.00%   │
│py/custom/rq : unused, unused, unused, unused       ├───────────────────────┘
│    trim/eff : 4.48%/16.0k, 94.64%                  │             [cpu: 42%]
└─ strategy: explore ────────── state: in progress ──┘

### With seeds configuration

┌─ process timing ────────────────────────────────────┬─ overall results ────┐
│        run time : 0 days, 0 hrs, 59 min, 56 sec     │  cycles done : 66    │
│   last new find : 0 days, 0 hrs, 2 min, 19 sec      │ corpus count : 393   │
│last saved crash : 0 days, 0 hrs, 37 min, 17 sec     │saved crashes : 39    │
│ last saved hang : none seen yet                     │  saved hangs : 0     │
├─ cycle progress ─────────────────────┬─ map coverage┴──────────────────────┤
│  now processing : 387.29 (98.5%)     │    map density : 4.21% / 11.79%     │
│  runs timed out : 0 (0.00%)          │ count coverage : 3.81 bits/tuple    │
├─ stage progress ─────────────────────┼─ findings in depth ─────────────────┤
│  now trying : havoc                  │ favored items : 26 (6.62%)          │
│ stage execs : 300/320 (93.75%)       │  new edges on : 34 (8.65%)          │
│ total execs : 1.75M                  │ total crashes : 252k (39 saved)     │
│  exec speed : 257.7/sec              │  total tmouts : 0 (0 saved)         │
├─ fuzzing strategy yields ────────────┴─────────────┬─ item geometry ───────┤
│   bit flips : 12/1.03M, 7/1.03M, 5/1.03M           │    levels : 7         │
│  byte flips : 1/129k, 0/129k, 0/129k               │   pending : 0         │
│ arithmetics : 19/9.03M, 1/18.0M, 0/18.0M           │  pend fav : 0         │
│  known ints : 0/1.16M, 4/4.90M, 4/7.22M            │ own finds : 382       │
│  dictionary : 0/0, 0/0, 0/507k, 0/507k             │  imported : 0         │
│havoc/splice : 62/1.08M, 0/0                        │ stability : 100.00%   │
│py/custom/rq : unused, unused, unused, unused       ├───────────────────────┘
│    trim/eff : 54.52%/549k, 99.89%                  │             [cpu: 35%]
└─ strategy: explore ────────── state: in progress ──┘

When running the application without seeds, the execution speed is very high (945.4/sec). This is because the fuzzer generates short and simple inputs. However, the total map coverage is quite low at 6.21%. 
The opposite happens when we run the application with seeds, the execution speed is much lower (257.7/sec) but the coverage is much higher at 11.79%. The execution speed decreased a lot because the fuzzer has to generate more complex inputs to achieve more coverage. 
The seedless run found 24 crashes while the seeded run found 39 crashe which was also expected because the better inputs allow the fuzzer to find more bugs that a seedless run cannot find.
 
 ## Part C results

 ┌─ process timing ────────────────────────────────────┬─ overall results ────┐
│        run time : 0 days, 0 hrs, 59 min, 54 sec     │  cycles done : 0     │
│   last new find : 0 days, 0 hrs, 15 min, 24 sec     │ corpus count : 342   │
│last saved crash : 0 days, 0 hrs, 8 min, 2 sec       │saved crashes : 27    │
│ last saved hang : none seen yet                     │  saved hangs : 0     │
├─ cycle progress ─────────────────────┬─ map coverage┴──────────────────────┤
│  now processing : 139*1 (40.6%)      │    map density : 6.54% / 11.80%     │
│  runs timed out : 1 (0.29%)          │ count coverage : 3.74 bits/tuple    │
├─ stage progress ─────────────────────┼─ findings in depth ─────────────────┤
│  now trying : trim 16/16             │ favored items : 18 (5.26%)          │
│ stage execs : 199/248 (80.24%)       │  new edges on : 30 (8.77%)          │
│ total execs : 115k                   │ total crashes : 1256 (27 saved)     │
│  exec speed : 24.36/sec (slow!)      │  total tmouts : 0 (0 saved)         │
├─ fuzzing strategy yields ────────────┴─────────────┬─ item geometry ───────┤
│   bit flips : 5/1.03M, 10/1.03M, 3/1.03M           │    levels : 3         │
│  byte flips : 0/129k, 0/129k, 1/129k               │   pending : 304       │
│ arithmetics : 6/9.03M, 2/18.0M, 0/18.0M            │  pend fav : 0         │
│  known ints : 0/1.16M, 1/4.90M, 2/7.22M            │ own finds : 331       │
│  dictionary : 0/0, 0/0, 0/120k, 0/120k             │  imported : 0         │
│havoc/splice : 22/2276, 0/0                         │ stability : 100.00%   │
│py/custom/rq : unused, unused, unused, unused       ├───────────────────────┘
│    trim/eff : 43.17%/49.6k, 99.90%                 │             [cpu: 30%]
└─ strategy: explore ────────── state: in progress ──┘


When using the ASAN and UBSAN sanitizers, the execution speed is much lower (24.36/sec) but the coverage reached 11.80%. The execution speed is very low because the sanitizers have to check shadow memory, redzones, and undefined behavior at runtime. 
This run has 27 saved crashes which is less than the seeded run in part B but these crashes might be caused by bugs that would not be detected in runs without the sanitizers.

## Part D results
### Machine specs

Apple M4 
16 GB RAM
MacOS Sequoia 15.5

### Mutator logic

The custom mutator parses the PNG file into chunks, modifies them then puts them back in the PNG file. It targets the IHDR chunk to try to get a buffer overflow and it randomly removes chunks to find crashes caused by missing data. The mutator also recalculates the CRC of every modified chunk to try finding more vulnerabilities. 

┌─ process timing ────────────────────────────────────┬─ overall results ────┐
│        run time : 0 days, 1 hrs, 1 min, 15 sec      │  cycles done : 38    │
│   last new find : 0 days, 0 hrs, 56 min, 53 sec     │ corpus count : 14    │
│last saved crash : 0 days, 1 hrs, 1 min, 13 sec      │saved crashes : 1     │
│ last saved hang : 0 days, 0 hrs, 13 min, 43 sec     │  saved hangs : 1     │
├─ cycle progress ─────────────────────┬─ map coverage┴──────────────────────┤
│  now processing : 10.258 (71.4%)     │    map density : 16.67% / 22.22%    │
│  runs timed out : 0 (0.00%)          │ count coverage : 16.00 bits/tuple   │
├─ stage progress ─────────────────────┼─ findings in depth ─────────────────┤
│  now trying : havoc                  │ favored items : 2 (14.29%)          │
│ stage execs : 12/50 (24.00%)         │  new edges on : 2 (14.29%)          │
│ total execs : 65.6k                  │ total crashes : 44.9k (1 saved)     │
│  exec speed : 0.00/sec (zzzz...)     │  total tmouts : 4856 (0 saved)      │
├─ fuzzing strategy yields ────────────┴─────────────┬─ item geometry ───────┤
│   bit flips : 0/1008, 0/1007, 0/1005               │    levels : 2         │
│  byte flips : 0/126, 0/125, 0/123                  │   pending : 0         │
│ arithmetics : 0/8806, 0/17.4k, 0/17.1k             │  pend fav : 0         │
│  known ints : 0/1127, 0/4718, 0/6861               │ own finds : 4         │
│  dictionary : 0/0, 0/0, 0/0, 0/0                   │  imported : 0         │
│havoc/splice : 0/30.6k, 0/0                         │ stability : 100.00%   │
│py/custom/rq : unused, 3/30.5k, unused, unused      ├───────────────────────┘
│    trim/eff : 46.20%/3936, 99.21%                  │             [cpu:131%]
└─ strategy: exploit ────────── state: finished... ──┘