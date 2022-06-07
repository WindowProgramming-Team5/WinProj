#pragma comment (lib,"msimg32.lib")

#include <Windows.h>
#include <tchar.h>
#include <random>

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800

#define MAX_MONSTER 100

#define PLAYER_WIDTH 80
#define PLAYER_HEIGHT 80

#define MAX_ITEM 100 

#define MAX_BULLET	100 

// #define JumpPower 65.f

using std::default_random_engine;
using std::random_device;
using std::uniform_int_distribution;

enum class Dir { LEFT = 0, RIGHT, UP, DOWN };
enum class ItemType { WOOD = 0, SPRING, TRAP };
enum class Stage { ONE = 0, TWO, THREE, ENDING };
enum class PlayMode { P1 = 0, P2 };

struct Bullet {
	RECT rect;
	int aIndex;
};

struct Player {
	RECT rect;
	Dir dir;
	Bullet bullets[MAX_BULLET];
	TCHAR lifeArr[5];
	int aIndex; // Animation Index
	int bIndex; // Bullet Index
	int score;
	int life;
	bool isJump;
	float gravity = 1.f;
	float dropSpeed = 0.f;
};

struct Monster {
	RECT rect;
	int aIndex;	// Animation Index
};

struct Item {
	RECT rect;
	ItemType itemType;
};

default_random_engine dre{ random_device{}() };
uniform_int_distribution<int> uiRandomItem{ 0,2 };

Item items[MAX_ITEM];

Player mario;
Player luigi;

RECT gateSpot;	// EndLineBox

RECT temp;	// Just temp
RECT marioTemp, luigiTemp;	// 점프키 눌렀을 때 위치 저장 ( RECT 값 갱신에 사용 ) 

Stage stage;

int itemTimeCount;

int itemIndex;
int randomItem;

int itemSequence;
int mX, mY;

float mJumpPower = 50.f;
float mJumpTime = 0.f;
float mJumpHeight;

float lJumpPower = 50.f;
float lJumpTime = 0.f;
float lJumpHeight;

bool setItem = false;

bool isFinish = false;

bool flag = false;

TCHAR time[] = L"TIME:";
TCHAR start[] = L"START!";
TCHAR marioLife[] = L"Mario:";
TCHAR luigiLife[] = L"Luigi:";

TCHAR tCount[5];
BITMAP Mariobmp, Luigibmp;
HINSTANCE g_hInst;
LPCTSTR lpszClass = L"Window class Name";
LPCTSTR lpszWindowName = L"Ultimate Mario Luigi";

// BOOL CheckBlock(RECT* r); 
BOOL isCollide(const RECT*);

void Gravity();

void PreviewItem(HDC, HDC, const HBITMAP, HBITMAP, const HBITMAP, HBITMAP, const HBITMAP, HBITMAP, const ItemType);

void DrawBackGround(HDC, HDC, const HBITMAP, HBITMAP, const Stage);

void DrawMario(HDC, HDC, const HBITMAP, HBITMAP);
void DrawLuigi(HDC, HDC, const HBITMAP, HBITMAP);
void DrawGate(HDC, HDC, const HBITMAP, HBITMAP, const Stage);
void DrawItem(HDC, HDC, const HBITMAP, HBITMAP, const HBITMAP, HBITMAP, const HBITMAP, HBITMAP);
void DrawEndingScene(HDC, HDC, const HBITMAP, HBITMAP);

void ShowText(HDC);

// Reset
void Reset(const HWND);

void ShowAllFrameRect(HDC, const RECT*, const RECT*, const RECT*, const RECT*);

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);
void CALLBACK TimerProc(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
	HWND hWnd;
	MSG Message;
	WNDCLASSEX WndClass;
	g_hInst = hInstance;

	WndClass.cbSize = sizeof(WndClass);
	WndClass.style = CS_HREDRAW | CS_VREDRAW;
	WndClass.lpfnWndProc = (WNDPROC)WndProc;
	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hInstance = hInstance;
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	WndClass.lpszMenuName = NULL;
	WndClass.lpszClassName = lpszClass;
	//WndClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	RegisterClassEx(&WndClass);

	hWnd = CreateWindow(lpszClass, lpszWindowName, WS_OVERLAPPEDWINDOW, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, (HMENU)NULL, hInstance, NULL);
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	while (GetMessage(&Message, 0, 0, 0)) {
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}
	return Message.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hDC, backMemDC, memDC;

	static HBITMAP backBitmap, oldBackBitmap;
	static HBITMAP backGround1, oldBackGround1;
	static HBITMAP backGround2, oldBackGround2;
	static HBITMAP backGround3, oldBackGround3;
	static HBITMAP hMario, oldMario;
	static HBITMAP hLuigi, oldLuigi;
	static HBITMAP gate, oldGate;
	static HBITMAP hWood, oldWood;
	static HBITMAP hSpring, oldSpring;
	static HBITMAP hTrap, oldTrap;
	static HBITMAP hEndingScene, oldEndingScene;

	static int cX, cY;

	static bool isInBlock;
	static int randomIndex;

	switch (uMsg)
	{
	case WM_CREATE:
		stage = Stage::ONE;

		RECT c;
		GetClientRect(hWnd, &c);

		mario.life = 3;
		luigi.life = 3;

		hMario = (HBITMAP)LoadImage(g_hInst, TEXT("Mario Overworld4.bmp"), IMAGE_BITMAP, 0, 0,
			LR_LOADFROMFILE | LR_CREATEDIBSECTION);
		hLuigi = (HBITMAP)LoadImage(g_hInst, TEXT("Luigi Overworld2.bmp"), IMAGE_BITMAP, 0, 0,
			LR_LOADFROMFILE | LR_CREATEDIBSECTION);
		gate = (HBITMAP)LoadImage(g_hInst, TEXT("Endless Challenge.bmp"), IMAGE_BITMAP, 0, 0,
			LR_LOADFROMFILE | LR_CREATEDIBSECTION);
		backGround1 = (HBITMAP)LoadImage(g_hInst, TEXT("Backgrounds SMB3.bmp"), IMAGE_BITMAP, 0, 0,
			LR_LOADFROMFILE | LR_CREATEDIBSECTION);
		backGround2 = static_cast<HBITMAP>(LoadImage(g_hInst, TEXT("Backgrounds SMW.bmp"), IMAGE_BITMAP, 0, 0,
			LR_LOADFROMFILE | LR_CREATEDIBSECTION));
		hWood = static_cast<HBITMAP>((LoadImage(g_hInst, TEXT("WoodBoard.bmp"), IMAGE_BITMAP, 0, 0,
			LR_LOADFROMFILE | LR_CREATEDIBSECTION)));
		hSpring = static_cast<HBITMAP>((LoadImage(g_hInst, TEXT("Spring.bmp"), IMAGE_BITMAP, 0, 0,
			LR_LOADFROMFILE | LR_CREATEDIBSECTION)));
		hTrap = static_cast<HBITMAP>((LoadImage(g_hInst, TEXT("Obstacle.bmp"), IMAGE_BITMAP, 0, 0,
			LR_LOADFROMFILE | LR_CREATEDIBSECTION)));
		hEndingScene = static_cast<HBITMAP>(LoadImage(g_hInst, TEXT("Opening Sequences.bmp"), IMAGE_BITMAP, 0, 0,
			LR_LOADFROMFILE | LR_CREATEDIBSECTION));

		GetObject(hMario, sizeof(BITMAP), &Mariobmp);
		GetObject(hLuigi, sizeof(BITMAP), &Luigibmp);

		gateSpot.left = 1050;
		gateSpot.top = 0;
		gateSpot.right = 1180;
		gateSpot.bottom = 130;

		/* Mario start position */
		mario.rect.left = 0;
		mario.rect.top = 680;
		mario.rect.right = mario.rect.left + PLAYER_WIDTH;
		mario.rect.bottom = mario.rect.top + PLAYER_HEIGHT;

		/* Luigi start position */
		luigi.rect.left = 20;
		luigi.rect.top = 680;
		luigi.rect.right = luigi.rect.left + PLAYER_WIDTH;
		luigi.rect.bottom = luigi.rect.top + PLAYER_HEIGHT;

		items[itemIndex].itemType = ItemType::WOOD;

		itemTimeCount = 6;

		/* Monster Spawn */
		SetTimer(hWnd, 1000, 5000, NULL);
		SetTimer(hWnd, 1, 1000, NULL);	// 아이템 시간

		SetTimer(hWnd, 8, 16, (TIMERPROC)TimerProc);
		break;
	case WM_MOUSEMOVE:
		mX = LOWORD(lParam);
		mY = HIWORD(lParam);
		InvalidateRect(hWnd, NULL, FALSE);
		break;
	case WM_LBUTTONUP:
	{
		if (itemTimeCount > 0) {
			int tempX = mX = LOWORD(lParam);
			int tempY = mY = HIWORD(lParam);

			switch (items[itemIndex].itemType)
			{
			case ItemType::WOOD:
				items[itemIndex].rect.left = tempX - 100;
				items[itemIndex].rect.top = tempY - 25;
				items[itemIndex].rect.right = tempX + 100;
				items[itemIndex].rect.bottom = tempY + 25;
				break;
			case ItemType::SPRING:
				items[itemIndex].rect.left = tempX - 25;
				items[itemIndex].rect.top = tempY - 25;
				items[itemIndex].rect.right = tempX + 25;
				items[itemIndex].rect.bottom = tempY + 25;
				break;
			case ItemType::TRAP:
				items[itemIndex].rect.left = tempX - 30;
				items[itemIndex].rect.top = tempY - 30;
				items[itemIndex].rect.right = tempX + 30;
				items[itemIndex].rect.bottom = tempY + 30;
				break;
			default:
				break;
			}
			itemIndex++;
			items[itemIndex].itemType = items[itemIndex - 1].itemType;
			InvalidateRect(hWnd, NULL, FALSE);
		}
		break;
	}
	case WM_KEYDOWN:
		switch (tolower(wParam))
		{
		case VK_UP:
			if (ItemType::WOOD == items[itemIndex].itemType)
				items[itemIndex].itemType = ItemType::TRAP;
			else if (ItemType::TRAP == items[itemIndex].itemType)
				items[itemIndex].itemType = ItemType::SPRING;
			else
				items[itemIndex].itemType = ItemType::WOOD;
			break;
		case VK_DOWN:
			if (ItemType::WOOD == items[itemIndex].itemType)
				items[itemIndex].itemType = ItemType::SPRING;
			else if (ItemType::SPRING == items[itemIndex].itemType)
				items[itemIndex].itemType = ItemType::TRAP;
			else
				items[itemIndex].itemType = ItemType::WOOD;
			break;
		case VK_SPACE:
			if (!mario.isJump) {
				mario.rect.top -= 3;
				mario.rect.bottom -= 3;
				mario.isJump = TRUE;
				marioTemp = mario.rect;
				SetTimer(hWnd, 3000, 1, NULL);
			}
			break;
		case VK_RETURN:
			if (!luigi.isJump) {
				luigi.rect.top -= 3;
				luigi.rect.bottom -= 3;
				luigi.isJump = TRUE;
				luigiTemp = luigi.rect;
				SetTimer(hWnd, 2000, 1, NULL);
			}
			break;
			// Mario Attack
		case VK_SHIFT:

			switch (mario.dir)
			{
			case Dir::RIGHT:
				break;
			default:
				break;
			}
			break;

			// Luigi Attack 
		case VK_BACK:

			switch (luigi.dir)
			{
			case Dir::LEFT:
				break;
			default:
				break;
			}
			break;
		case 'r':
			Reset(hWnd);
			break;
		case 'f':
			flag = !flag;
			break;
		case 'g':
			if (IntersectRect(&temp, &mario.rect, &gateSpot)) {
				if (Stage::ONE == stage)
					stage = Stage::TWO;
				else if (Stage::TWO == stage)
					stage = Stage::THREE;
				else if (Stage::THREE == stage)
					stage = Stage::ENDING;
				else
					stage = Stage::ONE;
				mario.score++;

			}
			else if (IntersectRect(&temp, &luigi.rect, &gateSpot)) {
				if (Stage::ONE == stage)
					stage = Stage::TWO;
				else if (Stage::TWO == stage)
					stage = Stage::THREE;
				else if (Stage::THREE == stage)
					stage = Stage::ENDING;
				else
					stage = Stage::ONE;
				luigi.score++;
			}

			if (IntersectRect(&temp, &mario.rect, &gateSpot) || IntersectRect(&temp, &luigi.rect, &gateSpot)) {
				/* Mario start position */
				mario.rect.left = 0;
				mario.rect.top = 680;
				mario.rect.right = mario.rect.left + PLAYER_WIDTH;
				mario.rect.bottom = mario.rect.top + PLAYER_HEIGHT;

				/* Luigi start position */
				luigi.rect.left = 20;
				luigi.rect.top = 680;
				luigi.rect.right = luigi.rect.left + PLAYER_WIDTH;
				luigi.rect.bottom = luigi.rect.top + PLAYER_HEIGHT;

				itemTimeCount = 6;

				for (int index = 0; index < itemIndex; ++index) {
					items[index].rect.left = 0;
					items[index].rect.top = 0;
					items[index].rect.right = 0;
					items[index].rect.bottom = 0;
				}

				itemIndex = 0;
				SetTimer(hWnd, 1, 1000, NULL);
			}
			break;
		default:
			break;
		}
		InvalidateRect(hWnd, NULL, FALSE);
		break;
	case WM_PAINT:
		hDC = BeginPaint(hWnd, &ps);
		backMemDC = CreateCompatibleDC(hDC);
		memDC = CreateCompatibleDC(hDC);

		backBitmap = CreateCompatibleBitmap(hDC, WINDOW_WIDTH, WINDOW_HEIGHT);
		oldBackBitmap = static_cast<HBITMAP>(SelectObject(backMemDC, backBitmap));
		PatBlt(backMemDC, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, WHITENESS);

		switch (stage)
		{
		case Stage::ONE:
			DrawBackGround(backMemDC, memDC, backGround1, oldBackGround1, stage);
			break;
		case Stage::TWO:
			DrawBackGround(backMemDC, memDC, backGround2, oldBackGround2, stage);
			break;
		case Stage::THREE:
			DrawBackGround(backMemDC, memDC, backGround2, oldBackGround2, stage);
			break;
			/* 마지막 부분은 Ending Scene만 나오게 수정해야함 */
		case Stage::ENDING:
			DrawEndingScene(backMemDC, memDC, hEndingScene, oldEndingScene);
			break;
		default:
			break;
		}

		if (0 != itemTimeCount)
			PreviewItem(backMemDC, memDC, hWood, oldWood, hSpring, oldSpring, hTrap, oldTrap, items[itemIndex].itemType);

		DrawGate(backMemDC, memDC, gate, oldGate, stage);
		DrawMario(backMemDC, memDC, hMario, oldMario);
		DrawLuigi(backMemDC, memDC, hLuigi, oldLuigi);

		DrawItem(backMemDC, memDC, hWood, oldWood, hSpring, oldSpring, hTrap, oldTrap);

		ShowText(backMemDC);

		if (flag) {
			for (int index = 0; index < itemIndex; ++index)
				ShowAllFrameRect(backMemDC, &mario.rect, &luigi.rect, &items[index].rect, &gateSpot);
		}

		BitBlt(hDC, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, backMemDC, 0, 0, SRCCOPY);

		DeleteDC(memDC);
		DeleteDC(backMemDC);
		DeleteObject(backBitmap);
		EndPaint(hWnd, &ps);
		break;
	case WM_TIMER:
		switch (wParam)
		{
			/* ItemTimeCount */
		case 1:
			itemTimeCount--;
			if (0 == itemTimeCount)
				KillTimer(hWnd, 1);
			break;

			/* Monster Spawning */
		case 1000:
			break;
		case 2000:
			if (luigi.isJump) {
				lJumpHeight = (lJumpTime * lJumpTime - lJumpPower * lJumpTime) / 4;
				lJumpTime += 0.8f;
				luigi.rect.bottom = luigiTemp.bottom + lJumpHeight;
				luigi.rect.top = luigiTemp.top + lJumpHeight;
				for (int index = 0; index < MAX_ITEM; ++index) {
					if (luigi.rect.top <= items[index].rect.bottom && luigi.rect.bottom > items[index].rect.bottom
						&& IntersectRect(&temp, &luigi.rect, &items[index].rect)) {
						lJumpTime = 0;
						lJumpHeight = 0;
						luigi.isJump = FALSE;
						luigi.rect.bottom += 5;
						luigi.rect.top += 5;
						KillTimer(hWnd, 2000);
						return 0;
					}
				}
				if (lJumpTime > lJumpPower) {
					lJumpTime = 0;
					lJumpHeight = 0;
					luigi.isJump = FALSE;
					KillTimer(hWnd, 2000);
				}
				if (isCollide(&luigi.rect)) {
					luigi.rect.bottom -= 5;
					luigi.rect.top -= 5;
					luigi.isJump = FALSE;
					lJumpTime = 0;
					lJumpHeight = 0;
					KillTimer(hWnd, 2000);
				}
			}
			break;
		case 3000:
			if (mario.isJump) {
				RECT temp;
				mJumpHeight = (mJumpTime * mJumpTime - mJumpPower * mJumpTime) / 4;
				mJumpTime += 1.f;
				mario.rect.bottom = marioTemp.bottom + mJumpHeight;
				mario.rect.top = marioTemp.top + mJumpHeight;
				for (int index = 0; index < MAX_ITEM; ++index) {
					if (mario.rect.top <= items[index].rect.bottom && mario.rect.bottom > items[index].rect.bottom
						&& IntersectRect(&temp, &mario.rect, &items[index].rect)) {
						mJumpTime = 0;
						mJumpHeight = 0;
						mario.isJump = FALSE;
						mario.rect.bottom += 5;
						mario.rect.top += 5;
						KillTimer(hWnd, 3000);
						return 0;
					}
				}
				if (mJumpTime > mJumpPower) {
					mJumpTime = 0;
					mJumpHeight = 0;
					mario.isJump = FALSE;
					KillTimer(hWnd, 3000);
				}
				if (isCollide(&mario.rect)) {
					for (int index = 0; index < MAX_ITEM; ++index) {
						if (IntersectRect(&temp, &mario.rect, &items[index].rect)) {
							mario.rect.bottom = items[index].rect.top - 1;
							mario.rect.top = mario.rect.bottom - 80;
							mario.isJump = FALSE;
							mJumpTime = 0;
							mJumpHeight = 0;
							KillTimer(hWnd, 3000);
						}
					}
				}
			}
			break;
		default:
			break;
		}
		break;
	case WM_DESTROY:
		KillTimer(hWnd, 1);
		KillTimer(hWnd, 1000);
		KillTimer(hWnd, 2000);
		KillTimer(hWnd, 3000);
		PostQuitMessage(0);
		break;
	default:
		break;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// -------------------------------------------------------------------------------- Draw ------------------------------------------------------------------------------------------------
void DrawBackGround(HDC backMemDC, HDC memDC, const HBITMAP backGround, HBITMAP oldBackGround, const Stage stage)
{
	oldBackGround = static_cast<HBITMAP>(SelectObject(memDC, backGround));

	switch (stage)
	{
	case Stage::ONE:
		TransparentBlt(backMemDC, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, memDC, 2, 11, 510, 510, RGB(255, 0, 0));
		break;
	case Stage::TWO:
		TransparentBlt(backMemDC, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, memDC, 3, 4191, 508, 508, RGB(122, 122, 122));
		break;
	case Stage::THREE:
		TransparentBlt(backMemDC, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, memDC, 515, 3146, 508, 508, RGB(255, 0, 0));
		break;
	default:
		break;
	}
	SelectObject(memDC, oldBackGround);
}

void DrawGate(HDC backMemDC, HDC memDC, const HBITMAP gate, HBITMAP oldGate, const Stage stage)
{
	oldGate = static_cast<HBITMAP>(SelectObject(memDC, gate));
	switch (stage)
	{
	case Stage::ONE:
		TransparentBlt(backMemDC, gateSpot.left, gateSpot.top, 130, 130, memDC, 204, 157, 198, 145, RGB(34, 177, 76));
		break;
	case Stage::TWO:
		TransparentBlt(backMemDC, gateSpot.left, gateSpot.top, 130, 130, memDC, 204, 157, 198, 145, RGB(34, 177, 76));
		break;
	case Stage::THREE:
		TransparentBlt(backMemDC, gateSpot.left, gateSpot.top, 130, 130, memDC, 1, 455, 359, 359, RGB(34, 177, 76));
		break;
	default:
		break;
	}
	SelectObject(memDC, oldGate);
}

void DrawMario(HDC backMemDC, HDC memDC, const HBITMAP hMario, HBITMAP oldMario)
{
	oldMario = static_cast<HBITMAP>(SelectObject(memDC, hMario));
	TransparentBlt(backMemDC, mario.rect.left, mario.rect.top, PLAYER_WIDTH, PLAYER_HEIGHT, memDC, Mariobmp.bmWidth / 7.7 * mario.aIndex, 0, Mariobmp.bmWidth / 7.7, Mariobmp.bmHeight, RGB(0, 174, 0));
	SelectObject(memDC, oldMario);
}

void DrawLuigi(HDC backMemDC, HDC memDC, const HBITMAP hLuigi, HBITMAP oldLuigi)
{
	oldLuigi = static_cast<HBITMAP>(SelectObject(memDC, hLuigi));
	TransparentBlt(backMemDC, luigi.rect.left, luigi.rect.top, PLAYER_WIDTH, PLAYER_HEIGHT, memDC, Luigibmp.bmWidth / 7.7 * luigi.aIndex, 0, Luigibmp.bmWidth / 7.7, Luigibmp.bmHeight, RGB(0, 174, 174));
	SelectObject(memDC, oldLuigi);
}

void DrawItem(HDC backMemDC, HDC memDC, const HBITMAP hWood, HBITMAP oldWood, const HBITMAP hSpring, HBITMAP oldSpring, const HBITMAP hTrap, HBITMAP oldTrap)
{
	for (int index = 0; index < itemIndex; ++index) {
		switch (items[index].itemType)
		{
		case ItemType::WOOD:
			oldWood = static_cast<HBITMAP>(SelectObject(memDC, hWood));
			TransparentBlt(backMemDC, items[index].rect.left, items[index].rect.top, 200, 50, memDC, 108, 44, 379, 51, RGB(163, 73, 164));
			SelectObject(memDC, oldWood);
			break;
		case ItemType::SPRING:
			oldSpring = static_cast<HBITMAP>(SelectObject(memDC, hSpring));
			TransparentBlt(backMemDC, items[index].rect.left, items[index].rect.top, 50, 50, memDC, 2, 2, 80, 90, RGB(0, 127, 14));
			SelectObject(memDC, oldSpring);
			break;
		case ItemType::TRAP:
			oldTrap = static_cast<HBITMAP>(SelectObject(memDC, hTrap));
			TransparentBlt(backMemDC, items[index].rect.left, items[index].rect.top, 60, 60, memDC, 50, 11, 16, 14, RGB(78, 74, 168));
			SelectObject(memDC, oldTrap);
		default:
			break;
		}
	}
}
void DrawEndingScene(HDC backMemDC, HDC memDC, const HBITMAP hEndingScene, HBITMAP oldEndingScene)
{
	oldEndingScene = static_cast<HBITMAP>(SelectObject(memDC, hEndingScene));

	if (luigi.score < mario.score)
		TransparentBlt(backMemDC, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, memDC, 3, 3062, 655, 692, RGB(122, 122, 122));
	else
		TransparentBlt(backMemDC, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, memDC, 667, 3060, 810, 830, RGB(122, 122, 122));

	SelectObject(memDC, oldEndingScene);
}

void ShowText(HDC backMemDC)
{
	HFONT hFont, oldFont;
	hFont = CreateFontW(40, 0, 0, 0, 0, 0, 0, 0, HANGUL_CHARSET, 0, 0, 0, 0, L"맑은고딕");
	oldFont = static_cast<HFONT>(SelectObject(backMemDC, hFont));
	SetTextColor(backMemDC, RGB(255, 0, 0));
	SetBkMode(backMemDC, TRANSPARENT);

	if (0 != itemTimeCount) {
		wsprintf(tCount, L"%d", itemTimeCount);
		TextOut(backMemDC, 120, 0, tCount, lstrlen(tCount));
	}
	else {
		TextOut(backMemDC, 120, 0, start, lstrlen(start));
	}
	// Time Text
	TextOut(backMemDC, 0, 0, time, lstrlen(time));

	DeleteObject(hFont);

	hFont = CreateFontW(40, 0, 0, 0, 0, 0, 0, 0, HANGUL_CHARSET, 0, 0, 0, 0, L"맑은고딕");
	oldFont = static_cast<HFONT>(SelectObject(backMemDC, hFont));

	SetTextColor(backMemDC, RGB(255, 0, 0));
	for (int index = 0; index < mario.life; ++index)
		TextOut(backMemDC, 120 + (30 * index), 50, L"♥", 1);
	TextOut(backMemDC, 0, 50, marioLife, lstrlen(marioLife));

	SetTextColor(backMemDC, RGB(0, 255, 0));
	for (int index = 0; index < luigi.life; ++index)
		TextOut(backMemDC, 120 + (30 * index), 100, L"♥", 1);
	TextOut(backMemDC, 0, 100, luigiLife, lstrlen(luigiLife));

	DeleteObject(hFont);
}



void PreviewItem(HDC backMemDC, HDC memDC, const HBITMAP hWood, HBITMAP oldWood, const HBITMAP hSpring, HBITMAP oldSpring, const HBITMAP hTrap, HBITMAP oldTrap, const ItemType type)
{
	switch (type)
	{
	case ItemType::WOOD:
		oldWood = static_cast<HBITMAP>(SelectObject(memDC, hWood));
		TransparentBlt(backMemDC, mX - 100, mY - 25, 200, 50, memDC, 108, 44, 379, 51, RGB(163, 73, 164));
		SelectObject(memDC, oldWood);
		break;
	case ItemType::SPRING:
		oldSpring = static_cast<HBITMAP>(SelectObject(memDC, hSpring));
		TransparentBlt(backMemDC, mX - 25, mY - 25, 50, 50, memDC, 2, 2, 80, 90, RGB(0, 127, 14));
		SelectObject(memDC, oldSpring);
		break;
	case ItemType::TRAP:
		oldTrap = static_cast<HBITMAP>(SelectObject(memDC, hTrap));
		TransparentBlt(backMemDC, mX - 30, mY - 30, 60, 60, memDC, 50, 11, 16, 14, RGB(78, 74, 168));
		SelectObject(memDC, oldTrap);
		break;
	default:
		break;
	}
}

void ShowAllFrameRect(HDC backMemDC, const RECT* marioRect, const RECT* luigiRect, const RECT* itemRect, const RECT* gateSpot)
{
	HBRUSH hBrush, oldBrush;

	hBrush = CreateSolidBrush(RGB(255, 0, 0));
	oldBrush = static_cast<HBRUSH>(SelectObject(backMemDC, hBrush));

	FrameRect(backMemDC, &mario.rect, hBrush);
	FrameRect(backMemDC, &luigi.rect, hBrush);

	DeleteObject(hBrush);

	hBrush = CreateSolidBrush(RGB(233, 129, 56));
	oldBrush = static_cast<HBRUSH>(SelectObject(backMemDC, hBrush));

	FrameRect(backMemDC, itemRect, hBrush);
	FrameRect(backMemDC, gateSpot, hBrush);

	DeleteObject(hBrush);
}

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
void CALLBACK TimerProc(HWND hWnd, UINT uMsg, UINT idEvent, DWORD dwTime) {
	//타이머에서는 객체들의 이동 처리
	Gravity();
	if (0 == itemTimeCount) {
		if (GetAsyncKeyState('W')) {
			if (mario.rect.top > 5) {
				mario.rect.top -= 5;
				mario.rect.bottom -= 5;
				mario.dir = Dir::UP;
				if (isCollide(&mario.rect)) {
					mario.rect.top += 5;
					mario.rect.bottom += 5;
				}
			}
			mario.aIndex++;
			mario.aIndex %= 7;
		}

		if (GetAsyncKeyState('A')) {
			if (mario.rect.left >= 5) {
				mario.rect.left -= 5;
				mario.rect.right -= 5;
				mario.dir = Dir::LEFT;
				if (isCollide(&mario.rect)) {
					mario.rect.left += 5;
					mario.rect.right += 5;
				}
			}
			mario.aIndex++;
			mario.aIndex %= 7;
		}

		if (GetAsyncKeyState('S')) {
			if (mario.rect.bottom < WINDOW_HEIGHT - 35) {
				mario.rect.top += 5;
				mario.rect.bottom += 5;
				mario.dir = Dir::DOWN;
				if (isCollide(&mario.rect)) {
					mario.rect.top -= 5;
					mario.rect.bottom -= 5;
				}
			}
			mario.aIndex++;
			mario.aIndex %= 7;
		}

		if (GetAsyncKeyState('D')) {
			if (mario.rect.right < WINDOW_WIDTH - 5) {
				mario.rect.left += 5;
				mario.rect.right += 5;
				mario.dir = Dir::RIGHT;
				if (isCollide(&mario.rect)) {
					mario.rect.left -= 5;
					mario.rect.right -= 5;
				}
			}
			mario.aIndex++;
			mario.aIndex %= 7;
		}

		if (GetAsyncKeyState('I')) {
			if (luigi.rect.top > 5) {
				luigi.rect.top -= 5;
				luigi.rect.bottom -= 5;
				luigi.dir = Dir::UP;
				if (isCollide(&luigi.rect)) {
					luigi.rect.top += 5;
					luigi.rect.bottom += 5;
				}
			}
			luigi.aIndex++;
			luigi.aIndex %= 7;
		}
		if (GetAsyncKeyState('J')) {
			if (luigi.rect.left >= 5) {
				luigi.rect.left -= 5;
				luigi.rect.right -= 5;
				luigi.dir = Dir::LEFT;
				if (isCollide(&luigi.rect)) {
					luigi.rect.left += 5;
					luigi.rect.right += 5;
				}

			}
			luigi.aIndex++;
			luigi.aIndex %= 7;
		}

		if (GetAsyncKeyState('K')) {
			if (luigi.rect.bottom < WINDOW_HEIGHT - 35) {
				luigi.rect.top += 5;
				luigi.rect.bottom += 5;
				luigi.dir = Dir::DOWN;
				if (isCollide(&luigi.rect)) {
					luigi.rect.top -= 5;
					luigi.rect.bottom -= 5;
				}
			}
			luigi.aIndex++;
			luigi.aIndex %= 7;
		}


		if (GetAsyncKeyState('L')) {
			if (luigi.rect.right < WINDOW_WIDTH - 5) {
				luigi.rect.left += 5;
				luigi.rect.right += 5;
				luigi.dir = Dir::RIGHT;
				if (isCollide(&luigi.rect)) {
					luigi.rect.left -= 5;
					luigi.rect.right -= 5;
				}
			}
			luigi.aIndex++;
			luigi.aIndex %= 7;
		}
	}
	InvalidateRect(hWnd, NULL, FALSE);
}

BOOL isCollide(const RECT* playerRect)
{
	for (int index = 0; index < MAX_ITEM; ++index) {
		if (IntersectRect(&temp, playerRect, &items[index].rect))
			return TRUE;
	}
	return FALSE;
}

void Reset(const HWND hWnd)
{
	// 리셋할 때 타이머 전부 죽이고 전부 다시 불러야 하나 
	stage = Stage::ONE;
	mario.score = 0;
	mario.life = 3;
	luigi.life = 3;

	luigi.score = 0;
	/* Mario start position */
	mario.rect.left = 0;
	mario.rect.top = 680;
	mario.rect.right = mario.rect.left + PLAYER_WIDTH;
	mario.rect.bottom = mario.rect.top + PLAYER_HEIGHT;

	/* Luigi start position */
	luigi.rect.left = 20;
	luigi.rect.top = 680;
	luigi.rect.right = luigi.rect.left + PLAYER_WIDTH;
	luigi.rect.bottom = luigi.rect.top + PLAYER_HEIGHT;

	itemTimeCount = 6;

	itemIndex = 0;

	SetTimer(hWnd, 1000, 5000, NULL);
	SetTimer(hWnd, 1, 1000, NULL);	// 아이템 시간
}

void Gravity()
{
	if (!mario.isJump) {
		for (int index = 0; index < MAX_ITEM; ++index) {
			if (mario.dropSpeed > 0 && IntersectRect(&temp, &mario.rect, &items[index].rect)) {
				mario.rect.bottom = items[index].rect.top + 5;
				mario.rect.top = mario.rect.bottom - 80;
				mario.dropSpeed = 0;
			}
		}
		RECT temp = mario.rect;
		temp.top += 5;
		temp.bottom += 5;
		if (isCollide(&temp)) {
			mario.dropSpeed = 0;
		}
		else if (mario.rect.bottom >= 760) {
			mario.rect.bottom = 760;
			mario.rect.top = 680;
			mario.dropSpeed = 0;
		}
		else if (mario.rect.bottom < 760) {
			mario.gravity = 1.f;
			mario.dropSpeed += mario.gravity;
			mario.rect.top += mario.dropSpeed;
			mario.rect.bottom += mario.dropSpeed;
		}
	}
	if (!luigi.isJump) {
		for (int index = 0; index < MAX_ITEM; ++index) {
			if (luigi.dropSpeed > 0 && IntersectRect(&temp, &luigi.rect, &items[index].rect)) {
				luigi.rect.bottom = items[index].rect.top + 5;
				luigi.rect.top = luigi.rect.bottom - 80;
				luigi.dropSpeed = 0;
			}
		}
		RECT temp = luigi.rect;
		temp.top += 5;
		temp.bottom += 5;
		if (isCollide(&temp)) {
			luigi.dropSpeed = 0;
		}
		else if (luigi.rect.bottom >= 760) {
			luigi.rect.bottom = 760;
			luigi.rect.top = 680;
			luigi.dropSpeed = 0;
		}
		else if (luigi.rect.bottom < 760) {
			luigi.gravity = 1.f;
			luigi.dropSpeed += luigi.gravity;
			luigi.rect.top += luigi.dropSpeed;
			luigi.rect.bottom += luigi.dropSpeed;
		}
	} 
}
