#include "Library.h"

#include "Assert.h"
namespace Cosmos
{
	template<typename T>
	inline bool Library<T>::Exists(std::string name)
	{
		return mContent.find(name) != mContent.end() ? true : false;
	}

	template<typename T>
	inline void Library<T>::Insert(std::string name, std::shared_ptr<T> object)
	{
		auto it = mContent.find(name);
		
		if (it != mContent.end())
		{
			VK_LOG("[Error]: An element with same name already exists, could not create another", name);
			return;
		}
		
		mContent[name] = object;
	}

	template<typename T>
	void Library<T>::Erase(std::string name)
	{
		auto it = mContent.find(name);

		if (it != mContent.end())
		{
			mContent.erase(name);
			return;
		}

		VK_LOG("[Error]: No element with name %s exists", name);
	}

}