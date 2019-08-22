
# Thread Scheduling

University of Victoria - CSC 360 Operating Systems (Fall 2014): Project 2

This project implements an automated control system for a simulated railway station that processes trains for departure (i.e., to emulate the scheduling of multiple threads sharing a common resource in a real operating system).

## Usage

    trains <txt_filename> <number_of_trains>

The <txt_filename> parameter is the name of the input file containing the definitions of the trains. A few sample files have been provided called "hightrains.txt" and "lonetrains.txt". The <number_of_trains> parameter is an integer greater than zero and is the number of trains specified in the input file. Check the "p2.pdf" file for more details.
