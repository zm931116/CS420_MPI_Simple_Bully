// Authors: Joshua Sonnenberg and Ben McConnel

#include <stdio.h>
#include "simplebully.h"

int MAX_ROUNDS = 3;
double TX_PROB = 1.0 - ERROR_PROB;



unsigned long int get_PRNG_seed()
{
	struct timeval tv;
	gettimeofday(&tv,NULL);
	unsigned long time_in_micros = 1000000 * tv.tv_sec + tv.tv_usec + getpid();

	return time_in_micros;
}

bool try_leader_elect()
{
    double prob = rand() / (double) RAND_MAX;
	return (prob > THRESHOLD);
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
    int successor;
    int predecessor;
    int *recieve_buf = (int *) malloc(2 * sizeof(int));
    int hello = HELLO_MSG;
    int current_leader = 0;
    int token;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_size(comm, &size);
    MPI_Comm_rank(comm, &rank);
    printf("\nNo. of procs = %d, proc ID = %d initialized...\n", size, rank);
    
    current_leader = atoi(argv[1]);
	int MAX_ROUNDS = atoi(argv[2]);
    int TX_PROB = atoi(argv[3]);

    MPI_Status status;

    srand(get_PRNG_seed());

    if (rank == 0)
    {
        predecessor = size - 1;
    }
    else
    {
        predecessor = rank - 1;
    }
        

    if (rank == (size - 1))
    {
        successor = 0;
    }
    else
    {
        successor = rank + 1;
    }

    printf("\n*******************************************************************"); 
	printf("\n*******************************************************************");
	printf("\n Initialization parameters:: \n\tMAX_ROUNDS = %d \n\tinitial leader = %d \n\tTX_PROB = %d\n", MAX_ROUNDS, current_leader, TX_PROB);
	printf("\n*******************************************************************");
	printf("\n*******************************************************************\n\n");

    for (int round = 0; round < MAX_ROUNDS; round++)
    {
        printf("\n*********************************** ROUND %d ******************************************\n", round);
	
        if (rank == current_leader)
        {
            if (try_leader_elect())
            {
                token = generate_token();
                recieve_buf[0] = rank;
                recieve_buf[1] = token;

                if (mpi_error = (MPI_Send(recieve_buf,
                                          2,
                                          MPI_INT,
                                          successor,
                                          LEADER_ELECTION_MSG_TAG,
                                          comm)) != MPI_SUCCESS)
                {
                    graceful_exit(rank, mpi_error);
                }

                printf("\n[rank %d][%d] SENT LEADER ELECTION MSG to node %d with TOKEN = %d, tag = %d\n", rank, round, successor, token, LEADER_ELECTION_MSG_TAG);
            }
            else
            {
                if (mpi_error = (MPI_Send(&hello,
                                          1,
                                          MPI_INT,
                                          successor,
                                          HELLO_MSG_TAG,
                                          comm)) != MPI_SUCCESS)
                {
                    graceful_exit(rank, mpi_error);
                }

                printf("\n[rank %d][%d] SENT HELLO MSG to node %d with TOKEN = %d, tag = %d\n", rank, round, successor, token, HELLO_MSG_TAG);
            }

            MPI_Request request;

            if (mpi_error = (MPI_Irecv(recieve_buf,
                                      2,
                                      MPI_INT,
                                      predecessor,
                                      MPI_ANY_TAG,
                                      comm,
                                      &request)) != MPI_SUCCESS)
            {
                graceful_exit(rank, mpi_error);
            }

            bool terminate = false;
            int flag = 0;
            time_t start;
            time_t end;
            double elapsed;
            start = time(NULL);

            while (!flag && !terminate)
            {
                if (terminate)
                {
                    MPI_Cancel(&request);
                    MPI_Request_free(&request);

                    printf("n[rank %d][%d] Leader didn't hear back!\n", rank, round);
					printf("\n[rank %d][%d] Cancelled speculative MPI_IRecv() issued earlier\n", rank, round);
                
                    break;
                }
                else
                {
                    MPI_Test(&request, &flag, &status);
                }

                end = time(NULL);
				elapsed = difftime(end, start);
				if (elapsed >= 3)
                {
					terminate = true;
                }
            }

            if (!terminate && flag)
            {
                switch (status.MPI_TAG)
                {
                    case HELLO_MSG_TAG:
                    {
                        printf("\n[rank %d][%d] HELLO MESSAGE completed ring traversal!\n", rank, round);

                        break;
                    }
                    case LEADER_ELECTION_MSG_TAG:
                    {
                        current_leader = recieve_buf[0];

                        if (mpi_error = (MPI_Send(recieve_buf,
                                                  2,
                                                  MPI_INT,
                                                  successor,
                                                  LEADER_ELECTION_RESULT_MSG_TAG,
                                                  comm)) != MPI_SUCCESS)
                        {
                            graceful_exit(rank, mpi_error);
                        }

                        if (mpi_error = (MPI_Recv(recieve_buf,
                                                  2,
                                                  MPI_INT,
                                                  predecessor,
                                                  LEADER_ELECTION_RESULT_MSG_TAG,
                                                  comm,
                                                  &status)) != MPI_SUCCESS)
                        {
                            graceful_exit(rank, mpi_error);
                        }

                        printf("\n[rank %d][%d] NEW LEADER FOUND! new leader = %d, with token = %d\n", rank, round, current_leader, recieve_buf[1]);	

                        break;
                    }

                    default:;
                }
            }
        }
        else
        {
            bool terminate = false;
            int flag = 0;
            time_t start;
            time_t end;
            double elapsed;
            start = time(NULL);

            while (!flag && !terminate)
            {
                if (terminate)
                {
                    printf("n[rank %d][%d] Leader didn't hear back!\n", rank, round);
					printf("\n[rank %d][%d] Cancelled speculative MPI_IRecv() issued earlier\n", rank, round);
                
                    break;
                }
                else
                {
                    MPI_Iprobe(predecessor,
                               MPI_ANY_TAG,
                               comm,
                               &flag,
                               &status);
                }

                end = time(NULL);
				elapsed = difftime(end, start);
				if (elapsed >= 3)
                {
					terminate = true;
                }
            }

            if (!terminate)
            {
                switch (status.MPI_TAG)
                {
                    case HELLO_MSG_TAG:
                    {
                        if (mpi_error = (MPI_Recv(&recieve_buf[0],
                                                   1,
                                                   MPI_INT,
                                                   predecessor,
                                                   HELLO_MSG_TAG,
                                                   comm,
                                                   &status)) != MPI_SUCCESS)
                        {
                            graceful_exit(rank, mpi_error);
                        }

                        if (get_prob() < TX_PROB)
                        {
                            printf("\n\t[rank %d][%d] Received and Forwarded HELLO MSG to next node = %d\n", rank, round, successor);

                            if (mpi_error = (MPI_Send(&hello,
                                                      1,
                                                      MPI_INT,
                                                      successor,
                                                      HELLO_MSG_TAG,
                                                      comm)) != MPI_SUCCESS)
                            {
                                graceful_exit(rank, mpi_error);
                            }
                        }
                        else
                        {
                            printf("\n\t[rank %d][%d] WILL NOT FORWARD HELLO MSG to next node = %d\n", rank, round, successor);
                        }

                        break;
                    }

                    case LEADER_ELECTION_MSG_TAG:
                    {
                        if (mpi_error = (MPI_Recv(recieve_buf,
                                                  2,
                                                  MPI_INT,
                                                  predecessor,
                                                  LEADER_ELECTION_MSG_TAG,
                                                  comm,
                                                  &status)) != MPI_SUCCESS)
                        {
                            graceful_exit(rank, mpi_error);
                        }

                        if (try_leader_elect())
                        {
                            token = generate_token();

                            printf("\n\t[rank %d][%d] My new TOKEN = %d\n", rank, round, token);
                        
                            if (token > recieve_buf[1])
                            {
                                recieve_buf[0] = rank;
                                recieve_buf[1] = token;
                            }
                        }
                        else
                        {
                            printf("\n\t[rank %d][%d] Will not participate in Leader Election.\n", rank, round);
                        }

                        if (mpi_error = (MPI_Send(recieve_buf,
                                                  2,
                                                  MPI_INT,
                                                  successor,
                                                  LEADER_ELECTION_MSG_TAG,
                                                  comm)) != MPI_SUCCESS)
                        {
                            graceful_exit(rank, mpi_error);
                        }

                        if (mpi_error = (MPI_Recv(recieve_buf,
                                                  2,
                                                  MPI_INT,
                                                  predecessor,
                                                  LEADER_ELECTION_RESULT_MSG_TAG,
                                                  comm,
                                                  &status)) != MPI_SUCCESS)
                        {
                            graceful_exit(rank, mpi_error);
                        }

                        current_leader = recieve_buf[0];

                        printf("\n\t[rank %d][%d] NEW LEADER :: node %d with TOKEN = %d\n", rank, round, current_leader, recieve_buf[1]);
                    
                        if (mpi_error = (MPI_Send(recieve_buf,
                                                  2,
                                                  MPI_INT,
                                                  successor,
                                                  LEADER_ELECTION_RESULT_MSG_TAG,
                                                  comm)) != MPI_SUCCESS)
                        {
                            graceful_exit(rank, mpi_error);
                        }
                    }
                }
            }
        }
    
    MPI_Barrier(comm);
    }
    printf("\n** Leader for NODE %d = %d\n", rank, current_leader);

    MPI_Finalize();

    return 0;
}