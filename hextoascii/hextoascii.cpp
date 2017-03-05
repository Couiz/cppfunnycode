#include <fstream>
#include <iostream>
#include <string>


unsigned char hexval(unsigned char c)
{
    if ('0' <= c && c <= '9')
        return c - '0';
    else if ('a' <= c && c <= 'f')
        return c - 'a' + 10;
    else if ('A' <= c && c <= 'F')
        return c - 'A' + 10;
    throw "out of range"; 
} // http://stackoverflow.com/questions/3790613/how-to-convert-a-string-of-hex-values-to-a-string

unsigned char HexToAscii(unsigned char h, unsigned char l)
{
    return (hexval(h) << 4) + hexval(l);
}

std::string CreateFileName(const char* name)
{
    std::string newName(name);
    newName.insert(newName.find('.'), "_ascii");

    return newName;
}

int main(int argc, char** argv)
{
    if(argc != 2 && argc != 3)
    {
        std::cout << "Using:\t<infile>\n\t<infile> <outfile>\n";
        return 1;
    }
    
    std::ifstream inFile(argv[1]);
    if(inFile.fail())
    {
        std::cout << "Can't find input file.\n";
        return 2;
    }

    std::ofstream outFile(argc == 2 ? CreateFileName(argv[1]) : argv[2]);

    char buf[2];
    while(inFile >> buf[0] >> buf[1])
    {
        outFile << HexToAscii(buf[0], buf[1]);
    }

    inFile.close();
    outFile.close();
    return 0;
}