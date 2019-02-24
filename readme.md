
## Resource allocation server

This project is a server together with clients built to demonstrate the use of **pthreads** library, unix pipes and signal handling.

#### Description
This project was an assignment for a Concurrent Programming course at the University of Warsaw.

There are k (1 <= k <= 99) types of resources and initially there are n (2 <= n <= 10000) units per each type. Clients need a number of units (no more than floor(n/2)) of the same resource. 

Clients work in pairs of same-resource clients, i.e. they can get their resources assigned, or they can return their resources only together with another client, the same during execution. Clients are paired on-the-run by the server.

Server can be stopped via an SIGINT (Ctrl + C) signal, in this case it ceases to accept any requests. When all previous requests have terminated (and all resources have been returned), it exits. The server can work for an arbitrary long amount of time and can serve any number of requests.
#### How to run?
Build instruction: `cmake ..; make serwer; make client`

Server process can be run with the command `./serwer K N`, where `K` and `N` stand for number of resource types and initial count of those resource types.

Client process can be run with the command `./klient k n w`, where `k` stands for a resource type, `n` for quantity used and `w` for type of work (with a resulting work duration)

#### Simple test
A test can be found in catalog tests and can be run with a `test.sh`. 
#### Communication
Communication between clients and servers is done via unix named pipes (fifos). Interrupts are handled with pthreads-based functions.
