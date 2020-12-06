#pragma once
#include <cassert>
#include <cstring>


//Struct that allows you to handle data as a 2D array (Does not take ownership)
template<class T>
struct IArray2D
{
	T* pData{ nullptr };
	size_t Width{};
	size_t Height{};

	T& at(size_t x, size_t y) {
		assert(x * Height + y < Height * Width && "Requesting element out of array bounds!");
		return pData[x * Height + y];
	}

	T* operator[](size_t x)
	{
		assert(x < Width && "Requesting element out of array bounds!");
		return &pData[x * Height];
	}
};


//Class that wraps the Array interface to handle ownership of the data.
template<class T>
class Array2D {
private:
	IArray2D<T> m_Interface{};

public:
	Array2D(size_t width, size_t height)
	{
		m_Interface.Width = width;
		m_Interface.Height = height;
		m_Interface.pData = new T[width * height];;
	}

	//copy constructor
	Array2D(Array2D const& other)
	{
		*this = other;
	}

	//assignment operator
	Array2D& operator=(Array2D const& other)
	{
		m_Interface.Width = other.m_Interface.Width;
		m_Interface.Height = other.m_Interface.Height;
		delete[] m_Interface.pData;
		m_Interface.pData = new T[m_Height * m_Width];
		memcpy(m_Interface.pData, other.m_Interface.pData, other.m_Interface.Height * other.m_Interface.Width * sizeof(T));
		return *this;
	}

	//move constructor
	Array2D(Array2D&& other) noexcept
	{
		*this = other;
	}

	//move operator
	Array2D& operator=(Array2D&& other) noexcept
	{
		size_t height = other.m_Interface.Width;
		size_t width = other.m_Interface.Height;
		T* pData = other.m_Interface.pData;
		other.m_Interface.Width = m_Interface.Width;
		other.m_Interface.Height = m_Interface.Height;
		other.m_Interface.pData = m_Interface.pData;
		m_Interface.Width = height;
		m_Interface.Height = width;
		m_Interface.pData = pData;
		return *this;
	}


	~Array2D() {
		if (m_Interface.pData)
			delete[] m_Interface.pData;
	}

	T& at(size_t x, size_t y) {
		return m_Interface.at(x,y);
	}

	T* operator[](size_t x)
	{
		return &m_Interface[x];
	}

	size_t GetWidth() const { return m_Interface.Height; }
	size_t GetHeight() const { return m_Interface.Width; }

};