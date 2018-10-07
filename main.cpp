#define _WIN32_WINNT 0x0500 //Needed for GetConsoleWindow()
#include <iostream>
#include <windows.h>
#include <gl/gl.h>
#include <D3D/dinput.h>
#include <fcntl.h> //for console
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <ctime>
#include <string>
#include <windows.h>

using namespace std;

struct gamepad
{
    gamepad()
    {
        last_x=0;
        last_y=0;
        key_up=key_right=key_down=key_left=false;
    }
    LPDIRECTINPUTDEVICE8 di_controller;
    long last_x;
    long last_y;

    bool key_up,key_right,key_down,key_left;//true if pressed
};

vector<gamepad> g_vec_gamepad;
LPDIRECTINPUT8 g_di_device;

//Direct Input Callbacks
BOOL CALLBACK enumCallback(const DIDEVICEINSTANCE* instance, VOID* context);
//BOOL CALLBACK enumAxesCallback(const DIDEVICEOBJECTINSTANCE* instance, VOID* context);//used for callibration, not needed
HRESULT poll(DIJOYSTATE2 *js,int joystick_index);

int main(int argc, char *argv[])
{
    cout<<"Dance Pad Input Fix\n\n";

    /*//focus stepmania
    HWND hwnd_stepmania;
    hwnd_stepmania = FindWindow(NULL,(LPCTSTR)"StepMania");
    if(hwnd_stepmania != 0)
    {
        cout<<"Found Stepmania\n";
        SetForegroundWindow(hwnd_stepmania);
    }
    else cout<<"Did not find Stepmania\n";

    //test sent keys
    while(true)
    {
        Sleep(1000);
        INPUT _ip;
        _ip.type = INPUT_KEYBOARD;
        _ip.ki.wScan = 0x25; // hardware scan code,   0x11=W,    0x1e=A,      0x1f=S,      0x20=D,
                             //                       0x17=I,    0x24=J,      0x25=K,      0x26=L,
        _ip.ki.time = 0;
        _ip.ki.dwExtraInfo = 0;
        _ip.ki.wVk = 0;//will be ignored by some applications (use wScan), but will work for notepad
        _ip.ki.dwFlags = 0 | KEYEVENTF_SCANCODE;//KEYEVENTF_SCANCODE have to be set to read wScan code
        SendInput(1, &_ip, sizeof(INPUT));
        //release
        _ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE;
        SendInput(1, &_ip, sizeof(INPUT));
    }*/

    //get command for key timeout
    float key_timeout=0.2;//default
    if(argc>1)
    {
        string commandline(argv[1]);
        float input_float=atof(commandline.c_str());
        if(input_float>0.0)
        {
            key_timeout=input_float;
            cout<<"Using key timeout: "<<key_timeout<<endl<<endl;
        }
    }

    //init joysticks
    HWND hwnd_console=GetConsoleWindow();
    HRESULT hr;
    // Create a DirectInput device
    hr=DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION,
                          IID_IDirectInput8, (VOID**)&g_di_device, NULL);
    if(FAILED(hr))
    {
        cout<<"ERROR: DirectInput8Create\n";
        system("PAUSE");
        return hr;
    }
    //cout<<"Direct Input Device created\n";

    //Find joysticks
    //cout<<"Joystick detection\n";
    hr = g_di_device->EnumDevices(DI8DEVCLASS_GAMECTRL, enumCallback,
                                  NULL, DIEDFL_ATTACHEDONLY);
    if(FAILED(hr))
    {
        cout<<"ERROR: EnumDevices\n";
        system("PAUSE");
        return hr;
    }
    // Make sure we got a joystick
    for(int i=0;i<(int)g_vec_gamepad.size();i++)
    {
        if(g_vec_gamepad[i].di_controller == NULL)
        {
            //joystick not found
            cout<<"ERROR: Bad joystick found: "<<i<<endl;
            system("PAUSE");
            return E_FAIL;
        }
    }
    cout<<"Number of Controllers: "<<(int)g_vec_gamepad.size()<<endl;

    //joystick settings
    //cout<<"Joystick settings\n";
    DIDEVCAPS capabilities;

    // Set the data format to "simple joystick" - a predefined data format
    // A data format specifies which controls on a device we are interested in,
    // and how they should be reported. This tells DInput that we will be
    // passing a DIJOYSTATE2 structure to IDirectInputDevice::GetDeviceState().
    for(int i=0;i<(int)g_vec_gamepad.size();i++)
    {
        hr = g_vec_gamepad[i].di_controller->SetDataFormat(&c_dfDIJoystick2);
        if(FAILED(hr))
        {
            cout<<"ERROR: SetDataFormat: "<<i<<endl;
            system("PAUSE");
            return hr;
        }
    }

    // Set the cooperative level to let DInput know how this device should
    // interact with the system and with other DInput applications.
/*FLAGS:
DISCL_BACKGROUND
    The application requires background access.
    If background access is granted, the device can be acquired at any time,
    even when the associated window is not the active window.
DISCL_EXCLUSIVE
    The application requires exclusive access.
    If exclusive access is granted, no other instance of the device can obtain
    exclusive access to the device while it is acquired. However,
    nonexclusive access to the device is always permitted, even if another
    application has obtained exclusive access. An application that acquires
    the mouse or keyboard device in exclusive mode should always unacquire
    the devices when it receives WM_ENTERSIZEMOVE and WM_ENTERMENULOOP
    messages. Otherwise, the user cannot manipulate the menu or move and
    resize the window.
DISCL_FOREGROUND (ERROR if hwnd window is not at the top)
    The application requires foreground access. If foreground access is
    granted, the device is automatically unacquired when the associated
    window moves to the background.
DISCL_NONEXCLUSIVE
    The application requires nonexclusive access. Access to the device does
    not interfere with other applications that are accessing the same device.
DISCL_NOWINKEY
    Disable the Windows logo key. Setting this flag ensures that the user
    cannot inadvertently break out of the application. Note, however, that
    DISCL_NOWINKEY has no effect when the default action mapping user
    interface (UI) is displayed, and the Windows logo key will operate
    normally as long as that UI is present.
*/
    //cout<<"Joystick cooperative level\n";
    for(int i=0;i<(int)g_vec_gamepad.size();i++)
    {
        hr = g_vec_gamepad[i].di_controller->SetCooperativeLevel(hwnd_console, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
        if(FAILED(hr))
        {
            //DIERR_INVALIDPARAM, DIERR_NOTINITIALIZED, E_HANDLE.
            cout<<"ERROR: SetCooperativeLevel: "<<i<<endl;
            if(hr==DIERR_INVALIDPARAM) cout<<"DIERR_INVALIDPARAM\n";
            if(hr==DIERR_NOTINITIALIZED) cout<<"DIERR_NOTINITIALIZED\n";
            if(hr==E_HANDLE) cout<<"E_HANDLE\n";
            system("PAUSE");
            return hr;
        }
    }

    // Determine how many axis the joystick has (so we don't error out setting
    // properties for unavailable axis)
    //cout<<"Joystick axis enumeration\n";
    for(int i=0;i<(int)g_vec_gamepad.size();i++)
    {
        capabilities.dwSize = sizeof(DIDEVCAPS);
        hr = g_vec_gamepad[i].di_controller->GetCapabilities(&capabilities);
        if(FAILED(hr))
        {
            cout<<"ERROR: GetCapabilities: "<<i<<endl;
            system("PAUSE");
            return hr;
        }
    }

    //cout<<"Joystick Initialization Complete\n";


    //select input to follow, if more than 2
    if((int)g_vec_gamepad.size()>2)
    {
        cout<<"\nThe number of inputs is more than 2, select inputs to be used\n";
        int player1_index=-1;
        int player2_index=-1;
        while(true)
        {
            player1_index=-1;
            player2_index=-1;
            cout<<"\nEnter ID for player 1 (0,1,2...): ";
            int input_int=0;
            cin>>input_int;
            if(input_int<(int)g_vec_gamepad.size() && input_int>=0)
            {
                //done with player 1
                player1_index=input_int;
            }
            else
            {
                cout<<"\nThat is not a valid number\n";
                continue;
            }
            //player 2
            cout<<"\nEnter ID for player 2 (0,1,2...): ";
            input_int=0;
            cin>>input_int;
            if(input_int<(int)g_vec_gamepad.size() && input_int>=0 && input_int!=player1_index)
            {
                //done with player 1
                player2_index=input_int;
                cout<<endl;
                break;
            }
            else if(input_int==player1_index)
            {
                cout<<"\nCan not use the same index as the first player\n";
            }
            else
            {
                cout<<"\nThat is not a valid number\n";
            }
        }

        //erase unwanted controllers
        for(int i=0;i<(int)g_vec_gamepad.size();i++)
        {
            if(i!=player1_index && i!=player2_index)
            {
                if(g_vec_gamepad[i].di_controller)
                 g_vec_gamepad[i].di_controller->Unacquire();

                g_vec_gamepad.erase(g_vec_gamepad.begin()+i);
            }
        }
    }

    //test if any inputs
    if(g_vec_gamepad.empty())
    {
        cout<<"\nNo controllers connected\n";
        system("PAUSE");
        return 1;
    }
    bool have_2_players=true;
    if((int)g_vec_gamepad.size()==1)
    {
        //read only WASD player
        have_2_players=false;
    }

    //cout<<"\Initialization complete\n";
    cout<<"\nWill translate controller input to keyboard input...";
    //Set up a generic keyboard event.
    INPUT ip;
    ip.type = INPUT_KEYBOARD;
    ip.ki.wScan = 0; // hardware scan code for key
    ip.ki.time = 0;
    ip.ki.dwExtraInfo = 0;
    //enter input loop
    int   key_state=0;//0 - nothing pressed
    float key_release_timer=key_timeout;
    float time_last_cycle=(float)clock()/CLOCKS_PER_SEC;
    while(true)
    {
        int player_index=0;
        //get controller input
        DIJOYSTATE2 gamepad_data;
        poll(&gamepad_data,player_index);//normal non-flickering
        //check for updated values
        if(g_vec_gamepad[player_index].last_x!=gamepad_data.lX || g_vec_gamepad[player_index].last_y!=gamepad_data.lY)
        {
            //nothing
            if(gamepad_data.lX==32511 && gamepad_data.lY==32511)
            {
                //nothing pressed, release pressed keys
                if(g_vec_gamepad[player_index].key_up)//release w
                {
                    ip.ki.wScan = 0x11;
                    ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_up=false;
                }
                if(g_vec_gamepad[player_index].key_left)//release a
                {
                    ip.ki.wScan = 0x1e;
                    ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_left=false;
                }
                if(g_vec_gamepad[player_index].key_down)//release s
                {
                    ip.ki.wScan = 0x1f;
                    ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_down=false;
                }
                if(g_vec_gamepad[player_index].key_right)//release d
                {
                    ip.ki.wScan = 0x20;
                    ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_right=false;
                }
            }
            //up
            if(gamepad_data.lX==32511 && gamepad_data.lY==0)
            {
                //Press the "W" key
                ip.ki.wScan = 0x11;
                ip.ki.dwFlags = 0 | KEYEVENTF_SCANCODE;
                SendInput(1, &ip, sizeof(INPUT));
                g_vec_gamepad[player_index].key_up=true;
                //release others
                if(g_vec_gamepad[player_index].key_left)//release a
                {
                    ip.ki.wScan = 0x1e;
                    ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_left=false;
                }
                if(g_vec_gamepad[player_index].key_down)//release s
                {
                    ip.ki.wScan = 0x1f;
                    ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_down=false;
                }
                if(g_vec_gamepad[player_index].key_right)//release d
                {
                    ip.ki.wScan = 0x20;
                    ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_right=false;
                }
            }
            //down
            if(gamepad_data.lX==32511 && gamepad_data.lY==65535)
            {
                //Press the "S" key
                ip.ki.wScan = 0x1f;
                ip.ki.dwFlags = 0 | KEYEVENTF_SCANCODE;
                SendInput(1, &ip, sizeof(INPUT));
                g_vec_gamepad[player_index].key_down=true;
                if(g_vec_gamepad[player_index].key_up)//release w
                {
                    ip.ki.wScan = 0x11;
                    ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_up=false;
                }
                if(g_vec_gamepad[player_index].key_left)//release a
                {
                    ip.ki.wScan = 0x1e;
                    ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_left=false;
                }
                if(g_vec_gamepad[player_index].key_right)//release d
                {
                    ip.ki.wScan = 0x20;
                    ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_right=false;
                }
            }
            //left
            if(gamepad_data.lX==0 && gamepad_data.lY==32511)
            {
                //Press the "A" key
                ip.ki.wScan = 0x1e;
                ip.ki.dwFlags = 0 | KEYEVENTF_SCANCODE;
                SendInput(1, &ip, sizeof(INPUT));
                g_vec_gamepad[player_index].key_left=true;
                if(g_vec_gamepad[player_index].key_up)//release w
                {
                    ip.ki.wScan = 0x11;
                    ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_up=false;
                }
                if(g_vec_gamepad[player_index].key_down)//release s
                {
                    ip.ki.wScan = 0x1f;
                    ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_down=false;
                }
                if(g_vec_gamepad[player_index].key_right)//release d
                {
                    ip.ki.wScan = 0x20;
                    ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_right=false;
                }
            }
            //right
            if(gamepad_data.lX==65535 && gamepad_data.lY==32511)
            {
                //Press the "D" key
                ip.ki.wScan = 0x20;
                ip.ki.dwFlags = 0 | KEYEVENTF_SCANCODE;
                SendInput(1, &ip, sizeof(INPUT));
                g_vec_gamepad[player_index].key_right=true;
                if(g_vec_gamepad[player_index].key_up)//release w
                {
                    ip.ki.wScan = 0x11;
                    ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_up=false;
                }
                if(g_vec_gamepad[player_index].key_left)//release a
                {
                    ip.ki.wScan = 0x1e;
                    ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_left=false;
                }
                if(g_vec_gamepad[player_index].key_down)//release s
                {
                    ip.ki.wScan = 0x1f;
                    ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_down=false;
                }
            }

            //up+down
            if(gamepad_data.lX==32511 && gamepad_data.lY==32767)
            {
                //Press the "W" key
                ip.ki.wScan = 0x11;
                ip.ki.dwFlags = 0 | KEYEVENTF_SCANCODE;
                SendInput(1, &ip, sizeof(INPUT));
                g_vec_gamepad[player_index].key_up=true;
                //Press the "S" key
                ip.ki.wScan = 0x1f;
                ip.ki.dwFlags = 0 | KEYEVENTF_SCANCODE;
                SendInput(1, &ip, sizeof(INPUT));
                g_vec_gamepad[player_index].key_down=true;

                if(g_vec_gamepad[player_index].key_left)//release a
                {
                    ip.ki.wScan = 0x1e;
                    ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_left=false;
                }
                if(g_vec_gamepad[player_index].key_right)//release d
                {
                    ip.ki.wScan = 0x20;
                    ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_right=false;
                }
            }
            //up+left
            if(gamepad_data.lX==0 && gamepad_data.lY==0)
            {
                //Press the "W" key
                ip.ki.wScan = 0x11;
                ip.ki.dwFlags = 0 | KEYEVENTF_SCANCODE;
                SendInput(1, &ip, sizeof(INPUT));
                g_vec_gamepad[player_index].key_up=true;
                //Press the "A" key
                ip.ki.wScan = 0x1e;
                ip.ki.dwFlags = 0 | KEYEVENTF_SCANCODE;
                SendInput(1, &ip, sizeof(INPUT));
                g_vec_gamepad[player_index].key_left=true;

                if(g_vec_gamepad[player_index].key_down)//release s
                {
                    ip.ki.wScan = 0x1f;
                    ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_down=false;
                }
                if(g_vec_gamepad[player_index].key_right)//release d
                {
                    ip.ki.wScan = 0x20;
                    ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_right=false;
                }
            }
            //up+right
            if(gamepad_data.lX==65535 && gamepad_data.lY==0)
            {
                //Press the "W" key
                ip.ki.wScan = 0x11;
                ip.ki.dwFlags = 0 | KEYEVENTF_SCANCODE;
                SendInput(1, &ip, sizeof(INPUT));
                g_vec_gamepad[player_index].key_up=true;
                //Press the "D" key
                ip.ki.wScan = 0x20;
                ip.ki.dwFlags = 0 | KEYEVENTF_SCANCODE;
                SendInput(1, &ip, sizeof(INPUT));
                g_vec_gamepad[player_index].key_right=true;

                if(g_vec_gamepad[player_index].key_left)//release a
                {
                    ip.ki.wScan = 0x1e;
                    ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_left=false;
                }
                if(g_vec_gamepad[player_index].key_down)//release s
                {
                    ip.ki.wScan = 0x1f;
                    ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_down=false;
                }
            }
            //left+right
            if(gamepad_data.lX==32767 && gamepad_data.lY==32511)
            {
                //Press the "A" key
                ip.ki.wScan = 0x1e;
                ip.ki.dwFlags = 0 | KEYEVENTF_SCANCODE;
                SendInput(1, &ip, sizeof(INPUT));
                g_vec_gamepad[player_index].key_left=true;
                //Press the "D" key
                ip.ki.wScan = 0x20;
                ip.ki.dwFlags = 0 | KEYEVENTF_SCANCODE;
                SendInput(1, &ip, sizeof(INPUT));
                g_vec_gamepad[player_index].key_right=true;

                if(g_vec_gamepad[player_index].key_up)//release w
                {
                    ip.ki.wScan = 0x11;
                    ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_up=false;
                }
                if(g_vec_gamepad[player_index].key_down)//release s
                {
                    ip.ki.wScan = 0x1f;
                    ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_down=false;
                }
            }
            //left+down
            if(gamepad_data.lX==0 && gamepad_data.lY==65535)
            {
                //Press the "A" key
                ip.ki.wScan = 0x1e;
                ip.ki.dwFlags = 0 | KEYEVENTF_SCANCODE;
                SendInput(1, &ip, sizeof(INPUT));
                g_vec_gamepad[player_index].key_left=true;
                //Press the "S" key
                ip.ki.wScan = 0x1f;
                ip.ki.dwFlags = 0 | KEYEVENTF_SCANCODE;
                SendInput(1, &ip, sizeof(INPUT));
                g_vec_gamepad[player_index].key_down=true;

                if(g_vec_gamepad[player_index].key_up)//release w
                {
                    ip.ki.wScan = 0x11;
                    ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_up=false;
                }
                if(g_vec_gamepad[player_index].key_right)//release d
                {
                    ip.ki.wScan = 0x20;
                    ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_right=false;
                }
            }
            //down+right
            if(gamepad_data.lX==65536 && gamepad_data.lY==65535)
            {
                //Press the "S" key
                ip.ki.wScan = 0x1f;
                ip.ki.dwFlags = 0 | KEYEVENTF_SCANCODE;
                SendInput(1, &ip, sizeof(INPUT));
                g_vec_gamepad[player_index].key_down=true;
                //Press the "D" key
                ip.ki.wScan = 0x20;
                ip.ki.dwFlags = 0 | KEYEVENTF_SCANCODE;
                SendInput(1, &ip, sizeof(INPUT));
                g_vec_gamepad[player_index].key_right=true;

                if(g_vec_gamepad[player_index].key_up)//release w
                {
                    ip.ki.wScan = 0x11;
                    ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_up=false;
                }
                if(g_vec_gamepad[0].key_left)//release a
                {
                    ip.ki.wScan = 0x1e;
                    ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_left=false;
                }
            }

            g_vec_gamepad[player_index].last_x=gamepad_data.lX;
            g_vec_gamepad[player_index].last_y=gamepad_data.lY;
        }

        //check player 2 -----------------------------------------------
        if(have_2_players)
        {
            player_index=1;
            poll(&gamepad_data,player_index);//flickering input with timeout

            float time_now=(float)clock()/CLOCKS_PER_SEC;
            key_release_timer-=time_now-time_last_cycle;
            time_last_cycle=time_now;
            //release buttons if timeout
            if(key_release_timer<0.0 && key_state!=0)
            {
                if(g_vec_gamepad[player_index].key_up)//release I
                {
                    ip.ki.wScan = 0x17;
                    ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_up=false;
                }
                if(g_vec_gamepad[player_index].key_left)//release J
                {
                    ip.ki.wScan = 0x24;
                    ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_left=false;
                }
                if(g_vec_gamepad[player_index].key_down)//release K
                {
                    ip.ki.wScan = 0x25;
                    ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_down=false;
                }
                if(g_vec_gamepad[player_index].key_right)//release L
                {
                    ip.ki.wScan = 0x26;
                    ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_right=false;
                }

                key_release_timer=key_timeout;
                key_state=0;
                //set last x/y to release values
                g_vec_gamepad[player_index].last_x=32511;
                g_vec_gamepad[player_index].last_y=32511;
            }

            //check for updated values
            if(g_vec_gamepad[player_index].last_x!=gamepad_data.lX || g_vec_gamepad[player_index].last_y!=gamepad_data.lY)
            {
                bool update_last_x_y=true;
                //nothing, will cause flickering if on
                if(gamepad_data.lX==32511 && gamepad_data.lY==32511)
                {
                    //this signal is from flickering, do not update last x/y
                    update_last_x_y=false;

                    /*This will cause flickering, keys will be reset in timeout above
                    //nothing pressed, release pressed keys
                    if(g_vec_gamepad[player_index].key_up)//release I
                    {
                        ip.ki.wScan = 0x17;
                        ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                        SendInput(1, &ip, sizeof(INPUT));
                        g_vec_gamepad[player_index].key_up=false;
                    }
                    if(g_vec_gamepad[player_index].key_left)//release J
                    {
                        ip.ki.wScan = 0x24;
                        ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                        SendInput(1, &ip, sizeof(INPUT));
                        g_vec_gamepad[player_index].key_left=false;
                    }
                    if(g_vec_gamepad[player_index].key_down)//release K
                    {
                        ip.ki.wScan = 0x25;
                        ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                        SendInput(1, &ip, sizeof(INPUT));
                        g_vec_gamepad[player_index].key_down=false;
                    }
                    if(g_vec_gamepad[player_index].key_right)//release L
                    {
                        ip.ki.wScan = 0x26;
                        ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                        SendInput(1, &ip, sizeof(INPUT));
                        g_vec_gamepad[player_index].key_right=false;
                    }*/
                }
                //up
                if(gamepad_data.lX==32511 && gamepad_data.lY==0)
                {
                    key_state=1;
                    //Press the "I" key
                    ip.ki.wScan = 0x17;
                    ip.ki.dwFlags = 0 | KEYEVENTF_SCANCODE;
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_up=true;
                    //release others
                    if(g_vec_gamepad[player_index].key_left)//release J
                    {
                        ip.ki.wScan = 0x24;
                        ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                        SendInput(1, &ip, sizeof(INPUT));
                        g_vec_gamepad[player_index].key_left=false;
                    }
                    if(g_vec_gamepad[player_index].key_down)//release K
                    {
                        ip.ki.wScan = 0x25;
                        ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                        SendInput(1, &ip, sizeof(INPUT));
                        g_vec_gamepad[player_index].key_down=false;
                    }
                    if(g_vec_gamepad[player_index].key_right)//release L
                    {
                        ip.ki.wScan = 0x20;
                        ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                        SendInput(1, &ip, sizeof(INPUT));
                        g_vec_gamepad[player_index].key_right=false;
                    }
                }
                //down
                if(gamepad_data.lX==32511 && gamepad_data.lY==65535)
                {
                    key_state=2;
                    //Press the "K" key
                    ip.ki.wScan = 0x25;
                    ip.ki.dwFlags = 0 | KEYEVENTF_SCANCODE;
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_down=true;
                    if(g_vec_gamepad[player_index].key_up)//release I
                    {
                        ip.ki.wScan = 0x17;
                        ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                        SendInput(1, &ip, sizeof(INPUT));
                        g_vec_gamepad[player_index].key_up=false;
                    }
                    if(g_vec_gamepad[player_index].key_left)//release J
                    {
                        ip.ki.wScan = 0x17;
                        ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                        SendInput(1, &ip, sizeof(INPUT));
                        g_vec_gamepad[player_index].key_left=false;
                    }
                    if(g_vec_gamepad[player_index].key_right)//release L
                    {
                        ip.ki.wScan = 0x26;
                        ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                        SendInput(1, &ip, sizeof(INPUT));
                        g_vec_gamepad[player_index].key_right=false;
                    }
                }
                //left
                if(gamepad_data.lX==0 && gamepad_data.lY==32511)
                {
                    key_state=3;
                    //Press the "J" key
                    ip.ki.wScan = 0x24;
                    ip.ki.dwFlags = 0 | KEYEVENTF_SCANCODE;
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_left=true;
                    if(g_vec_gamepad[player_index].key_up)//release I
                    {
                        ip.ki.wScan = 0x17;
                        ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                        SendInput(1, &ip, sizeof(INPUT));
                        g_vec_gamepad[player_index].key_up=false;
                    }
                    if(g_vec_gamepad[player_index].key_down)//release K
                    {
                        ip.ki.wScan = 0x25;
                        ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                        SendInput(1, &ip, sizeof(INPUT));
                        g_vec_gamepad[player_index].key_down=false;
                    }
                    if(g_vec_gamepad[player_index].key_right)//release L
                    {
                        ip.ki.wScan = 0x26;
                        ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                        SendInput(1, &ip, sizeof(INPUT));
                        g_vec_gamepad[player_index].key_right=false;
                    }
                }
                //right
                if(gamepad_data.lX==65535 && gamepad_data.lY==32511)
                {
                    key_state=4;
                    //Press the "L" key
                    ip.ki.wScan = 0x26;
                    ip.ki.dwFlags = 0 | KEYEVENTF_SCANCODE;
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_right=true;
                    if(g_vec_gamepad[player_index].key_up)//release I
                    {
                        ip.ki.wScan = 0x17;
                        ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                        SendInput(1, &ip, sizeof(INPUT));
                        g_vec_gamepad[player_index].key_up=false;
                    }
                    if(g_vec_gamepad[player_index].key_left)//release J
                    {
                        ip.ki.wScan = 0x17;
                        ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                        SendInput(1, &ip, sizeof(INPUT));
                        g_vec_gamepad[player_index].key_left=false;
                    }
                    if(g_vec_gamepad[player_index].key_down)//release K
                    {
                        ip.ki.wScan = 0x25;
                        ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                        SendInput(1, &ip, sizeof(INPUT));
                        g_vec_gamepad[player_index].key_down=false;
                    }
                }

                //up+down
                if(gamepad_data.lX==32511 && gamepad_data.lY==32767)
                {
                    key_state=5;
                    //Press the "I" key
                    ip.ki.wScan = 0x17;
                    ip.ki.dwFlags = 0 | KEYEVENTF_SCANCODE;
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_up=true;
                    //Press the "K" key
                    ip.ki.wScan = 0x25;
                    ip.ki.dwFlags = 0 | KEYEVENTF_SCANCODE;
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_down=true;

                    if(g_vec_gamepad[player_index].key_left)//release J
                    {
                        ip.ki.wScan = 0x17;
                        ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                        SendInput(1, &ip, sizeof(INPUT));
                        g_vec_gamepad[player_index].key_left=false;
                    }
                    if(g_vec_gamepad[player_index].key_right)//release L
                    {
                        ip.ki.wScan = 0x26;
                        ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                        SendInput(1, &ip, sizeof(INPUT));
                        g_vec_gamepad[player_index].key_right=false;
                    }
                }
                //up+left
                if(gamepad_data.lX==0 && gamepad_data.lY==0)
                {
                    key_state=6;
                    //Press the "I" key
                    ip.ki.wScan = 0x17;
                    ip.ki.dwFlags = 0 | KEYEVENTF_SCANCODE;
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_up=true;
                    //Press the "J" key
                    ip.ki.wScan = 0x24;
                    ip.ki.dwFlags = 0 | KEYEVENTF_SCANCODE;
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_left=true;

                    if(g_vec_gamepad[player_index].key_down)//release K
                    {
                        ip.ki.wScan = 0x25;
                        ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                        SendInput(1, &ip, sizeof(INPUT));
                        g_vec_gamepad[player_index].key_down=false;
                    }
                    if(g_vec_gamepad[player_index].key_right)//release L
                    {
                        ip.ki.wScan = 0x26;
                        ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                        SendInput(1, &ip, sizeof(INPUT));
                        g_vec_gamepad[player_index].key_right=false;
                    }
                }
                //up+right
                if(gamepad_data.lX==65535 && gamepad_data.lY==0)
                {
                    key_state=7;
                    //Press the "I" key
                    ip.ki.wScan = 0x17;
                    ip.ki.dwFlags = 0 | KEYEVENTF_SCANCODE;
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_up=true;
                    //Press the "L" key
                    ip.ki.wScan = 0x26;
                    ip.ki.dwFlags = 0 | KEYEVENTF_SCANCODE;
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_right=true;

                    if(g_vec_gamepad[player_index].key_left)//release J
                    {
                        ip.ki.wScan = 0x17;
                        ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                        SendInput(1, &ip, sizeof(INPUT));
                        g_vec_gamepad[player_index].key_left=false;
                    }
                    if(g_vec_gamepad[player_index].key_down)//release K
                    {
                        ip.ki.wScan = 0x25;
                        ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                        SendInput(1, &ip, sizeof(INPUT));
                        g_vec_gamepad[player_index].key_down=false;
                    }
                }
                //left+right
                if(gamepad_data.lX==32767 && gamepad_data.lY==32511)
                {
                    key_state=8;
                    //Press the "J" key
                    ip.ki.wScan = 0x24;
                    ip.ki.dwFlags = 0 | KEYEVENTF_SCANCODE;
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_left=true;
                    //Press the "L" key
                    ip.ki.wScan = 0x26;
                    ip.ki.dwFlags = 0 | KEYEVENTF_SCANCODE;
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_right=true;

                    if(g_vec_gamepad[player_index].key_up)//release I
                    {
                        ip.ki.wScan = 0x17;
                        ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                        SendInput(1, &ip, sizeof(INPUT));
                        g_vec_gamepad[player_index].key_up=false;
                    }
                    if(g_vec_gamepad[player_index].key_down)//release K
                    {
                        ip.ki.wScan = 0x25;
                        ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                        SendInput(1, &ip, sizeof(INPUT));
                        g_vec_gamepad[player_index].key_down=false;
                    }
                }
                //left+down
                if(gamepad_data.lX==0 && gamepad_data.lY==65535)
                {
                    key_state=9;
                    //Press the "J" key
                    ip.ki.wScan = 0x24;
                    ip.ki.dwFlags = 0 | KEYEVENTF_SCANCODE;
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_left=true;
                    //Press the "K" key
                    ip.ki.wScan = 0x25;
                    ip.ki.dwFlags = 0 | KEYEVENTF_SCANCODE;
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_down=true;

                    if(g_vec_gamepad[player_index].key_up)//release I
                    {
                        ip.ki.wScan = 0x17;
                        ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                        SendInput(1, &ip, sizeof(INPUT));
                        g_vec_gamepad[player_index].key_up=false;
                    }
                    if(g_vec_gamepad[player_index].key_right)//release L
                    {
                        ip.ki.wScan = 0x26;
                        ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                        SendInput(1, &ip, sizeof(INPUT));
                        g_vec_gamepad[player_index].key_right=false;
                    }
                }
                //down+right
                if(gamepad_data.lX==65536 && gamepad_data.lY==65535)
                {
                    key_state=10;
                    //Press the "K" key
                    ip.ki.wScan = 0x25;
                    ip.ki.dwFlags = 0 | KEYEVENTF_SCANCODE;
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_down=true;
                    //Press the "L" key
                    ip.ki.wScan = 0x26;
                    ip.ki.dwFlags = 0 | KEYEVENTF_SCANCODE;
                    SendInput(1, &ip, sizeof(INPUT));
                    g_vec_gamepad[player_index].key_right=true;

                    if(g_vec_gamepad[player_index].key_up)//release I
                    {
                        ip.ki.wScan = 0x17;
                        ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                        SendInput(1, &ip, sizeof(INPUT));
                        g_vec_gamepad[player_index].key_up=false;
                    }
                    if(g_vec_gamepad[0].key_left)//release J
                    {
                        ip.ki.wScan = 0x17;
                        ip.ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_SCANCODE; // KEYEVENTF_KEYUP for key release
                        SendInput(1, &ip, sizeof(INPUT));
                        g_vec_gamepad[player_index].key_left=false;
                    }
                }

                if(update_last_x_y)
                {
                    g_vec_gamepad[player_index].last_x=gamepad_data.lX;
                    g_vec_gamepad[player_index].last_y=gamepad_data.lY;

                    //reset timer
                    key_release_timer=key_timeout;
                }
            }
            //if current value is the same as last, reset timer (key is still pressed)
            else key_release_timer=key_timeout;
        }
    }
    cout<<"Shutdown\n";

    //remove the joystick
    for(int i=0;i<(int)g_vec_gamepad.size();i++)
    {
        if(g_vec_gamepad[i].di_controller)
        {
            g_vec_gamepad[i].di_controller->Unacquire();
        }
    }

    return 0;
}

BOOL CALLBACK
enumCallback(const DIDEVICEINSTANCE* instance, VOID* context)
{
    HRESULT hr;

    //store all joysticks
    g_vec_gamepad.push_back( gamepad() );

    // Obtain an interface to the enumerated joystick.
    hr = g_di_device->CreateDevice(instance->guidInstance, &g_vec_gamepad.back().di_controller, NULL);

    // If it failed, then we can't use this joystick. (Maybe the user unplugged
    // it while we were in the middle of enumerating it.)
    if(FAILED(hr))
    {
        //remove that joystick
        g_vec_gamepad.pop_back();

        return DIENUM_CONTINUE;
    }

    return DIENUM_CONTINUE;
    // Stop enumeration. Note: we're just taking the first joystick we get. You
    // could store all the enumerated joysticks and let the user pick.
    //return DIENUM_STOP;
}

/*BOOL CALLBACK
enumAxesCallback(const DIDEVICEOBJECTINSTANCE* instance, VOID* context)
{
    HWND hDlg = (HWND)context;

    DIPROPRANGE propRange;
    propRange.diph.dwSize       = sizeof(DIPROPRANGE);
    propRange.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    propRange.diph.dwHow        = DIPH_BYID;
    propRange.diph.dwObj        = instance->dwType;
    propRange.lMin              = -1000;
    propRange.lMax              = +1000;

    // Set the range for the axis
    HRESULT hr;
    hr = joystick->SetProperty(DIPROP_RANGE, &propRange.diph);
    if (FAILED(hr))
    {
        return DIENUM_STOP;
    }

    return DIENUM_CONTINUE;
}*/

HRESULT
poll(DIJOYSTATE2 *js,int joystick_index)
{
    HRESULT hr;

    if(g_vec_gamepad[joystick_index].di_controller == NULL)
    {
        return S_OK;
    }


    // Poll the device to read the current state
    hr = g_vec_gamepad[joystick_index].di_controller->Poll();
    if(FAILED(hr))
    {
        // DInput is telling us that the input stream has been
        // interrupted. We aren't tracking any state between polls, so
        // we don't have any special reset that needs to be done. We
        // just re-acquire and try again.
        hr = g_vec_gamepad[joystick_index].di_controller->Acquire();
        while(hr == DIERR_INPUTLOST)
        {
            hr = g_vec_gamepad[joystick_index].di_controller->Acquire();
        }

        // If we encounter a fatal error, return failure.
        if((hr == DIERR_INVALIDPARAM) || (hr == DIERR_NOTINITIALIZED))
        {
            return E_FAIL;
        }

        // If another application has control of this device, return successfully.
        // We'll just have to wait our turn to use the joystick.
        if(hr == DIERR_OTHERAPPHASPRIO)
        {
            return S_OK;
        }
    }

    // Get the input's device state
    hr = g_vec_gamepad[joystick_index].di_controller->GetDeviceState(sizeof(DIJOYSTATE2), js);
    if(FAILED(hr))
    {
        return hr; // The device should have been acquired during the Poll()
    }

    return S_OK;
}
