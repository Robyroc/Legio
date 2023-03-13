<img src="./Legio.svg">

Library to introduce fault-tolerance in MPI in the form of graceful degradation

> We are led on by courage, and obedience, and fortitude, which [...] does not desert us in our ill fortune.
>
> -- <cite>Titus Flavius Josephus</cite>

## Overview

Legio is a library that introduces fault-tolerance in MPI applications in the form of graceful degradation. It's designed for embarrassingly parallel applications. It is based on [ULFM](http://journals.sagepub.com/doi/10.1177/1094342013488238).

## Usage

One of the key aspects of Legio is the transparency of integration: no changes in the code are needed, integration is performed via linking. Legio leverages PMPI to catch all the calls toward MPI and wraps them with the appropriate code needed.
If you have ULFM already installed, run:

    $ export ULFM_PREFIX <path-to-ulfm-install-folder>

To install the Legio library, follow the standard CMake installation procedure:

    $ mkdir build
    $ cd build
    $ cmake -DCMAKE_INSTALL_PREFIX=<installation_prefix> ..
    $ make
    $ make install

## MPI functions supported

The list of the functions supported can be found in [this file](./calls_support.csv), together with an analysis on the difficulty of integration for the non-supported ones and a brief note on what should be done.

Support for other calls is under development.

## Configuration

It is possible to configure the behaviour of the Legio library at configuration time, changing some CMake variables. The following table shows all the possible configurations knobs with their meanings.

| Variable             | Values                        | Default | Meaning                                                                                  |
|----------------------|-------------------------------|---------|------------------------------------------------------------------------------------------|
| WITH_TESTS           | On/Off                        | On      | Build example programs from legiotest directory                                          |
| WITH_SESSION_TESTS   | On/Off                        | Off     | Build sessions unit tests                                                                |
| WITH_EXAMPLES        | On/Off                        | Off     | Build example ULFM programs from testsrc directory                                       |
| BROADCAST_RESILIENCY | On/Off                        | Off     | Specify if the execution can continue whenever the root of a broadcast operation fails   |
| SEND_RESILIENCY      | On/Off                        | On      | Specify if the execution can continue whenever the destination of a send operation fails |
| NUM_RETRY            | any strictly positive integer | 3       | Number of retries for a failed send operation                                            |
| RECV_RESILIENCY      | On/Off                        | Off     | Specify if the execution can continue whenever the source of a recv operation fails      |
| REDUCE_RESILIENCY    | On/Off                        | On      | Specify if the execution can continue whenever the root of a reduce operation fails      |
| GET_RESILIENCY       | On/Off                        | Off     | Specify if the execution can continue whenever the source of a get operation fails       |
| PUT_RESILIENCY       | On/Off                        | On      | Specify if the execution can continue whenever the destination of a put operation fails  |
| GATHER_RESILIENCY    | On/Off                        | On      | Specify if the execution can continue whenever the root of a gather operation fails      |
| GATHER_SHIFT         | On/Off                        | Off     | Specify if failures impact the way data is distributed among the processes               |
| SCATTER_RESILIENCY   | On/Off                        | Off     | Specify if the execution can continue whenever the root of a scatter operation fails     |
| SCATTER_SHIFT        | On/Off                        | Off     | Specify if failures impact the way data is collected from the processes                  |
| LOG_LEVEL            | 1-4                           | 2       | Specify the log level (1->None, 2->Errors, 3->Errors&info, 4->Full)                      |
| SESSION_THREAD       | On/Off                        | Off     | Use a separate thread to handle the horizon communicator initialisation                  |
| WITH_RESTART         | On/Off                        | On      | Include critical nodes restart functionalities                                           |
| CUBE_ALGORITHM       | On/Off                        | Off     | Use the Hypercube LDA instead of the Tree-based one                                      |

To change the default configuration of the Legio library, add options to the cmake command in the form `-D[Variable]=[Value]`.