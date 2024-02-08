#include "../inc/prototype.hpp"

int main(int argc, char** argv)
{
	if (argc <= 1)
	{
		std::cout << "No binary file path given!\n";
		return 1;
	}
	else if (argc == 2)
	{
		parameter_type bin{ std::string{argv[1]} };
		parameter_type log{ std::nullopt };

		wain(std::move(bin), std::move(log));
	}
	else
	{
		parameter_type bin{ std::string{argv[1]} };
		parameter_type log{ std::string{argv[2]} };

		wain(std::move(bin), std::move(log));
	}

	return 0;
}
