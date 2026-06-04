
#include <Arduino.h>
#include <FastLED.h>

using namespace fl;

#define NUM_LEDS 15
//#define IS_SERPINTINE true  // Type of matrix display, you probably have this.

CRGB leds[NUM_LEDS];

//int index = 0;
//int t = 0;
//uint8_t nom = 1;    
uint8_t indexOfLed = 1;    
uint8_t brightness = 64;
//const uint8_t LENGTH = 64;
//char parsed[LENGTH];
//const uint8_t LINES = 8;
//char parsing[LINES][LENGTH];

#include <stdint.h>
#include <stdlib.h>

#define NO_SLOT (struct slot *)(-1)
#define NO_DOT (struct dot *)(-1)
#define VERTICAL (uint8_t)1
#define HORIZONTAL (uint8_t)0
#define SQUARE (uint8_t)-1

struct dot {
    uint8_t x;
    uint8_t y;
};

//const uint8_t
#define NUM_VARIETIES 5
//const uint8_t
#define MAX_LENGTH 10

struct component {
    int dimensions; // must be >= 0, otherwise indicates that there is no component at that position
    uint8_t complexity;
    char marking[MAX_LENGTH];
    char type[MAX_LENGTH];
    uint8_t priority;
};

#define MAX_CONTENTS_NUM 8

struct slot {
    const uint8_t number = -1; // 0...N
    struct component contents[MAX_CONTENTS_NUM]; // must contain a component with dimensions >= 0 at [0] if not empty, 
                                                 // order of contents must be continuous before that with dims <= -1
};

//const uint8_t ROWS = 3, COLS = 5;
#define ROWS 3
#define COLS 5

struct slot grid[ROWS][COLS];

/*

                              ^ y
                              | ROWS - 1
                              |
                              |
                              |
                              |
                              |
<-----------------------------0 0
x COLS - 1                    0
*/

struct slot* getBySerial(uint8_t number) {
    if (number >= ROWS * COLS) {
        return  NO_SLOT;
    }
    return &(grid[number / ROWS][number % ROWS]);
}

// border-inclusive
uint8_t checkRect_e(struct dot* begin, struct dot* end) {
    if (begin->x > end->x || begin->y > end->y) {
        return 1; // yes, it is empty
    }
    for (uint8_t i = begin->x; i <= end->x; i++) {
        for (uint8_t j = begin->y; j <= end->y; j++) {
            if (grid[j][i].contents[0].dimensions >= 0) { // if slot not empty
                return 0;
            }
        }
    }
    return 1;
}

uint8_t vert_hor(struct dot *begin, struct dot *end) {
    if ((end->y - begin->y) > (end->x - begin->x)) {
        return VERTICAL;
    } else if ((end->x - begin->x) > (end->y - begin->y)) {
        return HORIZONTAL;
    } else {
        return SQUARE;
    }
}

// for distributing components with complexity = 0
// not maximal rectangle in terms of measures and dimensions exactly, but maximum rectangle adjusted for usability
// either very tall and (likely) thin or thick but (likely) low
struct dot getMaxRect_atdot(struct dot* begin) {
    struct dot end;
    end.x = begin->x;
    end.y = begin->y;
    uint8_t vertical = 1;
    while (checkRect_e(begin, &end)) {
        end.y++;
    }
    if (end.y == begin->y) {
        while (checkRect_e(begin, &end)) {
            end.x++;
        }
        if (end.x == begin->x) {
            return end;
        }
        vertical = 0;
    }
    if (vertical) {
        while (checkRect_e(begin, &end)) {
            end.x++;
        }
    } else {
        while (checkRect_e(begin, &end)) {
            end.y++;
        }
    }
    return end;
}

// matrix for selecting neighbours when calculating free region
const uint8_t selection_matrix[9] = {
    0, 1, 0,
    1, 0, 1,
    0, 1, 0
};

#define SELECTION_MATRIX_LEFT_TOP 0
#define SELECTION_MATRIX_TOP 1
#define SELECTION_MATRIX_RIGHT_TOP 2
#define SELECTION_MATRIX_LEFT 3
#define SELECTION_MATRIX_CENTER 4
#define SELECTION_MATRIX_RIGHT 5
#define SELECTION_MATRIX_LEFT_BOTTOM 6
#define SELECTION_MATRIX_BOTTOM 7
#define SELECTION_MATRIX_RIGHT_BOTTOM 8

// for distributing components with complexity = 1
struct dot *free_region(struct dot *initial) {
    if (grid[initial->y][initial->x].contents[0].dimensions >= 0) {
        return NO_DOT;
    }
    // need to delete somewhere
    struct dot *region = (struct dot*) malloc((ROWS * COLS + 1) * sizeof(struct dot)); // we can't have more dots
                                                                                       // the last one is for terminal dot
    uint8_t was[ROWS][COLS]; // visited slots / dots
    // telling computer we haven't visited any slot yet
    for (uint8_t i = 0; i < ROWS; i++) {
        for (uint8_t j = 0; j < COLS; j++) {
            was[i][j] = 0;
        }
    }
    // exept initial slot
    was[initial->y][initial->x] = 1;
    // making region empty
    for (uint8_t i = 0; i < ROWS * COLS + 1; i++) {
        region[i].x = -1;
        region[i].y = -1;
    }
    region[0] = *(initial);
    uint16_t next_index = 0;
    uint8_t canWe = 1;
    //uint8_t buf_index = next_index;
    uint8_t end_index = next_index; // inclusive
    while (canWe) {
        was[region[next_index].y][region[next_index].x] = 1;
        //buf_index = next_index;
        // left
        if (selection_matrix[SELECTION_MATRIX_LEFT] == 1 &&
            region[next_index].x + 1 < ROWS &&
            was[region[next_index].y + 1][region[next_index].x] == 0 &&
            grid[region[next_index].y + 1][region[next_index].x].contents[0].dimensions < 0) {
            //buf_index++;
            end_index++;
            if (end_index >= ROWS * COLS) {
                end_index = 0;
            }
            region[end_index].x = region[next_index].x + 1;
            region[end_index].y = region[next_index].y;
        }
        // top
        if (selection_matrix[SELECTION_MATRIX_TOP] == 1 && 
            region[next_index].y + 1 < COLS &&
            was[region[next_index].y][region[next_index].x + 1] == 0 &&
            grid[region[next_index].y][region[next_index].x + 1].contents[0].dimensions < 0) {
            //buf_index++;
            end_index++;
            if (end_index >= ROWS * COLS) {
                end_index = 0;
            }
            region[end_index].x = region[next_index].x;
            region[end_index].y = region[next_index].y + 1;
        }
        // right
        if (selection_matrix[SELECTION_MATRIX_RIGHT] == 1 &&
            region[next_index].x - 1 >= 0 &&
            was[region[next_index].y - 1][region[next_index].x] == 0 &&
            grid[region[next_index].y - 1][region[next_index].x].contents[0].dimensions < 0) {
            //buf_index++;
            end_index++;
            if (end_index >= ROWS * COLS) {
                end_index = 0;
            }
            region[end_index].x = region[next_index].x - 1;
            region[end_index].y = region[next_index].y;
        }
        // bottom
        if (selection_matrix[SELECTION_MATRIX_BOTTOM] == 1 &&
            region[next_index].y - 1 >= 0 &&
            was[region[next_index].y][region[next_index].x - 1] == 0 &&
            grid[region[next_index].y][region[next_index].x - 1].contents[0].dimensions < 0) {
            //buf_index++;
            end_index++;
            if (end_index >= ROWS * COLS) {
                end_index = 0;
            }
            region[end_index].x = region[next_index].x;
            region[end_index].y = region[next_index].y - 1;
        }
        // left-top
        if (selection_matrix[SELECTION_MATRIX_LEFT_TOP] == 1 &&
            region[next_index].x + 1 < ROWS &&
            region[next_index].y + 1 < COLS &&
            was[region[next_index].y + 1][region[next_index].x + 1] == 0 &&
            grid[region[next_index].y + 1][region[next_index].x + 1].contents[0].dimensions < 0) {
            //buf_index++;
            end_index++;
            if (end_index >= ROWS * COLS) {
                end_index = 0;
            }
            region[end_index].x = region[next_index].x + 1;
            region[end_index].y = region[next_index].y + 1;
        }
        // right-top
        if (selection_matrix[SELECTION_MATRIX_RIGHT_TOP] == 1 && 
            region[next_index].y + 1 < COLS &&
            region[next_index].x - 1 >= 0 &&
            was[region[next_index].y - 1][region[next_index].x + 1] == 0 &&
            grid[region[next_index].y - 1][region[next_index].x + 1].contents[0].dimensions < 0) {
            //buf_index++;
            end_index++;
            if (end_index >= ROWS * COLS) {
                end_index = 0;
            }
            region[end_index].x = region[next_index].x - 1;
            region[end_index].y = region[next_index].y + 1;
        }
        // right-bottom
        if (selection_matrix[SELECTION_MATRIX_RIGHT_BOTTOM] == 1 &&
            region[next_index].x - 1 >= 0 &&
            region[next_index].y - 1 >= 0 &&
            was[region[next_index].y - 1][region[next_index].x - 1] == 0 &&
            grid[region[next_index].y - 1][region[next_index].x - 1].contents[0].dimensions < 0) {
            //buf_index++;
            end_index++;
            if (end_index >= ROWS * COLS) {
                end_index = 0;
            }
            region[end_index].x = region[next_index].x - 1;
            region[end_index].y = region[next_index].y - 1;
        }
        // left-bottom
        if (selection_matrix[SELECTION_MATRIX_LEFT_BOTTOM] == 1 &&
            region[next_index].y - 1 >= 0 &&
            region[next_index].x + 1 < ROWS &&
            was[region[next_index].y + 1][region[next_index].x - 1] == 0 &&
            grid[region[next_index].y + 1][region[next_index].x - 1].contents[0].dimensions < 0) {
            //buf_index++;
            end_index++;
            if (end_index >= ROWS * COLS) {
                end_index = 0;
            }
            region[end_index].x = region[next_index].x + 1;
            region[end_index].y = region[next_index].y - 1;
        }
        if(end_index <= next_index) {
            canWe = 0;
        } else {
            next_index++;
        }
    }
    return region;
}

// we need map str->num

uint8_t glob_priorities[NUM_VARIETIES];
char glob_types[NUM_VARIETIES][MAX_LENGTH];

uint8_t equals(char *str1, const char str2[]) {
    uint8_t i = 0;
    for (; str1[i] != '\0' && str1[i] != '\n' && str1[i] != ' ' && 
           str2[i] != '\0' && str2[i] != '\n' && str2[i] != ' ' && i < MAX_LENGTH; i++) {
        if (str1[i] != str2[i]) {
            return 0;
        }
    }
    // if (str1[i] != str2[i]) {
    //     return 0;
    // }
    return 1;
}

#define INVALID_PRIORITY 255

// considered not optimizing it thus not upgrading to suffix tree
uint8_t priorOf(char *str) {
    uint8_t i = 0;
    while (i < NUM_VARIETIES && !equals(str, glob_types[i])) {
        i++;
    }
    if (i == NUM_VARIETIES) {
        return INVALID_PRIORITY;
    }
    return glob_priorities[i];
}

// considered not upgrading even this, as it is not demanded in context
// but might upgrade to batcher's sort or quicksort
void sortByPriority(struct component *cmps, uint16_t num) {
    struct component t;
    for (uint8_t i = 0; i < num; i++) {
        for (uint8_t j = i; j < num; j++) {
            if (cmps[i].priority < cmps[j].priority) { // increasing priority
                t = cmps[i];
                cmps[i] = cmps[j];
                cmps[j] = t;
            }
        }
    }
}

const struct dot INVALID_DOT = {255, 255};

void findSameInRect(struct component *com, struct dot *begin, struct dot *end, struct dot *res) {
    uint16_t ind = 0;
    for (uint8_t i = begin->y; i < end->y; i++) {
        for (uint8_t j = begin->x; j < end->x; j++) {
            for (uint8_t k = 0; grid[i][j].contents[k].dimensions >= 0; k++) {
                if (equals(grid[i][j].contents[k].marking, com->marking) && equals(grid[i][j].contents[k].type, com->type)) {
                    res->x = j;
                    res->y = i;
                    return;
                }
            }
        }
    }
    //return INVALID_DOT;
    res->x = INVALID_DOT.x;
    res->y = INVALID_DOT.y;
}

void findFreeInRect(struct dot *begin, struct dot *end, struct dot *res) {
    uint16_t ind = 0;
    for (uint8_t i = begin->y; i < end->y; i++) {
        for (uint8_t j = begin->x; j < end->x; j++) {
            if(grid[i][j].contents[0].dimensions <= -1) {
                res->x = j;
                res->y = i;
                return;
            }
        }
    }
    //return INVALID_DOT;
    res->x = INVALID_DOT.x;
    res->y = INVALID_DOT.y;
}

void putInTheEndOfSlot(struct dot *where, struct component *com) {
    uint8_t k = 0;
    for (; grid[where->y][where->x].contents[k].dimensions > -1 && k < MAX_CONTENTS_NUM; k++) {} // compute position of free space or ensure it doent's exist
    if (k == MAX_CONTENTS_NUM) { // what?
        return;
    }
    grid[where->y][where->x].contents[k] = *com; // we copy it's data there
}

// will upgrade in future
void distributeInRect(struct component *cmps, uint16_t num, struct dot *begin, struct dot *end) {
    sortByPriority(cmps, num);
    // uint16_t ind = 0;
    // for (uint8_t i = begin->y; i < end->y; i++) {
    //     for (uint8_t j = begin->x; j < end->x; j++) {
    //         if (ind < num) {
    //             grid[i][j] = cmps[ind];
    //             ind++;
    //         }
    //     }
    // }
    for (uint16_t i = 0; i < num; i++) {
        struct dot where; // ... to put this component
        findSameInRect(&cmps[i], begin, end, &where);
        if (where.x == INVALID_DOT.x && where.y == INVALID_DOT.y) { // if we didn't find appropriate slot
            findFreeInRect(begin, end, &where);
        }
        if (where.x == INVALID_DOT.x && where.y == INVALID_DOT.y) { // if we didn't find appropriate slot
            // we leave this component as it is, later we will need to check if we could distribute everything
            return;
        }
        // there we are guaranteed a slot with "dims <= -1" component at the end
        putInTheEndOfSlot(&where, &cmps[i]);
    }
}

// "rectangular-only" workflow has proven itself efficient enough as for now, 
// so won't use the rest until further improvement of the process :P

// a piece of cake!
void findFreeInRegion(struct dot *region, struct dot *res) {
    uint16_t ind = 0;
    for (; ind < ROWS * COLS + 1; ind++) {
        if (region[ind].x != -1 && region[ind].y != -1) {
            res->x = region[ind].x;
            res->y = region[ind].y;
            return;
        }
    }
    //return INVALID_DOT;
    res->x = INVALID_DOT.x;
    res->y = INVALID_DOT.y;
}

void findSameInRegion(struct component *com, struct dot *region, struct dot *res) {
    uint16_t ind = 0;
    for (; ind < ROWS * COLS + 1; ind++) {
        for (uint8_t k = 0; grid[region[ind].y][region[ind].x].contents[k].dimensions >= 0; k++) {
            if (equals(grid[region[ind].y][region[ind].x].contents[k].marking, com->marking) && equals(grid[region[ind].y][region[ind].x].contents[k].type, com->type)) {
                res->x = region[ind].x;
                res->y = region[ind].y;
                return;
            }
        }
    }
    //return INVALID_DOT;
    res->x = INVALID_DOT.x;
    res->y = INVALID_DOT.y;
}

// will drastically upgrade in the future
void distributeInRegion(struct component *cmps, uint16_t num, struct dot *region) {
    sortByPriority(cmps, num);
    for (uint16_t i = 0; i < num; i++) {
        struct dot where;
        findSameInRegion(&cmps[i], region, &where);
        if (where.x == INVALID_DOT.x && where.y == INVALID_DOT.y) { // if we didn't find appropriate slot
            findFreeInRegion(region, &where);
        }
        if (where.x == INVALID_DOT.x && where.y == INVALID_DOT.y) { // if we didn't find appropriate slot
            // we leave this component as it is, later we will need to check if we could distribute everything
            return;
        }
        // there we are guaranteed a slot with "dims <= -1" component at the end
        putInTheEndOfSlot(&where, &cmps[i]);
    }
}

// :P

#include <stdio.h>

void outputGrid() {
    for (uint8_t i = ROWS - 1; i >= 0 && i < ROWS; i--) {
        for (uint8_t j = COLS - 1; j >= 0 && j < COLS; j--) {
            if (grid[i][j].contents[0].dimensions >= 0) {
                Serial.print(" *");
            } else {
                Serial.print(" 0");
            }
        }
        Serial.print('\n');
    }
}

void initGrid() {
    for (uint8_t i = ROWS - 1; i >= 0 && i < ROWS; i--) {
        for (uint8_t j = COLS - 1; j >= 0 && j < COLS; j--) {
            for (uint8_t k = 0; k < MAX_CONTENTS_NUM; k++) {
                struct component t;
                t.dimensions = -1;
                t.marking[0] = '\0';
                t.type[0] = '\0';
                grid[i][j].contents[k] = t;
            }
            // if (grid[i][j].contents[0].dimensions >= 0) {
            //     Serial.print(" *");
            // } else {
            //     Serial.print(" 0");
            // }
            //grid[i][j].contents = *(struct slot*)malloc(sizeof(struct slot));
            
        }
        Serial.print('\n');
    }
}

void makeComponentInvalid(struct component *com) {
    com->dimensions = -1;
    com->complexity = 0;
    com->priority = 0;
    for (uint8_t i = 0; i < MAX_LENGTH; i++) {
        com->marking[i] = '\0';
        com->type[i] = '\0';
    }
}

// deletes specific component
void deleteFromSlot(struct dot *where, struct component *com) {
    uint8_t k = MAX_CONTENTS_NUM - 1;
    for (; k < 256; k--) {
        if (grid[where->y][where->x].contents[k].dimensions > -1 && 
            equals(grid[where->y][where->x].contents[k].marking, com->marking) && 
            equals(grid[where->y][where->x].contents[k].type, com->type)) {
            makeComponentInvalid(&grid[where->y][where->x].contents[k]);
            return;
        }
    }
}

// deletes whatever is invalid and tells us what it was
void ThrowOneAwayFromSlot(struct dot *where, struct component *com) {
    uint8_t k = MAX_CONTENTS_NUM - 1;
    for (; k < 256; k--) {
        if (grid[where->y][where->x].contents[k].dimensions > -1) {
            com->complexity = grid[where->y][where->x].contents[k].complexity;
            com->dimensions = grid[where->y][where->x].contents[k].dimensions;
            com->priority = grid[where->y][where->x].contents[k].priority;
            for (uint8_t h = 0; h < MAX_LENGTH; h++) {
                com->marking[h] = grid[where->y][where->x].contents[k].marking[h];
                com->type[h] = grid[where->y][where->x].contents[k].type[h];
            }
            makeComponentInvalid(&grid[where->y][where->x].contents[k]);
            return;
        }
    }
}

// deletes whatever is invalid without telling us what it was
void ThrowOneAwayFromSlot(struct dot *where) {
    uint8_t k = MAX_CONTENTS_NUM - 1;
    for (; k < 256; k--) {
        if (grid[where->y][where->x].contents[k].dimensions > -1) {
            makeComponentInvalid(&grid[where->y][where->x].contents[k]);
            return;
        }
    }
}

struct dot begin = {3, 0};
struct dot begin1 = {3, 0};
//begin1.x = 3;
struct dot end1 = {5, 3};
// for components with complexity = 0
struct dot begin2 = {0, 0};
struct dot end2 = {2, 3};
struct component sonet[64];
uint8_t current_index = 0;
uint8_t what_to_do;
struct component current;

void setup() {
  Serial.begin(9600);
  //parsed[LENGTH - 1] = '\0';
    //Serial.begin(9600);
    //t=0;
    //index=0;
    // No ScreenMap necessary for strips.
    current = *(struct component*)malloc(sizeof(struct component)); // int uint string string uint
    FastLED.addLeds<NEOPIXEL, 2>(leds, NUM_LEDS);
    initGrid();
}

uint8_t indexByCoordinates(uint8_t x, uint8_t y) {
    struct dot dots[15] = 
    {{0, 0}, {0, 1}, {0, 2}, {1, 2}, {1, 1}, {1, 0}, /**/ {2, 0}, {2, 1}, {2, 2}, {3, 2}, {3, 1}, {3, 0}, /**/ {4, 0}, {4, 1}, {4, 2}};
    for (uint8_t i = 0; i < 15; i++) {
        if (dots[i].x == x && dots[i].y == y) {
            return i;
        }
    }
    return 255;
}

void loop() {
    
    for (int i = 0; i < 64; i++) { current.marking[i] = '\0';}//char bbuuff[64];
    for (int i = 0; i < 64; i++) { current.type[i] = '\0';}//char bbuuff[64];

    if (Serial.available()) {
        char bbuuff[64];
        for (int i = 0; i < 64; i++) { bbuuff[i] = '\0';}//char bbuuff[64];
        Serial.readBytesUntil(' ', bbuuff, MAX_LENGTH);
        sscanf(bbuuff, "%d", &what_to_do);
        Serial.print("got what to do: ");
        Serial.println(what_to_do);
    // }
    
    // if (Serial.available()) {
        for (int i = 0; i < 64; i++) { bbuuff[i] = '\0';}//char bbuuff[64];
        Serial.readBytesUntil(' ', bbuuff, MAX_LENGTH);
        sscanf(bbuuff, "%d", &current.dimensions);
        Serial.print("got dimensions: ");
        Serial.println(current.dimensions);
    // }
    
    // if (Serial.available()) {
        for (int i = 0; i < 64; i++) { bbuuff[i] = '\0';}//char bbuuff[64];
        Serial.readBytesUntil(' ', bbuuff, MAX_LENGTH);
        sscanf(bbuuff, "%d", &current.complexity);
        Serial.print("got complexity: ");
        Serial.println(current.complexity);

    // }

    
    // if (Serial.available()) {
        Serial.readBytesUntil(' ', &current.marking[0], MAX_LENGTH);
        Serial.print("got marking: ");
        Serial.println(current.marking);
    // }
    // if (Serial.available()) {
        Serial.readBytesUntil(' ', &current.type[0], MAX_LENGTH);
        Serial.print("got type: ");
        Serial.println(current.type);
    // }

    // if (Serial.available()) {
        for (int i = 0; i < 64; i++) { bbuuff[i] = '\0';}//char bbuuff[64];
        Serial.readBytesUntil('\n', bbuuff, MAX_LENGTH);
        sscanf(bbuuff, "%d", &current.priority);
        Serial.print("got priority: ");
        Serial.println(current.priority);
        sonet[0] = current;
    } else {
        what_to_do = 250;
    }
    if (what_to_do == 1) { // take
        struct dot where;
        if (current.complexity == 0) {
            findSameInRect(&sonet[0], &begin2, &end2, &where);
        } else {
            findSameInRect(&sonet[0], &begin1, &end1, &where);
        }
        if (where.x == INVALID_DOT.x || where.y == INVALID_DOT.y) {
            Serial.print("Не нашли такого. Положи.\n");
        } else {
            Serial.print(where.x);
            Serial.print(" ");
            Serial.println(where.y);
            indexOfLed = indexByCoordinates(where.x, where.y);
        }
    } else if (what_to_do == 0) { // put
        if (current.complexity == 0) {
            distributeInRect(&sonet[0], 1, &begin2, &end2);
        } else {
            distributeInRect(&sonet[0], 1, &begin1, &end1);
        }
    } else if (what_to_do == 2) {
        struct dot where;
        if (current.complexity == 0) {
            findSameInRect(&sonet[0], &begin2, &end2, &where);
        } else {
            findSameInRect(&sonet[0], &begin1, &end1, &where);
        }
        if (where.x == INVALID_DOT.x || where.y == INVALID_DOT.y) {
            Serial.print("Не нашли такого. Положи.\n");
        } else {
            Serial.print(where.x);
            Serial.print(" ");
            Serial.println(where.y);
            indexOfLed = indexByCoordinates(where.x, where.y);
            deleteFromSlot(&where, &current);
        }
    }

    uint8_t value8;
    for (int x = 0; x < NUM_LEDS; x++) {
        
        if (indexOfLed == x) {
          value8 = brightness;
        } else {
          value8 = 0;
        }
        
        leds[x] = CRGB(value8, value8, value8);
    }
    FastLED.show();
    if (what_to_do < 2)
        outputGrid();
}
