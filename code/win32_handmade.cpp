/********************************************************************
 *
 *Author:  Rob Graham.
 *Created: Fri May 26 20:13:47 2017
 *Notice:  (C) Copyright 2017-2018 by Rob Graham. All Rights Reserved.
 *
********************************************************************/

/*
TODO(Rob): This is not final platform layer

- Saved game locations
- Getting handle to our own executable file
- Asset loading path
- Threading (how to launch a thread)
- Raw input (support multiple keyboards)
- Sleep/timeBeginPeriod
- Clip cursor for multi-monitor support
- Fullscreen support
- WM_SETCURSOR to control cursor visibility
- QueryCancelAutoplay
- WM_ACTIVATEAPP for when we're not the active application
- Blit speed improvements
- Hardware acceleration (OpenGL / Direct3D)
- GetKeyboardLayout for international keyboards

 Just a partial list of stuff.
*/

#include "handmade_platform.h"

#include <malloc.h>
#include <windows.h>
#include <stdio.h>
#include <xinput.h>
#include <dsound.h>

#include "win32_handmade.h"

// TODO(Rob): This is a global for now.
global_variable bool32 GlobalRunning;
global_variable bool32 GlobalPause;
global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
global_variable int64 GlobalPerfCountFrequency;
global_variable bool32 DEBUGGlobalShowCursor;
global_variable WINDOWPLACEMENT GlobalWindowPosition = {sizeof(GlobalWindowPosition)};

// NOTE(Rob): XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* State)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return (ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_get_state* XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// NOTE(Rob): XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* Vibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return (ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_set_state* XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

// NOTE(Rob): DirectSoundCreate
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);;

internal void
ToggleFullscreen(HWND Window)
{
    DWORD Style = GetWindowLong(Window, GWL_STYLE);
    if (Style & WS_OVERLAPPEDWINDOW)
    {
        MONITORINFO MonitorInfo = {sizeof(MonitorInfo)};
        if (GetWindowPlacement(Window, &GlobalWindowPosition) && 
            GetMonitorInfo(MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo))
        {
            SetWindowLong(Window, GWL_STYLE, Style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(Window, HWND_TOP, 
                         MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
                         MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
                         MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else
    {
        SetWindowLong(Window, GWL_STYLE, Style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(Window, &GlobalWindowPosition);
        SetWindowPos(Window, 0, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

internal int
StringLength(char *String)
{
    int Count = 0;
    while (*String++)
    {
        ++Count;
    }
    return (Count);
}

internal void
CatStrings(size_t SourceACount, char *SourceA,
           size_t SourceBCount, char *SourceB,
           size_t DestCount, char *Dest)
{
    for (int Index = 0; Index < SourceACount; ++Index)
    {
        *Dest++ = *SourceA++;
    }
    for (int Index = 0; Index < SourceBCount; ++Index)
    {
        *Dest++ = *SourceB++;
    }
    
    *Dest++ = 0;
}

internal void
Win32BuildEXEPathFilename(win32_state *State, char *FileName,
                          int DestCount, char *Dest)
{
    CatStrings(State->OnePastLastEXEFileNameSlash - State->EXEFileName, State->EXEFileName,
               StringLength(FileName), FileName,
               DestCount, Dest);
}

DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory)
{
    if (Memory)
    {
        VirtualFree(Memory, 0 , MEM_RELEASE);
    }
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile)
{
    debug_read_file_result Result = {};
    
    HANDLE FileHandle = CreateFileA(Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize;
        if (GetFileSizeEx(FileHandle, &FileSize))
        {
            uint32 FileSize32 = SafeTruncateSizeUint64(FileSize.QuadPart);
            Result.Contents = VirtualAlloc(0, FileSize32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if (Result.Contents)
            {
                DWORD BytesRead;
                if (ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) && 
                    (FileSize32 == BytesRead))
                {
                    Result.ContentsSize = FileSize32;
                }
                else
                {
                    DEBUGPlatformFreeFileMemory(Thread, Result.Contents);
                    Result.Contents = 0;
                }
            }
            else
            {
            }
        }
        CloseHandle(FileHandle);
    }
    else
    {
    }
    
    return (Result);
}

DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile)
{
    bool32 Result = false;
    HANDLE FileHandle = CreateFileA(Filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD BytesWritten;
        if (WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0))
        {
            Result = (BytesWritten == MemorySize);
        }
        else 
        {
        }
        
        CloseHandle(FileHandle);
    }
    else
    {
    }
    
    return (Result);
}

inline FILETIME
Win32GetLastWriteTime(char *Filename)
{
    FILETIME LastWriteTime = {};
    
    WIN32_FILE_ATTRIBUTE_DATA Data;
    if (GetFileAttributesEx(Filename, GetFileExInfoStandard, &Data))
    {
        LastWriteTime = Data.ftLastWriteTime;
    }
    
    return (LastWriteTime);
}

internal win32_game_code
Win32LoadGameCode(char *Filename, char *TempDLLName, char *LockFileName)
{
    win32_game_code Result = {};
    
    WIN32_FILE_ATTRIBUTE_DATA Ignored;
    if (!GetFileAttributesEx(LockFileName, GetFileExInfoStandard, &Ignored))
    {
        Result.DLLLastWriteTime = Win32GetLastWriteTime(Filename);
        
        CopyFile(Filename, TempDLLName, FALSE);
        
        Result.GameCodeDLL = LoadLibraryA(TempDLLName);
        if (Result.GameCodeDLL)
        {
            Result.UpdateAndRender = (game_update_and_render*)GetProcAddress(Result.GameCodeDLL, "GameUpdateAndRender");
            Result.GetSoundSamples = (game_get_sounds_samples*)GetProcAddress(Result.GameCodeDLL, "GameGetSoundSamples");
            
            Result.IsValid = (Result.UpdateAndRender && Result.GetSoundSamples);
        }
    }
    
    if (!Result.IsValid)
    {
        Result.UpdateAndRender = 0;
        Result.GetSoundSamples = 0;
    }
    
    return (Result);
}

internal void
Win32UnloadGameCode(win32_game_code *GameCode)
{
    if (GameCode->GameCodeDLL)
    {
        FreeLibrary(GameCode->GameCodeDLL);
        GameCode->GameCodeDLL = 0;
    }
    
    GameCode->IsValid = false;
    GameCode->UpdateAndRender = 0;
    GameCode->GetSoundSamples = 0;
}

internal void
Win32LoadXInput()
{
    // TODO(Rob): Test this on windows 8.
    HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
    if (!XInputLibrary)
    {
        // TODO(Rob): Diagnostic.
        LoadLibraryA("xinput9_1_0.dll");
    }
    
    if (!XInputLibrary)
    {
        // TODO(Rob): Diagnostic.
        LoadLibraryA("xinput1_3.dll");
    }
    
    if (XInputLibrary)
    {
        XInputGetState = (x_input_get_state*)GetProcAddress(XInputLibrary, "XInputGetState");
        if (!XInputGetState) {XInputGetState = XInputGetStateStub;}
        
        XInputSetState = (x_input_set_state*)GetProcAddress(XInputLibrary, "XInputSetState");
        if (!XInputSetState) {XInputSetState = XInputSetStateStub;}
        
        // NOTE(Rob): Diagnostic.
    }
    else
    {
        // TODO(Rob): Diagnostic.
    }
}

internal void
Win32InitDSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize)
{
    // NOTE(Rob): Load the library.
    HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
    if (DSoundLibrary)
    {
        // NOTE(Rob): Get a directsound object.
        direct_sound_create* DirectSoundCreate = (direct_sound_create*)
            GetProcAddress(DSoundLibrary, "DirectSoundCreate");
        
        // TODO(Rob): Double check with works on XP.
        LPDIRECTSOUND DirectSound;
        if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
        {
            WAVEFORMATEX WaveFormat = {};
            WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
            WaveFormat.nChannels = 2;
            WaveFormat.wBitsPerSample = 16;
            WaveFormat.nSamplesPerSec = SamplesPerSecond;
            WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
            WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
            WaveFormat.cbSize = 0;
            
            if (SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
            {
                // NOTE(Rob): "Create" a primary Buffer.
                DSBUFFERDESC BufferDescription = {};
                BufferDescription.dwSize = sizeof(BufferDescription);
                BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
                
                // TODO(Rob): DSBCAPS_GLOBALFOCUS?
                
                LPDIRECTSOUNDBUFFER primaryBuffer;
                if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &primaryBuffer, 0)))
                {
                    if (SUCCEEDED(primaryBuffer->SetFormat(&WaveFormat)))
                    {
                        // NOTE(Rob): We have set the format.
                        OutputDebugStringA("Primary Buffer format set.\n");
                    }
                    else
                    {
                        // TODO(Rob): Diagnostic.
                    }
                    
                }
                else
                {
                    // NOTE(Rob): Diagnostic.
                }
            }
            else
            {
                // TODO(Rob): Diagnostic.
            }
            
            // NOTE(Rob): "Create" a secondary nuffer.
            DSBUFFERDESC BufferDescription = {};
            BufferDescription.dwSize = sizeof(BufferDescription);
            BufferDescription.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
            BufferDescription.dwBufferBytes = BufferSize;
            BufferDescription.lpwfxFormat = &WaveFormat;
            
            if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0)))
            {
                OutputDebugStringA("Secondary Buffer create successfully.\n");
            }
        }
        else
        {
            // TODO(Rob): Diagnostic
        }
    }
    else
    {
        // TODO(Rob): Diagnostic
    }
}

internal win32_window_dimension
Win32GetWindowDimension(HWND Window)
{
    win32_window_dimension Result;
    
    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;
    
    return (Result);
}

internal void
Win32ResizeDIBSection(win32_offscreen_buffer* Buffer, int Width, int Height)
{
    // TODO(Rob): Bulletproof this.
    // Maybe don't free first, free after, then free first if that fails.
    
    if (Buffer->Memory)
    {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }
    
    Buffer->Width = Width;
    Buffer->Height = Height;
    
    int BytesPerPixel = 4;
    Buffer->BytesPerPixel = BytesPerPixel;
    
    // NOTE(Rob): When the biHeight field is negative, this is the clue to windows
    // to treat this bitmap as top-down, the pixel coordinates start top-left.
    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;
    
    // Allocate memory ourselves.
    int BitmapMemorySize = (Buffer->Width * Buffer->Height) * BytesPerPixel;
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    
    Buffer->Pitch = Width * BytesPerPixel;
    
    // TODO(Rob): PRobably want to clear this to black.
}

internal void
Win32DisplayBufferInWindow(win32_offscreen_buffer* Buffer,
                           HDC DeviceContext, int WindowWidth, int WindowHeight)
{
    if ((WindowWidth >= Buffer->Width*2) &&
        (WindowHeight >= Buffer->Height*2))
    {
        int OffsetX = (WindowWidth - (2*Buffer->Width)) / 2;
        int OffsetY = (WindowHeight - (2*Buffer->Height)) / 2;
        
        StretchDIBits(DeviceContext,
                      OffsetX, OffsetY, 2*Buffer->Width, 2*Buffer->Height,
                      0, 0, Buffer->Width, Buffer->Height,
                      Buffer->Memory,
                      &Buffer->Info,
                      DIB_RGB_COLORS, SRCCOPY);
    }
    else
    {
        int OffsetX = 10;
        int OffsetY = 10;
        
        PatBlt(DeviceContext, 0, 0, WindowWidth, OffsetY, BLACKNESS);
        PatBlt(DeviceContext, 0, OffsetY + Buffer->Height, WindowWidth, WindowHeight, BLACKNESS);
        PatBlt(DeviceContext, 0, 0, OffsetX, WindowHeight, BLACKNESS);
        PatBlt(DeviceContext, OffsetX + Buffer->Width, 0, WindowWidth, WindowHeight, BLACKNESS);
        
        // TODO(Rob): Aspect ratio correction.
        StretchDIBits(DeviceContext,
                      OffsetX, OffsetY, Buffer->Width, Buffer->Height,
                      0, 0, Buffer->Width, Buffer->Height,
                      Buffer->Memory,
                      &Buffer->Info,
                      DIB_RGB_COLORS, SRCCOPY);
    }
}

LRESULT CALLBACK
Win32MainWindowCallback(HWND Window,
                        UINT Message,
                        WPARAM WParam,
                        LPARAM LParam)
{
    LRESULT Result = 0;
    
    switch (Message)
    {
        case WM_CLOSE:
        {
            // TODO(Rob): Handle this with a Message to the user.
            GlobalRunning = false;
        } break;
        
        case WM_SETCURSOR:
        {
            if (DEBUGGlobalShowCursor)
            {
                Result = DefWindowProcA(Window, Message, WParam, LParam);
            }
            else
            {
                SetCursor(0);
            }
        } break;
        
        case WM_ACTIVATEAPP:
        {
#if 0
            if (WParam == TRUE)
            {
                SetLayeredWindowAttributes(Window, RGB(0, 0, 0), 255, LWA_ALPHA);
            }
            else
            {
                SetLayeredWindowAttributes(Window, RGB(0, 0, 0), 80, LWA_ALPHA);
            }
#endif
            
        } break;
        
        case WM_DESTROY:
        {
            // TODO(Rob): Handle this as an error - recreate Window?
            GlobalRunning = false;
        } break;
        
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            Assert("Keyboard input came in through a non-dispatch message.");
        } break;
        
        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            
            win32_window_dimension Dimension = Win32GetWindowDimension(Window);
            
            Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext,
                                       Dimension.Width, Dimension.Height);
            EndPaint(Window, &Paint);
        } break;
        
        default:
        {
            //OutputDebugStringA("WM_default\n");
            Result = DefWindowProcA(Window, Message, WParam, LParam);
        } break;
    }
    
    return Result;
}

internal void
Win32ClearBuffer(win32_sound_output *SoundOutput)
{
    VOID* Region1;
    DWORD Region1Size;
    VOID* Region2;
    DWORD Region2Size;
    if (SUCCEEDED(GlobalSecondaryBuffer->Lock(0, SoundOutput->SecondaryBufferSize,
                                              &Region1, &Region1Size,
                                              &Region2, &Region2Size,
                                              0)))
    {
        uint8 *DestSample = (uint8 *)Region1;
        for (DWORD ByteIndex = 0; ByteIndex < Region1Size; ++ByteIndex)
        {
            *DestSample++ = 0;
        }
        DestSample = (uint8 *)Region2;
        for (DWORD ByteIndex = 0; ByteIndex < Region2Size; ++ByteIndex)
        {
            *DestSample++ = 0;
        }
        
        GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
}

internal void
Win32FillSoundBuffer(win32_sound_output *SoundOutput, DWORD ByteToLock, DWORD BytesToWrite,
                     game_sound_output_buffer *SourceBuffer)
{
    // TODO(Rob): More strenuous test.
    VOID* Region1;
    DWORD Region1Size;
    VOID* Region2;
    DWORD Region2Size;
    if (SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock, BytesToWrite,
                                              &Region1, &Region1Size,
                                              &Region2, &Region2Size,
                                              0)))
    {
        // TODO(Rob): Assert that region sizes are valid.
        
        // TODO(Rob): Collapse these two loops.
        DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;
        int16 *DestOut = (int16 *)Region1;
        int16 *SourceSample = SourceBuffer->Samples;
        for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex)
        {
            *DestOut++ = *SourceSample++;
            *DestOut++ = *SourceSample++;
            ++SoundOutput->RunningSampleIndex;
        }
        
        DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;
        DestOut = (int16 *)Region2;
        for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex)
        {
            *DestOut++ = *SourceSample++;
            *DestOut++ = *SourceSample++;
            ++SoundOutput->RunningSampleIndex;
        }
        
        GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
}

internal void
Win32ProcessXInputDigitalButton(DWORD XInputButtonState, 
                                game_button_state *OldState, DWORD ButtonBit,
                                game_button_state *NewState)
{
    NewState->EndedDown = ((XInputButtonState & ButtonBit) == ButtonBit);
    NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}

internal real32
Win32ProcessXInputStickValue(SHORT Value, SHORT DeadZoneThreshold)
{
    real32 Result = 0;
    if (Value < -DeadZoneThreshold)
    {
        Result = (real32)((Value + DeadZoneThreshold) / (32768.0f - DeadZoneThreshold));
    }
    else if (Value > DeadZoneThreshold)
    {
        Result = (real32)((Value - DeadZoneThreshold) / (32767.0f - DeadZoneThreshold));
    }
    
    return (Result);
}

internal void
Win32ProcessKeyboardMessage(game_button_state *NewState, bool32 IsDown)
{
    if (NewState->EndedDown != IsDown)
    {
        NewState->EndedDown = IsDown;
        ++NewState->HalfTransitionCount;
    }
}

internal void
Win32GetInputFileLocation(win32_state *State, bool32 InputStream, 
                          int SlotIndex, int DestCount, char *Dest)
{
    char Temp[64];
    wsprintf(Temp, "loop_edit_%d_%s.hmi", SlotIndex, InputStream ? "input" : "state");
    Win32BuildEXEPathFilename(State, Temp, DestCount, Dest);
}

internal win32_replay_buffer *
Win32GetReplayBuffer(win32_state *State, unsigned int Index)
{
    Assert(Index > 0);
    Assert(Index < ArrayCount(State->ReplayBuffers));
    win32_replay_buffer *Result = &State->ReplayBuffers[Index];
    return (Result);
}

internal void
Win32BeginRecordingInput(win32_state *State, int InputRecordingIndex)
{
    win32_replay_buffer *ReplayBuffer = Win32GetReplayBuffer(State, InputRecordingIndex);
    if (ReplayBuffer->MemoryBlock)
    {
        State->InputRecordingIndex = InputRecordingIndex;
        
        char Filename[WIN32_STATE_FILE_NAME_COUNT];
        Win32GetInputFileLocation(State, true, InputRecordingIndex, sizeof(Filename), Filename);
        State->RecordingHandle = CreateFileA(Filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
        
#if 0
        LARGE_INTEGER FilePosition;
        FilePosition.QuadPart = State->TotalSize;
        SetFilePointerEx(State->RecordingHandle, FilePosition, 0, FILE_BEGIN);
#endif
        CopyMemory(ReplayBuffer->MemoryBlock, State->GameMemoryBlock, State->TotalSize);
    }
}

internal void
Win32EndRecordingInput(win32_state *State)
{
    CloseHandle(State->RecordingHandle);
    State->InputRecordingIndex = 0;
}

internal void
Win32BeginInputPlayback(win32_state *State, int InputPlayingIndex)
{
    win32_replay_buffer *ReplayBuffer = Win32GetReplayBuffer(State, InputPlayingIndex);
    if (ReplayBuffer->MemoryBlock)
    {
        State->InputPlayingIndex = InputPlayingIndex;
        
        char Filename[WIN32_STATE_FILE_NAME_COUNT];
        Win32GetInputFileLocation(State, true, InputPlayingIndex, sizeof(Filename), Filename);
        State->PlaybackHandle = CreateFileA(Filename, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
        
#if 0
        LARGE_INTEGER FilePosition;
        FilePosition.QuadPart = State->TotalSize;
        SetFilePointerEx(State->PlaybackHandle, FilePosition, 0, FILE_BEGIN);
#endif
        CopyMemory(State->GameMemoryBlock, ReplayBuffer->MemoryBlock, State->TotalSize);
    }
}

internal void
Win32EndInputPlayback(win32_state *State)
{
    CloseHandle(State->PlaybackHandle);
    State->InputPlayingIndex = 0;
}

internal void
Win32RecordInput(win32_state *State, game_input *NewInput)
{
    DWORD BytesWritten;
    WriteFile(State->RecordingHandle, NewInput, sizeof(*NewInput), &BytesWritten, 0);
}

internal void
Win32PlaybackInput(win32_state *State, game_input *NewInput)
{
    DWORD BytesRead;
    if(ReadFile(State->PlaybackHandle, NewInput, sizeof(*NewInput), &BytesRead, 0))
    {
        if (BytesRead == 0)
        {
            // NOTE(Rob): We've hit the end of the stream so loop.
            int PlayingIndex = State->InputPlayingIndex;
            Win32EndInputPlayback(State);
            Win32BeginInputPlayback(State, PlayingIndex);
            ReadFile(State->PlaybackHandle, NewInput, sizeof(*NewInput), &BytesRead, 0);
        }
    }
}

internal void
Win32ProcessPendingMessages(win32_state *State, game_controller_input* KeyboardController)
{
    
    MSG Message;
    while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
    {
        switch (Message.message)
        {
            case WM_QUIT:
            {
                GlobalRunning = false;
            } break;
            
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                uint32 VKCode = (uint32)Message.wParam;
                bool32 WasDown = ((Message.lParam & (1 << 30)) != 0);
                bool32 IsDown = ((Message.lParam & (1 << 31)) == 0);
                
                if (WasDown != IsDown) 
                {
                    if (VKCode == 'W')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveUp, IsDown);
                    }
                    else if (VKCode == 'A')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveLeft, IsDown);
                    }
                    else if (VKCode == 'S')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveDown, IsDown);
                    }
                    else if (VKCode == 'D')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveRight, IsDown);
                    }
                    else if (VKCode == 'Q')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->LeftShoulder, IsDown);
                    }
                    else if (VKCode == 'E')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->RightShoulder, IsDown);
                    }
                    else if (VKCode == VK_UP)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->ActionUp, IsDown);
                    }
                    else if (VKCode == VK_LEFT)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->ActionLeft, IsDown);
                    }
                    else if (VKCode == VK_DOWN)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->ActionDown, IsDown);
                    }
                    else if (VKCode == VK_RIGHT)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->ActionRight, IsDown);
                    }
                    else if (VKCode == VK_ESCAPE)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->Back, IsDown);
                    }
                    else if (VKCode == VK_SPACE)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->Start, IsDown);
                    }
#if HANDMADE_INTERNAL
                    else if (VKCode == 'P')
                    {
                        if (IsDown)
                        {
                            GlobalPause = !GlobalPause;
                        }
                    }
                    else if (VKCode == 'L')
                    {
                        if(IsDown)
                        {
                            if(State->InputPlayingIndex == 0)
                            {
                                if(State->InputRecordingIndex == 0)
                                {
                                    Win32BeginRecordingInput(State, 1);
                                }
                                else
                                {
                                    Win32EndRecordingInput(State);
                                    Win32BeginInputPlayback(State, 1);
                                }
                            }
                            else
                            {
                                Win32EndInputPlayback(State);
                            }
                        }
                    }
#endif
                    if (IsDown)
                    {
                        
                        bool32 AltKeyWasDown = ((Message.lParam & (1 << 29)) != 0);
                        if ((VKCode == VK_F4) && AltKeyWasDown)
                        {
                            GlobalRunning = false;
                        }
                        if ((VKCode == VK_RETURN) && AltKeyWasDown)
                        {
                            if (Message.hwnd)
                            {
                                ToggleFullscreen(Message.hwnd);
                            }
                        }
                    }
                    
                }
                
            } break;
            default:
            {
                TranslateMessage(&Message);
                DispatchMessageA(&Message);
            } break;
        }
    }
}

inline LARGE_INTEGER
Win32GetWallClock(void)
{
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return (Result);
}

inline real32
Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
    real32 Result = ((real32)(End.QuadPart - Start.QuadPart) /
                     (real32)GlobalPerfCountFrequency);
    return (Result);
}

#if 0
internal void
Win32DebugDrawVertical(win32_offscreen_buffer *BackBuffer, 
                       int X, int Top, int Bottom, uint32 Color)
{
    if (Top < 0)
    {
        Top = 0;
    }
    
    if (Bottom > BackBuffer->Height)
    {
        Bottom = BackBuffer->Height;
    }
    
    if ((X >= 0) && (X < BackBuffer->Width))
    {
        uint8 *Pixel = ((uint8 *)BackBuffer->Memory + 
                        X*BackBuffer->BytesPerPixel + 
                        Top*BackBuffer->Pitch); 
        for (int Y = Top; Y < Bottom; ++Y)
        {
            *(uint32 *)Pixel = Color;
            Pixel += BackBuffer->Pitch;
        }
    }
}

inline void
Win32DrawSoundBufferMarker(win32_offscreen_buffer *BackBuffer, 
                           win32_sound_output *SoundOutput, 
                           real32 C, int PadX, int Top, int Bottom,
                           DWORD Value, uint32 Color)
{
    real32 XReal32 = (C * (real32)Value);
    int X = PadX + (int)XReal32;
    Win32DebugDrawVertical(BackBuffer, X, Top, Bottom, Color);
}

internal void
Win32DebugSyncDisplay(win32_offscreen_buffer *BackBuffer, 
                      int MarkerCount, win32_debug_time_marker *Markers,
                      int CurrentMarkerIndex,
                      win32_sound_output *SoundOutput, real32 TargetSecondsPerFrame)
{
    int PadX = 16;
    int PadY = 16;
    int LineHeight = 64;
    
    real32 C = (real32)(BackBuffer->Width - 2*PadX) / (real32)SoundOutput->SecondaryBufferSize;
    
    for (int MarkerIndex = 0; MarkerIndex < MarkerCount; ++MarkerIndex)
    {
        win32_debug_time_marker *ThisMarker = &Markers[MarkerIndex];
        Assert(ThisMarker->OutputPlayCursor < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->OutputWriteCursor < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->OutputLocation < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->OutputByteCount < SoundOutput->SecondaryBufferSize); 
        Assert(ThisMarker->FlipPlayCursor < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->FlipWriteCursor < SoundOutput->SecondaryBufferSize);
        
        DWORD PlayColor = 0xFFFFFFFF;
        DWORD WriteColor = 0xFFFF0000;;
        DWORD ExpectedFlipColor = 0xFFFFFF00;
        DWORD PlayWindowColor = 0xFFFF00FF;
        
        int Top = PadY;
        int Bottom = PadY + LineHeight;
        if (MarkerIndex == CurrentMarkerIndex)
        {
            Top += LineHeight+PadY;
            Bottom += LineHeight+PadY;
            
            int FirstTop = Top;
            
            Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputPlayCursor, PlayColor);
            Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputWriteCursor, WriteColor);
            
            Top += LineHeight+PadY;
            Bottom += LineHeight+PadY;
            
            Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputLocation, PlayColor);
            Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom,ThisMarker->OutputByteCount + ThisMarker->OutputLocation, WriteColor);
            
            Top += LineHeight+PadY;
            Bottom += LineHeight+PadY;
            
            Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, FirstTop, Bottom, ThisMarker->ExpectedFlipCursor, ExpectedFlipColor);
        }
        
        Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->FlipPlayCursor, PlayColor);
        Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->FlipPlayCursor + 480*SoundOutput->BytesPerSample, PlayWindowColor);
        Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->FlipWriteCursor, WriteColor);
        
    }
}
#endif

internal void 
Win32GetEXEFileName(win32_state *State)
{
    // NOTE(Rob): Never use MAX_PATH in code that is user-facing, because it can be dangerous and can 
    // lead to bad results.
    DWORD SizeOfFilename = GetModuleFileNameA(0, State->EXEFileName, sizeof(State->EXEFileName));
    State->OnePastLastEXEFileNameSlash = State->EXEFileName;
    for (char *Scan = State->EXEFileName; *Scan; ++Scan)
    {
        if (*Scan == '\\')
        {
            State->OnePastLastEXEFileNameSlash = Scan + 1;
        }
    }
}

int CALLBACK
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR CommandLine,
        int ShowCode)
{
    win32_state Win32State = {};
    
    LARGE_INTEGER GlobalPerfCountFrequencyResult;
    QueryPerformanceFrequency(&GlobalPerfCountFrequencyResult);
    GlobalPerfCountFrequency = GlobalPerfCountFrequencyResult.QuadPart;
    
    Win32GetEXEFileName(&Win32State);
    
    char SourceGameCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
    Win32BuildEXEPathFilename(&Win32State, "handmade.dll",
                              sizeof(SourceGameCodeDLLFullPath), SourceGameCodeDLLFullPath);
    
    char TempGameCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
    Win32BuildEXEPathFilename(&Win32State, "handmade_temp.dll",
                              sizeof(TempGameCodeDLLFullPath), TempGameCodeDLLFullPath);
    
    
    char GameCodeLockFullPath[WIN32_STATE_FILE_NAME_COUNT];
    Win32BuildEXEPathFilename(&Win32State, "lock.tmp",
                              sizeof(GameCodeLockFullPath), GameCodeLockFullPath);
    
    // NOTE(Rob): Set the Windows scheduler granularity to 1ms
    // so that our Sleep() can be more granular.
    UINT DesiredSchedulerMS = 1;
    bool32 SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);
    
    Win32LoadXInput();
    
#if HANDMADE_INTERNAL
    DEBUGGlobalShowCursor = true;
#endif
    WNDCLASSA WindowClass = {};
    
    Win32ResizeDIBSection(&GlobalBackBuffer, 960, 540);
    
    WindowClass.style = CS_HREDRAW|CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
    //    WindowClass.hIcon;
    WindowClass.lpszClassName = "HandemadeHeroWindowClass";
    
    if (RegisterClassA(&WindowClass))
    {
        HWND Window = 
            CreateWindowExA(
            0,//WS_EX_TOPMOST|WS_EX_LAYERED,
            WindowClass.lpszClassName,
            "TowerRPG Project",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            Instance,
            0);
        if (Window)
        {
            
            // TODO(Rob): How to reliably query this on Windows?
            HDC RefreshDC = GetDC(Window);
            int MonitorRefreshHz = 120;
            int Win32RefreshRate = GetDeviceCaps(RefreshDC, VREFRESH);
            if (Win32RefreshRate > 1)
            {
                //MonitorRefreshHz = Win32RefreshRate;
            }
            real32 GameUpdateHz = (MonitorRefreshHz / 2.0f);
            real32 TargetSecondsPerFrame = 1.0f / (real32)GameUpdateHz;
            ReleaseDC(Window, RefreshDC);
            
            // NOTE(Rob): Sound test.
            win32_sound_output SoundOutput = {};
            SoundOutput.SamplesPerSecond = 48000;
            SoundOutput.BytesPerSample = sizeof(int16)*2;
            SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
            // TODO(Rob): Get rid of LatencySampleCount.
            // TODO(Rob): Actually compute this variance and see what the lowest reasonable value is.
            SoundOutput.SafetyBytes = (int)(((real32)SoundOutput.SamplesPerSecond*(real32)SoundOutput.BytesPerSample / GameUpdateHz) / 2.0f);
            Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
            Win32ClearBuffer(&SoundOutput);
            GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
            
            GlobalRunning = true;
            
#if 0
            // NOTE(Rob): This tests the PlayCursor/WriteCursor update frequency
            while(GlobalRunning)
            {
                DWORD PlayCursor;
                DWORD WriteCursor;
                GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor);
                char TextBuffer[256];
                _snprintf_s(TextBuffer, sizeof(TextBuffer),
                            "PC:%u WC:%u\n", PlayCursor, WriteCursor);
                OutputDebugStringA(TextBuffer);
            }
#endif
            // TODO(Rob): Pool with bitmap virtualalloc
            int16 *Samples = (int16 *)VirtualAlloc(0, SoundOutput.SecondaryBufferSize,
                                                   MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            
#if HANDMADE_INTERNAL
            LPVOID BaseAddress = (LPVOID)Terabytes(2);
#else
            LPVOID BaseAddress = 0;
#endif
            game_memory GameMemory = {};
            GameMemory.PermanentStorageSize = Megabytes(256);
            GameMemory.TransientStorageSize = Gigabytes(1);
            GameMemory.DEBUGPlatformFreeFileMemory = DEBUGPlatformFreeFileMemory;
            GameMemory.DEBUGPlatformReadEntireFile = DEBUGPlatformReadEntireFile;
            GameMemory.DEBUGPlatformWriteEntireFile = DEBUGPlatformWriteEntireFile;
            
            // TODO(Rob): Use mem large pages and call adjust token privildedges when not on Windows XP.
            Win32State.TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
            Win32State.GameMemoryBlock = VirtualAlloc(BaseAddress, (size_t)Win32State.TotalSize,
                                                      MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            GameMemory.PermanentStorage = Win32State.GameMemoryBlock;
            GameMemory.TransientStorage = ((uint8 *)GameMemory.PermanentStorage + 
                                           GameMemory.PermanentStorageSize);
            
            for (int ReplayIndex = 1; ReplayIndex < ArrayCount(Win32State.ReplayBuffers); ++ ReplayIndex)
            {
                win32_replay_buffer *ReplayBuffer = &Win32State.ReplayBuffers[ReplayIndex];
                
                Win32GetInputFileLocation(&Win32State, false, ReplayIndex, 
                                          sizeof(ReplayBuffer->Filename), ReplayBuffer->Filename);
                
                ReplayBuffer->FileHandle = CreateFileA(
                    ReplayBuffer->Filename, GENERIC_READ|GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
                
                LARGE_INTEGER MaxSize;
                MaxSize.QuadPart = Win32State.TotalSize;
                ReplayBuffer->MemoryMap = CreateFileMapping(
                    ReplayBuffer->FileHandle, 0, PAGE_READWRITE, 
                    MaxSize.HighPart, MaxSize.LowPart, 0);
                
                ReplayBuffer->MemoryBlock = MapViewOfFile(
                    ReplayBuffer->MemoryMap, FILE_MAP_ALL_ACCESS, 0, 0, Win32State.TotalSize);
                
                if (ReplayBuffer->MemoryBlock)
                {
                }
                else
                {
                    // TODO(Rob): Diagnostics.
                }
            }
            
            if (Samples && GameMemory.PermanentStorage && GameMemory.TransientStorage)
            {
                game_input Input[2] = {};
                game_input *NewInput = &Input[0];
                game_input *OldInput = &Input[1];
                
                LARGE_INTEGER LastCounter = Win32GetWallClock();
                LARGE_INTEGER FlipWallClock = Win32GetWallClock();
                
                int DebugTimeMarkerIndex = 0;
                win32_debug_time_marker DebugTimeMarkers[30] = {0};
                
                // TODO(Rob): Handle the startup especially.
                DWORD AudioLatencyBytes = 0;
                real32 AudioLatencySeconds = 0;
                bool32 SoundIsValid = false;
                
                win32_game_code Game = Win32LoadGameCode(SourceGameCodeDLLFullPath, 
                                                         TempGameCodeDLLFullPath,
                                                         GameCodeLockFullPath);
                uint32 LoadCounter = 0;
                
                int64 LastCycleCounter = __rdtsc();
                while (GlobalRunning)
                {
                    NewInput->dtForFrame = TargetSecondsPerFrame;
                    FILETIME NewDLLWriteTime = Win32GetLastWriteTime(SourceGameCodeDLLFullPath);
                    if (CompareFileTime(&NewDLLWriteTime, &Game.DLLLastWriteTime) != 0)
                    {
                        Win32UnloadGameCode(&Game);
                        Game = Win32LoadGameCode(SourceGameCodeDLLFullPath, 
                                                 TempGameCodeDLLFullPath,
                                                 GameCodeLockFullPath);
                    }
                    
                    // TODO(Rob): Zero macro.
                    // TODO(Rob): We can't Zero everything because the up/down state will be wrong.
                    game_controller_input *OldKeyboardController = GetController(OldInput, 0);
                    game_controller_input *NewKeyboardController = GetController(NewInput, 0);
                    *NewKeyboardController = {};
                    NewKeyboardController->IsConnected = true;
                    for(int ButtonIndex = 0; ButtonIndex < ArrayCount(OldKeyboardController->Buttons); ++ButtonIndex)
                    {
                        NewKeyboardController->Buttons[ButtonIndex].EndedDown = 
                            OldKeyboardController->Buttons[ButtonIndex].EndedDown;
                    }
                    
                    Win32ProcessPendingMessages(&Win32State, NewKeyboardController);
                    
                    
                    if (!GlobalPause)
                    {
                        POINT MouseP;
                        GetCursorPos(&MouseP);
                        ScreenToClient(Window, &MouseP);
                        NewInput->MouseX = MouseP.x;
                        NewInput->MouseY = MouseP.y;
                        NewInput->MouseZ = 0;
                        Win32ProcessKeyboardMessage(&NewInput->MouseButtons[0], 
                                                    GetKeyState(VK_LBUTTON) & (1 << 15));
                        Win32ProcessKeyboardMessage(&NewInput->MouseButtons[1], 
                                                    GetKeyState(VK_RBUTTON) & (1 << 15));
                        Win32ProcessKeyboardMessage(&NewInput->MouseButtons[2], 
                                                    GetKeyState(VK_MBUTTON) & (1 << 15));
                        Win32ProcessKeyboardMessage(&NewInput->MouseButtons[3], 
                                                    GetKeyState(VK_XBUTTON1) & (1 << 15));
                        Win32ProcessKeyboardMessage(&NewInput->MouseButtons[4], 
                                                    GetKeyState(VK_XBUTTON2) & (1 << 15));
                        
                        DWORD MaxControllerCount = XUSER_MAX_COUNT;
                        // NOTE(Rob): We subtract one from MaxController to account for the keyboard.
                        if (MaxControllerCount > (ArrayCount(NewInput->Controllers) - 1))
                        {
                            MaxControllerCount = (ArrayCount(NewInput->Controllers) - 1);
                        }
                        
                        // TODO(Rob): Need to not poll disconnected controllers to avoid
                        // xinput framerate hit on older libraries.
                        // TODO(Rob): Should we poll this more frequently?
                        for (DWORD ControllerIndex = 0; ControllerIndex < MaxControllerCount; ++ControllerIndex)
                        {
                            DWORD OurControllerIndex = ControllerIndex + 1;
                            game_controller_input *OldController = GetController(OldInput, OurControllerIndex);
                            game_controller_input *NewController = GetController(NewInput, OurControllerIndex);
                            
                            XINPUT_STATE ControllerState;
                            if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
                            {
                                NewController->IsConnected = true;
                                NewController->IsAnalog = OldController->IsAnalog;
                                
                                // TODO(Rob): See if controller.dwPacketNumber increments too rapidly.
                                XINPUT_GAMEPAD* Pad = &ControllerState.Gamepad;
                                
                                NewController->IsAnalog = true;
                                NewController->StickAverageX = Win32ProcessXInputStickValue(
                                    Pad->sThumbLX,XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                                NewController->StickAverageY = Win32ProcessXInputStickValue(
                                    Pad->sThumbLY,XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                                if ((NewController->StickAverageX != 0.0f) ||
                                    (NewController->StickAverageY != 0.0f))
                                {
                                    NewController->IsAnalog = true;
                                }
                                
                                if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP)
                                {
                                    NewController->StickAverageY = 1.0f;
                                    NewController->IsAnalog = false;
                                }
                                if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
                                {
                                    NewController->StickAverageY = -1.0f;
                                    NewController->IsAnalog = false;
                                }
                                if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
                                {
                                    NewController->StickAverageX = -1.0f;
                                    NewController->IsAnalog = false;
                                }
                                if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
                                {
                                    NewController->StickAverageX = 1.0f;
                                    NewController->IsAnalog = false;
                                }
                                
                                real32 Threshold = 0.5f;
                                Win32ProcessXInputDigitalButton(
                                    (NewController->StickAverageX < -Threshold) ? 1 : 0, 
                                    &OldController->MoveLeft, 1,
                                    &NewController->MoveLeft);
                                Win32ProcessXInputDigitalButton(
                                    (NewController->StickAverageX > Threshold) ? 1 : 0, 
                                    &OldController->MoveRight, 1,
                                    &NewController->MoveRight);
                                Win32ProcessXInputDigitalButton(
                                    (NewController->StickAverageY < -Threshold) ? 1 : 0, 
                                    &OldController->MoveDown, 1,
                                    &NewController->MoveDown);
                                Win32ProcessXInputDigitalButton(
                                    (NewController->StickAverageY > Threshold) ? 1 : 0, 
                                    &OldController->MoveUp, 1,
                                    &NewController->MoveUp);
                                
                                Win32ProcessXInputDigitalButton(Pad->wButtons, 
                                                                &OldController->ActionDown, XINPUT_GAMEPAD_A,
                                                                &NewController->ActionDown);
                                Win32ProcessXInputDigitalButton(Pad->wButtons, 
                                                                &OldController->ActionUp, XINPUT_GAMEPAD_B,
                                                                &NewController->ActionUp);
                                Win32ProcessXInputDigitalButton(Pad->wButtons, 
                                                                &OldController->ActionLeft, XINPUT_GAMEPAD_X,
                                                                &NewController->ActionLeft);
                                Win32ProcessXInputDigitalButton(Pad->wButtons, 
                                                                &OldController->ActionRight, XINPUT_GAMEPAD_Y,
                                                                &NewController->ActionRight);
                                Win32ProcessXInputDigitalButton(Pad->wButtons, 
                                                                &OldController->LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER,
                                                                &NewController->LeftShoulder);
                                Win32ProcessXInputDigitalButton(Pad->wButtons, 
                                                                &OldController->RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER,
                                                                &NewController->RightShoulder);
                                Win32ProcessXInputDigitalButton(Pad->wButtons, 
                                                                &OldController->Start, XINPUT_GAMEPAD_START,
                                                                &NewController->Start);
                                Win32ProcessXInputDigitalButton(Pad->wButtons, 
                                                                &OldController->Back, XINPUT_GAMEPAD_BACK,
                                                                &NewController->Back);
                            }
                            else
                            {
                                // NOTE(Rob): This controller is not available.
                                NewController->IsConnected = false;
                            }
                        }
                        thread_context Thread = {};
                        
                        game_offscreen_buffer Buffer = {};
                        Buffer.Memory = GlobalBackBuffer.Memory;
                        Buffer.Width = GlobalBackBuffer.Width;
                        Buffer.Height = GlobalBackBuffer.Height;
                        Buffer.Pitch = GlobalBackBuffer.Pitch;
                        Buffer.BytesPerPixel = GlobalBackBuffer.BytesPerPixel;
                        
                        if (Win32State.InputRecordingIndex)
                        {
                            Win32RecordInput(&Win32State, NewInput);
                        }
                        else if (Win32State.InputPlayingIndex)
                        {
                            Win32PlaybackInput(&Win32State, NewInput);
                        }
                        if (Game.UpdateAndRender)
                        {
                            Game.UpdateAndRender(&Thread, &GameMemory, NewInput, &Buffer);
                        }
                        
                        LARGE_INTEGER AudioWallClock = Win32GetWallClock();
                        real32 FromBeginToAudioSeconds = Win32GetSecondsElapsed(FlipWallClock, AudioWallClock);
                        
                        DWORD PlayCursor;
                        DWORD WriteCursor;
                        if (GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK)
                        {
                            /* NOTE(Rob): 
                            We define a safety value that is the number of samples with think our game update loop may vary by,
                            let's say up to 2ms.
                            
                            When we wake up to write audio, we will look at see what the play cursor position is and
                            we will forecast the postition ahead where we think the play cursor will be on the next
                            frame boundary. 
                            
                            We will then look to see if the write cursor is the frame before that by at least our safe value,
                            if it is, the target fill position is that frame boundary plus one frame. Ths will 
                            give us perfect audio sync in the case of a card that has low enough latency.
                            
                            If the write cursor is after the safety margin, then we assume then we can never sync
                            the audio perfectly, so we will write one frames worth of audio plus the safety margin's guard samples.
                            (1ms, or something deterimined to be safe, whatever we think the varibility of our frame computation)
                            */
                            
                            if (!SoundIsValid)
                            {
                                SoundOutput.RunningSampleIndex = WriteCursor / SoundOutput.BytesPerSample;
                                SoundIsValid = true;
                            }
                            
                            DWORD ByteToLock = ((SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % 
                                                SoundOutput.SecondaryBufferSize);
                            
                            DWORD ExpectedSoundBytesPerFrame = (int)((real32)(SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample) / 
                                                                     GameUpdateHz);
                            
                            real32 SecondsLeftUntilFlip = (TargetSecondsPerFrame - FromBeginToAudioSeconds);
                            DWORD ExpectedBytesUntilFlip = (DWORD)((SecondsLeftUntilFlip/TargetSecondsPerFrame)*
                                                                   (real32)ExpectedSoundBytesPerFrame);
                            DWORD ExpectedFrameBoundaryByte = PlayCursor + ExpectedSoundBytesPerFrame;
                            
                            DWORD SafeWriteCursor = WriteCursor;
                            if (SafeWriteCursor < PlayCursor)
                            {
                                SafeWriteCursor += SoundOutput.SecondaryBufferSize;
                            }
                            Assert(SafeWriteCursor >= PlayCursor);
                            SafeWriteCursor += SoundOutput.SafetyBytes;
                            
                            bool32 AudioCardIsLowLatency = (SafeWriteCursor < ExpectedFrameBoundaryByte);
                            
                            DWORD TargetCursor = 0;
                            if(AudioCardIsLowLatency)
                            {
                                TargetCursor = (ExpectedFrameBoundaryByte + ExpectedSoundBytesPerFrame);
                            }
                            else
                            {
                                TargetCursor = (WriteCursor + ExpectedSoundBytesPerFrame + SoundOutput.SafetyBytes);
                            }
                            TargetCursor = (TargetCursor % SoundOutput.SecondaryBufferSize);
                            
                            DWORD BytesToWrite = 0;
                            if (ByteToLock > TargetCursor)
                            {
                                BytesToWrite = (SoundOutput.SecondaryBufferSize - ByteToLock);
                                BytesToWrite += TargetCursor;
                            }
                            else
                            {
                                BytesToWrite = TargetCursor - ByteToLock;
                            }
                            game_sound_output_buffer SoundBuffer = {};
                            SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
                            SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
                            SoundBuffer.Samples = Samples;
                            if (Game.GetSoundSamples)
                            {
                                Game.GetSoundSamples(&Thread, &GameMemory, &SoundBuffer);
                            }
                            
#if HANDMADE_INTERNAL
                            win32_debug_time_marker *Marker = &DebugTimeMarkers[DebugTimeMarkerIndex];
                            Marker->OutputPlayCursor = PlayCursor;
                            Marker->OutputWriteCursor = WriteCursor;
                            Marker->OutputLocation = ByteToLock;
                            Marker->OutputByteCount = BytesToWrite;
                            Marker->ExpectedFlipCursor = ExpectedFrameBoundaryByte;
                            
                            DWORD UnwrappedWriteCursor = WriteCursor;
                            if (WriteCursor < PlayCursor)
                            {
                                UnwrappedWriteCursor += SoundOutput.SecondaryBufferSize;
                            }
                            AudioLatencyBytes = UnwrappedWriteCursor - PlayCursor;
                            AudioLatencySeconds = (((real32)AudioLatencyBytes / (real32)SoundOutput.BytesPerSample) / 
                                                   (real32)SoundOutput.SamplesPerSecond);
                            
#if 0
                            char TextBuffer[256];
                            _snprintf_s(TextBuffer, sizeof(TextBuffer),
                                        "BTL:%u TC:%u BTW:%u - PC:%u WC:%u DELTA:%u (%fs)\n",
                                        ByteToLock, TargetCursor, BytesToWrite, PlayCursor, 
                                        WriteCursor, AudioLatencyBytes, AudioLatencySeconds);
                            OutputDebugStringA(TextBuffer);
#endif
#endif
                            Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);
                        }
                        else
                        {
                            SoundIsValid = false;
                        }
                        
                        LARGE_INTEGER WorkCounter = Win32GetWallClock();
                        real32 WorkSecondsElapased = Win32GetSecondsElapsed(LastCounter, WorkCounter);
                        
                        // TODO(Rob): Not tested yet - pRobably buggy!
                        real32 SecondsElapsedForFrame = WorkSecondsElapased;
                        if (SecondsElapsedForFrame < TargetSecondsPerFrame)
                        {
                            
                            if (SleepIsGranular)
                            {
                                DWORD SleepMS = (DWORD)(1000.0f * (TargetSecondsPerFrame - SecondsElapsedForFrame));
                                
                                if (SleepMS > 0)
                                {
                                    Sleep(SleepMS);
                                }
                            }
                            real32 TestSecondsElapsedForFrame = (real32)(Win32GetSecondsElapsed(LastCounter,
                                                                                                Win32GetWallClock()) / 1000.0f);
                            
                            if(TestSecondsElapsedForFrame < TargetSecondsPerFrame)
                            {
                                // TODO(Rob): Log missed sleep here.
                            }
                            
                            while (SecondsElapsedForFrame < TargetSecondsPerFrame)
                            {
                                SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
                            }
                            
                        }
                        else
                        {
                            // TODO(Rob): MISSED FRAME RATE!
                            // TODO(Rob): Logging.
                        }
                        
                        LARGE_INTEGER EndCounter = Win32GetWallClock();
                        real32 MSPerFrame = 1000.0f * Win32GetSecondsElapsed(LastCounter, EndCounter);
                        LastCounter = EndCounter;
                        
                        win32_window_dimension Dimension = Win32GetWindowDimension(Window);
                        HDC DeviceContext = GetDC(Window);
                        Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, 
                                                   Dimension.Width, Dimension.Height);
                        ReleaseDC(Window, DeviceContext);
                        
                        FlipWallClock = Win32GetWallClock();
                        
#if HANDMADE_INTERNAL
                        // NOTE(Rob): This is debug code
                        {
                            if (GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK)
                            {
                                Assert(DebugTimeMarkerIndex < ArrayCount(DebugTimeMarkers));
                                win32_debug_time_marker *Marker = &DebugTimeMarkers[DebugTimeMarkerIndex];
                                
                                Marker->FlipPlayCursor = PlayCursor;
                                Marker->FlipWriteCursor = WriteCursor;
                            }
                        }
#endif
                        
                        game_input *Temp = NewInput;
                        NewInput = OldInput;
                        OldInput = Temp;
                        
#if 1
                        uint64 EndCycleCounter = __rdtsc();
                        uint64 CyclesElapsed = EndCycleCounter - LastCycleCounter;
                        LastCycleCounter = EndCycleCounter;
                        
                        real64 FPS = 0;
                        real64 MCPF = ((real64)CyclesElapsed / (1000.0f * 1000.0f));
                        
                        char OBuffer[256];
                        sprintf_s(OBuffer, "%.02fms/f,  %.02ff/s,  %.02fmc/f\n", MSPerFrame, FPS, MCPF);
                        OutputDebugStringA(OBuffer);
#endif
                        
#if HANDMADE_INTERNAL
                        ++DebugTimeMarkerIndex;
                        if (DebugTimeMarkerIndex == ArrayCount(DebugTimeMarkers))
                        {
                            DebugTimeMarkerIndex = 0;
                        }
#endif
                    }
                }
            }
            else
            {
                // TODO(Rob): Logging.
            }
        }
        else
        {
            // TODO(Rob): Logging.
        }
    }
    else
    {
        // TODO(Rob): Logging.
    }
    
    return (0);
}