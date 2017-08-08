#pragma once
#include <iostream>
#include <string>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <cerrno>
#include "class_FileManager.hh"

const int sizeByte_SC  = 15;
const int sizeByte_PSC = 5;
const int sizeByte_RSC = 1;
const int sizeByte_PIN = 2;

int PrepareSC(int chipNo);
int PreparePSC(int CurrentCh);
int PrepareReadSC(int chipNo);
int PrepareFPGA();
void Debug();
