#include "indexing.h"

#include <iostream>
#include <stdexcept>

int main(int argc, char* argv[])
{
	// Verify that a file argument was provided
	if (argc < 2)
	{
		// Display our usage syntax and exit
		std::clog << "Usage:" << std::endl;
		std::clog << argv[0] << " <FILE>" << std::endl;
		return 1;
	}
	
	try
	{
		// Determine whether we are indexing a PDB file or a PE file
		std::filesystem::path infile(argv[1]);
		if (infile.extension().compare(".pdb") == 0 || infile.extension().compare(".PDB") == 0)
		{
			std::cout << SignatureForPDB(infile) << std::endl;
		}
		else
		{
			std::cout << HashForPE(infile) << std::endl;
		}
	}
	catch (const std::runtime_error& err)
	{
		std::clog << "Error: " << err.what() << std::endl;
		return 1;
	}

	return 0;
}
