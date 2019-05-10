
CIS630, Spring 2019, Term Project 1


Please complete the following information, save this file and submit it along with your program

Your First and Last name: Amie Corso
Your Student ID: 951528694

Submission Date: 5/10/19

What programming language did you use to write your code? 
    c

Does your program compile on ix (CIS department server)? Yes/No
    yes

How should we compile your program on ix? (please provide a makefile)
    "make"

Does your program run on ix? Yes/No
    yes

Does your program calculate the credit values accurately? Yes/No
    yes

Does your program have a limit for the number of nodes it can handle in the input graph? Yes/No
If yes, what is the limit on graph size that your program can handle?
    yes, 2,000,000 nodes
    (I know this is bad form, but as this is not part of the rubric I expect this to be ok! A possible solution that doesn't take too long would be to fseek to the end of the partition file and observe only the largest node ID before allocating any space.  In this case, that would work because the IDs are sorted, but I believe that sorted IDs weren't necessarily guaranteed in the specification, so it would be necessary to read the entire file once before performing any allocations in order to allocate the exact amount of space.)

How long does it take for your program with two partitions to read the Flickr input files, perform 5 round and write 
the output of each round in the output files on ix-dev?
    <15 seconds (for both 2 and 4 partitions)

Does your program run correctly with four partitions on ix-dev? Yes/No
    yes

Does you program end gracefully after completing the specified number of rounds? Yes/No
    yes
