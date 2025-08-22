#include <iostream>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>

using namespace std;

// Function declarations
void reveal(int, int);
void create_mine_positions();
void cell_number(int, int);
void create_table();
void open_cell();
void game();
void print_table(char arr[10][10]);
void place_or_remove_flag();
void input_symbol();
bool end_game_win_check();
void* calculate_elapsed_time(void* arg);

// Global variables
char table[10][10];
char table_mine_positions[10][10];
char symbol;
int flag_counter = 0;
int mines_flagged_counter = 0;
bool end_game_lose = false;
int elapsed_time = 0; // Shared variable for elapsed time
int start_time = 0;   // Store the start time for each new game
int pipe_fd[2]; // Pipe file descriptors

// Game functions
void cell_number(int i, int j) {
    if (i >= 0 && i < 10 && j >= 0 && j < 10 && table_mine_positions[i][j] != 'X')
        table_mine_positions[i][j]++;
}

void create_mine_positions() {
    int counter = 0;
    srand(time(NULL));
    for (int i = 0; i < 10; i++)
        for (int j = 0; j < 10; j++)
            table_mine_positions[i][j] = '0';

    while (counter < 10) {
        int i = rand() % 10;
        int j = rand() % 10;
        if (table_mine_positions[i][j] == '0') {
            table_mine_positions[i][j] = 'X';
            cell_number(i - 1, j);
            cell_number(i + 1, j);
            cell_number(i, j - 1);
            cell_number(i, j + 1);
            cell_number(i - 1, j - 1);
            cell_number(i - 1, j + 1);
            cell_number(i + 1, j - 1);
            cell_number(i + 1, j + 1);
            counter++;
        }
    }
}

void create_table() {
    for (int i = 0; i < 10; i++)
        for (int j = 0; j < 10; j++)
            table[i][j] = '*';
}

void print_table(char arr[10][10]) {
    cout << "    ";
    for (int i = 0; i < 10; i++)
        cout << i << " ";

    cout << endl << "  ";
    for (int i = 0; i < 32; i++)
        cout << "_";
    cout << endl;

    for (int i = 0; i < 10; i++) {
        cout << i << "|";
        for (int j = 0; j < 10; j++)
            cout << arr[i][j] << " ";
        cout << endl;
    }
}

void open_cell() {
    int i, j;
    do
        cin >> i >> j;
    while (i < 0 || i > 9 || j < 0 || j > 9);

    if (table_mine_positions[i][j] == 'X') {
        table[i][j] = 'X';
        end_game_lose = true;
        for (int i = 0; i < 10; i++)
            for (int j = 0; j < 10; j++)
                if (table_mine_positions[i][j] == 'X')
                    table[i][j] = 'X';
    } else
        reveal(i, j);
}

void place_or_remove_flag() {
    int i, j;
    do
        cin >> i >> j;
    while (i < 0 || i > 9 || j < 0 || j > 9);

    if (table[i][j] == '*') {
        table[i][j] = 'F';
        flag_counter++;
        if (table_mine_positions[i][j] == 'X')
            mines_flagged_counter++;
    } else if (table[i][j] == 'F') {
        table[i][j] = '*';
        flag_counter--;
        if (table_mine_positions[i][j] == 'X')
            mines_flagged_counter--;
    }
}

void input_symbol() {
    cin >> symbol;
    switch (symbol) {
        case 'o': open_cell(); break;
        case 'f': place_or_remove_flag(); break;
        default: input_symbol();
    }
}

void reveal(int i, int j) {
    if (table[i][j] == '*' && table_mine_positions[i][j] != 'X' && i >= 0 && i < 10 && j >= 0 && j < 10) {
        table[i][j] = table_mine_positions[i][j];
        if (table_mine_positions[i][j] == '0') {
            reveal(i, j - 1);
            reveal(i, j + 1);
            reveal(i - 1, j - 1);
            reveal(i + 1, j - 1);
            reveal(i + 1, j + 1);
            reveal(i - 1, j + 1);
            reveal(i - 1, j);
            reveal(i + 1, j);
        }
    }
}

bool end_game_win_check() {
    return (flag_counter == 10 && mines_flagged_counter == 10);
}

void* calculate_elapsed_time(void* arg) {
    start_time = time(0);  // Set start time when the timer begins
    while (!end_game_lose && !end_game_win_check()) {
        time_t current_time = time(0);
        elapsed_time = current_time - start_time;  // Update elapsed time
        // Write the updated elapsed time to the pipe
        write(pipe_fd[1], &elapsed_time, sizeof(elapsed_time));
        sleep(1); // Sleep for 1 second to simulate the timer
    }
    return nullptr;
}

void game() {
    create_table();
    create_mine_positions();

    // Create a pipe
    if (pipe(pipe_fd) == -1) {
        cerr << "Pipe creation failed!" << endl;
        return;
    }

    pid_t pid = fork();
    if (pid == 0) {
        // Child process - Timer
        pthread_t time_thread;
        pthread_create(&time_thread, nullptr, calculate_elapsed_time, nullptr);
        pthread_join(time_thread, nullptr); // Wait for timer thread to finish
        exit(0); // End child process
    } else {
        // Parent process - Game logic
        while (!end_game_lose && !end_game_win_check()) {
            print_table(table);
            cout << endl << "Flags: " << flag_counter << endl;

            // Read the elapsed time from the pipe after each turn
            read(pipe_fd[0], &elapsed_time, sizeof(elapsed_time));

            cout << "Total Steps: " << elapsed_time << endl;
            input_symbol();
        }
        wait(NULL); // Wait for the child process to finish
    }

    if (end_game_lose) {
        print_table(table);
        cout << "\nGAME OVER. You stepped on a mine and LOST.\n";
    }

    if (end_game_win_check()) {
        cout << "You WON! Time to complete: " << elapsed_time << " seconds\n";
    }
}

int main() {
    cout << "Rules:\nEnter 'o', then i and j to open cell[i][j].\nEnter 'f', then i and j to place or remove flag.\n\nPress any key to play...";
    cin.ignore();
    cin.get();
    system("clear");

    game();
    return 0;
}

