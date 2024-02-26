#include "math.h"

TT::Time::Time() {
	lastTime = (float) glfwGetTime();
	delta = 0.0f;
	time = 0.0f;
}

void TT::Time::update() {
	float time = (float) glfwGetTime();
	delta = time - lastTime;
	lastTime = time;

	this->time += delta;
}

float TT::Time::getDelta() {
	return delta;
}
float TT::Time::getTime() {
	return time;
}