#include <d3dx9.h>
#include <d3d9.h>
#include <windows.h>
#include <fstream>

const char lpszClassName[] = "ClassName";

LPDIRECT3D9             lpD3D = NULL;         // Direct3D
LPDIRECT3DDEVICE9       lpD3DDevice = NULL;         // Direct3DDevice

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_CREATE:
		return 0;
	}
	return DefWindowProc(hwnd, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
	WNDCLASS winc;
	MSG msg;

	STARTUPINFO stinfo;
	GetStartupInfo(&stinfo);

	winc.style = CS_HREDRAW | CS_VREDRAW;
	winc.lpfnWndProc = WndProc;
	winc.cbClsExtra = winc.cbWndExtra = 0;
	winc.hInstance = hInstance;
	winc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	winc.hCursor = LoadCursor(NULL, IDC_ARROW);
	winc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	winc.lpszMenuName = NULL;
	winc.lpszClassName = TEXT(lpszClassName);

	if (!RegisterClass(&winc)) return 0;

	HWND window_handle = CreateWindow(
		// 登録しているウィンドウクラス構造体の名前
		TEXT(lpszClassName),
		// ウィンドウ名(タイトルバーに表示される)
		TEXT("window"),
		// ウィンドウスタイル
		//WS_DISABLED,
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		// 表示位置
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		// サイズ
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		// 親ハンドル
		NULL,
		// メニューハンドル
		NULL,
		// インスタンスハンドル
		hInstance,
		// WM_CREATEメッセージでlpparamに渡したい値
		NULL);

	if (window_handle == NULL)
		return 0;

	// Direct3Dオブジェクトの取得
	lpD3D = Direct3DCreate9(D3D_SDK_VERSION);
	if (!lpD3D)
	{
		MessageBoxW(window_handle, L"Direct3Dが作成出来ませんでした", L"エラー", MB_OK | MB_ICONHAND);
		return -1;
	}

	// Direct3Dデバイスの設定をセット
	D3DPRESENT_PARAMETERS param;
	ZeroMemory(&param, sizeof(param));
	param.Windowed = TRUE;                             // ウィンドウモードか
	param.SwapEffect = D3DSWAPEFFECT_DISCARD;            // 垂直同期でフリップ
	param.BackBufferCount = 1;                                // バックバッファの数
	param.BackBufferWidth = stinfo.dwXSize;                              // 画面の幅
	param.BackBufferHeight = stinfo.dwYSize;                              // 画面の高さ
	param.FullScreen_RefreshRateInHz = 0;                                // リフレッシュレート(0=デフォルトを使用)
	param.EnableAutoDepthStencil = TRUE;                             // 自動的にZバッファとステンシルバッファを作成する
	param.AutoDepthStencilFormat = D3DFMT_D24S8;                     // Zバッファとステンシルバッファのフォーマット

	// 現在のディスプレイのバックバッファフォーマットを取得
	D3DDISPLAYMODE dm;
	HRESULT ret = lpD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &dm);   // DEFAULT指定でプライマリアダプタを選択
	if (FAILED(ret))
	{
		MessageBoxW(window_handle, L"Direct3Dが作成出来ませんでした", L"エラー", MB_OK | MB_ICONHAND);
		return -1;
	}
	param.BackBufferFormat = dm.Format;                                     // バックバッファのフォーマットをコピー

	// Direct3DDeviceを作成
	ret = lpD3D->CreateDevice(
		D3DADAPTER_DEFAULT,                             // デフォルトの3Dデバイスを使用
		D3DDEVTYPE_HAL,                                 // ハードウェアを使用する
		window_handle,                                       // ウインドウのハンドル
		D3DCREATE_HARDWARE_VERTEXPROCESSING,            // ハードウェア頂点処理を行う(VGAがDX8に対応していない場合はSOFTWAREを設定)
		&param,                                         // 上記で設定した構造体のポインタ
		&lpD3DDevice);                                 // 作成されたデバイスを受け取る変数のポインタ
	if (FAILED(ret))
	{
		// エラーなら終了
		lpD3D->Release();
		MessageBoxW(window_handle, L"Direct3DDeviceが作成出来ませんでした", L"エラー", MB_OK | MB_ICONHAND);
		return -1;
	}

	// メインループ
	while (1)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		Sleep(15);
	}

	// 開放処理
	lpD3DDevice->Release();
	lpD3D->Release();

	return 0;
}