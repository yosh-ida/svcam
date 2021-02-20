#include <d3dx9.h>
#include <d3d9.h>
#include <windows.h>
#include <fstream>

const char lpszClassName[] = "ClassName";

typedef struct _D3DTLVERTEX {
	float   x, y, z;                                  // 頂点のXYZ座標
	float   rhw;                                    // 描画時にXYZから割られる数（小難しい話なのでここは常に1となると覚えよう）
	DWORD   color;                                  // 頂点の色(32bitアルファ値あり)
	float   tu, tv;                                 // 頂点に割り当てるテクスチャのUV値
} D3DTLVERTEX, *LPD3DTLVERTEX;

// 頂点の情報
#define D3DFVF_TLVERTEX     (D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1)


LPDIRECT3D9             lpD3D = NULL;         // Direct3D
LPDIRECT3DDEVICE9       lpD3DDevice = NULL;         // Direct3DDevice
LPDIRECT3DTEXTURE9      lpTex = NULL;
D3DTLVERTEX             v[4];

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

	HWND window_handle = CreateWindowEx(
		WS_EX_TOOLWINDOW,
		// 登録しているウィンドウクラス構造体の名前
		TEXT(lpszClassName),
		// ウィンドウ名(タイトルバーに表示される)
		TEXT("window"),
		// ウィンドウスタイル
		//WS_DISABLED,
		WS_VISIBLE | WS_DISABLED,
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

	ret = D3DXCreateTextureFromFileExA(
		lpD3DDevice,            // Direct3DDeviceのポインタ
		"C:\\Users\\Haru\\Documents\\svcam\\Release\\bm.png",               // ファイル名
		D3DX_DEFAULT,           // テクスチャの幅(画像から取得させる)
		D3DX_DEFAULT,           // テクスチャの高さ(画像から取得させる)
		1,                      // ミップマップテクスチャを作成するレベル(1だとこのテクスチャのみ作成される)
		0,                      // レンダリングターゲットとはしない
		D3DFMT_UNKNOWN,         // テクスチャのフォーマットを画像から取得
		D3DPOOL_MANAGED,        // テクスチャが消失しても自動的に退避させておく
		D3DX_FILTER_POINT,      // テクスチャの描画時に一番近いドットを使用する
		D3DX_FILTER_NONE,       // ミップマップテクスチャのフィルタを使用しない
		0,                      // カラーキーを使用しない(アルファがあればそれを使う)
		NULL,                   // 作成されたテクスチャの情報を返さなくてもよい場合はNULL
		NULL,                   // パレット情報を返さなくてもよい場合はNULL
		&lpTex);               // 作成されたテクスチャを受け取る変数のポインタ
	if (FAILED(ret)) {
		// 作成エラー
		lpD3DDevice->Release();
		lpD3D->Release();
		MessageBoxW(window_handle, L"テクスチャが作成出来ませんでした", L"エラーだにょ", MB_OK | MB_ICONHAND);
		return -1;
	}

	// 頂点の初期化
	ZeroMemory(&v, sizeof(v));

	v[0].x = 160.0f;
	v[0].y = 120.0f;
	v[0].z = 0.0f;
	v[0].rhw = 1.0f;
	v[0].color = 0xFFFFFFFF;
	v[0].tu = 0.0f;
	v[0].tv = 0.0f;

	v[1].x = 480.0f;
	v[1].y = 120.0f;
	v[1].z = 0.0f;
	v[1].rhw = 1.0f;
	v[1].color = 0xFFFFFFFF;
	v[1].tu = 1.0f;
	v[1].tv = 0.0f;

	v[2].x = 160.0f;
	v[2].y = 360.0f;
	v[2].z = 0.0f;
	v[2].rhw = 1.0f;
	v[2].color = 0xFFFFFFFF;
	v[2].tu = 0.0f;
	v[2].tv = 1.0f;

	v[3].x = 480.0f;
	v[3].y = 360.0f;
	v[3].z = 0.0f;
	v[3].rhw = 1.0f;
	v[3].color = 0xFFFFFFFF;
	v[3].tu = 1.0f;
	v[3].tv = 1.0f;

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
		// バックバッファのクリア
		lpD3DDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

		// シーンの開始
		lpD3DDevice->BeginScene();

		// テクスチャのセット
		lpD3DDevice->SetTexture(0, lpTex);

		// 使用する頂点フォーマットのセット
		lpD3DDevice->SetFVF(D3DFVF_TLVERTEX);

		// 頂点リストの描画
		lpD3DDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(D3DTLVERTEX));

		// シーンの終了
		lpD3DDevice->EndScene();

		// フリップして実際に画面に反映
		lpD3DDevice->Present(NULL, NULL, NULL, NULL);

		Sleep(15);
	}

	// 開放処理
	lpTex->Release();
	lpD3DDevice->Release();
	lpD3D->Release();

	return 0;
}