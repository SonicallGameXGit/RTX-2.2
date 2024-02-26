#pragma once
#include "graphics.h"

namespace TT {
	class Keyboard {
	public:
		static bool isPressed(int key);
	};
	class Mouse {
	public:
		static void initialize();
		static void update();

		static void setGrabbed(bool grabbed);
		
		static bool isGrabbed();
		static bool isPressed(int button);

		static glm::vec2 getVelocity();
		static glm::vec2 getPosition();
	private:
		static glm::vec2 velocity;
		static glm::vec2 lastPosition;
	};
};