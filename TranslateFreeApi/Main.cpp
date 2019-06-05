#include <stdio.h>
#include <Windows.h>
#include <string>
#include <wininet.h>

#include "jsoncpp/json.h"

#pragma comment(lib, "wininet.lib")

using namespace std;

string Utf8ToAnsi(string Utf8Str)
{
	// Utf-8 -> Unicode
	INT Length = MultiByteToWideChar(CP_UTF8, 0, Utf8Str.c_str(), -1, nullptr, 0);
	WCHAR *UnicodeBuffer = (WCHAR*)malloc((Length + 1) * 2);
	Length = MultiByteToWideChar(CP_UTF8, 0, Utf8Str.c_str(), -1, UnicodeBuffer, Length);
	UnicodeBuffer[Length] = L'\0';

	// Unicode -> Ansi
	Length = WideCharToMultiByte(CP_ACP, 0, UnicodeBuffer, -1, nullptr, 0, nullptr, nullptr);
	CHAR *AnsiBuffer = (CHAR*)malloc(Length + 1);
	Length = WideCharToMultiByte(CP_ACP, 0, UnicodeBuffer, -1, AnsiBuffer, Length, nullptr, nullptr);
	AnsiBuffer[Length] = '\0';

	string Out = AnsiBuffer;

	free(AnsiBuffer);
	free(UnicodeBuffer);
	return Out;
}

string InternetGet(string HostName, INTERNET_PORT Port, string ObjectName)
{
#define ONCE_READ_SIZE	( 0x100 )

	// 打开句柄
	HINTERNET hInternet = InternetOpenA("Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0)", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
	if (hInternet == NULL) {
		return "";
	}

	// 连接主机
	HINTERNET hConnect = InternetConnectA(hInternet, HostName.c_str(), Port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
	if (hConnect == NULL) {
		InternetCloseHandle(hInternet);
		return "";
	}

	// 创建请求
	HINTERNET hRequest = HttpOpenRequestA(hConnect, "GET", ObjectName.c_str(), "HTTP/1.1", NULL, NULL, INTERNET_FLAG_RELOAD, 0);
	if (hRequest == NULL) {
		InternetCloseHandle(hConnect);
		InternetCloseHandle(hInternet);
		return "";
	}

	// 发送请求
	CHAR AdditionalHeader[] = 
		"Accept: */*\r\n"
		"Accept-Language: zh-CN\r\n";
	if (!HttpSendRequestA(hRequest, AdditionalHeader, (DWORD)strlen(AdditionalHeader), NULL, 0)) {
		InternetCloseHandle(hConnect);
		InternetCloseHandle(hInternet);
		return "";
	}

	// 读取数据
	string Buffer;
	while (true)
	{
		DWORD BytesRead = 0;
		CHAR TempBuffer[ONCE_READ_SIZE + 1];
		RtlZeroMemory(TempBuffer, sizeof(TempBuffer));

		InternetReadFile(hRequest, TempBuffer, ONCE_READ_SIZE, &BytesRead);
		if (BytesRead) {
			TempBuffer[BytesRead] = '\0';
			Buffer += TempBuffer;
		}
		else {
			break;
		}
	}

	InternetCloseHandle(hRequest);
	InternetCloseHandle(hConnect);
	InternetCloseHandle(hInternet);

	return Buffer;
}

string YoudaoTranslate(string Origin)
{
	// http://fanyi.youdao.com/translate?&doctype=json&type=AUTO&i=计算
	string Response = Utf8ToAnsi(InternetGet("fanyi.youdao.com", 80, "/translate?&doctype=json&type=auto&i=" + Origin));

	string result = "";

	Json::CharReaderBuilder ReaderBuilder;
	Json::Value Root;
	Json::String Error;

	Json::CharReader *JsonReader = ReaderBuilder.newCharReader();

	if (!JsonReader->parse(Response.c_str(), Response.c_str() + Response.length(), &Root, &Error) || !Error.empty()) {
		goto Exit;
	}

	/*
	
		printf("ErrorCode: %d\n", Root["errorCode"].asInt());
		printf("ElapsedTime: %d\n", Root["elapsedTime"].asInt());

		printf("translateResult - src: %s\n", Root["translateResult"][0][0]["src"].asString().c_str());
		printf("translateResult - tgt: %s\n", Root["translateResult"][0][0]["tgt"].asString().c_str());

	*/

	if (Root["errorCode"].asInt()) {
		goto Exit;
	}

	result = Root["translateResult"][0][0]["tgt"].asString();

Exit:

	delete[] JsonReader;
	return result;
}
string GoogleTranslate(string Origin, string TargetLanguage = "auto")
{
	// http://translate.google.cn/translate_a/single?client=gtx&dt=t&dj=1&ie=UTF-8&sl=auto&tl=zh_TW&q=calculate
	string Response = Utf8ToAnsi(InternetGet("translate.google.cn", 80, "translate_a/single?client=gtx&dt=t&dj=1&ie=Ansi&sl=auto&tl=" + TargetLanguage + "&q=" + Origin));

	string result = "";

	Json::CharReaderBuilder ReaderBuilder;
	Json::Value Root;
	Json::String Error;

	Json::CharReader *JsonReader = ReaderBuilder.newCharReader();

	if (!JsonReader->parse(Response.c_str(), Response.c_str() + Response.length(), &Root, &Error) || !Error.empty()) {
		goto Exit;
	}
	
	/*
		printf("trans: %s\n", Root["sentences"][0]["trans"].asString().c_str());
		printf("orig: %s\n", Root["sentences"][0]["orig"].asString().c_str());

		printf("src: %s\n", Root["src"].asString().c_str());
		printf("confidence: %f\n", Root["confidence"].asFloat());
	*/
	result = Root["sentences"][0]["trans"].asString();

Exit:

	delete[] JsonReader;
	return result;
}

int main()
{
	printf("%s\n", YoudaoTranslate("Fuck you!").c_str());
	printf("%s\n", YoudaoTranslate("那你去找物管啊！").c_str());
	printf("%s\n", YoudaoTranslate("白上 フブキ").c_str());

	printf("\n");

	printf("%s\n", GoogleTranslate("Fuck you!").c_str());
	printf("%s\n", GoogleTranslate("那你去找物管啊！", "ja").c_str());
	printf("%s\n", GoogleTranslate("白上 フブキ", "zh_CN").c_str());

	getchar();
	return 0;
}