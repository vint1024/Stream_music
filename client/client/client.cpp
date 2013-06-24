// client.cpp: определяет точку входа для приложения.
//
#include "stdafx.h"
#include "Resource.h"
#include <MMSystem.h>
#include <dsound.h>
#include <string>
#include <windows.h>
#include <winsock2.h>
#include <fstream>
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "dsound.lib")
#pragma comment(lib, "DXGUID.LIB")
#define MAX_LOADSTRING	100
#define START_ID		22
#define STOP_ID			23
#define EDIT_ID			99
#define EDIT2_ID		100
#define LIST_ID			1919
#define ADD_ID			1717
#define EXIT_ID			1818

WSADATA					wsaData;
int						SIZE_BUFER					= 65536;
bool					END_FILE					= false;
SOCKET					SendRecvSocket;													// сокет для приема и передачи
sockaddr_in				ServerAddr, ClientAddr;											// это будет адрес сервера и клиентов
int						err, maxlen					= SIZE_BUFER, 
						ClientAddrSize				= sizeof(ClientAddr);				// код ошибки, размер буферов и размер структуры адреса
char					*recvbuf					= new char[maxlen];					// буфер приема
char					*result_string				= new char[maxlen];
//char					TransFileName[256];
// .WAV file header
typedef struct sWaveHeader {
	char  RiffSig[4];         // 'RIFF'
	long  WaveformChunkSize;  // 8
	char  WaveSig[4];         // 'WAVE'
	char  FormatSig[4];       // 'fmt ' (notice space after)
	long  FormatChunkSize;    // 16
	short FormatTag;          // WAVE_FORMAT_PCM
	short Channels;           // # of channels
	long  SampleRate;         // sampling rate
	long  BytesPerSec;        // bytes per second
	short BlockAlign;         // sample block alignment
	short BitsPerSample;      // bits per second
	char  DataSig[4];         // 'data'
	long  DataSize;           // size of waveform data
} sWaveHeader;
void copybuf(char *out, char *in, int size)
{
	for (int i = 0; i<size; ++i)
		out[i]=in[i];
}
IDirectSound8		*g_pDS;								// DirectSound component
IDirectSoundBuffer8 *g_pDSBuffer;						// Sound Buffer object
IDirectSoundNotify8 *g_pDSNotify;						// Notification object
HANDLE				 g_hThread;							// Thread handle
DWORD				 g_ThreadID;						// Thread ID #
HANDLE				 g_hEvents[4];						// Notification handles
//long				 g_Size, g_Pos, g_Left;				// Streaming pointers
sWaveHeader			 DataFile;
bool				 FPlay = false;
HINSTANCE			 hInst;								// текущий экземпляр
TCHAR				 szTitle[MAX_LOADSTRING];			// Текст строки заголовка
TCHAR				 szWindowClass[MAX_LOADSTRING];		// имя класса главного окна

// Отправить объявления функций, включенных в этот модуль кода:
ATOM				MyRegisterClass			(HINSTANCE hInstance);
BOOL				InitInstance			(HINSTANCE, int);
LRESULT CALLBACK	WndProc					(HWND, UINT, WPARAM, LPARAM);
void				PlayStreamedSound		(sWaveHeader *Hdr);
IDirectSoundBuffer8 *CreateBufferFromWAV	(sWaveHeader *Hdr);
BOOL				LoadSoundData			(IDirectSoundBuffer8 *pDSBuffer, long LockPos, long Size);
DWORD				HandleNotifications		(LPVOID lpvoid);

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	MSG msg;
	HACCEL hAccelTable;
	// Инициализация глобальных строк
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_CLIENT, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);
	// Выполнить инициализацию приложения:
	if (!InitInstance (hInstance, nCmdShow)) { return FALSE; }
	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CLIENT));
	// Цикл основного сообщения:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return (int) msg.wParam;
}
//  ФУНКЦИЯ: MyRegisterClass()
//  НАЗНАЧЕНИЕ: регистрирует класс окна.
//  КОММЕНТАРИИ:
//    Эта функция и ее использование необходимы только в случае, если нужно, чтобы данный код
//    был совместим с системами Win32, не имеющими функции RegisterClassEx'
//    которая была добавлена в Windows 95. Вызов этой функции важен для того,
//    чтобы приложение получило "качественные" мелкие значки и установило связь
//    с ними.
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;
	wcex.cbSize			= sizeof(WNDCLASSEX);
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= NULL;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= NULL;
	return RegisterClassEx(&wcex);
}
//   ФУНКЦИЯ: InitInstance(HINSTANCE, int)
//   НАЗНАЧЕНИЕ: сохраняет обработку экземпляра и создает главное окно.
//   КОММЕНТАРИИ:
//        В данной функции дескриптор экземпляра сохраняется в глобальной переменной, а также
//        создается и выводится на экран главное окно программы.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;
   hInst = hInstance; // Сохранить дескриптор экземпляра в глобальной переменной
   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW && !WS_MINIMIZEBOX, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);
   if (!hWnd)
   {
      return FALSE;
   }
   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);
   return TRUE;
}
//  ФУНКЦИЯ: WndProc(HWND, UINT, WPARAM, LPARAM)
//  НАЗНАЧЕНИЕ:  обрабатывает сообщения в главном окне.
//  WM_COMMAND	- обработка меню приложения
//  WM_PAINT	-Закрасить главное окно
//  WM_DESTROY	 - ввести сообщение о выходе и вернуться.
//
//
HWND startButton, hEdit, hList, addButton, exitButton, stopButton, hEdit2;
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	switch (message)
	{
	case WM_CREATE:
		{
			hdc = BeginPaint(hWnd, &ps);
			startButton = CreateWindowEx(0,TEXT("button"),TEXT("Connect"),WS_VISIBLE|WS_CHILD,700, 20, 150, 30, hWnd, (HMENU)START_ID, hInst, 0);
			stopButton	= CreateWindowEx(0,TEXT("button"),TEXT("Stop"),WS_VISIBLE|WS_CHILD,700, 60, 150, 30, hWnd, (HMENU)STOP_ID, hInst, 0);
			exitButton	= CreateWindowEx(0,TEXT("button"),TEXT("Exit"),WS_VISIBLE|WS_CHILD,700, 350, 150, 30, hWnd, (HMENU)EXIT_ID, hInst, 0);
			hEdit		= CreateWindowEx(0,TEXT("edit"),TEXT("127.0.0.1"), WS_CHILD | WS_VISIBLE| ES_LEFT | ES_MULTILINE,60, 10, 400, 20, hWnd, (HMENU)EDIT_ID, hInst, 0);
			hEdit2		= CreateWindowEx(0,TEXT("edit"),TEXT("12345"), WS_CHILD | WS_VISIBLE| ES_LEFT | ES_MULTILINE,60, 40, 400, 20, hWnd, (HMENU)EDIT2_ID, hInst, 0);
			EndPaint(hWnd, &ps);
			break;
		}
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Разобрать выбор:
		switch (wmId)
		{
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		case EXIT_ID:
			DestroyWindow(hWnd);
			break;
		case START_ID:
		{
			WSAStartup(MAKEWORD(2,2), &wsaData);							// Initialize Winsock
			SendRecvSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);		// Create a SOCKET for connecting to server
																			// Setup the TCP listening socket
			ServerAddr.sin_family=AF_INET;									//семейство адресов 
			char S[255];
			SendMessage(hEdit, WM_GETTEXT, 255, (LPARAM)S);
			ServerAddr.sin_addr.s_addr = inet_addr( S );
			SendMessage(hEdit2, WM_GETTEXT, 255, (LPARAM)S);
			int tmp = atoi(S);
			ServerAddr.sin_port=htons(tmp);
			DWORD val = 20;
			setsockopt (SendRecvSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&val, sizeof DWORD);		//без этого вызова висим вечно
			sendto(SendRecvSocket,"1", strlen("1"), 0, (sockaddr *)&ServerAddr,sizeof(ServerAddr));		// отправляем запрос на сервер
			err = recvfrom(SendRecvSocket,recvbuf,maxlen,0,0,0);										// получаем результат
			if (err == SOCKET_ERROR) 
			{
				char strerr[256];
				int tmp = WSAGetLastError();
				sprintf(strerr,"%d",tmp);
				MessageBox(hWnd,(LPCSTR)strerr,"ERROR number: ",  MB_ICONERROR);
				closesocket(SendRecvSocket);
				WSACleanup();
				break;
			}
			copybuf(reinterpret_cast<char*>(&DataFile), recvbuf, sizeof(sWaveHeader));
			FPlay = true;
			InvalidateRect(hWnd, NULL, TRUE);
			if(FAILED(DirectSoundCreate8(NULL, &g_pDS, NULL)))											// Initialize and configure DirectSound
			{
				MessageBox(NULL, "Unable to create DirectSound object", "Error", MB_OK);
				return 0;
			}
			g_pDS->SetCooperativeLevel(hWnd, DSSCL_NORMAL);
			PlayStreamedSound(&DataFile);
			break;
		}
		case STOP_ID:
		{
			if (FPlay)
			{
				FPlay = false;
				// Stop sound
				g_pDSBuffer->Stop();
				// Kill the thread
				if(g_hThread != NULL) 
				{
					TerminateThread(g_hThread, 0);
					CloseHandle(g_hThread);
				}
				// Release the handles
				for(int i=0;i<4;i++)
					CloseHandle(g_hEvents[i]);
				// Release DirectSound objects
				g_pDSBuffer->Release();
				g_pDS->Release();
				closesocket(SendRecvSocket);
				WSACleanup();
			}
			break;
		}
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
	{
		hdc = BeginPaint(hWnd, &ps);
		if (FPlay)
		{
			std::string tmp_s = "Wave form chunk size = ";
			char tmp_char_srt[255];
			sprintf(tmp_char_srt, "%d",DataFile.WaveformChunkSize);
			tmp_s += tmp_char_srt;
			TextOut(hdc, 15, 200, tmp_s.c_str(), strlen(tmp_s.c_str()));
			tmp_s = "Format chunk size = ";
			sprintf(tmp_char_srt, "%d",DataFile.FormatChunkSize);
			tmp_s += tmp_char_srt;
			TextOut(hdc, 15, 220, tmp_s.c_str(), strlen(tmp_s.c_str()));
			tmp_s = "Channels = ";
			sprintf(tmp_char_srt, "%d",DataFile.Channels);
			tmp_s += tmp_char_srt;
			TextOut(hdc, 15, 240, tmp_s.c_str(), strlen(tmp_s.c_str()));
			tmp_s = "Bytes Per Sec = ";
			sprintf(tmp_char_srt, "%d",DataFile.BytesPerSec);
			tmp_s += tmp_char_srt;
			TextOut(hdc, 15, 260, tmp_s.c_str(), strlen(tmp_s.c_str()));
			tmp_s = "Data Size = ";
			sprintf(tmp_char_srt, "%d",DataFile.DataSize);
			tmp_s += tmp_char_srt;
			TextOut(hdc, 15, 280, tmp_s.c_str(), strlen(tmp_s.c_str()));
			tmp_s = "Wave Sig = ";
			tmp_s += DataFile.WaveSig[0];
			tmp_s += DataFile.WaveSig[1];
			tmp_s += DataFile.WaveSig[2];
			tmp_s += DataFile.WaveSig[3];
			TextOut(hdc, 15, 180, tmp_s.c_str(), strlen(tmp_s.c_str()));
		}
		EndPaint(hWnd, &ps);
		break;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
IDirectSoundBuffer8 *CreateBufferFromWAV(sWaveHeader *Hdr)
{
	IDirectSoundBuffer *pDSB;
	IDirectSoundBuffer8 *pDSBuffer;
	DSBUFFERDESC dsbd;
	WAVEFORMATEX wfex;
	// check the sig fields, returning if an error
	if(	memcmp(Hdr->RiffSig,	"RIFF", 4) || memcmp(Hdr->WaveSig,	"WAVE", 4) || memcmp(Hdr->FormatSig,	"fmt ", 4) || memcmp(Hdr->DataSig,	"data", 4))
		return NULL;
	ZeroMemory(&wfex, sizeof(WAVEFORMATEX));													// setup the playback format
	wfex.wFormatTag = WAVE_FORMAT_PCM;
	wfex.nChannels = Hdr->Channels;
	wfex.nSamplesPerSec = Hdr->SampleRate;
	wfex.wBitsPerSample = Hdr->BitsPerSample;
	wfex.nBlockAlign = wfex.wBitsPerSample / 8 * wfex.nChannels;
	wfex.nAvgBytesPerSec = wfex.nSamplesPerSec * wfex.nBlockAlign;
	ZeroMemory(&dsbd, sizeof(DSBUFFERDESC));													// create the sound buffer using the header data
	dsbd.dwSize = sizeof(DSBUFFERDESC);
	dsbd.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_LOCSOFTWARE |  DSBCAPS_GLOBALFOCUS;
	dsbd.dwBufferBytes =SIZE_BUFER;
	dsbd.lpwfxFormat = &wfex;
	if(FAILED(g_pDS->CreateSoundBuffer(&dsbd, &pDSB, NULL)))
		return NULL;
	if(FAILED(pDSB->QueryInterface(IID_IDirectSoundBuffer8, (void**)&pDSBuffer)))				// get newer interface
	{
			pDSB->Release();
			return NULL;
	}
	return pDSBuffer;																			// return the interface
}
BOOL LoadSoundData(IDirectSoundBuffer8 *pDSBuffer, long LockPos, long Size)
{
	BYTE *Ptr1, *Ptr2;
	DWORD Size1, Size2;
	if(!Size)
		return FALSE;
	if(FAILED(pDSBuffer->Lock(LockPos, Size, (void**)&Ptr1, &Size1, (void**)&Ptr2, &Size2, 0)))	// lock the sound buffer at position specified
		return FALSE;
	// read in the data
	char tmp[256];
	sprintf(tmp, "%d", Size1);
	sendto(SendRecvSocket,tmp, strlen(tmp), 0, (sockaddr *)&ServerAddr,sizeof(ServerAddr));		// отправляем запрос на сервер
	err = recvfrom(SendRecvSocket,(char*)Ptr1,Size1,0,0,0);										// получаем результат
	if (err != SOCKET_ERROR && err !=Size1)
		END_FILE = true;
	if(Ptr2 != NULL)
	{
		
		sprintf(tmp, "%d", Size2);
		sendto(SendRecvSocket,tmp, strlen(tmp), 0, (sockaddr *)&ServerAddr,sizeof(ServerAddr));	// отправляем запрос на сервер
		err = recvfrom(SendRecvSocket,(char*)Ptr2,Size2,0,0,0);									// получаем результат
		if (err != SOCKET_ERROR && err !=Size2)
			END_FILE = true;
	}
	pDSBuffer->Unlock(Ptr1, Size1, Ptr2, Size2);												// unlock it
	return TRUE;																				// return a success
}

void PlayStreamedSound(sWaveHeader *Hdr)
{
	DSBPOSITIONNOTIFY   dspn[4];
	long i;
	if((g_pDSBuffer = CreateBufferFromWAV(Hdr)) == NULL)										// Create a 2 second buffer to stream in wave
	{
		return;
	}
	// Get streaming size and pointers
	//g_Size = Hdr->DataSize;
	//g_Pos = sizeof(sWaveHeader);
	//g_Left = g_Size;
	// Create a thread for notifications
	if((g_hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)HandleNotifications, NULL, 0, &g_ThreadID)) == NULL)
		return;
	// Create the notification interface
	if(FAILED(g_pDSBuffer->QueryInterface(IID_IDirectSoundNotify8, (void**)&g_pDSNotify)))
		return;
	// Create the event handles and set the notifications
	for(i=0;i<4;i++) 
	{
		g_hEvents[i] = CreateEvent(NULL,FALSE,FALSE,NULL);
		dspn[i].dwOffset = SIZE_BUFER/4 * (i+1) - 1; //место в кот происход уведомление
		dspn[i].hEventNotify = g_hEvents[i];
	}
	g_pDSNotify->SetNotificationPositions(4, dspn);
	// Fill buffer completely with sound
	LoadSoundData(g_pDSBuffer, 0,SIZE_BUFER);
	//g_Left -=SIZE_BUFER;
	//g_Pos  +=SIZE_BUFER;
	// Play sound looping
	g_pDSBuffer->SetCurrentPosition(0);
	g_pDSBuffer->SetVolume(DSBVOLUME_MAX);
	g_pDSBuffer->Play(0,0,DSBPLAY_LOOPING);
	return;
}

DWORD HandleNotifications(LPVOID lpvoid)
{
	DWORD dwResult, Num;

	while(1) 
	{
		// Wait for a message
		dwResult = MsgWaitForMultipleObjects(4, g_hEvents,FALSE, INFINITE, QS_ALLEVENTS);
		// Get notification #
		Num = dwResult - WAIT_OBJECT_0;
		// Check for update #
		if(Num >=0 && Num < 4) 
		{
			LoadSoundData(g_pDSBuffer, Num*(SIZE_BUFER/4), (SIZE_BUFER/4));
			if(END_FILE)
			{
				END_FILE = false;
				g_pDSBuffer->Stop();
				ExitThread(0);
			}
		}
	}
	return 0L;
}