#include "common.h"
#include "board_utils.h"


#define BOARD_FILE "board"
#define FIFO_NAME "fifo"
#define STEP_COUNT 500
#define WAIT_N 10

#define PORT 12345

#define EPOLL_MAX_EVENTS 10

void usage(char* program_name)
{
    fprintf(stderr, "Usage: \n");

    fprintf(stderr, "\t%s n m\n", program_name);
    fprintf(stderr, "\t  n, m - board width and height, respectively\n");

    exit(EXIT_FAILURE);
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

    if ((fd = open(BOARD_FILE, O_RDWR | O_CREAT | O_TRUNC, 0666)) < 0)
    {
        ERR("Open");
    }

    if (ftruncate(fd, boardSize) < 0)
        ERR("ftruncate");

    char* board = mmap(NULL, boardSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    fill_board(board, n, m);

    if (close(fd) < 0)
        ERR("close");

    // seed
    srand(time(NULL));
    int x = rand() % n;
    int y = rand() % m;
    set_char(board, x, y, n, m, EXPEDITION_CHAR);

    for (int i = 0; i < STEP_COUNT; i++)
    {
        set_char(board, x, y, n, m, TRAIL_CHAR);
        char move = get_random_move(board, x, y, n, m);
        move_pos(board, move, n, m, &x, &y);
        set_char(board, x, y, n, m, EXPEDITION_CHAR);
        ms_sleep(100);
    }

    printf("Smok-Expedition completed!\n");

    if (munmap(board, boardSize) < 0)
        ERR("munmap");

    return EXIT_SUCCESS;
}