#include "common.h"
#include "board_utils.h"


#define BOARD_FILE "board"
#define FIFO_NAME "fifo"
#define STEP_COUNT 500
#define WAIT_N 10

#define PORT 12345

#define EPOLL_MAX_EVENTS 10

typedef struct{
    pthread_mutex_t mutex;
    char* board;
    int n;
    int m;
} shared_t;


void usage(char* program_name)
{
    fprintf(stderr, "Usage: \n");

    fprintf(stderr, "\t%s n m\n", program_name);
    fprintf(stderr, "\t  n, m - board width and height, respectively\n");

    exit(EXIT_FAILURE);
}


void donWork(shared_t* shared)
{
    ms_sleep(WAIT_N*100);
    int fdFifo = open(FIFO_NAME, O_RDONLY);
    if (fdFifo < 0)
        ERR("open");

    srand(time(NULL));
    int x = rand() % shared->n;
    int y = rand() % shared->m;
    // set_char(shared->board, x, y, shared->n, shared->m, EXPEDITION_CHAR);

    for (int i = 0; i < STEP_COUNT; i++)
    {

        pthread_mutex_lock(&shared->mutex);

        char readMove;
        pthread_mutex_lock(&shared->mutex);
        int size_read = read(fdFifo, &readMove, sizeof(readMove));
        pthread_mutex_unlock(&shared->mutex);

        if (size_read < 0)
            ERR("read");
        if (!has_trail(shared->board, x, y, shared->n, shared->m))
        {
            printf("Karramba!\n");
        }
        else{
            set_char(shared->board, x, y, shared->n, shared->m, EMPTY_CHAR);
        }
    
        char move = get_random_move(shared->board, x, y, shared->n, shared->m);
        move_pos(shared->board, move, shared->n, shared->m, &x, &y);




        pthread_mutex_unlock(&shared->mutex);
        ms_sleep(100);
        
    }

    close(fdFifo);

    munmap(shared->board, shared->m * (shared->n +1));
    munmap(shared, sizeof(shared_t));
}



void doExpedition(shared_t *shared)
{
    unlink(FIFO_NAME);
    if (mkfifo(FIFO_NAME, 0666)<0)
        ERR("mkfifo"); 
    pid_t pid = fork();
    if (pid < 0)
        ERR("fork");

    if (pid == 0)
    {
        donWork(shared);

        exit(EXIT_SUCCESS);
    }
    else
    {

        
        int fdFifo = open(FIFO_NAME, O_WRONLY);
        if (fdFifo < 0)
        ERR("open");
        srand(time(NULL));
        int x = rand() % shared->n;
        int y = rand() % shared->m;
        // set_char(shared->board, x, y, shared->n, shared->m, EXPEDITION_CHAR);

        for (int i = 0; i < STEP_COUNT; i++)
        {
            pthread_mutex_lock(&shared->mutex);
            set_char(shared->board, x, y, shared->n, shared->m, TRAIL_CHAR);
            char move = get_random_move(shared->board, x, y, shared->n, shared->m);
            move_pos(shared->board, move, shared->n, shared->m, &x, &y);


            char fifoSendMove = get_random_move(shared->board, x, y, shared->n, shared->m);
            pthread_mutex_unlock(&shared->mutex);
            write(fdFifo, &fifoSendMove, sizeof(FIFO_NAME));
            pthread_mutex_lock(&shared->mutex);
            set_char(shared->board, x, y, shared->n, shared->m, EXPEDITION_CHAR);
            pthread_mutex_unlock(&shared->mutex);
            ms_sleep(100);
        }
        close(FIFO_NAME);
    }
    close(FIFO_NAME);
}



int main(int argc, char** argv)
{
    if (argc != 3)
        usage(argv[0]);

    int m, n;
    n = atoi(argv[1]);
    m = atoi(argv[2]);

    if (n <= 2 || m <= 2)
    {
        usage(argv[0]);
    }

    size_t boardSize = m * (n + 1);

    int fd;

    shared_t *shared = mmap(NULL, sizeof(shared_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    shared->m = m;
    shared->n = n;
    if ((fd = open(BOARD_FILE, O_RDWR | O_CREAT | O_TRUNC, 0666)) < 0)
    {
        ERR("Open");
    }

    if (ftruncate(fd, boardSize) < 0)
        ERR("ftruncate");

    shared->board = mmap(NULL, boardSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (close(fd) < 0)
        ERR("close");

    
    fill_board(shared->board, n, m);

    
    

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&shared->mutex, &attr);

    pthread_mutexattr_destroy(&attr);
    
    // seed
    

    printf("Smok-Expedition completed!\n");

    

    pthread_mutex_destroy(&shared->mutex);
    if (munmap(shared->board, boardSize) < 0)
        ERR("munmap");
    munmap(shared, sizeof(shared_t));

    return EXIT_SUCCESS;
}