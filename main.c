#include <stdio.h>
#include "simplebully.h"


int MAX_ROUNDS = 10;					// number of rounds to run the algorithm
double TX_PROB = 1.0 - ERROR_PROB;		// probability of transmitting a packet successfully


unsigned long int get_PRNG_seed()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	unsigned long time_in_micros = 1000000 * tv.tv_sec + tv.tv_usec + getpid();//find the microseconds for seeding srand()

	return time_in_micros;
}

bool try_leader_elect()
{
	// first toss a coin: if prob > 0.5 then attempt to elect self as leader
	// Otherwise, just keep listening for any message
	double prob = rand() / (double) RAND_MAX;       // number between [0.0, 1.0]
	bool leader_elect = (prob > THRESHOLD);
	
	return leader_elect;
}

bool try_hello(int probability)
{
	double hello = rand() / (double) probability;
	bool flag = (hello > probability);

	return flag;
}

void graceful_exit(int rank, int error)
{
    char error_string[50];
    int err_str_len;

    MPI_Error_string(error, error_string, &err_str_len);
    fprintf(stderr, "%d: %s\n", rank, error_string);
    MPI_Finalize();
    exit(EXIT_FAILURE);
}


int main(int argc, char *argv[])
{
	int rank;
	int size;
	int mpi_error;
	
	MPI_Init(&argc, &argv);
	MPI_Comm_size(comm, &size);
	MPI_Comm_rank(comm, &rank);

	if (size < 4)
	{
		printf("Error, won't work without at least four workers!\n");
		graceful_exit(rank, mpi_error);
	}

	int arg_one = atoi(argv[1]);
	int arg_two = atoi(argv[2]);
	int arg_three = atoi(argv[3]);

	if (arg_one > size || arg_one < 0)
	{
		printf("Error, initial leader is greater or less than worker count!\n");
		graceful_exit(rank, mpi_error);
	}

	srand(get_PRNG_seed());

	MPI_Status *status = (MPI_Status *) malloc((size - 1) * sizeof(MPI_Status));
    	
	// argv[1]: designated initial leader
	// argv[2]: how many rounds to run the algorithm
	// argv[3]: packet trasnmission success/failure probability
	int current_leader = arg_one;
	int successor = (rank + 1) % 5;
	int predecessor = rank - 1;

	if (predecessor < 0)
	{
		predecessor = (size - 1);
	}

	int rounds = arg_two;

	int probability;
	if (argc == 4)
		probability = arg_three;
	else
		probability = TX_PROB;

	int hello = HELLO_MSG;
	int *receive_array[2];
	int token;

	MPI_Request *request = (MPI_Request *) malloc((size - 1) * sizeof(MPI_Request));
    
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
				token = rand() % MAX_TOKEN_VALUE;
				int election[2];
				election[0] = rank;
				election[1] = token;

				if (mpi_error = (MPI_Send(election,
										  1,
										  MPI_INT,
										  successor,
										  LEADER_ELECTION_MSG_TAG,
										  comm)) != MPI_SUCCESS)
				{
					graceful_exit(rank, mpi_error);
				}				
				
				printf("\n[rank %d][%d] SENT LEADER ELECTION MSG to node %d with TOKEN = %d, tag = %d\n", rank, round, successor, token, LEADER_ELECTION_MSG_TAG);
				fflush(stdout);
			} 
			else
			{
				// Otherwise, send a periodic HELLO message around the ring
				if (mpi_error = (MPI_Send(&hello, 
										  1,
										  MPI_INT,
										  successor,
										  HELLO_MSG_TAG,
										  comm) != MPI_SUCCESS))
				{
					graceful_exit(rank, mpi_error);
				}

				printf("\n[rank %d][%d] SENT HELLO MSG to node %d with TOKEN = %d, tag = %d\n", rank, round, successor, token, HELLO_MSG_TAG);
				fflush(stdout);
			}
		
			// Now issue a speculative MPI_IRecv() to receive data back
			int *receive_array[2];
			MPI_Request *request = (MPI_Request *) malloc((size - 1) * sizeof(MPI_Request));

			if (mpi_error = (MPI_Irecv(receive_array,
									   2,
									   MPI_INT,
									   MPI_ANY_SOURCE,
									   MPI_ANY_TAG,
									   comm,
									   request)) != MPI_SUCCESS)
			{
				graceful_exit(rank, mpi_error);
			}
			
			// If time out happens, then we shall remove the pending MPI_IRecv() [for demonstrating MPI_Test(), MPI_Cancel() and MPI_Request_free()]
			// Otherwise, we receive the message and decide appropriate action based on the message TAG
			int flag = 0;
			bool terminate = false;
			time_t start, end;
			double elapsed;
			start = time(NULL);

			while (true)
			{
				if (terminate)
				{
					printf("\n[rank %d][%d] TIME OUT on HELLO MESSAGE! Cancelling speculative MPI_IRecv() issued earlier\n", rank, round);

					printf("\n[rank %d][%d] Cancelled speculative MPI_IRecv() issued earlier\n", rank, round);
					fflush(stdout);
					break;
				}
				else
				{
					// TODO: MPI_Test() instead?
					if (MPI_Iprobe(predecessor, MPI_ANY_TAG, comm, &flag, status) == true)
					{
						// If HELLO MSG received, do nothing
						// If LEADER ELECTION message, then determine who is the new leader and send out a new leader notification message
						switch (status->MPI_TAG) 
						{
							case HELLO_MSG_TAG:
								printf("\n[rank %d][%d] HELLO MESSAGE completed ring traversal!\n", rank, round);
								fflush(stdout);
								break;
							case LEADER_ELECTION_MSG_TAG:
								// Send a new leader message
								// TODO: Who is the new leader? then broadcast

								current_leader = *receive_array[0];

								for (int i = 0; i < size; i++)
								{
									if (mpi_error = (MPI_Send(receive_array,
														      2,
														  	  MPI_INT,
														  	  i,
														  	  LEADER_ELECTION_MSG_TAG,
														  	  comm)) != MPI_SUCCESS)
								{
									graceful_exit(rank, mpi_error);
								}

								if (mpi_error = (MPI_Irecv(receive_array,
														   2,
														   MPI_INT,
														   rank,
														   LEADER_ELECTION_MSG_TAG,
														   comm,
														   request)) != MPI_SUCCESS)
								{
									graceful_exit(rank, mpi_error);
								}

								printf("\n[rank %d][%d] NEW LEADER FOUND! new leader = %d, with token = %d\n", rank, round, current_leader, *receive_array[1]);
								fflush(stdout);

								break;
								}
								
							default: ;	// do nothing
									
						}

						break;
					}
				}

				end = time(NULL);
				elapsed = difftime(end, start);
				if (elapsed >= 3)
					terminate = true;
			}		

			MPI_Barrier(comm);
		}
		else
		{
			bool terminate = false;
			time_t start, end;
			double elapsed;
			start = time(NULL);
			int flag = 0;

			while (true)
			{
				if (terminate)
				{
					printf("\n[rank %d][%d] TIME OUT on HELLO MESSAGE! Cancelling speculative MPI_IRecv() issued earlier\n", rank, round);

					printf("\n[rank %d][%d] Cancelled speculative MPI_IRecv() issued earlier\n", rank, round);
					fflush(stdout);
					break;
				}
				else
				{
					if (MPI_Iprobe(predecessor, MPI_ANY_TAG, comm, &flag, status) == true)
					{
						// If HELLO MSG received, do nothing
						// If LEADER ELECTION message, then determine who is the new leader and send out a new leader notification message
						switch (status->MPI_TAG) 
						{
							case HELLO_MSG_TAG:
								// With a probability 'p', forward the message to next node
								// This simulates link or node failure in a distributed system

								if (try_hello(probability))
								{
									if (mpi_error = (MPI_Send(&hello, 
															  1,
															  MPI_INT,
															  successor,
															  HELLO_MSG_TAG,
															  comm) != MPI_SUCCESS))
									{
										graceful_exit(rank, mpi_error);
									}

									printf("\n\t[rank %d][%d] Received and Forwarded HELLO MSG to next node = %d\n", rank, round, successor);
									fflush(stdout);
								}
								else
								{
									printf("\n\t[rank %d][%d] WILL NOT FORWARD HELLO MSG to next node = %d\n", rank, round, successor);
									fflush(stdout);
								}
								break;

							case LEADER_ELECTION_MSG_TAG:
								// Fist probabilistically see if wants to become a leader.
								// If yes, then generate own token and test if can become leader.
								// If can become leader, then update the LEADER ELECTION Message appropriately and retransmit to next node
								// Otherwise, just forward the original received LEADER ELECTION Message
								if (try_leader_elect())
								{
									token = rand() % MAX_TOKEN_VALUE;
									printf("\n\t[rank %d][%d] My new TOKEN = %d\n", rank, round, token);
									fflush(stdout);
								}
								else
								{
									printf("\n\t[rank %d][%d] Will not participate in Leader Election.\n", rank, round);
									fflush(stdout);
								}

								// Forward the LEADER ELECTION Message
								int election[2];
								election[0] = rank;
								election[1] = token;

								if (mpi_error = (MPI_Send(election,
										 				  1,
														  MPI_INT,
														  successor,
														  LEADER_ELECTION_MSG_TAG,
														  comm)) != MPI_SUCCESS)
								{
									graceful_exit(rank, mpi_error);
								}			

								// Finally, wait to hear from current leader who will be the next leader
								MPI_Request *request = (MPI_Request *) malloc((size - 1) * sizeof(MPI_Request));
								if (mpi_error = (MPI_Irecv(receive_array, 
														   2,
														   MPI_INT,
														   MPI_ANY_SOURCE,
														   MPI_ANY_TAG,
														   comm,
														   request)) != MPI_SUCCESS)
								{
									graceful_exit(rank, mpi_error);
								}

								printf("\n\t[rank %d][%d] NEW LEADER :: node %d with TOKEN = %d\n", rank, round, current_leader, *receive_array[1]);
								fflush(stdout);
								
								// Forward the LEADER ELECTION RESULT MESSAGE
								election[0] = *receive_array[0];
								election[1] = *receive_array[1];

								if (mpi_error = (MPI_Send(election,
										 				  1,
														  MPI_INT,
														  successor,
														  LEADER_ELECTION_MSG_TAG,
														  comm)) != MPI_SUCCESS)
								{
									graceful_exit(rank, mpi_error);
								}
								break;
							default: ;	// do nothing
						}

						break;
					}

					MPI_Barrier(comm);
				}

				end = time(NULL);
				elapsed = difftime(end, start);
				if (elapsed >= 3)
					terminate = true;
			}
	
		}
		// Finally hit barrier for synchronization of all nodes before starting new round of message sending
	}
	printf("\n** Leader for NODE %d = %d\n", rank, current_leader);

	MPI_Finalize();

	return 0;
}
