#include <iostream>
#include <stdio.h>

#include "Stream.h"

Stream::Stream() : managed(false), buffer(nullptr) {
	currentOffset = 0;
	lastReaderIndex = 0;
	for (int i = 0; i < 32; i++)
		bitMaskOut[i] = (1 << i) - 1;
	frameStackPtr = -1;
	memset(frameStack, 0, frameStackSize * sizeof(int));
}

Stream::Stream(int newSize) : managed(true) {
	buffer = new char[newSize];
	memset(buffer, 0, newSize);

	currentOffset = 0;
	lastReaderIndex = 0;
	length = newSize;
	for (int i = 0; i < 32; i++)
		bitMaskOut[i] = (1 << i) - 1;
	frameStackPtr = -1;
	memset(frameStack, 0, frameStackSize * sizeof(int));
}

Stream::Stream(const std::vector<char>& _v) : managed(false) {
	buffer = const_cast<char*>(&_v[0]);

	currentOffset = 0;
	lastReaderIndex = 0;
	length = _v.size();
	for (int i = 0; i < 32; i++)
		bitMaskOut[i] = (1 << i) - 1;
	frameStackPtr = -1;
	memset(frameStack, 0, frameStackSize * sizeof(int));
}

Stream::Stream(char* _data, std::size_t _length) : buffer(_data), length(static_cast<int>(_length)), managed(false) {
	currentOffset = 0;
	lastReaderIndex = 0;
	for (int i = 0; i < 32; i++)
		bitMaskOut[i] = (1 << i) - 1;
	frameStackPtr = -1;
	memset(frameStack, 0, frameStackSize * sizeof(int));
}

Stream::~Stream() {
	if (managed && buffer) {
		delete[] buffer;
		buffer = nullptr;
	}
	// Clean up other resources or state as needed
}

int Stream::peek(int position)
{
	return (buffer[currentOffset + position] & 0xff);
}

int Stream::remaining() {
	return currentOffset < length ? length - currentOffset : 0;
}

bool Stream::markReaderIndex()
{
	lastReaderIndex = currentOffset;
	return false;
}

bool Stream::resetReaderIndex()
{
	currentOffset = lastReaderIndex;
	return false;
}

bool Stream::deleteReaderBlock() {
	const int block_size = currentOffset - lastReaderIndex;
	if (block_size > 0) {
		const int rem_block_size = length - currentOffset;
		if (rem_block_size > 0)
			memmove(buffer + lastReaderIndex, buffer + currentOffset, rem_block_size);
		length -= block_size;
		currentOffset = lastReaderIndex;
	}
	return false;
}

bool Stream::clearBuf()
{
	lastReaderIndex = 0;
	currentOffset = 0;
	ZeroMemory(buffer, length);
	length = 0;
	return false;
}

void Stream::createFrame(int id) {
	buffer[currentOffset++] = (unsigned char)id;
}

void Stream::createFrameVarSize(int id) { // creates a variable sized
											// frame
	buffer[currentOffset++] = (unsigned char)id;
	buffer[currentOffset++] = 0; // placeholder for size byte
	if (frameStackPtr >= frameStackSize - 1) {
		printf("Stack overflow\n");
	}
	else
		frameStack[++frameStackPtr] = currentOffset;
}

void Stream::createFrameVarSizeWord(int id) { // creates a variable sized
												// frame
	buffer[currentOffset++] = (unsigned char)id;
	writeWord(0); // placeholder for size word
	if (frameStackPtr >= frameStackSize - 1) {
		printf("Stack overflow\n");
	}
	else
		frameStack[++frameStackPtr] = currentOffset;
}

void Stream::endFrameVarSize() {// ends a variable sized frame
	if (frameStackPtr < 0)
		printf("Stack empty (byte)\n");
	else
		writeFrameSize(currentOffset - frameStack[frameStackPtr--]);
}

void Stream::endFrameVarSizeWord() { // ends a variable sized frame
	if (frameStackPtr < 0)
		printf("Stack empty (short)\n");
	else
		writeFrameSizeWord(currentOffset - frameStack[frameStackPtr--]);
}


//Write types
void Stream::writeByte(int i) {
	buffer[currentOffset++] = (char)i; //umm (byte)?
}

void Stream::writeByteA(int i) {
	buffer[currentOffset++] = (char)(i + 128);
}

void Stream::writeByteC(int i) {
	buffer[currentOffset++] = (char)(-i);
}

void Stream::writeBytes(char abyte0[], int i, int j) {
	for (int k = j; k < j + i; k++)
		buffer[currentOffset++] = abyte0[k];

}

void Stream::writeByteS(int i) {
	buffer[currentOffset++] = (char)(128 - i);
}

void Stream::writeBytes_reverse(char abyte0[], int i, int j) {
	for (int k = (j + i) - 1; k >= j; k--)
		buffer[currentOffset++] = abyte0[k];

}

void Stream::writeBytes_reverseA(char abyte0[], int i, int j) {
	for (int k = (j + i) - 1; k >= j; k--)
		buffer[currentOffset++] = (char)(abyte0[k] + 128);

}

void Stream::write3Byte(int i) {
	buffer[currentOffset++] = (char)(i >> 16);
	buffer[currentOffset++] = (char)(i >> 8);
	buffer[currentOffset++] = (char)i;
}

void Stream::writeDWord(int i) {
	buffer[currentOffset++] = (char)(i >> 24);
	buffer[currentOffset++] = (char)(i >> 16);
	buffer[currentOffset++] = (char)(i >> 8);
	buffer[currentOffset++] = (char)i;
}

void Stream::writeDWord_v1(int i) {
	buffer[currentOffset++] = (char)(i >> 8);
	buffer[currentOffset++] = (char)i;
	buffer[currentOffset++] = (char)(i >> 24);
	buffer[currentOffset++] = (char)(i >> 16);
}

void Stream::writeDWord_v2(int i) {
	buffer[currentOffset++] = (char)(i >> 16);
	buffer[currentOffset++] = (char)(i >> 24);
	buffer[currentOffset++] = (char)i;
	buffer[currentOffset++] = (char)(i >> 8);
}

void Stream::writeDWordBigEndian(int i) {
	buffer[currentOffset++] = (char)i;
	buffer[currentOffset++] = (char)(i >> 8);
	buffer[currentOffset++] = (char)(i >> 16);
	buffer[currentOffset++] = (char)(i >> 24);
}

void Stream::writeFrameSize(int i) {
	buffer[currentOffset - i - 1] = (char)i;
}

void Stream::writeFrameSizeWord(int i) {
	buffer[currentOffset - i - 2] = (char)(i >> 8);
	buffer[currentOffset - i - 1] = (char)i;
}

void Stream::writeQWord(unsigned __int64 l) {
	buffer[currentOffset++] = (char)(unsigned int)(l >> 56);
	buffer[currentOffset++] = (char)(unsigned int)(l >> 48);
	buffer[currentOffset++] = (char)(unsigned int)(l >> 40);
	buffer[currentOffset++] = (char)(unsigned int)(l >> 32);
	buffer[currentOffset++] = (char)(unsigned int)(l >> 24);
	buffer[currentOffset++] = (char)(unsigned int)(l >> 16);
	buffer[currentOffset++] = (char)(unsigned int)(l >> 8);
	buffer[currentOffset++] = (char)(unsigned int)l;
}

void Stream::writeString(const char* s) {
	memcpy(buffer + currentOffset, s, strlen(s));
	currentOffset += strlen(s);
	buffer[currentOffset++] = 0;
}

void Stream::writeWord(int i) {
	buffer[currentOffset++] = (char)(i >> 8);
	buffer[currentOffset++] = (char)i;
}

void Stream::writeWordA(int i) {
	buffer[currentOffset++] = (char)(i >> 8);
	buffer[currentOffset++] = (char)(i + 128);
}

void Stream::writeWordBigEndian(int i) {
	buffer[currentOffset++] = (char)i;
	buffer[currentOffset++] = (char)(i >> 8);
}
void Stream::writeWordBigEndian_dup(int i) {
	buffer[currentOffset++] = (char)i;
	buffer[currentOffset++] = (char)(i >> 8);
}

void Stream::writeWordBigEndianA(int i) {
	buffer[currentOffset++] = (char)(i + 128);
	buffer[currentOffset++] = (char)(i >> 8);
}

//bit editing
void Stream::initBitAccess() {
	bitPosition = currentOffset * 8;
}

void Stream::writeBits(int numBits, int value) {
	int bytePos = bitPosition >> 3;
	int bitOffset = 8 - (bitPosition & 7);
	bitPosition += numBits;
	for (; numBits > bitOffset; bitOffset = 8) {
		buffer[bytePos] &= ~bitMaskOut[bitOffset];		// mask out the desired area
		buffer[bytePos++] |= (value >> (numBits - bitOffset)) & bitMaskOut[bitOffset];
		numBits -= bitOffset;
	}
	if (numBits == bitOffset) {
		buffer[bytePos] &= ~bitMaskOut[bitOffset];
		buffer[bytePos] |= value & bitMaskOut[bitOffset];
	}
	else {
		buffer[bytePos] &= ~(bitMaskOut[numBits] << (bitOffset - numBits));
		buffer[bytePos] |= (value & bitMaskOut[numBits]) << (bitOffset - numBits);
	}
}

void Stream::finishBitAccess() {
	currentOffset = (bitPosition + 7) / 8;
}

//read types

void Stream::readBytes(char abyte0[], int i, int j) {
	for (int k = j; k < j + i; k++)
		abyte0[k] = buffer[currentOffset++];

}

void Stream::readBytes_reverse(char abyte0[], int i, int j) {
	for (int k = (j + i) - 1; k >= j; k--) {
		abyte0[k] = buffer[currentOffset++];

	}
}

void Stream::readBytes_reverseA(char abyte0[], int i, int j) {
	for (int k = (j + i) - 1; k >= j; k--)
		abyte0[k] = (char)(buffer[currentOffset++] - 128);

}

unsigned int Stream::readDWord() {
	currentOffset += 4;
	return ((buffer[currentOffset - 4] & 0xff) << 24)
		+ ((buffer[currentOffset - 3] & 0xff) << 16)
		+ ((buffer[currentOffset - 2] & 0xff) << 8)
		+ (buffer[currentOffset - 1] & 0xff);
}

int Stream::readDWord_v1() {
	currentOffset += 4;
	return ((buffer[currentOffset - 2] & 0xff) << 24)
		+ ((buffer[currentOffset - 1] & 0xff) << 16)
		+ ((buffer[currentOffset - 4] & 0xff) << 8)
		+ (buffer[currentOffset - 3] & 0xff);
}

int Stream::readDWord_v2() {
	currentOffset += 4;
	return ((buffer[currentOffset - 3] & 0xff) << 24)
		+ ((buffer[currentOffset - 4] & 0xff) << 16)
		+ ((buffer[currentOffset - 1] & 0xff) << 8)
		+ (buffer[currentOffset - 2] & 0xff);
}

unsigned  __int64 Stream::readQWord() {
	unsigned int dw1 = readDWord();
	unsigned int dw2 = readDWord();
	return (((__int64)dw1) << 32) | (__int64)dw2;
}

char Stream::readSignedByte() {
	return buffer[currentOffset++];
}

char Stream::readSignedByteA() {
	return (char)(buffer[currentOffset++] - 128);
}

char Stream::readSignedByteC() {
	return (char)(-buffer[currentOffset++]);
}

char Stream::readSignedByteS() {
	return (char)(128 - buffer[currentOffset++]);
}

int Stream::readSignedWord() {
	currentOffset += 2;
	int i = ((buffer[currentOffset - 2] & 0xff) << 8)
		+ (buffer[currentOffset - 1] & 0xff);
	if (i > 32767) {
		i -= 0x10000;
	}
	return i;
}
int Stream::readSignedWordA() {
	currentOffset += 2;
	int i = ((buffer[currentOffset - 2] & 0xff) << 8)
		+ (buffer[currentOffset - 1] - 128 & 0xff);
	if (i > 32767) {
		i -= 0x10000;
	}
	return i;
}

int Stream::readSignedWordBigEndian() {
	currentOffset += 2;
	int i = ((buffer[currentOffset - 1] & 0xff) << 8)
		+ (buffer[currentOffset - 2] & 0xff);
	if (i > 32767)
		i -= 0x10000;
	return i;
}

int Stream::readSignedWordBigEndianA() {
	currentOffset += 2;
	int i = ((buffer[currentOffset - 1] & 0xff) << 8)
		+ (buffer[currentOffset - 2] - 128 & 0xff);
	if (i > 32767)
		i -= 0x10000;
	return i;
}

int Stream::read3Byte() {
	currentOffset += 3;
	int i = (((buffer[currentOffset - 3] & 0xff) << 16)
		+ ((buffer[currentOffset - 2] & 0xff) << 8)
		+ (0xff & buffer[currentOffset - 1]));
	if (i > 8388607) {
		i -= 16777216;
	}
	return i;
}

void Stream::readString(char* output) {
	int count = 0;
	byte b;
	int outputOffset = 0;
	while ((b = buffer[currentOffset++]) != 0 && count++ < 5000) {
		output[outputOffset++] = b;
	}
	output[outputOffset++] = '\0';//Null Terminator
}

unsigned char Stream::readUnsignedByte() {
	return buffer[currentOffset++] & 0xff;
}

unsigned char Stream::readUnsignedByteA() {
	return buffer[currentOffset++] - 128 & 0xff;
}

unsigned char Stream::readUnsignedByteC() {
	return -buffer[currentOffset++] & 0xff;
}

unsigned char Stream::readUnsignedByteS() {
	return 128 - buffer[currentOffset++] & 0xff;
}

int Stream::readUnsignedWord() {
	currentOffset += 2;
	return ((buffer[currentOffset - 2] & 0xff) << 8)
		+ (buffer[currentOffset - 1] & 0xff);
}

int Stream::readUnsignedWordA() {
	currentOffset += 2;
	return ((buffer[currentOffset - 2] & 0xff) << 8)
		+ (buffer[currentOffset - 1] - 128 & 0xff);
}

int Stream::readUnsignedWordBigEndian() {
	currentOffset += 2;
	return ((buffer[currentOffset - 1] & 0xff) << 8)
		+ (buffer[currentOffset - 2] & 0xff);
}

int Stream::readUnsignedWordBigEndianA() {
	currentOffset += 2;
	return ((buffer[currentOffset - 1] & 0xff) << 8)
		+ (buffer[currentOffset - 2] - 128 & 0xff);
}
