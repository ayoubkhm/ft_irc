#include <iostream>

int main(int argc,char **argv)
{
	if (argc == 3)
	{
		try
		{
			
		}
		catch(const std::exception& e)
		{
			std::cerr << e.what() << '\n';
		}
	}
	else
	{
		std::cout << "Invalid args" << std::endl;
		return 1;
	}
	return 0;
}