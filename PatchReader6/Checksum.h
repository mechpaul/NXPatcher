#define SIZESBOX 256

struct Checksum
{
	unsigned int checksum;
	unsigned int verify;
};

void Generatesbox(int* sbox);
void CompareChecksums(unsigned int a, unsigned int b);
unsigned int CalculateChecksum(unsigned char* eachBlock, int* sbox, long lengthOfBlock, unsigned int rollingChecksum);
unsigned int CalculateChecksumFile(char* fileName);
