/*
 * Game of Life - Competitive Biotope Mode (Red vs Blue)
 * 
 * Rules:
 * - Standard Conway survival rules (2 or 3 neighbors).
 * - Birth rule (3 neighbors): Majority color of parents determines child color.
 * - Teams: Red (X) vs Blue (O).
 * - Goal: Highest population after MAX_ROUNDS.
 * // KI-Agent unterstützt
 */

#include <stdio.h>

#include <stdlib.h>

#include <time.h> 

#include <unistd.h> 

#include "game_logic.h" 

#include "gui.h" // KI-Agent unterstützt



// Prototypen

void print_world(World *current_gen, int rows, int cols);



int main(int argc, char *argv[]) {

    // KI-Agent unterstützt: Switching to GUI mode

    printf("Starting Biotope GUI...\n");

    run_gui_app();

    return 0;



    /* CLI LOGIC COMMENTED OUT FOR GUI MIGRATION

    printf("argc: %i\n", argc);

    if (argc < 2)

    {

        printf("./main <rows> <columns> <delay milli-sec>\n");

        return 1;

    }

    

    int rows = atoi(argv[1]);

    int cols = atoi(argv[2]);

    

    if (rows > 2000 || cols > 2000) { 

        printf("Error: Grid too large (max 2000x2000)\n");

        return 1;

    }



    int delay_my = atoi(argv[3]) * 1000;

    

    World *current_gen = create_world(rows, cols);

    World *next_gen = create_world(rows, cols);



    init_world(current_gen, rows, cols);



    int turns = 0;

    while (turns < MAX_ROUNDS) 

     {

        print_world(current_gen, rows, cols);

        printf("Turn: %i / %d\n", turns, MAX_ROUNDS); 

        usleep(delay_my);

        update_generation(current_gen, next_gen, rows, cols);



        World *temp = current_gen;

        current_gen = next_gen;

        next_gen = temp;

        

        turns++;

    }



    int final_red = 0;

    int final_blue = 0;

    for (int i = 0; i < rows * cols; i++) {

        if (current_gen->grid[i] == TEAM_RED) final_red++;

        else if (current_gen->grid[i] == TEAM_BLUE) final_blue++;

    }



    printf("\n--- GAME OVER ---\n");

    printf("Final Score:\nRed: %d\nBlue: %d\n", final_red, final_blue);

    if (final_red > final_blue) {

        printf("Winner: RED TEAM\n");

    } else if (final_blue > final_red) {

        printf("Winner: BLUE TEAM\n");

    } else {

        printf("Result: DRAW\n");

    }



    free_world(current_gen);

    free_world(next_gen);

    */

    // return 0;

}



void print_world(World *current_gen, int rows, int cols)

{

    system("clear");

    int red_count = 0;

    int blue_count = 0;



    for (int i = 0; i < (rows * cols); i++)

    {

        if (current_gen->grid[i] == TEAM_RED) {

            printf("X"); 

            red_count++;

        } else if (current_gen->grid[i] == TEAM_BLUE) {

            printf("O");

            blue_count++;

        } else {

            printf(" ");

        }



        if ((i+1) % cols == 0)

        {

            printf("\n");

        }

    }

    printf("\nStats: Red: %d | Blue: %d\n", red_count, blue_count);

}
