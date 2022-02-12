#ifndef GEOMETRYHELPER_H
#define GEOMETRYHELPER_H

#include <glad/glad.h>
#include "Shader.h"
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "SceneUtils.h"
#include <limits>

namespace Utils
{
	const float fullScreenQuad_verts[]
	{
		-1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f
	};


	void GetMinMax(std::vector<glm::vec3> points, glm::vec3 &min, glm::vec3 &max)
	{
		float
			min_x, min_y, min_z,
			max_x, max_y, max_z;

		min_x = min_y = min_z = std::numeric_limits<float>::max();
		max_x = max_y = max_z = std::numeric_limits<float>::min();

		for (int i = 0; i < points.size(); i++)
		{
			// Compute Minimum ======================================
			if (points[i].x < min_x)
				min_x = points[i].x;
			if (points[i].y < min_y)
				min_y = points[i].y;
			if (points[i].z < min_z)
				min_z = points[i].z;

			// Compute Maximum ======================================
			if (points[i].x > max_x)
				max_x = points[i].x;
			if (points[i].y > max_y)
				max_y = points[i].y;
			if (points[i].z > max_z)
				max_z = points[i].z;
		}

		min = glm::vec3(min_x, min_y, min_z);
		max = glm::vec3(max_x, max_y, max_z);

	}

	void GetShadowMatrices(glm::vec3 position, glm::vec3 direction, std::vector<glm::vec3> bboxPoints, glm::mat4 &view, glm::mat4 &proj)
	{
		glm::vec3 center = (position + direction);
		glm::vec3 worldZ = glm::vec3(0.0f, 0.0f, 1.0f);
		
		/*
		* NOTE: the lookAt matrix maps eye to the origin and center to the negative Z axis
		* (the opposite of what you think)...
		*/
		view = glm::lookAt(position, center, worldZ);

		// this containter stores the bbox points in camera space
		std::vector<glm::vec3> projPoints = std::vector<glm::vec3>();

		for (int i = 0; i < bboxPoints.size(); i++)
		{
			glm::vec3 projPoint =view * glm::vec4(bboxPoints[i], 1.0f);

			projPoints.push_back(projPoint);
		}

		glm::vec3 min, max;

		GetMinMax(projPoints, min, max);

		proj = glm::ortho(
			min.x,
			max.x,
			min.y,
			max.y,

			-1.0f * max.z,
			-1.0f * min.z
		);
	}

	void GetTightNearFar(std::vector<glm::vec3> bboxPoints, glm::mat4 view, float& near, float& far)
	{
		
		// this containter stores the bbox points in camera space
		std::vector<glm::vec3> projPoints = std::vector<glm::vec3>();

		for (int i = 0; i < bboxPoints.size(); i++)
		{
			glm::vec3 projPoint = view * glm::vec4(bboxPoints[i], 1.0f);

			projPoints.push_back(projPoint);
		}

		glm::vec3 min, max;

		GetMinMax(projPoints, min, max);

		near = -0.9f * max.z;
		far = -1.1f * min.z;
		
	}
}
class BoundingBox
{
private:
	glm::vec3 _min, _max, _center, _diagonal;
	float _size;

public:
	BoundingBox(std::vector<glm::vec3> points)
	{

		_min = glm::vec3(1, 1, 1) * std::numeric_limits<float>::max();
		_max = glm::vec3(1, 1, 1) * std::numeric_limits<float>::min();

		Update(points);
	}

	void Update(std::vector<glm::vec3> points)
	{

		glm::vec3 nMin = glm::vec3();
		glm::vec3 nMax = glm::vec3();

		Utils::GetMinMax(points, nMin, nMax);

		_min = glm::vec3(
			glm::min(nMin.x, _min.x),
			glm::min(nMin.y, _min.y),
			glm::min(nMin.z, _min.z));

		_max = glm::vec3(
			glm::max(nMax.x, _max.x),
			glm::max(nMax.y, _max.y),
			glm::max(nMax.z, _max.z));

		_diagonal = _max - _min;
		_center = _min + _diagonal * 0.5f;
		_size = glm::length(_diagonal);
	}

	std::vector<glm::vec3> GetLines()
	{
		return
			std::vector<glm::vec3>
		{
			// Bottom =======================================================================
			_min,
				_min + glm::vec3(1, 0, 0) * _diagonal.x,
				_min + glm::vec3(1, 0, 0) * _diagonal.x,
				_min + glm::vec3(1, 0, 0) * _diagonal.x + glm::vec3(0, 1, 0) * _diagonal.y,
				_min + glm::vec3(1, 0, 0) * _diagonal.x + glm::vec3(0, 1, 0) * _diagonal.y,
				_min + glm::vec3(0, 1, 0) * _diagonal.y,
				_min + glm::vec3(0, 1, 0) * _diagonal.y,
				_min,

				// Top =======================================================================
				_min + glm::vec3(0, 0, 1) * _diagonal.z,
				_min + glm::vec3(1, 0, 0) * _diagonal.x + glm::vec3(0, 0, 1) * _diagonal.z,
				_min + glm::vec3(1, 0, 0) * _diagonal.x + glm::vec3(0, 0, 1) * _diagonal.z,
				_min + glm::vec3(1, 0, 0) * _diagonal.x + glm::vec3(0, 1, 0) * _diagonal.y + glm::vec3(0, 0, 1) * _diagonal.z,
				_min + glm::vec3(1, 0, 0) * _diagonal.x + glm::vec3(0, 1, 0) * _diagonal.y + glm::vec3(0, 0, 1) * _diagonal.z,
				_min + glm::vec3(0, 1, 0) * _diagonal.y + glm::vec3(0, 0, 1) * _diagonal.z,
				_min + glm::vec3(0, 1, 0) * _diagonal.y + glm::vec3(0, 0, 1) * _diagonal.z,
				_min + glm::vec3(0, 0, 1) * _diagonal.z,

				// Sides =======================================================================
				_min,
				_min + glm::vec3(0, 0, 1) * _diagonal.z,
				_min + glm::vec3(1, 0, 0) * _diagonal.x,
				_min + glm::vec3(1, 0, 0) * _diagonal.x + glm::vec3(0, 0, 1) * _diagonal.z,
				_min + glm::vec3(1, 0, 0) * _diagonal.x + glm::vec3(0, 1, 0) * _diagonal.y,
				_min + glm::vec3(1, 0, 0) * _diagonal.x + glm::vec3(0, 1, 0) * _diagonal.y + glm::vec3(0, 0, 1) * _diagonal.z,
				_min + glm::vec3(0, 1, 0) * _diagonal.y,
				_min + glm::vec3(0, 1, 0) * _diagonal.y + glm::vec3(0, 0, 1) * _diagonal.z,
		};
	}

	std::vector<glm::vec3> GetPoints()
	{
		return
			std::vector<glm::vec3>
		{
				// Bottom =======================================================================
				_min,
				_min + glm::vec3(1, 0, 0) * _diagonal.x,				
				_min + glm::vec3(1, 0, 0) * _diagonal.x + glm::vec3(0, 1, 0) * _diagonal.y,
				_min + glm::vec3(0, 1, 0) * _diagonal.y,

				// Top =======================================================================
				_min + glm::vec3(0, 0, 1) * _diagonal.z,				
				_min + glm::vec3(1, 0, 0) * _diagonal.x + glm::vec3(0, 0, 1) * _diagonal.z,
				_min + glm::vec3(1, 0, 0) * _diagonal.x + glm::vec3(0, 1, 0) * _diagonal.y + glm::vec3(0, 0, 1) * _diagonal.z,			
				_min + glm::vec3(0, 0, 1) * _diagonal.z,	
		};
	}
	glm::vec3 Center() { return _center; };
	glm::vec3 Diagonal() { return _diagonal; };
	float Size() { return _size; };
};




#endif