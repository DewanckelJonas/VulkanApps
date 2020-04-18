#pragma once

//MODIFIED FREE CAMERA CLASS FROM LEARN OPENGL
//https://learnopengl.com/Getting-started/Camera

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <iostream>

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum Camera_Movement {
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT,
	NONE
};

// Default camera values
const float YAW = 0.f;
const float PITCH = 0.f;
const float SPEED = 2.5f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 45.0f;


// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class Camera
{
public:
	// Constructor with vectors
	Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, -2.f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH) : Front(glm::vec3(0.0f, 0.0f, 1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
	{
		Position = position;
		WorldUp = up;
		Yaw = yaw;
		Pitch = pitch;
		updateCameraVectors();
	}
	// Constructor with scalar values
	Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) : Front(glm::vec3(0.0f, 0.0f, 1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
	{
		Position = glm::vec3(posX, posY, posZ);
		WorldUp = glm::vec3(upX, upY, upZ);
		Yaw = yaw;
		Pitch = pitch;
		updateCameraVectors();
	}

	// Returns the view matrix calculated using Euler Angles and the LookAt Matrix
	glm::mat4 GetViewMatrix()
	{
		return glm::lookAtLH(GetPosition(), GetPosition() + Front, glm::vec3(0.f, 1.f, 0.f));
		/*return glm::inverse(glm::mat4x4
				{
					glm::vec4(Left.x, Up.x, Front.x, 0), glm::vec4(Left.y, Up.y, Front.y, 0), glm::vec4(Left.z, Up.z, Front.z, 0.f), glm::vec4(Position.x, Position.y, Position.z, 1.f)
				});*/
	}

	glm::mat4 GetProjectionMatrix(float width, float height, float nearPlane, float farPlane)
	{
		float ar = width / height;
		float zRange = nearPlane - farPlane;
		float tanHalfFOV = tanf(glm::radians(90.f / 2.f));

		return glm::mat4x4
		{
			glm::vec4(1.f/(ar*tanHalfFOV), 0.f, 0.f, 0.f),
			glm::vec4(0, 1.f / (tanHalfFOV), 0.f, 0.f),
			glm::vec4(0, 0, (-nearPlane - farPlane) / zRange, 1.f),
			glm::vec4(0.f, 0.f, (2 * farPlane * nearPlane) / zRange, 0.f)
		};
	}

	// Processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
	void ProcessKeyboard(Camera_Movement direction, float deltaTime)
	{
		if (direction == NONE)
			return;
		float velocity = MovementSpeed * deltaTime;
		if (direction == FORWARD)
			Position += Front * velocity;
		if (direction == BACKWARD)
			Position -= Front * velocity;
		if (direction == LEFT)
			Position +=  Left * velocity;
		if (direction == RIGHT)
			Position -= Left * velocity;
	}

	// Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
	void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true)
	{
		xoffset *= MouseSensitivity;
		yoffset *= MouseSensitivity;

		Yaw += xoffset;
		Pitch += yoffset;

		// Make sure that when pitch is out of bounds, screen doesn't get flipped
		if (constrainPitch)
		{
			if (Pitch > 89.0f)
				Pitch = 89.0f;
			if (Pitch < -89.0f)
				Pitch = -89.0f;
		}

		// Update Front, Right and Up Vectors using the updated Euler angles
		updateCameraVectors();
	}

	// Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
	void ProcessMouseScroll(float yoffset)
	{
		if (Zoom >= 1.0f && Zoom <= 45.0f)
			Zoom -= yoffset;
		if (Zoom <= 1.0f)
			Zoom = 1.0f;
		if (Zoom >= 45.0f)
			Zoom = 45.0f;
	}

	void SetMovementSpeed(float speed)
	{
		MovementSpeed = speed;
	}

	glm::vec3 GetPosition()
	{
		return Position;
	}

private:

	// Camera Attributes
	glm::vec3 Position;
	glm::vec3 Front;
	glm::vec3 Up;
	glm::vec3 Left;
	glm::vec3 WorldUp;
	// Euler Angles
	float Yaw;
	float Pitch;
	// Camera options
	float MovementSpeed;
	float MouseSensitivity;
	float Zoom;

	// Calculates the front vector from the Camera's (updated) Euler Angles
	void updateCameraVectors()
	{
		Front.x = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
		Front.y = -sin(glm::radians(Pitch));
		Front.z = cos(glm::radians(Yaw))* cos(glm::radians(Pitch));
		glm::normalize(Front);
		Left = glm::normalize(glm::cross(Front, WorldUp));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
		Up = glm::normalize(glm::cross(Left, Front));
		std::cout << "Front: " << Front.x << ", " << Front.y << ", " << Front.z << std::endl;
		std::cout << "Left" << Left.x << ", " << Left.y << ", " << Left.z << std::endl;
		std::cout << "Up" << Up.x << ", " << Up.y << ", " << Up.z << std::endl;
	}
};
