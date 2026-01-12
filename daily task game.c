#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>

struct Player {
    double xp;
    int level;
    int energy;
    int lastDay; // use tm_yday
};

struct Task{
    char TaskName[100];
    char isNew;
    int estimatedTime;
    int energyNeeded;
    int xpGained;
};

void checkLevel(struct Player *p){
    // calculate XP needed to lvl up
    while(1){
        double xpNeeded = 137* pow((p->level+1),1.13);
        if (p->xp >= xpNeeded) {
            p->level++;
            p->xp = p->xp - xpNeeded; // remaining XP
            printf("Congrats! You become LVL %d.\n", p->level);
        }else{
            break;
        }
    }
}

void checkEnergy(struct Player *p,struct Task *t){
    p->energy -= t->energyNeeded;
    if(p->energy <0) {
        p->energy = 0;
    }
}

void resetEnergy(struct Player *p){
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    int today = tm_info->tm_yday;

    if(today !=  p->lastDay){
        p->energy = 100;
        p->lastDay = today;
        printf("New day has begun! Energy replenished!\n");
    }
}

char isNewTask(struct Task *t){
    printf("Is your task a new concept to you?(Y/N)\n");
    scanf(" %c", &t->isNew);
    if(t->isNew != 'n' && t->isNew != 'N' && t->isNew != 'y' && t->isNew != 'Y') {
        printf("Invalid input, try again!\n");
        return isNewTask(t);
    }
    return t->isNew;
}

void calculateEnergyAndXp(struct Task *t){
    if(t->isNew == 'y' || t->isNew == 'Y'){
        t->energyNeeded = t->estimatedTime * 5;
        t->xpGained = t->estimatedTime * 237;
    }
    else if(t->isNew == 'n' || t->isNew == 'N'){
        t->energyNeeded = t->estimatedTime * 3;
        t->xpGained = t->estimatedTime * 157;
    }
}

void addTask(struct Task *t){
    printf("What is the name of task? \n");
    scanf(" %[^\n]", t->TaskName);

    printf("How much do you think it will take to accomplish this task? \n");
    scanf("%d", &t->estimatedTime);
    getchar();

    isNewTask(t);
    calculateEnergyAndXp(t);
}

void addNewTask(struct Task **tasks, int *taskCount){
    struct Task *tmp = realloc(*tasks, sizeof(struct Task) * (*taskCount + 1));
    if(tmp == NULL){
        printf("Memory allocation failed while adding a new task.\n");
        return;
    }
    *tasks = tmp;
    addTask(&(*tasks)[*taskCount]);
    (*taskCount)++;
}

void initPlayer(struct Player *p){
    p->xp = 0;
    p->level = 1;
    p->energy = 100;
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    p->lastDay = tm_info->tm_yday; // safer way of day tracking
}

void completeTask(struct Player *p, struct Task **tasks, int *taskCount, int index){
    struct Task *t = &(*tasks)[index];
    if(p->energy >= t->energyNeeded){
        p->xp += t->xpGained;
        checkEnergy(p, t);
        checkLevel(p);
        printf("Task '%s' completed! XP +%d, Energy -%d\n",
               t->TaskName, t->xpGained, t->energyNeeded);

        // delete task
        if(index < *taskCount - 1){
            memmove(&(*tasks)[index], &(*tasks)[index+1],
                    (*taskCount - index - 1) * sizeof(struct Task));
        }
        (*taskCount)--;

        // shrink memory allocation
        if(*taskCount > 0){
            struct Task *tmp = realloc(*tasks, sizeof(struct Task) * (*taskCount));
            if(tmp != NULL){
                *tasks = tmp;
            }
        } else {
            free(*tasks);
            *tasks = NULL;
        }
    } else {
        printf("Not enough energy to complete '%s'.\n", t->TaskName);
    }
}

void showPlayerStatus(struct Player *p){
    printf("\n--- Player Status ---\n");
    printf("Level: %d\n", p->level);
    printf("XP: %.2f\n", p->xp);
    printf("Energy: %d\n", p->energy);
    printf("---------------------\n");
}

void showTask(struct Task *t){
    printf("\n--- Task Info ---\n");
    printf("Name: %s\n", t->TaskName);
    printf("Estimated Time: %d\n", t->estimatedTime);
    printf("Energy Needed: %d\n", t->energyNeeded);
    printf("XP Gained: %d\n", t->xpGained);
    printf("-----------------\n");
}

void listTasks(struct Task tasks[], int taskCount){
	int i;
    if(taskCount == 0){
        printf("No tasks yet.\n");
        return;
    }
    for(i = 0; i < taskCount; i++){
        printf("%d. %s (Energy: %d, XP: %d)\n",
               i+1, tasks[i].TaskName, tasks[i].energyNeeded, tasks[i].xpGained);
    }
}

void saveGame(struct Player *p, struct Task tasks[], int taskCount){
    FILE *f = fopen("save.dat", "wb");
    if(f == NULL){
        printf("Failed to open save file for writing.\n");
        return;
    }

    fwrite(p, sizeof(struct Player), 1, f);           // player info
    fwrite(&taskCount, sizeof(int), 1, f);            // task count
    if(taskCount > 0){
        fwrite(tasks, sizeof(struct Task), taskCount, f); // tasks
    }
    fclose(f);
}

int loadGame(struct Player *p, struct Task **tasks, int *taskCount){
    FILE *f = fopen("save.dat", "rb");
    if(f == NULL) return 0; // no file -> new game

    size_t readP = fread(p, sizeof(struct Player), 1, f);
    size_t readC = fread(taskCount, sizeof(int), 1, f);
    if(readP != 1 || readC != 1){
        printf("Save file is corrupted.\n");
        fclose(f);
        return 0;
    }

    if(*taskCount > 0){
        *tasks = malloc(sizeof(struct Task) * (*taskCount));
        if(*tasks == NULL){
            printf("Memory allocation failed while loading tasks.\n");
            fclose(f);
            return 0;
        }
        size_t readT = fread(*tasks, sizeof(struct Task), *taskCount, f);
        if(readT != (size_t)(*taskCount)){
            printf("Task data is corrupted or incomplete.\n");
            free(*tasks);
            *tasks = NULL;
            fclose(f);
            return 0;
        }
    } else {
        *tasks = NULL;
    }
    fclose(f);

    return 1; // success
}

int main(){
    struct Task *tasks = NULL;
    int taskCount = 0;
    struct Player player;

    if(loadGame(&player, &tasks, &taskCount) == 0){
        initPlayer(&player);
    }

    int choice;
    while(1){
        resetEnergy(&player);
        printf("\n--- MENU ---\n");
        printf("1. Add Task\n");
        printf("2. List Tasks\n");
        printf("3. Complete Task\n");
        printf("4. Show Player Status\n");
        printf("0. Exit\n");
        printf("Choice: ");
        if(scanf("%d", &choice) != 1){
            printf("Invalid input.\n");
            // clear input
            int ch;
            while((ch = getchar()) != '\n' && ch != EOF);
            continue;
        }
        getchar(); // clear buffer

        if(choice == 1){
            addNewTask(&tasks, &taskCount);
        }
        else if(choice == 2){
            listTasks(tasks, taskCount);
        }
        else if(choice == 3){
            listTasks(tasks, taskCount);
            if(taskCount > 0){
                printf("Which task number to complete? ");
                int num;
                if(scanf("%d", &num) != 1){
                    printf("Invalid input.\n");
                    int ch;
                    while((ch = getchar()) != '\n' && ch != EOF);
                    continue;
                }
                getchar();
                if(num > 0 && num <= taskCount){
                    completeTask(&player, &tasks, &taskCount, num-1);
                } else {
                    printf("Invalid task number.\n");
                }
            }
        }
        else if(choice == 4){
            showPlayerStatus(&player);
        }
        else if(choice == 0){
            break;
        }
        else{
            printf("Invalid choice, try again.\n");
        }
    }

    saveGame(&player, tasks, taskCount);
    free(tasks);
    return 0;
}
