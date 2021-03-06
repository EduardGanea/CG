//-----------------------------------------------------------------------------
// File: CGameApp.cpp
//
// Desc: Game Application class, this is the central hub for all app processing
//
// Original design by Adam Hoult & Gary Simmons. Modified by Mihai Popescu.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// CGameApp Specific Includes
//-----------------------------------------------------------------------------
#include "CGameApp.h"
#include <fstream>
extern HINSTANCE g_hInst;

//-----------------------------------------------------------------------------
// CGameApp Member Functions
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Name : CGameApp () (Constructor)
// Desc : CGameApp Class Constructor
//-----------------------------------------------------------------------------
CGameApp::CGameApp()
{
	// Reset / Clear all required values
	m_hWnd			= NULL;
	m_hIcon			= NULL;
	m_hMenu			= NULL;
	m_pBBuffer		= NULL;
	m_pPlayer       = NULL;
	m_pPlayer2      = NULL;
	m_LastFrameRate = 0;
}

//-----------------------------------------------------------------------------
// Name : ~CGameApp () (Destructor)
// Desc : CGameApp Class Destructor
//-----------------------------------------------------------------------------
CGameApp::~CGameApp()
{
	// Shut the engine down
	ShutDown();
}

//-----------------------------------------------------------------------------
// Name : InitInstance ()
// Desc : Initialises the entire Engine here.
//-----------------------------------------------------------------------------
bool CGameApp::InitInstance( LPCTSTR lpCmdLine, int iCmdShow )
{
	// Create the primary display device
	if (!CreateDisplay()) { ShutDown(); return false; }
	
	// Build Objects
	if (!BuildObjects()) 
	{ 
		MessageBox( 0, _T("Failed to initialize properly. Reinstalling the application may solve this problem.\nIf the problem persists, please contact technical support."), _T("Fatal Error"), MB_OK | MB_ICONSTOP);
		ShutDown(); 
		return false; 
	}

	// Set up all required game states
	SetupGameState();

	// Success!
	return true;
}

//-----------------------------------------------------------------------------
// Name : CreateDisplay ()
// Desc : Create the display windows, devices etc, ready for rendering.
//-----------------------------------------------------------------------------
bool CGameApp::CreateDisplay()
{
	LPTSTR			WindowTitle		= _T("GameFramework");
	LPCSTR			WindowClass		= _T("GameFramework_Class");
	USHORT			Width			= 800;
	USHORT			Height			= 600;
	RECT			rc;
	WNDCLASSEX		wcex;

	
	wcex.cbSize			= sizeof(WNDCLASSEX);
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= CGameApp::StaticWndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= g_hInst;
	wcex.hIcon			= LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_ICON));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= 0;
	wcex.lpszClassName	= WindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_ICON));

	if(RegisterClassEx(&wcex)==0)
		return false;

	// Retrieve the final client size of the window
	::GetClientRect( m_hWnd, &rc );
	m_nViewX		= rc.left;
	m_nViewY		= rc.top;
	m_nViewWidth	= rc.right - rc.left;
	m_nViewHeight	= rc.bottom - rc.top;

	m_hWnd = CreateWindow(WindowClass, WindowTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, Width, Height, NULL, NULL, g_hInst, this);

	if (!m_hWnd)
		return false;

	// Show the window
	ShowWindow(m_hWnd, SW_SHOWMAXIMIZED);

	// Success!!
	return true;
}

//-----------------------------------------------------------------------------
// Name : BeginGame ()
// Desc : Signals the beginning of the physical post-initialisation stage.
//		From here on, the game engine has control over processing.
//-----------------------------------------------------------------------------
int CGameApp::BeginGame()
{
	MSG		msg;

	// Start main loop
	while(true) 
	{
		// Did we recieve a message, or are we idling ?
		if ( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) ) 
		{
			if (msg.message == WM_QUIT) break;
			TranslateMessage( &msg );
			DispatchMessage ( &msg );
		} 
		else 
		{
			// Advance Game Frame.
			FrameAdvance();

		} // End If messages waiting
	
	} // Until quit message is receieved

	return 0;
}

//-----------------------------------------------------------------------------
// Name : ShutDown ()
// Desc : Shuts down the game engine, and frees up all resources.
//-----------------------------------------------------------------------------
bool CGameApp::ShutDown()
{
	// Release any previously built objects
	ReleaseObjects ( );
	
	// Destroy menu, it may not be attached
	if ( m_hMenu ) DestroyMenu( m_hMenu );
	m_hMenu		 = NULL;

	// Destroy the render window
	SetMenu( m_hWnd, NULL );
	if ( m_hWnd ) DestroyWindow( m_hWnd );
	m_hWnd		  = NULL;
	
	// Shutdown Success
	return true;
}


//-----------------------------------------------------------------------------
// Name : StaticWndProc () (Static Callback)
// Desc : This is the main messge pump for ALL display devices, it captures
//		the appropriate messages, and routes them through to the application
//		class for which it was intended, therefore giving full class access.
// Note : It is VITALLY important that you should pass your 'this' pointer to
//		the lpParam parameter of the CreateWindow function if you wish to be
//		able to pass messages back to that app object.
//-----------------------------------------------------------------------------
LRESULT CALLBACK CGameApp::StaticWndProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	// If this is a create message, trap the 'this' pointer passed in and store it within the window.
	if ( Message == WM_CREATE ) SetWindowLong( hWnd, GWL_USERDATA, (LONG)((CREATESTRUCT FAR *)lParam)->lpCreateParams);

	// Obtain the correct destination for this message
	CGameApp *Destination = (CGameApp*)GetWindowLong( hWnd, GWL_USERDATA );
	
	// If the hWnd has a related class, pass it through
	if (Destination) return Destination->DisplayWndProc( hWnd, Message, wParam, lParam );
	
	// No destination found, defer to system...
	return DefWindowProc( hWnd, Message, wParam, lParam );
}

//-----------------------------------------------------------------------------
// Name : DisplayWndProc ()
// Desc : The display devices internal WndProc function. All messages being
//		passed to this function are relative to the window it owns.
//-----------------------------------------------------------------------------
LRESULT CGameApp::DisplayWndProc( HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam )
{
	static UINT			fTimer;	
	
	// Determine message type
	switch (Message)
	{
		case WM_CREATE:
			break;
		
		case WM_CLOSE:
			PostQuitMessage(0);
			break;
		
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		
		case WM_SIZE:
			if ( wParam == SIZE_MINIMIZED )
			{
				// App is inactive
				m_bActive = false;
			
			} // App has been minimized
			else
			{
				// App is active
				m_bActive = true;

				// Store new viewport sizes
				m_nViewWidth  = LOWORD( lParam );
				m_nViewHeight = HIWORD( lParam );
		
			
			} // End if !Minimized

			break;

		case WM_LBUTTONDOWN:
			// Capture the mouse
			SetCapture( m_hWnd );
			GetCursorPos( &m_OldCursorPos );
			break;

		case WM_LBUTTONUP:
			// Release the mouse
			ReleaseCapture( );
			break;

		case WM_KEYDOWN:
			switch (wParam)
			{
			case VK_ESCAPE:
				PostQuitMessage(0);
				break;
			case VK_RETURN:
				fTimer = SetTimer(m_hWnd, 1, 70, NULL);
				m_pPlayer->Explode();
				m_pPlayer->DecreaseLives();
				break;
			case 'Q':
				fTimer = SetTimer(m_hWnd, 1, 70, NULL);
				m_pPlayer2->Explode();
				m_pPlayer2->DecreaseLives();
				break;
			case 'H':
				m_pPlayer2->Shoot(-1);
				
				break;
			case 'O':
				m_pPlayer->RotateLeft();
				break;
			case 'R':
				m_pPlayer2->RotateLeft();
				break;
			case VK_SPACE:
				m_pPlayer->Shoot(1);			
				break;
			case 'S':
				SaveGame(m_pPlayer, m_pPlayer2);
				break;
			case 'L':
				LoadGame(m_pPlayer, m_pPlayer2);
				break;

			}
			break;

		case WM_TIMER:
			switch (wParam)
			{
			case 1:
				if (!m_pPlayer->AdvanceExplosion()) {
					fTimer = SetTimer(m_hWnd, 1, 70, NULL);
					
				}

			case 2:
				if (!m_pPlayer2->AdvanceExplosion()) {
					fTimer = SetTimer(m_hWnd, 1, 70, NULL);
					

				}
			case 3:
				
				if (m_pPlayer->Collision(m_pPlayer, m_pPlayer2)) {
					fTimer = SetTimer(m_hWnd, 1, 70, NULL);
					m_pPlayer->Explode();
					m_pPlayer->DecreaseLives();
					m_pPlayer->Position() = Vec2(100, 400);
					m_pPlayer2->Explode();
					m_pPlayer2->DecreaseLives();
					m_pPlayer2->Position() = Vec2(600, 0);
					
					
				}
					

			}
				break;
			
		case WM_COMMAND:
			break;

		default:
			return DefWindowProc(hWnd, Message, wParam, lParam);

	} // End Message Switch
	
	return 0;
}

//-----------------------------------------------------------------------------
// Name : BuildObjects ()
// Desc : Build our demonstration meshes, and the objects that instance them
//-----------------------------------------------------------------------------
bool CGameApp::BuildObjects()
{
	m_pBBuffer = new BackBuffer(m_hWnd, m_nViewWidth, m_nViewHeight);
	m_pPlayer = new CPlayer(m_pBBuffer,1);
	m_pPlayer2 = new  CPlayer(m_pBBuffer,2);

	
	if(!m_imgBackground.LoadBitmapFromFile("data/background.bmp", GetDC(m_hWnd)))
		return false;

	// Success!
	return true;
}

//-----------------------------------------------------------------------------
// Name : SetupGameState ()
// Desc : Sets up all the initial states required by the game.
//-----------------------------------------------------------------------------
void CGameApp::SetupGameState()
{
	m_pPlayer->Position() = Vec2(100, 400);
	m_pPlayer2->Position() = Vec2(600, 0);

	
}

//-----------------------------------------------------------------------------
// Name : ReleaseObjects ()
// Desc : Releases our objects and their associated memory so that we can
//		rebuild them, if required, during our applications life-time.
//-----------------------------------------------------------------------------
void CGameApp::ReleaseObjects()
{
	if (m_pPlayer != NULL)
	{
		delete m_pPlayer;
		m_pPlayer = NULL;
	}

	if (m_pBBuffer != NULL)
	{
		delete m_pBBuffer;
		m_pBBuffer = NULL;
	}

	if (m_pPlayer2 != NULL)
	{
		delete m_pPlayer2;
		m_pPlayer2 = NULL;
	}

	
	

}

//-----------------------------------------------------------------------------
// Name : FrameAdvance () (Private)
// Desc : Called to signal that we are now rendering the next frame.
//-----------------------------------------------------------------------------
void CGameApp::FrameAdvance()
{
	static TCHAR FrameRate[ 50 ];
	static TCHAR TitleBuffer[ 255 ];
	
	// Advance the timer
	m_Timer.Tick( );

	// Skip if app is inactive
	if ( !m_bActive ) return;
	
	// Get / Display the framerate
	if ( m_LastFrameRate != m_Timer.GetFrameRate() )
	{
		m_LastFrameRate = m_Timer.GetFrameRate( FrameRate, 50 );
		sprintf_s(TitleBuffer, _T("Game : %s  Lives: %d-%d"), FrameRate,m_pPlayer->GetLives(), m_pPlayer2->GetLives());
		SetWindowText( m_hWnd, TitleBuffer );

	} // End if Frame Rate Altered

	//Game is ended when the lives of one player reach 0
	if (!m_pPlayer->GetLives())
	{
		MessageBox(m_hWnd, "Second Player Wins", "Game over", MB_OK);
		PostQuitMessage(0);
	}
	else if (!m_pPlayer2->GetLives())
	{
		MessageBox(m_hWnd, "First Player Wins", "Game over", MB_OK);
		PostQuitMessage(0);
	}
	//end game

	// Poll & Process input devices
	ProcessInput();

	// Animate the game objects
	AnimateObjects();

	// Drawing the game objects
	DrawObjects();
}

//-----------------------------------------------------------------------------
// Name : ProcessInput () (Private)
// Desc : Simply polls the input devices and performs basic input operations
//-----------------------------------------------------------------------------
void CGameApp::ProcessInput()
{
	static UCHAR pKeyBuffer[256];
	ULONG		Direction = 0, Direction2 = 0, Direction3 = 0;
	POINT		CursorPos;
	float		X = 0.0f, Y = 0.0f;

	// Retrieve keyboard state
	if (!GetKeyboardState(pKeyBuffer)) return;

	// Check the relevant keys
	if (pKeyBuffer[VK_UP] & 0xF0) Direction |= CPlayer::DIR_FORWARD;
	if (pKeyBuffer[VK_DOWN] & 0xF0) Direction |= CPlayer::DIR_BACKWARD;
	if (pKeyBuffer[VK_LEFT] & 0xF0) Direction |= CPlayer::DIR_LEFT;
	if (pKeyBuffer[VK_RIGHT] & 0xF0) Direction |= CPlayer::DIR_RIGHT;

	if (pKeyBuffer['W'] & 0xF0) Direction2 |= CPlayer::DIR_FORWARD;
	if (pKeyBuffer['S'] & 0xF0) Direction2 |= CPlayer::DIR_BACKWARD;
	if (pKeyBuffer['A'] & 0xF0) Direction2 |= CPlayer::DIR_LEFT;
	if (pKeyBuffer['D'] & 0xF0) Direction2 |= CPlayer::DIR_RIGHT;


	if (pKeyBuffer['W'] & 0xF0) Direction2 |= CPlayer::DIR_FORWARD;
	if (pKeyBuffer['S'] & 0xF0) Direction2 |= CPlayer::DIR_BACKWARD;
	if (pKeyBuffer['A'] & 0xF0) Direction2 |= CPlayer::DIR_LEFT;
	if (pKeyBuffer['D'] & 0xF0) Direction2 |= CPlayer::DIR_RIGHT;
	
	
	// Move the player
	m_pPlayer->Move(Direction);
	m_pPlayer2->Move(Direction2);
	
	
	
	// Now process the mouse (if the button is pressed)
	if ( GetCapture() == m_hWnd )
	{
		// Hide the mouse pointer
		SetCursor( NULL );

		// Retrieve the cursor position
		GetCursorPos( &CursorPos );

		// Reset our cursor position so we can keep going forever :)
		SetCursorPos( m_OldCursorPos.x, m_OldCursorPos.y );

	} // End if Captured
}

//-----------------------------------------------------------------------------
// Name : AnimateObjects () (Private)
// Desc : Animates the objects we currently have loaded.
//-----------------------------------------------------------------------------
void CGameApp::AnimateObjects()
{
	m_pPlayer->Update(m_Timer.GetTimeElapsed());
	m_pPlayer2->Update(m_Timer.GetTimeElapsed());
	


}

//-----------------------------------------------------------------------------
// Name : DrawObjects () (Private)
// Desc : Draws the game objects
//-----------------------------------------------------------------------------
void CGameApp::DrawObjects()
{
	
	m_pBBuffer->reset();
	DrawBackground();
	
	
	m_pPlayer->Draw();
	m_pPlayer2->Draw();
	
	
	if (m_pPlayer->bulletCollision(m_pPlayer,m_pPlayer2,1)) {
		m_pPlayer2->Explode();
		m_pPlayer2->DecreaseLives();
		
		
	}

	if (m_pPlayer->bulletCollision(m_pPlayer2, m_pPlayer,2)) {
		m_pPlayer->Explode();
		m_pPlayer->DecreaseLives();
		

	}
	if (m_pPlayer->rotateDirection == 1)
		m_pPlayer->fire(-1, 0);
	if (m_pPlayer->rotateDirection == 2)
		m_pPlayer->fire(1, 0);
	if (m_pPlayer->rotateDirection == 4)
		m_pPlayer->fire(0, -1);
	if (m_pPlayer->rotateDirection == 8)
		m_pPlayer->fire(0, 1);

	m_pPlayer2->fire(1,0);
	
	m_pBBuffer->present();
	

}

void CGameApp::DrawBackground()
{
	static int currentY =m_imgBackground.Height();

	static size_t lastTime = ::GetTickCount();
	size_t currentTime = ::GetTickCount();

	if (currentTime - lastTime > 100)
	{
		lastTime = currentTime;
		currentY -= 10;
		if (currentY < 0)
			currentY = m_imgBackground.Height();
	}

	m_imgBackground.Paint(m_pBBuffer->getDC(), 0, currentY);
	
}

void CGameApp::SaveGame(CPlayer* Player1, CPlayer* Player2) {
	std::ofstream save;
	save.open("save.txt");
	save << Player1->GetLives() << '\n' << Player2->GetLives() << '\n';
	save << Player1->Position().x << " " << Player1->Position().y << '\n';
	save << Player2->Position().x << " " << Player2->Position().y << '\n';
	save.close();
	::MessageBox(m_hWnd, "Game saved", "Save", MB_OK);
}


//-------------------------------------------------------------
// Load previously saved game
//-------------------------------------------------------------
void CGameApp::LoadGame(CPlayer* Player1, CPlayer* Player2) {
	::MessageBox(m_hWnd, "Loading game...", "Load", MB_OK);
	std::ifstream save;
	save.open("save.txt");
	int live1, live2;
	Vec2 currentPosition1, currentPosition2;
	save >> live1 >> live2;
	save >> currentPosition1.x >> currentPosition1.y;
	save >> currentPosition2.x >> currentPosition2.y;
	Player1->SetLives(live1);
	Player2->SetLives(live2);
	Player1->SetPosition(currentPosition1);
	Player2->SetPosition(currentPosition2);
	save.close();
	::MessageBox(m_hWnd, "Game loaded", "Load", MB_OK);
}