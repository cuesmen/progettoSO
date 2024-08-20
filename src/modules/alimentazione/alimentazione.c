#include "alimentazione.h"

int step_alimentazione;

int main(int argc, char *argv[]) {

    if (argc < 1) {
        printf("Usage: %s <STEP_ALIMENTAZIONE>\n", argv[0]);
        return 1;
    }

    step_alimentazione = atoi(argv[1]);
}