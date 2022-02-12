#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>


class FrameBuffer
{
private:
	unsigned int _id;
	unsigned int _idTexDepth;
	std::vector<unsigned int> _idTexCol;
	unsigned int _width, _height;
	bool _depth, _color;
	
	void Initialize(unsigned int width, unsigned int height, bool depth, bool color, int colorAttachments)
	{
		//TODO: hardcoded formats are not the best, right?
		glGenFramebuffers(1, &_id);

		if (depth)
		{
			glGenTextures(1, &_idTexDepth);
			glBindTexture(GL_TEXTURE_2D, _idTexDepth);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
				width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			glBindTexture(GL_TEXTURE_2D, 0);
		}

		if (color)
		{
			for (int i = 0; i < colorAttachments; i++)
			{
				unsigned int id = 0;
				glGenTextures(1, &id);
				glBindTexture(GL_TEXTURE_2D, id);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F,
					width, height, 0, GL_RGB, GL_FLOAT, NULL);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

				glBindTexture(GL_TEXTURE_2D, 0);

				_idTexCol.push_back(id);
			}
		}

		glBindFramebuffer(GL_FRAMEBUFFER, _id);


		if (color)
		{
			for (int i = 0; i < colorAttachments; i++)
			{
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, _idTexCol[i], 0);
			}

			if (depth)
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _idTexDepth, 0);

		}
		else if (depth)
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _idTexDepth, 0);

			glDrawBuffer(GL_NONE);
			glReadBuffer(GL_NONE);
		}

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

	}
public:
	FrameBuffer(unsigned int width, unsigned int height, bool color, int colorAttachments, bool depth)
		:_width(width), _height(height), _depth(depth), _color(color)
	{
		Initialize(_width, _height, _depth, _color, colorAttachments);
	}
	
	//~FrameBuffer()
	//{
	//	FreeUnmanagedResources();
	//}

	void FreeUnmanagedResources()
	{
		if (_idTexCol[0] != 0)
		{
			for (int i = 0; i < _idTexCol.size(); i++)
			{
				glDeleteTextures(1, &_idTexCol[i]);
			}

			_idTexCol.clear();
		}
		if (_idTexDepth != 0)
		{
			glDeleteTextures(1, &_idTexDepth);
			_idTexDepth = 0;
		}
		if (_id != 0)
		{
			glDeleteFramebuffers(1, &_id);
			_id = 0;
		}
	}
	void Bind(bool read, bool write)
	{
		int mask = (read ? GL_READ_FRAMEBUFFER : 0) | (write ? GL_DRAW_FRAMEBUFFER : 0);
		glBindFramebuffer(mask, _id);
	}

	void Unbind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	unsigned int Id()
	{
		return _id;
	}

	unsigned int DepthTextureId()
	{
		return _idTexDepth;
	}

	unsigned int ColorTextureId()
	{
		return _idTexCol[0];
	}

	unsigned int ColorTextureId(int attachment)
	{
		return _idTexCol[attachment];
	}

	unsigned int Width()
	{
		return _width;
	}

	unsigned int Height()
	{
		return _height;
	}

	void CopyFromOtherFbo(FrameBuffer* other, bool color, int attachment, bool depth, glm::ivec2 rect0, glm::ivec2 rect1)
	{

		unsigned int id = other != NULL ? other->_id : 0;

		unsigned int flag = (color ? GL_COLOR_BUFFER_BIT : 0) | (depth ? GL_DEPTH_BUFFER_BIT : 0);

		// Bind read buffer
		glBindFramebuffer(GL_READ_FRAMEBUFFER, id);

		// Bind draw buffer
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _id);

		if(color)
			glDrawBuffer(GL_COLOR_ATTACHMENT0 + attachment);

		// Blit
		glBlitFramebuffer(rect0.x, rect0.y, rect1.x, rect1.y, rect0.x, rect0.y, rect1.x, rect1.y, flag, GL_NEAREST);

		// Unbind
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void CopyToOtherFbo(FrameBuffer* other, bool color, int attachment, bool depth, glm::ivec2 rect0, glm::ivec2 rect1)
	{

		unsigned int id = other != NULL ? other->_id : 0;

		unsigned int flag = (color ? GL_COLOR_BUFFER_BIT : 0) | (depth ? GL_DEPTH_BUFFER_BIT : 0);

		// Bind read buffer
		glBindFramebuffer(GL_READ_FRAMEBUFFER, _id);

		// Bind draw buffer
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, id);

		if (color)
		{
			if(id!=0)
				glDrawBuffer(GL_COLOR_ATTACHMENT0 + attachment);
			else
				glDrawBuffer(GL_BACK);
		}

		// Blit
		glBlitFramebuffer(rect0.x, rect0.y, rect1.x, rect1.y, rect0.x, rect0.y, rect1.x, rect1.y, flag, GL_NEAREST);

		// Unbind
		if (color && id != 0)
			glDrawBuffer(GL_COLOR_ATTACHMENT0);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
};


#endif
