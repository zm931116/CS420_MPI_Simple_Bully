#include <stdio.h>
#include "simplebully.h"


MPI_Comm comm = MPI_COMM_WORLD;

int MAX_ROUNDS = 1;						// number of rounds to run the algorithm
double TX_PROB = 1.0 - ERROR_PROB;		// probability of transmitting a packet successfully

unsigned long int get_PRNG_seed()
{
	struct timeval tv;
	gettimeofday(&tv,NULL);
	unsigned long time_in_micros = 1000000 * tv.tv_sec + tv.tv_usec + getpid();//find the microseconds for seeding srand()

	return time_in_micros;
}        

bool is_timeout(time_t start_time)
{
	// YOUR CODE GOES HERE
}


bool try_leader_elect()
{
	// first toss a coin: if prob > 0.5 then attempt to elect self as leader
	// Otherwise, just keep listening for any message
	double prob = rand() / (double) RAND_MAX;       // number between [0.0, 1.0]
	bool leader_elect = (prob > THRESHOLD);
	
	return leader_elect;
}

void graceful_exit(int rank, int error)
{
    char error_string[ERR_BUF_SIZE];
    int err_str_len;

    MPI_Error_string(error, error_string, &err_str_len);
    fprintf(stderr, "%d: %s\n", rank, error_string);
    MPI_Finalize();
    exit(EXIT_FAILURE);
}


int main(int argc, char *argv[])
{
	int rank
	int size;
	mpi_error;
	
	
	MPI_Init(&argc, &argv);
	MPI_Comm_size(comm, &size);
	MPI_Comm_rank(comm, &rank);

	if (size < 4)
	{
		printf("Error, won't work without at least four workers!\n");
		graceful_exit(rank, mpi_error;)
	}

	if (argv[1] > size || argv[1] < 0)
	{
		printf("Error, initial leader is greater or less than worker count!\n");
		graceful_exit(rank, mpi_error);
	}	

	srand(get_PRNG_seed());
    	
	// argv[1]: designated initial leader
	// argv[2]: how many rounds to run the algorithm
	// argv[3]: packet trasnmission success/failure probability
	int current_leader = argv[1];
	int successor = (current_leader + 1) % 5;
	int predecessor = current_leader - 1;

	if (predecessor < 0)
	{
		predecessor = (size - 1);
	}

	int rounds = argv[2];
	int probability = argv[3];
	int token;
    
	printf("\n*******************************************************************");
	printf("\n*******************************************************************");
	printf("\n Initialization parameters:: \n\tMAX_ROUNDS = %d \n\tinitial leader = %d \n\tTX_PROB = %f\n", MAX_ROUNDS, current_leader, TX_PROB);
	printf("\n*******************************************************************");
	printf("\n*******************************************************************\n\n");

	for (int round = 0; round < MAX_ROUNDS; round++)
	{
		printf("\n*********************************** ROUND %d ******************************************\n", round);
	
		if (rank == current_leader)
		{
        	if (try_leader_elect())
        	{
				// then send a leader election message to next node on ring, after
				// generating a random token number. Largest token among all nodes will win.
				
				printf("\n[rank %d][%d] SENT LEADER ELECTION MSG to node %d with TOKEN = %d, tag = %d\n", rank, round, succ, mytoken, LEADER_ELECTION_MSG_TAG);
				fflush(stdout);
			} 
			else
			{
				// Otherwise, send a periodic HELLO message around the ring
			
				printf("\n[rank %d][%d] SENT HELLO MSG to node %d with TOKEN = %d, tag = %d\n", rank, round, succ, mytoken, HELLO_MSG_TAG);
				fflush(stdout);
			}
		
			// Now issue a speculative MPI_IRecv() to receive data back
			int recv_buf[2];

			
			// If time out happens, then we shall remove the pending MPI_IRecv() [for demonstrating MPI_Test(), MPI_Cancel() and MPI_Request_free()]
			// Otherwise, we receive the message and decide appropriate action based on the message TAG
			if ( )
			{
				printf("\n[rank %d][%d] TIME OUT on HELLO MESSAGE! Cancelling speculative MPI_IRecv() issued earlier\n", rank, round);

				printf("\n[rank %d][%d] Cancelled speculative MPI_IRecv() issued earlier\n", rank, round);
				fflush(stdout);
			}
			
			if ( )
			{
				// If HELLO MSG received, do nothing
				// If LEADER ELECTION message, then determine who is the new leader and send out a new leader notification message
				switch (status.MPI_TAG) 
				{
					case HELLO_MSG_TAG:
						printf("\n[rank %d][%d] HELLO MESSAGE completed ring traversal!\n", rank, round);
						fflush(stdout);
						break;
					case LEADER_ELECTION_MSG_TAG:
						// Send a new leader message
						printf("\n[rank %d][%d] NEW LEADER FOUND! new leader = %d, with token = %d\n", rank, round, current_leader, recv_buf[1]);
						fflush(stdout);
						break;
					default: ;	// do nothing
				}
			}
		}
		else
		{
			if ( )
			{
				// You want to first receive the message so as to remove it from the MPI Buffer	

				if (status.MPI_TAG == HELLO_MSG_TAG)
				{
					// With a probability 'p', forward the message to next node
					// This simulates link or node failure in a distributed system
					if ( ) 
					{
						printf("\n\t[rank %d][%d] Received and Forwarded HELLO MSG to next node = %d\n", rank, round, succ);
						fflush(stdout);
					}
					else
					{
						printf("\n\t[rank %d][%d] WILL NOT FORWARD HELLO MSG to next node = %d\n", rank, round, succ);
						fflush(stdout);
					}
				} 
				else if (status.MPI_TAG == LEADER_ELECTION_MSG_TAG)
				{
					// Fist probabilistically see if wants to become a leader.
					// If yes, then generate own token and test if can become leader.
					// If can become leader, then update the LEADER ELECTION Message appropriately and retransmit to next node
					// Otherwise, just forward the original received LEADER ELECTION Message
					if ( )
					{
						printf("\n\t[rank %d][%d] My new TOKEN = %d\n", rank, round, mytoken);
						fflush(stdout);
					}
					else
					{
							printf("\n\t[rank %d][%d] Will not participate in Leader Election.\n", rank, round);
							fflush(stdout);
					}

					// Forward the LEADER ELECTION Message

					// Finally, wait to hear from current leader who will be the next leader
					printf("\n\t[rank %d][%d] NEW LEADER :: node %d with TOKEN = %d\n", rank, round, current_leader, recv_buf[1]);
					fflush(stdout);
					
					// Forward the LEADER ELECTION RESULT MESSAGE
				}		
			}	
		}
		// Finally hit barrier for synchronization of all nodes before starting new round of message sending
	}
	printf("\n** Leader for NODE %d = %d\n", rank, current_leader);

	MPI_Finalize();

	return 0;
}
