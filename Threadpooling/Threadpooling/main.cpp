#include <iostream>
#include "ThreadPool.h"
#include <chrono>

void Foo(int num)
{
	std::cout << "Foo: " << num << std::endl;
}

void Boo(int num, int zero)
{
	std::cout << "Boo: " << num + zero << std::endl;
}

class Player
{
public:
	~Player()
	{
		std::cout << "Destroying Player!" << std::endl;
	}

	int GetDamage()
	{
		return 1;
	}

	void AddDamage()
	{
		{
			std::lock_guard<std::mutex> lock(myMutex);
			for (int i = 0; i < 10000; ++i)
			{
				myDamage++;
				std::cout << "PlayerDamage: " << myDamage << std::endl;
			}
		}
	}

	int AddHealth(int num)
	{
		return num;
	}

	int myDamage = 0;
	std::mutex myMutex;
};

int main()
{
	Bass::ThreadPool pool;

	std::shared_ptr<Player> player = std::make_shared<Player>();

	auto start = std::chrono::high_resolution_clock::now();

	pool.AddTask(&Player::AddDamage, player);
	pool.AddTask(&Player::AddDamage, player);
	pool.AddTask(&Player::AddDamage, player);


	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	while (pool.GetTasksInProgress() > 0)
	{

	}

	if (pool.GetTasksInProgress() == 0)
	{
		auto end = std::chrono::high_resolution_clock::now();
		auto time = std::chrono::duration_cast<std::chrono::milliseconds>((end - start)).count();

		std::cout << "Time it took: " << time << std::endl;
	}

	return 0;
}

