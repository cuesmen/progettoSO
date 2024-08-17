#include "master.h"
#include "config.h"

Config conf;

int main(){
    loadConfig(&conf);
    printConfig(&conf);
}