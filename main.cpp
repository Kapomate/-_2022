
#include "stdafx.h"
#include "visualise.h"

struct ICMPheader
{
	unsigned char	byType;
	unsigned char	byCode;
	unsigned short	nChecksum;
	unsigned short	nId;
	unsigned short	nSequence;
};

struct IPheader
{
	unsigned char	byVerLen;
	unsigned char	byTos;
	unsigned short	nTotalLength;
	unsigned short	nId;
	unsigned short	nOffset;
	unsigned char	byTtl;
	unsigned char	byProtocol;
	unsigned short	nChecksum;
	unsigned int	nSrcAddr;
	unsigned int	nDestAddr;
};


unsigned short CalcChecksum(char* pBuffer, int nLen);
bool ValidateChecksum(char* pBuffer, int nLen);
bool Initialize();
bool UnInitialize();
bool ResolveIP(char* pszRemoteHost, char** pszIPAddress);
std::string add_one_to_bin(std::string string1);
std::string to_binary_string(std::string string1);
std::string bin_to_ip(std::string binary);
std::string deletedots(std::string str);
std::string bin_to_dec(std::string binary);



int main(int argc, char* argv[])
{


	std::vector <std::string> splittedip;
	std::vector <std::string> routers; //������ ������������
	std::vector <std::string> ipaddresses;
	std::vector <std::string> pingeddevs; //������ ������, ���������� �� ������
	std::vector <std::string> iplist; //������ ������������ ������� �� ���� ����������
	std::vector <std::vector<std::string>> switches(argc - 2); //������ ��������, �������� � ���� ������ ������. ��������� �� ������ ����.
	std::string token;
	std::vector <std::string> mask; //����� ��� ������� ���������

	std::string buffer = "";
	std::string digitizedip = "";
	size_t pos = 0;
	std::string delimiter = ",";

	std::string marsh = argv[argc - 1];
	std::string imtired;
	imtired = marsh.substr(1, marsh.size() - 2);

	//������� ��� ��������� ���� � ������ routers

	while ((pos = imtired.find(delimiter)) != std::string::npos) {
		token = imtired.substr(0, pos);
		routers.push_back(token);
		imtired.erase(0, pos + delimiter.length());
	}
	token = imtired.substr(0, imtired.find(','));
	routers.push_back(token);

	for (int r = 0; r < switches.size(); r++)
	{
		switches[r].push_back(routers[r]);
	}

	//����������� ��� ��������� ip-������ ��� �������� ����������
	for (int t = 1; t < argc - 1; t++) {
		std::string s = argv[t];
		delimiter = "-";
		token = "";
		digitizedip = "";
		buffer = "";
		splittedip.clear();
		ipaddresses.clear();

		//����� ���������� ...-... �� ��� �����
		while ((pos = s.find(delimiter)) != std::string::npos) {
			token = s.substr(0, pos);
			splittedip.push_back(token);
			s.erase(0, pos + delimiter.length());
		}
		token = s.substr(0, s.find('/'));
		//��������� �������� ����� ��� ������� ��������� �������
		mask.push_back(s.substr(s.find('/') + 1));

		splittedip.push_back(token);
		//������������ ������ � ���� 32-������� �������� ��������
		for (int i = 0; i < splittedip.size(); i++)
		{
			delimiter = ".";
			pos = 0;
			token = "";
			digitizedip = "";
			while ((pos = splittedip[i].find(delimiter)) != std::string::npos) {
				token = splittedip[i].substr(0, pos);
				digitizedip += to_binary_string(token);
				splittedip[i].erase(0, pos + delimiter.length());
			}
			digitizedip += to_binary_string(splittedip[i]);
			ipaddresses.push_back(digitizedip);
		}

		buffer = ipaddresses[0];
		//���������� ��� ��������� ������ � ���������� �� � iplist
		while (buffer != ipaddresses[1])
		{
			buffer = add_one_to_bin(buffer);
			iplist.push_back(bin_to_ip(buffer));
		}

	}
	//� ����� �������� ������������ ��� ������ �� iplist
	for (int e = 0; e < iplist.size(); e++)
	{
		if (Initialize() == false)
		{
			return -1;
		}

		int nSequence = 0;
		int nMessageSize = 32;	//������ ���������
		int nTimeOut = 200;	//�����, �� ������� ������ ���� ������� �����
		int nCount = 4;	//���������� �������

		char* pszRemoteIP = NULL, * pSendBuffer = NULL, * pszRemoteHost = NULL;

		int n = iplist[e].length();


		//������ �������� ������ �����, � �������� ����� �������� ������
		char char_array[33];
		strcpy_s(char_array, iplist[e].c_str());
		pszRemoteHost = char_array;

		//��������, ������� �� ����������� ������
		if (ResolveIP(pszRemoteHost, &pszRemoteIP) == false)
		{
			std::cerr << std::endl << "Unable to resolve hostname" << std::endl;
			return -1;
		}

		std::cout << "Pinging " << pszRemoteHost << " [" << pszRemoteIP << "] with " << nMessageSize << " bytes of data." << std::endl << std::endl;
		ICMPheader sendHdr;

		SOCKET sock;
		sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);	//������ �����, ����������� ICMP

		SOCKADDR_IN dest;	//����� ���������� ��� �������� ICMP-�������
		dest.sin_addr.S_un.S_addr = inet_addr(pszRemoteIP);
		dest.sin_family = AF_INET;
		dest.sin_port = rand();	//�������� ��������� ����

		int nResult = 0;

		fd_set fdRead;
		SYSTEMTIME timeSend, timeRecv;
		int nTotalRoundTripTime = 0, nMaxRoundTripTime = 0, nMinRoundTripTime = -1, nRoundTripTime = 0;
		int nPacketsSent = 0, nPacketsReceived = 0;

		timeval timeInterval = { 0, 0 };
		timeInterval.tv_usec = nTimeOut * 1000;

		sendHdr.nId = htons(rand());	//����� �������

		while (nPacketsSent < nCount)
		{
			//�������� ����� ��������� ��� �������� ��������� � ������
			pSendBuffer = new char[sizeof(ICMPheader) + nMessageSize];

			sendHdr.byCode = 0;	//��� echo � reply = 0
			sendHdr.nSequence = htons(nSequence++);
			sendHdr.byType = 8;	//��� echo = 8
			sendHdr.nChecksum = 0;	//����������� �����

			memcpy_s(pSendBuffer, sizeof(ICMPheader), &sendHdr, sizeof(ICMPheader));	//������� ��������� � ������ ��������� �����
			memset(pSendBuffer + sizeof(ICMPheader), 'x', nMessageSize);	//�������� ��������� ��������� ������������ ���������

			//��������� ����������� �����
			sendHdr.nChecksum = htons(CalcChecksum(pSendBuffer, sizeof(ICMPheader) + nMessageSize));

			//������� ������� � ����� � ����������� ������
			memcpy_s(pSendBuffer, sizeof(ICMPheader), &sendHdr, sizeof(ICMPheader));

			nResult = sendto(sock, pSendBuffer, sizeof(ICMPheader) + nMessageSize, 0, (SOCKADDR*)&dest, sizeof(SOCKADDR_IN));

			//�������� ����� �������� ������ ��� ����������� �������� �������
			::GetSystemTime(&timeSend);

			++nPacketsSent;



			FD_ZERO(&fdRead);
			FD_SET(sock, &fdRead);

			//���� ������
			if ((nResult = select(0, &fdRead, NULL, NULL, &timeInterval))
				== SOCKET_ERROR)
			{
				std::cerr << std::endl << "An error occured in select operation: " << "WSAGetLastError () = " <<
					WSAGetLastError() << std::endl;
				delete[]pSendBuffer;
				return -1;
			}

			if (nResult > 0 && FD_ISSET(sock, &fdRead))
			{
				//�������� ����� ��� �������� ������
				char* pRecvBuffer = new char[1500];
				//���� �������� ������ 
				if ((nResult = recvfrom(sock, pRecvBuffer, 1500, 0, 0, 0))
					== SOCKET_ERROR)
				{
					std::cerr << std::endl << "An error occured in recvfrom operation: " << "WSAGetLastError () = " <<
						WSAGetLastError() << std::endl;
					UnInitialize();
					delete[]pSendBuffer;
					delete[]pRecvBuffer;
					return -1;
				}

				//������� ����� ��������� ������
				::GetSystemTime(&timeRecv);

				//�� �������� �����, ������� �� ������ �� ���� ��������� � ��������� ICMP
				ICMPheader recvHdr;
				char* pICMPbuffer = NULL;

				//����� ����� �������� ��������� IP, ������� �� ������������ �� 20 ���� ������, ����� ��������� ��������� ICMP
				pICMPbuffer = pRecvBuffer + sizeof(IPheader);

				//����� ��������� ICMP ����������� ����� ��������� ������� ��������� IP ��
				//������ ������� ���������
				int nICMPMsgLen = nResult - sizeof(IPheader);

				//�������� ��������� ICMP
				memcpy_s(&recvHdr, sizeof(recvHdr), pICMPbuffer, sizeof(recvHdr));

				//�������� ��������� IP �� ������
				IPheader ipHdr;
				memcpy_s(&ipHdr, sizeof(ipHdr), pRecvBuffer, sizeof(ipHdr));

				recvHdr.nId = recvHdr.nId;
				recvHdr.nSequence = recvHdr.nSequence;
				recvHdr.nChecksum = ntohs(recvHdr.nChecksum);

				//���������, �������� �� ����� echo, ��������� �� ������ ��������
				//� ����������� �����
				if (recvHdr.byType == 0 &&
					recvHdr.nId == sendHdr.nId &&
					recvHdr.nSequence == sendHdr.nSequence &&
					ValidateChecksum(pICMPbuffer, nICMPMsgLen) &&
					memcmp(pSendBuffer + sizeof(ICMPheader), pRecvBuffer + sizeof(ICMPheader) + sizeof(IPheader),
						nResult - sizeof(ICMPheader) - sizeof(IPheader)) == 0)
				{
					//���� ��� ������
					int nSec = timeRecv.wSecond - timeSend.wSecond;
					if (nSec < 0)
					{
						nSec = nSec + 60;
					}

					int nMilliSec = abs(timeRecv.wMilliseconds - timeSend.wMilliseconds);

					int nRoundTripTime = 0;
					nRoundTripTime = abs(nSec * 1000 - nMilliSec);
					nTotalRoundTripTime = nTotalRoundTripTime + nRoundTripTime;

					if (nMinRoundTripTime == -1)
					{
						nMinRoundTripTime = nRoundTripTime;
						nMaxRoundTripTime = nRoundTripTime;
					}
					else if (nRoundTripTime < nMinRoundTripTime)
					{
						nMinRoundTripTime = nRoundTripTime;
					}
					else if (nRoundTripTime > nMaxRoundTripTime)
					{
						nMaxRoundTripTime = nRoundTripTime;
					}

					++nPacketsReceived;
					//���� �������� ���� �� ���� �����, ��������� ������� � �������, ��� ���� ��������
					pingeddevs.push_back(pszRemoteIP); break;
				}
				else
				{
					break;
				}
				//������ ������
				delete[]pRecvBuffer;
			}

			//������ ������
			delete[]pSendBuffer;
		}

		std::cout << '\r' << std::endl;
	}

	//��������� ������ �������� switch �� �������:
	//� 0 �������� ������� ���������� ���������� ����� ����
	//����� ��� ������� ��������� ���������� ������ ������
	for (int r = 0; r < switches.size(); r++)
	{
		for (int y = 0; y < pingeddevs.size(); y++)
		{
			if (deletedots(routers[r]).substr(0, stoi(mask[r])) == deletedots(pingeddevs[y]).substr(0, stoi(mask[r])))
			{
				switches[r].push_back('.' + bin_to_dec(deletedots(pingeddevs[y]).substr(stoi(mask[r]))));
				//std::cout<<('.' + bin_to_dec(deletedots(pingeddevs[y]).substr(stoi(mask[r]))));
			}
		}
	}

	std::vector<std::string> comms;
	comms.push_back(" ");
	for (int i = 0; i < routers.size(); i++)
	{
		comms.push_back(routers[i]);
	}
	std::vector<std::vector<std::string>> sw_to_vis;

	for (int i = 0; i < routers.size(); i++)
	{
		sw_to_vis.push_back({ routers[i] });
	}

	for (int i = 0; i < switches.size(); i++)
	{
		for (int j = 0; j < sw_to_vis.size(); j++)
		{
			if (sw_to_vis[j][0] == switches[i][0])
			{
				sw_to_vis[j] = switches[i];
				break;
			}
		}
	}

	visualise(sw_to_vis, comms);

	if (UnInitialize() == false)
	{
		return -1;
	}

	return 0;
}
// ������� ����������� �����
unsigned short CalcChecksum(char* pBuffer, int nLen)
{
	unsigned short nWord;
	unsigned int nSum = 0;
	int i;

	//�������� 16-������ ����� �� ������ ����
	//�������� 8-������ ���� � ������ � ������
	for (i = 0; i < nLen; i = i + 2)
	{
		nWord = ((pBuffer[i] << 8) & 0xFF00) + (pBuffer[i + 1] & 0xFF);
		nSum = nSum + (unsigned int)nWord;
	}

	//�������� ������ 16 ��� �� 32-������ ����� � �������
	while (nSum >> 16)
	{
		nSum = (nSum & 0xFFFF) + (nSum >> 16);
	}

	//���������� �� �������
	nSum = ~nSum;

	return ((unsigned short)nSum);
}

//�������� ����������� �����
bool ValidateChecksum(char* pBuffer, int nLen)
{
	unsigned short nWord;
	unsigned int nSum = 0;
	int i;

	//�������� 16-������ ����� �� ������ ����
	//�������� 8-������ ���� � ������ � ������
	for (i = 0; i < nLen; i = i + 2)
	{
		nWord = ((pBuffer[i] << 8) & 0xFF00) + (pBuffer[i + 1] & 0xFF);
		nSum = nSum + (unsigned int)nWord;
	}

	//�������� ������ 16 ��� �� 32-������ ����� � �������
	while (nSum >> 16)
	{
		nSum = (nSum & 0xFFFF) + (nSum >> 16);
	}
	return ((unsigned short)nSum == 0xFFFF);
}

bool Initialize()
{
	//�������������� WinSock
	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) == SOCKET_ERROR)
	{
		std::cerr << std::endl << "An error occured in WSAStartup operation: " << "WSAGetLastError () = " << WSAGetLastError() << std::endl;
		return false;
	}

	SYSTEMTIME time;
	::GetSystemTime(&time);

	//�������� ��������� ��������� ����� ������� ��������� � �������������
	srand(time.wMilliseconds);

	return true;
}

bool UnInitialize()
{
	//Cleanup
	if (WSACleanup() == SOCKET_ERROR)
	{
		std::cerr << std::endl << "An error occured in WSACleanup operation: WSAGetLastError () = " << WSAGetLastError() << std::endl;
		return false;
	}

	return true;
}

//��������, ������� �� ����������� ������
bool ResolveIP(char* pszRemoteHost, char** pszIPAddress)
{
	hostent* pHostent = gethostbyname(pszRemoteHost);
	if (pHostent == NULL)
	{
		std::cerr << std::endl << "An error occured in gethostbyname operation: WSAGetLastError () = " << WSAGetLastError() << std::endl;
		return false;
	}

	in_addr in;
	memcpy_s(&in, sizeof(in_addr), pHostent->h_addr_list[0], sizeof(in_addr));
	*pszIPAddress = inet_ntoa(in);

	return true;
}

//��������� ������ � 32-������ �������� ��������
std::string to_binary_string(std::string string1)
{

	int n = stoi(string1);
	std::string buffer; // ������� ������ � �������� �������
	// ������� ������ ������� �� ���������
	buffer.reserve(std::numeric_limits<unsigned int>::digits);
	do
	{
		buffer += char('0' + n % 2); // ��������� � �����
		n = n / 2;
	} while (n > 0);
	while (buffer.size() < 8) buffer += '0';
	return std::string(buffer.crbegin(), buffer.crend()); // ������������� ���������
}

//������� ���������� � ��������� ����� �������
std::string add_one_to_bin(std::string string1)
{
	int i = string1.size();
	while (string1[i - 1] != '0')
	{
		string1[i - 1] = '0';
		i--;
	}
	string1[i - 1] = '1';
	return string1;
}

//������� �������� ��������� ����� � ����������
std::string bin_to_dec(std::string binary)
{
	int result = 0;
	for (int i = binary.size() - 1; i >= 0; i--)
	{
		if (binary[i] == '1') result = result + pow(2, binary.size() - 1 - i);
	}
	return std::to_string(result);
}

//������� 32-������� ��������� ����� � ������ ip-������
std::string bin_to_ip(std::string binary)
{
	std::string decimalnum;
	for (int i = 1; i < 4; i++)
	{
		decimalnum += bin_to_dec(binary.substr((i - 1) * 8, 8));
		decimalnum += '.';
	}
	decimalnum += bin_to_dec(binary.substr((4 - 1) * 8, 8));
	return decimalnum;
}

//���������� ip � 32-������ �������� ��������
std::string deletedots(std::string str)
{
	std::string ret = ""; int i = 0; std::string buf;

	while (str[i])
	{

		while (str[i] != '.' && i != str.size())
		{
			buf += str[i];
			i++;
		}
		ret += to_binary_string(buf);
		buf = "";
		if (i != str.size()) i++;
	}
	return ret;
}