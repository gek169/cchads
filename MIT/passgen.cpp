#include <iostream>
#include <cstdlib>
#include <cmath>
#include <cstdio>
#include <chrono>
#include <random>
#include <vector>

int main (int argc, char** argv){
	if(argc < 2){
		std::cout << "\nUSAGE: passgen -P masterpass -D domainname -C characters -L length -R requiredchars" << std::endl;
		return 0;
	}
	std::string master = ""; std::string domain = ""; std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890_";
	std::string required = "";
	std::string lastarg = "";
	unsigned int length = 24;
	for(int i = 1; i < argc; i++){ //process commandline args
		if(lastarg == "-P") {master = argv[i];}
		else if(lastarg == "-D") {domain = argv[i];}
		else if(lastarg == "-C") {chars = argv[i];}
		else if(lastarg == "-L") {length = atoi(argv[i]);}
		else if(lastarg == "-R") {required = argv[i];}
		lastarg = argv[i];
	}
	master = domain + master; //how we do it!
	std::cout << "\nDomain: " << domain << "\nMaster: " << master << "\nRequired: " << required;
	std::seed_seq seed1 (master.begin(), master.end()); // seed random number gen
	std::minstd_rand0 generator (seed1); //create random number gen with seed
	std::string pass = "";
	for(unsigned int i = 0; i < length; i++)
	{
		pass.push_back(chars[generator() % chars.size()]); //Generate the actual password from random numbers.
	}

	if(required.size() >= length)
		{
			std::cout << "Bad required relative to length." << std::endl;
		}
	if(required != "") //If there are characters that are required to be in the password, put them in!
	{	
		std::cout << "\nProcessing required..." << std::endl;
		std::vector<unsigned int> passindices; passindices.clear();
		while(passindices.size() < required.size())
		{
			unsigned int candidate = generator()%pass.size();
            bool fuck = false;
			for(unsigned int b: passindices)
				if(b == candidate)
					fuck = true;
			if(!fuck)
                passindices.push_back(candidate);			
		}
		std::cout << "\nIndices generated." << std::endl;
		for(unsigned int i = 0; i < required.size(); i++)
		{
			pass[passindices[i]] = required[i];
			
		}
		
	}
	std::cout << "\nYour password is:\n" << pass << std::endl;
	std::cout << "\nWrite down the args you used to generate this password, except your master password." << std::endl;
	return 0;
}
