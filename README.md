# Multithreaded Scheduling Simulator

### How to Run:

#### Generating CPU bursts and interarrival times from an exponentially distributed random variable
```
$ make
$ ./schedule <N> <Bcount> <minB> <avgB> <minA> <avgA> <ALG>
```

Example run
```
$ make
$ ./schedule 375 100 200 1000 1500 FCFS
```

#### Reading CPU bursts and interarrival times from a file
```
$ ./schedule <N> <ALG> -f <inprefix>
```

Example run
```
$ ./schedule 4 FCFS -f infile
```

### How to Use:
