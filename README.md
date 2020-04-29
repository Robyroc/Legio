# Legio
Library to introduce fault-tolerance in MPI in the form of graceful degradation

> We are led on by courage, and obedience, and fortitude, which [...] does not desert us in our ill fortune.
>
> -- <cite>Titus Flavius Josephus</cite>

## Overview
Legio is a library that introduces fault-tolerance in MPI applications in the form of graceful degradation. It's designed for embarrassingly parallel applications. It is based on [ULFM](https://fault-tolerance.org/2017/11/03/ulfm-2-0/).
## Usage
One of the key aspects of Legio is the transparency of integration: no changes in the code are needed, integration is performed via linking. Legio leverages PMPI to catch all the calls toward MPI and wraps them with the appropriate code needed.
If you have ULFM already installed, run:

    $ export ULFM_PREFIX <path-to-ulfm-build-folder>

To compile your application with Legio, put the sources inside the /src folder, then run

    $ make all

To run the application linked with Legio, run

    $ make run

## MPI functions supported
As of now, Legio will introduce fault tolerance on the following functions over MPI_COMM_WORLD

 - MPI_Barrier
 - MPI_Bcast
 - MPI_Send
 - MPI_Recv
 - MPI_Allreduce
 - MPI_Reduce
 - MPI_Win_create
 - MPI_Win_allocate
 - MPI_Win_free
 - MPI_Win_fence
 - MPI_Get
 - MPI_Put
 - MPI_Scatter
 - MPI_Gather
 - MPI_File_open
 - MPI_File_close
 - MPI_File_read_at
 - MPI_File_write_at
 - MPI_File_read_at_all
 - MPI_File_write_at_all
 - MPI_File_seek
 - MPI_File_get_position
 - MPI_File_seek_shared
 - MPI_File_get_position_shared
 - MPI_File_read_all
 - MPI_File_write_all
 - MPI_File_set_view
 - MPI_File_read
 - MPI_File_write
 - MPI_File_read_shared
 - MPI_File_write_shared
 - MPI_File_read_ordered
 - MPI_File_write_ordered

Last two calls need some refinement to adjust position in case of fault.

Support for other calls is under development, as well as support for other communicators.
## Configuration
Under development. 
