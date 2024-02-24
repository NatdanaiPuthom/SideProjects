#pragma once

class BaseDeleter
{
public:
	virtual ~BaseDeleter() {}
	virtual void deletePointer() = 0;
};

template <typename T>
struct TypedDeleter
{
	TypedDeleter() = default; // Adding default constructor

	void operator()(void* ptr) const
	{
		delete static_cast<T*>(ptr);
	}
};

template<typename T>
class CustomUniquePtr
{
public:
	CustomUniquePtr(T* ptr)
		: m_ptr(static_cast<void*>(ptr)),
		m_deleter(std::make_unique<TypedDeleter<T>>(ptr)) {}


	~CustomUniquePtr() {
		if (m_deleter) {
			m_deleter->deletePointer();
			delete m_deleter;
		}
	}

	// Disallow copying (similar to std::unique_ptr)
	CustomUniquePtr(const CustomUniquePtr&) = delete;
	CustomUniquePtr& operator=(const CustomUniquePtr&) = delete;

	// Allow moving
	CustomUniquePtr(CustomUniquePtr&& other) noexcept
		: m_ptr(other.m_ptr), m_deleter(other.m_deleter) {
		other.m_ptr = nullptr;
		other.m_deleter = nullptr;
	}

	CustomUniquePtr& operator=(CustomUniquePtr&& other) noexcept {
		if (this != &other) {
			m_ptr = other.m_ptr;
			m_deleter = other.m_deleter;
			other.m_ptr = nullptr;
			other.m_deleter = nullptr;
		}
		return *this;
	}

	template<typename T>
	T* get() const {
		return static_cast<T*>(m_ptr);
	}

private:
	void* m_ptr;
	BaseDeleter* m_deleter;
};



struct Deleter
{
	template<typename T>
	void operator()(T* ptr) const
	{
		delete ptr;
	}
};

template<typename T>
class NatdanaiSmartVoidPointersDestroyer
{
public:
	explicit NatdanaiSmartVoidPointersDestroyer(T* aPointer = nullptr) : myPointer(aPointer) {}
	~NatdanaiSmartVoidPointersDestroyer() { delete myPointer; }
	NatdanaiSmartVoidPointersDestroyer(const NatdanaiSmartVoidPointersDestroyer& aOther) = delete;
	NatdanaiSmartVoidPointersDestroyer& operator=(const NatdanaiSmartVoidPointersDestroyer& aOther) = delete;

	NatdanaiSmartVoidPointersDestroyer(NatdanaiSmartVoidPointersDestroyer&& aOther) noexcept : myPointer(aOther.myPointer)
	{
		aOther.myPointer = nullptr;
	}

	NatdanaiSmartVoidPointersDestroyer& operator=(NatdanaiSmartVoidPointersDestroyer&& aOther) noexcept
	{
		if (this != &aOther)
		{
			delete myPointer;
			myPointer = aOther.myPointer;
			aOther.myPointer = nullptr;
		}

		return *this;
	}

	T* Get() const { return myPointer; }
	T& operator*() const { return *myPointer; }
	T* operator->() const { return myPointer; }

private:
	T* myPointer;
};