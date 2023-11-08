// dllmain.cpp : Defines the entry point for the DLL application.
#pragma warning(disable : 4996)
#include "pch.h"
#include "TcpClientSocket.hpp"
#include<string>
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}


const int BUFFER_SIZE = 2048;
const int HEAD_SIZE = 24;

int Search(const char* src, const char* pattern, int start, int srcSize, int patternSize) {
	for (int i = start; i < srcSize; i++) {
		if (memcmp(&src[i], pattern, patternSize) == 0)
			return i;
	}

	return -1;
}

bool WriteImage(TcpClientSocket* tcpClient, const char* path, int size) {
	char* pattern = new  char[] {
		0x00,
			0x00,
			0x00,
			0x00,
			0x00,
			0x00,
			0x00,
			0x00,
			0x04,
			0x00,
			0x00,
			0x00,
			0x00,
			0x00,
			0x00 };

	int start = 4;
	char* buffer = new char[BUFFER_SIZE];
	char* data = new char[size * 2];
	int written = 0;
	int lastMatch = 0;
	while (written < size) {
		ZeroMemory(buffer, BUFFER_SIZE);
		int ret = tcpClient->receiveData(buffer, BUFFER_SIZE);

		if (written == 0) {
			memcpy(&data[written], &buffer[21], ret - 21);
			written += ret - 21;
		}
		else {
			memcpy(&data[written], buffer, ret);
			written += ret;
		}

		do {
			int i = Search(data, pattern, lastMatch, written, 15);
			if (i == -1 || i + HEAD_SIZE >= written) {
				break;
			}

			start += 4;
			pattern[8] = start & 0x000000ff;
			pattern[9] = (start & 0x0000ff00) >> 8;
			pattern[10] = (start & 0x00ff0000) >> 16;
			pattern[11] = (start & 0xff000000) >> 24;

			memcpy(&data[i], &data[i + HEAD_SIZE], written - (i + HEAD_SIZE));

			written -= HEAD_SIZE;
			for (int j = lastMatch; j < i; j++) {
				data[j] = (unsigned char)data[j] ^ 0x54;
			}

			lastMatch = i;
		} while (true);
	}

	while (lastMatch < size) {
		data[lastMatch] = (unsigned char)data[lastMatch] ^ 0x54;
		lastMatch++;
	}

	FILE* file = fopen(path, "wb");
	fwrite(data, sizeof(char), size, file);
	fclose(file);

	delete[] pattern;
	delete[] buffer;
	delete[] data;


	return true;
}

extern "C" __declspec(dllexport)
int GetImage(const  char* host, short port,const char* dir, const  char* fileName, const char* savePath) {
	TcpClientSocket tcpClient(host, port);
	tcpClient.openConnection();

	char* buffer = new char[BUFFER_SIZE];
	ZeroMemory(buffer, BUFFER_SIZE);
	const char* data = "10006:\r\n";
	if (!tcpClient.sendData((void*)data, strlen(data))) {
		return 1;
	}

	ZeroMemory(buffer, BUFFER_SIZE);
	tcpClient.receiveLine(buffer, BUFFER_SIZE);
	if (strcmp(buffer, "20006:\r\n") != 0) {
		return 2;
	}

	char* cmd = new char[BUFFER_SIZE];
	ZeroMemory(cmd, BUFFER_SIZE);
	strcpy(cmd, dir);
	strcat(cmd, "\r\n");
	strcat(cmd, fileName);
	strcat(cmd, "\r\n");
	if (!tcpClient.sendData(cmd, strlen(cmd))) {
		return 3;
	}

	char* result = new char[BUFFER_SIZE];
	ZeroMemory(result, BUFFER_SIZE);
	strcpy(result, fileName);
	strcat(result, "\r\n");

	ZeroMemory(buffer, BUFFER_SIZE);
	tcpClient.receiveLine(buffer, BUFFER_SIZE);
	if (strcmp(buffer, result) > 0) {
		return 4;
	}

	ZeroMemory(buffer, BUFFER_SIZE);
	tcpClient.receiveLine(buffer, BUFFER_SIZE);
	if (strcmp(buffer, "END\r\n") > 0) {
		return 5;
	}

	delete[] result;

	ZeroMemory(cmd, BUFFER_SIZE);
	strcpy(cmd, "10001:\r\n");
	if (!tcpClient.sendData(cmd, strlen(cmd))) {
		return 6;
	}

	ZeroMemory(buffer, BUFFER_SIZE);
	tcpClient.receiveLine(buffer, BUFFER_SIZE);
	if (strcmp(buffer, "10002:\r\n") != 0) {
		return 7;
	}

	cmd[0xc] = 0x48;
	cmd[0xd] = 0x1;
	cmd[0x14] = 0x1;
	cmd[0x155] = 0x1;
	memcpy(&cmd[0x15], fileName, strlen(fileName));
	memcpy(&cmd[0x8d], dir, strlen(dir));
	memcpy(&cmd[0xf1], dir, strlen(dir));
	if (!tcpClient.sendData(cmd, 1048)) {
		return 8;
	}

	ZeroMemory(buffer, BUFFER_SIZE);
	tcpClient.receiveLine(buffer, BUFFER_SIZE);

	char* ptr = strchr(buffer, ':');
	if (ptr == NULL) {
		return 9;
	}

	int size = std::stoi(++ptr);
	ZeroMemory(cmd, BUFFER_SIZE);
	strcpy(cmd, "10005:0\r\n");
	if (!tcpClient.sendData(cmd, strlen(cmd))) {
		return 10;
	}

	delete[] cmd;
	delete[] buffer;

	if (!WriteImage(&tcpClient, savePath, size)) {
		return 11;
	}

	return 0;
}
