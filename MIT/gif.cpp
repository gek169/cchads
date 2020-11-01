#include <vector>
#include <cstdint>
#include <cstdlib>
#include "include/gif.h"
int main()
{
	int width = 200;
	int height = 200;
	std::vector<uint8_t> black(width * height * 4, 0);
	std::vector<uint8_t> blue(width * height * 4, 255);
	for(size_t i = 0; i < blue.size(); i+=4)
		{blue[i] = 0;blue[i+1] = 0;blue[i+2] = 255;blue[i+3] = 255;}

	auto fileName = "bwgif.gif";
	int delay = 100;
	GifWriter g;
	GifBegin(&g, fileName, width, height, delay);
	GifWriteFrame(&g, black.data(), width, height, delay);
	GifWriteFrame(&g, blue.data(), width, height, delay);
	GifEnd(&g);
	system("firefox bwgif.gif");
	return 0;
}
