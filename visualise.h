#pragma once
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define PI 3.14159265
#define C_RAD 0.5
#define SUB_RAD 0.3

#include <glfw3.h>
#include <vector>
#include <string>
#include <math.h>
#include <cstdio>
#include "stb_easy_font.h"
#include <iostream>
#include "stdafx.h"
#include <iostream>
#include <cstdio>
#include <winsock2.h>
#include <windows.h>
#include <vector>
#include <string>
#include <math.h>

void print_string(float x, float y, const char* text, float r, float g, float b);
float xcoord(float r, float angle);
float ycoord(float r, float angle);
void draw(float r, float angle, float cx, float cy, std::vector<std::string> input, float bias);
void visualise(std::vector<std::vector<std::string>> data, std::vector<std::string> Comutators);
