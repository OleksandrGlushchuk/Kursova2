#pragma once

#include <utility>

#include "d3d.hpp"

namespace engine
{
	// A helping struct holding a raw pointer to a D3D resource

	template <typename T>
	struct DxResPtr
	{
		DxResPtr() { m_ptr = nullptr; }
		~DxResPtr() { release(); }

		DxResPtr(const DxResPtr& other) : DxResPtr() { *this = other; }
		DxResPtr& operator=(const DxResPtr& other)
		{
			if (m_ptr) 
				m_ptr->Release();
			m_ptr = other.m_ptr;
			if(other.m_ptr)
				m_ptr->AddRef();
			return *this;
		}

		DxResPtr(DxResPtr&& other) noexcept : DxResPtr() { *this = std::move(other); }
		DxResPtr& operator=(DxResPtr&& other) noexcept
		{
			if (m_ptr) m_ptr->Release();
			m_ptr = other.m_ptr;
			other.m_ptr = nullptr;
			return *this;
		}

		const T* const ptr() const { return m_ptr; }
		T* & ptr() { return m_ptr; }

		T* operator->() const { return m_ptr; }
		T* operator->() { return m_ptr; }

		operator T* () const { return m_ptr; }
		operator T* () { return m_ptr; }

		T** reset()
		{
			release();
			return &m_ptr;
		}

		void reset(T* ptr)
		{
			release();
			m_ptr = ptr;
		}

		void release()
		{
			if (m_ptr)
			{
				m_ptr->Release();
				m_ptr = nullptr;
			}
		}

		bool valid() const
		{
			return m_ptr;
		}

		bool operator==(const DxResPtr<T>& res) const { return m_ptr == res.m_ptr; }
		bool operator!=(const DxResPtr<T>& res) const { return m_ptr != res.m_ptr; }

	protected:
		T* m_ptr;
	};
}