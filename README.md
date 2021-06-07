# Multithreaded Scheduling Simulator

### How to Run:

#### Generating CPU bursts and interarrival times from an exponentially distributed random variable
```
$ make
$ ./schedule <N> <Bcount> <minB> <avgB> <minA> <avgA> <ALG>
```

- N is the number of PS threads
- Bcount is the number of bursts that each thread will generate 
- avgB is the parameter of random exponential distribution of burst time 
- minB specifies the minimum value of burst time  
- avgA is the parameter of random exponential distribution of interarrival time between two consecutive bursts
- minA specifies the minimum value of interarrival time between two bursts 
- ALG specifies the scheduling algorithm to use from “FCFS”, “SJF”, “PRIO”, “VRUNTIME”. 

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
