#include <iostream>
#include <thread>
#include <sstream>
#include <ctime>
#include <vector>

#include <Windows.h>

// nScreenWidth不能低于50，否则控制台无法拉成完整的矩形
int nScreenWidth = 120;
int nScreenHeight = 30;

 //游戏区域
int nFieldWidth = 12;
int nFieldHeight = 18;
unsigned char* pField = nullptr;

//方块形状
std::wstring tetromino[7];

//初始化游戏区域边框
void InitPlayField()
{
	pField = new unsigned char[nFieldWidth * nFieldHeight];
	for (int i = 0; i < nFieldHeight; i++)
		for (int j = 0; j < nFieldWidth; j++)
			pField[i * nFieldWidth + j] = (i == 0 || i == nFieldHeight - 1 || j == 0 || j == nFieldWidth - 1) ? 9 : 0;
};

//获取一个旋转后的点之前的下标(这地方理解起来有点绕)
int Rotate(int nRotation, int nX, int nY)
{
	int nIndex = 0;

	switch (nRotation %= 4)
	{
	// 0度
    case 0:								// 0  1  2  3
		nIndex = nY * 4 + nX;			// 4  5  6  7
		break;							// 8  9 10 11
										//12 13 14 15

	case 1:								//12  8  4  0
		nIndex = 12 + nY - nX * 4;		//13  9  5  1
		break;							//14 10  6  2
										//15 11  7  3

	case 2:								//15 14 13 12
		nIndex = 15 - 4 * nY - nX;		//11 10  9  8
		break;							// 7  6  5  4
										// 3  2  1  0

	case 3:								// 3  7 11 15
		nIndex = 3 - nY + 4 * nX;		// 2  6 10 14
		break;							// 1  5  9 13
        								// 0  4  8 12
	}

	return nIndex;
}

//判断一个方块是否可以放置到指定位置
bool DoesPieceFit(const int& pieceIndex, const int& rotation, const int& posX, const int& posY)
{
	for(int i = 0;i < 4;i++)
		for (int j = 0; j < 4; j++)
		{
			//获取旋转后点之前的下标（为了获取该点对应的字符）
			int index = Rotate(rotation, j, i);
			//获取点在screen里面的下标
            int fieldIndex = (posY + i) * nFieldWidth + (posX + j);

			if (posX + j >= 0 && posX + j < nFieldWidth)
			{
				if (posY + i >= 0 && posY + i < nFieldHeight)
				{
					//如果该点对应的字符不是'.'且该点在游戏区域边框上
					if (tetromino[pieceIndex][index] != L'.' && pField[fieldIndex] != 0)
					{
                        return false;
					}
				}
			}
		}
	return true;
}


int main()
{
	//创建屏幕字符缓冲区
	wchar_t *screen = new wchar_t[nScreenWidth * nScreenHeight];
	DWORD dwBytesWritten = 0;
	for (int i = 0; i < nScreenWidth * nScreenHeight; i++) screen[i] = L' ';
	
	//创建控制台屏幕缓冲区
	HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	//设置该缓冲区为活动缓冲区
	if (!SetConsoleActiveScreenBuffer(hConsole))
	{
		const wchar_t* errorString = L"设置活动缓冲区失败!";
		WriteConsoleOutputCharacter(hConsole, errorString, 10, {0, 0}, &dwBytesWritten);
		return 0;
	}

	//相关变量初始化
	bool bKey[4] = {0};						//上下左右按键是否被按下
	int nCurrentPiece = 6;					//当前方块形状
	int nCurrentRotation = 0;				//当前旋转角度(0: 0度, 1: 90度, 2: 180度, 3: 270度)
	int nCurrentX = nFieldWidth / 2 - 2;	//当前块x坐标
	int nCurrentY = 1;						//当前块y坐标
	bool bRotateHold = false;				//是否需要旋转
	bool bForceDown = false;				//是否需要强制下落
	int nLoopCount = 0;						//循环次数
	int	nForceDownTotal = 20;				//循环多少次执行一次下落
	std::vector<int> vLines;				//需要消除的行
	int nScore = 0;							//得分
	bool bGameOver = false;					//游戏是否结束

	//设置随机数种子
	std::srand(static_cast<unsigned int>(std::time(0)));

	//初始化方块形状
	tetromino[0].append(L"..X...X...X...X.");
	tetromino[1].append(L"..X..XX...X.....");
	tetromino[2].append(L".XX..XX.........");
	tetromino[3].append(L"..X..XX..X......");
	tetromino[4].append(L".X...XX...X.....");
	tetromino[5].append(L".X...X...XX.....");
	tetromino[6].append(L"..X...X..XX.....");

	//初始化游戏边框
	InitPlayField();

	while (!bGameOver)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(50));

		//是否下落
		nLoopCount++;
		bForceDown = nLoopCount == nForceDownTotal;

		// 虚拟键码分别对应“上下左右”
		for (int i = 0; i < 4; i++)
			bKey[i] = (0x8000 & GetAsyncKeyState((unsigned char)("\x26\x28\x25\x27"[i]))) != 0;

		if (bKey[1] && DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX, nCurrentY+1)) nCurrentY++;
		if (bKey[2] && DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX - 1, nCurrentY)) nCurrentX--;
		if (bKey[3] && DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX + 1, nCurrentY)) nCurrentX++;

		//直到松掉旋转键后，才可以再次按下旋转键
		if (bKey[0] && DoesPieceFit(nCurrentPiece, nCurrentRotation+1, nCurrentX, nCurrentY))
		{
			if(bRotateHold) nCurrentRotation++;
			bRotateHold = false;
		}
		else bRotateHold = true;

		if (bForceDown)
		{
			nLoopCount = 0;

			//如果方块可以下落则下落，不能则说明到底部了，将当前方块放置到游戏区域
			if(DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX, nCurrentY+1)) nCurrentY++;
			else
			{
				for(int i = 0;i < 4;i++)
					for (int j = 0; j < 4; j++)
						if(tetromino[nCurrentPiece][Rotate(nCurrentRotation, j, i)] != L'.')
							pField[(nCurrentY + i)*nFieldWidth + nCurrentX + j] = nCurrentPiece+1;

				//如果该行没有空格，则消除该行
				for (int i = 0; i < 4; i++)
				{
					if (nCurrentY + i >= nFieldHeight - 1) continue;

					bool bLineFilled = true;
					for (int j = 1; j < nFieldWidth - 1; j++)
					{
						if (pField[(i + nCurrentY) * nFieldWidth + j] == 0)
						{
							bLineFilled = false;
							break;
						}
					}

					if (bLineFilled)
						vLines.push_back(i + nCurrentY);
				}

				//生成新的方块
				nCurrentPiece = std::rand() % 7;
				nCurrentX = nFieldWidth / 2 - 2;
				nCurrentY = 1;

				//计分
				nScore += 25;

				//如果新方块放置失败，则游戏结束
				bGameOver = !DoesPieceFit(nCurrentPiece, nCurrentRotation, nCurrentX, nCurrentY);
			}
		}

		if (!vLines.empty())
		{
			// 先显示为消除之前的画面，再显示消除之后的画面
			WriteConsoleOutputCharacter(hConsole, screen, nScreenWidth * nScreenHeight, { 0,0 }, &dwBytesWritten);
			std::this_thread::sleep_for(std::chrono::milliseconds(200));

			//在pField中消除行
			for (auto line : vLines)
			{
				for (int i = line; i > 0; i--)
				{
					if (i == 1)
					{
						for (int j = 1; j < nFieldWidth - 1; j++)
							pField[nFieldWidth + j] = 0;
						break;
					}

					for (int j = 1; j < nFieldWidth - 1; j++)
						pField[i * nFieldWidth + j] = pField[(i - 1) * nFieldWidth + j];
				}
			}
		}

		//清空要删除的行
		vLines.clear();

		//绘制游戏边缘区域（Window控制台换行是基于控制台的宽度，所以必须要鼠标把控制台宽度拖动到合适大小才能正常显示）
		for (int i = 0; i < nFieldHeight; i++)
			for (int j = 0; j < nFieldWidth; j++)
				screen[(i + 2) * nScreenWidth + (j + 2)] = L" ABCDEFG=#"[pField[i * nFieldWidth + j]];

		//绘制当前方块(i为行，j为列)
		for (int i = 0; i < 4; i++)
			for (int j = 0; j < 4; j++)
				if (tetromino[nCurrentPiece][Rotate(nCurrentRotation, j, i)] != L'.')
					screen[(nCurrentY + 2 + i) * nScreenWidth + (nCurrentX + j + 2)] = 65 + nCurrentPiece;

		// Draw Score
		swprintf_s(&screen[2 * nScreenWidth + nFieldWidth + 6], 16, L"SCORE: %8d", nScore);

		// Display Frame
		WriteConsoleOutputCharacter(hConsole, screen, nScreenWidth * nScreenHeight, { 0,0 }, &dwBytesWritten);
	}

	CloseHandle(hConsole);
	std::cout << "Game Over!! Score:" << nScore << std::endl;
	system("pause");
	return 0;
}