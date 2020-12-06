#pragma once
#include "Array2D.h"

#include <cassert>
#include <cstring>


//Struct that allows you to handle data as a 2D array (Does not take ownership)
template<class T>
struct IArray3D
{
	T* pData{ nullptr };
	size_t Width{};
	size_t Height{};
	size_t Depth{};

	T& at(size_t x, size_t y, size_t z) {
		assert(x*Height*Depth + y*Depth + z< Height * Width && "Requesting element out of array bounds!");
		return pData[x*Height*Depth + y*Depth + z];
	}

	IArray2D<T>& operator[](size_t x)
	{
		assert(x < Width && "Requesting element out of array bounds!");
		IArray2D<T> array2DSlice;
		array2DSlice.Width = Height;
		array2DSlice.Height = Depth;
		array2DSlice.pData = &pData[x * Height * Depth];
		return array2DSlice;
	}
};


//Class that wraps the Array interface to handle ownership of the data.
template<class T>
class Array3D {
private:
	IArray3D<T> m_Interface{};

public:
	Array3D(size_t width, size_t height, size_t depth)
	{
		m_Interface.Width = width;
		m_Interface.Height = height;
		m_Interface.Depth = depth;
		m_Interface.pData = new T[width * height * depth];
		for (size_t i = 0; i < width*height*depth; i++)
		{
			m_Interface.pData[i] = T();
		}
	}

	//copy constructor
	Array3D(Array3D const& other)
	{
		*this = other;
	}

	//assignment operator
	Array3D& operator=(Array3D const& other)
	{
		m_Interface.Width = other.m_Interface.Width;
		m_Interface.Height = other.m_Interface.Height;
		m_Interface.Depth = other.m_Interface.Depth;
		delete[] m_Interface.pData;
		m_Interface.pData = new T[m_Interface.Height * m_Interface.Width * m_Interface.Depth];
		memcpy(m_Interface.pData, other.m_Interface.pData, m_Interface.Height * m_Interface.Width * m_Interface.Depth * sizeof(T));
		return *this;
	}

	//move constructor
	Array3D(Array3D&& other) noexcept
	{
		*this = other;
	}

	//move operator
	Array3D& operator=(Array3D&& other) noexcept
	{
		size_t height = other.m_Interface.Width;
		size_t width = other.m_Interface.Height;
		size_t depth = other.m_Interface.Depth;
		T* pData = other.m_Interface.pData;
		other.m_Interface.Width = m_Interface.Width;
		other.m_Interface.Height = m_Interface.Height;
		other.m_Interface.Depth = m_Interface.Depth;
		other.m_Interface.pData = m_Interface.pData;
		m_Interface.Width = height;
		m_Interface.Height = width;
		
		m_Interface.pData = pData;
		return *this;
	}


	~Array3D() {
		if (m_Interface.pData)
			delete[] m_Interface.pData;
	}

	T& at(size_t x, size_t y, size_t z) {
		return m_Interface.at(x, y, z);
	}

	IArray2D<T>& operator[](size_t x)
	{
		return m_Interface[x];
	}

	size_t GetWidth() const { return m_Interface.Height; }
	size_t GetHeight() const { return m_Interface.Width; }

};
