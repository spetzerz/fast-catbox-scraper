#pragma once
#include <stdlib.h>

int get_random_number(int min, int max) {
	if (min >= max) {
		return min;
	}
	return (rand() % (max - min)) + min;
}
