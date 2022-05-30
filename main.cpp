
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
	std::vector <std::string> routers; //Вектор коммутаторов
	std::vector <std::string> ipaddresses;
	std::vector <std::string> pingeddevs; //Вектор хостов, ответивших на запрос
	std::vector <std::string> iplist; //Вектор всевозможных адресов из всех диапазонов
	std::vector <std::vector<std::string>> switches(argc - 2); //Вектор векторов, хранящий в себе адреса хостов. Разделены по номеру сети.
	std::string token;
	std::vector <std::string> mask; //Маски для каждого диапазона

	std::string buffer = "";
	std::string digitizedip = "";
	size_t pos = 0;
	std::string delimiter = ",";

	std::string marsh = argv[argc - 1];
	std::string imtired;
	imtired = marsh.substr(1, marsh.size() - 2);

	//Запишем все известные сети в вектор routers

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

	//Сгенерируем все возможные ip-адреса для заданных диапазонов
	for (int t = 1; t < argc - 1; t++) {
		std::string s = argv[t];
		delimiter = "-";
		token = "";
		digitizedip = "";
		buffer = "";
		splittedip.clear();
		ipaddresses.clear();

		//Делим промежуток ...-... на две части
		while ((pos = s.find(delimiter)) != std::string::npos) {
			token = s.substr(0, pos);
			splittedip.push_back(token);
			s.erase(0, pos + delimiter.length());
		}
		token = s.substr(0, s.find('/'));
		//Записыаем значение маски для данного диапазона адресов
		mask.push_back(s.substr(s.find('/') + 1));

		splittedip.push_back(token);
		//Представляем адреса в виде 32-битного двочного значения
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
		//Генерируем все возможные адреса и записываем их в iplist
		while (buffer != ipaddresses[1])
		{
			buffer = add_one_to_bin(buffer);
			iplist.push_back(bin_to_ip(buffer));
		}

	}
	//В цикле пытаемся пропинговать все адреса из iplist
	for (int e = 0; e < iplist.size(); e++)
	{
		if (Initialize() == false)
		{
			return -1;
		}

		int nSequence = 0;
		int nMessageSize = 32;	//Размер сообщения
		int nTimeOut = 200;	//Время, за которое должен быть получен ответ
		int nCount = 4;	//Количество попыток

		char* pszRemoteIP = NULL, * pSendBuffer = NULL, * pszRemoteHost = NULL;

		int n = iplist[e].length();


		//Задаем значение адреса хоста, к которому будем посылать запрос
		char char_array[33];
		strcpy_s(char_array, iplist[e].c_str());
		pszRemoteHost = char_array;

		//Проверим, явлется ли допрустимым именем
		if (ResolveIP(pszRemoteHost, &pszRemoteIP) == false)
		{
			std::cerr << std::endl << "Unable to resolve hostname" << std::endl;
			return -1;
		}

		std::cout << "Pinging " << pszRemoteHost << " [" << pszRemoteIP << "] with " << nMessageSize << " bytes of data." << std::endl << std::endl;
		ICMPheader sendHdr;

		SOCKET sock;
		sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);	//Пустой сокет, спользующий ICMP

		SOCKADDR_IN dest;	//Адрес назначения для отправки ICMP-запроса
		dest.sin_addr.S_un.S_addr = inet_addr(pszRemoteIP);
		dest.sin_family = AF_INET;
		dest.sin_port = rand();	//Выбираем случайный порт

		int nResult = 0;

		fd_set fdRead;
		SYSTEMTIME timeSend, timeRecv;
		int nTotalRoundTripTime = 0, nMaxRoundTripTime = 0, nMinRoundTripTime = -1, nRoundTripTime = 0;
		int nPacketsSent = 0, nPacketsReceived = 0;

		timeval timeInterval = { 0, 0 };
		timeInterval.tv_usec = nTimeOut * 1000;

		sendHdr.nId = htons(rand());	//Номер попытки

		while (nPacketsSent < nCount)
		{
			//Создадим буфер сообщений для хранения заголовка и данных
			pSendBuffer = new char[sizeof(ICMPheader) + nMessageSize];

			sendHdr.byCode = 0;	//Для echo и reply = 0
			sendHdr.nSequence = htons(nSequence++);
			sendHdr.byType = 8;	//Для echo = 8
			sendHdr.nChecksum = 0;	//Контрольная сумма

			memcpy_s(pSendBuffer, sizeof(ICMPheader), &sendHdr, sizeof(ICMPheader));	//Запишем заголовок в раннее созданный буфер
			memset(pSendBuffer + sizeof(ICMPheader), 'x', nMessageSize);	//Заполним сообщение некоторым произвольным значением

			//Посчитаем контрольную сумму
			sendHdr.nChecksum = htons(CalcChecksum(pSendBuffer, sizeof(ICMPheader) + nMessageSize));

			//Запишем обратно в буфер с посчитанной суммой
			memcpy_s(pSendBuffer, sizeof(ICMPheader), &sendHdr, sizeof(ICMPheader));

			nResult = sendto(sock, pSendBuffer, sizeof(ICMPheader) + nMessageSize, 0, (SOCKADDR*)&dest, sizeof(SOCKADDR_IN));

			//Запомнин время отправки пакета для дальнейшего подсчета времени
			::GetSystemTime(&timeSend);

			++nPacketsSent;



			FD_ZERO(&fdRead);
			FD_SET(sock, &fdRead);

			//Если ошибка
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
				//Создадим буфер для хранения ответа
				char* pRecvBuffer = new char[1500];
				//Если возникла ошибка 
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

				//Запишем время получения ответа
				::GetSystemTime(&timeRecv);

				//Мы получили ответ, поэтому мы строим из него заголовок и сообщение ICMP
				ICMPheader recvHdr;
				char* pICMPbuffer = NULL;

				//Ответ также включает заголовок IP, поэтому мы перемещаемся на 20 байт вперед, чтобы прочитать заголовок ICMP
				pICMPbuffer = pRecvBuffer + sizeof(IPheader);

				//Длина сообщения ICMP вычисляется путем вычитания размера заголовка IP из
				//общего размера сообщения
				int nICMPMsgLen = nResult - sizeof(IPheader);

				//Создадим заголовок ICMP
				memcpy_s(&recvHdr, sizeof(recvHdr), pICMPbuffer, sizeof(recvHdr));

				//Создадим заголовок IP из ответа
				IPheader ipHdr;
				memcpy_s(&ipHdr, sizeof(ipHdr), pRecvBuffer, sizeof(ipHdr));

				recvHdr.nId = recvHdr.nId;
				recvHdr.nSequence = recvHdr.nSequence;
				recvHdr.nChecksum = ntohs(recvHdr.nChecksum);

				//Проверяем, является ли ответ echo, совпадают ли номера операции
				//и контрольная сумма
				if (recvHdr.byType == 0 &&
					recvHdr.nId == sendHdr.nId &&
					recvHdr.nSequence == sendHdr.nSequence &&
					ValidateChecksum(pICMPbuffer, nICMPMsgLen) &&
					memcmp(pSendBuffer + sizeof(ICMPheader), pRecvBuffer + sizeof(ICMPheader) + sizeof(IPheader),
						nResult - sizeof(ICMPheader) - sizeof(IPheader)) == 0)
				{
					//Если все хорошо
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
					//Если получили хотя бы один ответ, прерываем перебор и считаем, что хост доступен
					pingeddevs.push_back(pszRemoteIP); break;
				}
				else
				{
					break;
				}
				//Чистим память
				delete[]pRecvBuffer;
			}

			//Чистим память
			delete[]pSendBuffer;
		}

		std::cout << '\r' << std::endl;
	}

	//Заполняем вектор векторов switch по правилу:
	//В 0 значение каждого подвектора записываем номер сети
	//Далее под нулевым элементом записываем адреса хостов
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
// Подсчет контрольной суммы
unsigned short CalcChecksum(char* pBuffer, int nLen)
{
	unsigned short nWord;
	unsigned int nSum = 0;
	int i;

	//Создадим 16-битные слова из каждых двух
	//соседних 8-битных слов в пакете и сложим
	for (i = 0; i < nLen; i = i + 2)
	{
		nWord = ((pBuffer[i] << 8) & 0xFF00) + (pBuffer[i + 1] & 0xFF);
		nSum = nSum + (unsigned int)nWord;
	}

	//Возьмите только 16 бит из 32-битной суммы и сложите
	while (nSum >> 16)
	{
		nSum = (nSum & 0xFFFF) + (nSum >> 16);
	}

	//Дополнение до единицы
	nSum = ~nSum;

	return ((unsigned short)nSum);
}

//Проверка контрольной суммы
bool ValidateChecksum(char* pBuffer, int nLen)
{
	unsigned short nWord;
	unsigned int nSum = 0;
	int i;

	//Создадим 16-битные слова из каждых двух
	//соседних 8-битных слов в пакете и сложим
	for (i = 0; i < nLen; i = i + 2)
	{
		nWord = ((pBuffer[i] << 8) & 0xFF00) + (pBuffer[i + 1] & 0xFF);
		nSum = nSum + (unsigned int)nWord;
	}

	//Возьмите только 16 бит из 32-битной суммы и сложите
	while (nSum >> 16)
	{
		nSum = (nSum & 0xFFFF) + (nSum >> 16);
	}
	return ((unsigned short)nSum == 0xFFFF);
}

bool Initialize()
{
	//Инициализируем WinSock
	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) == SOCKET_ERROR)
	{
		std::cerr << std::endl << "An error occured in WSAStartup operation: " << "WSAGetLastError () = " << WSAGetLastError() << std::endl;
		return false;
	}

	SYSTEMTIME time;
	::GetSystemTime(&time);

	//Заполним генератор случайных чисел текущим значением в миллисекундах
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

//Проверим, явлется ли допрустимым именем
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

//Переводит строку в 32-битное двоичное значение
std::string to_binary_string(std::string string1)
{

	int n = stoi(string1);
	std::string buffer; // символы ответа в обратном порядке
	// выделим память заранее по максимуму
	buffer.reserve(std::numeric_limits<unsigned int>::digits);
	do
	{
		buffer += char('0' + n % 2); // добавляем в конец
		n = n / 2;
	} while (n > 0);
	while (buffer.size() < 8) buffer += '0';
	return std::string(buffer.crbegin(), buffer.crend()); // разворачиваем результат
}

//Функция добавления к двоичному числу единицы
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

//Функция перевода двоичного числа в десятичное
std::string bin_to_dec(std::string binary)
{
	int result = 0;
	for (int i = binary.size() - 1; i >= 0; i--)
	{
		if (binary[i] == '1') result = result + pow(2, binary.size() - 1 - i);
	}
	return std::to_string(result);
}

//Перевод 32-битного двоичного числа в формат ip-адреса
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

//Первращает ip в 32-битное двоичное значение
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