#pragma once

#include <memory>
#include <string>
#include <unordered_map>

namespace Cosmos
{
	template<typename T>
	class Library
	{
	public:
		
		// constructor
		Library() = default;

		// destructor
		~Library() = default;

	public:

		// checks if a given content/element exists within the library
		bool Exists(std::string name);

		// inserts a new content/element into the library
		void Insert(std::string name, std::shared_ptr<T> object);

		// erases a given content/element from the library
		void Erase(std::string name);

	private:

		std::unordered_map<std::string, std::shared_ptr<T>> mContent;
	};
	
}