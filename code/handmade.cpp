#include "handmade.h"
#include "handmade_random.h"
#include "handmade_tile.cpp"

internal void 
GameOutputSound(game_state *GameState, game_sound_output_buffer *SoundBuffer, int ToneHz)
{
    int16 ToneVolume = 1000;
    int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;
    
    int16 *SampleOut = SoundBuffer->Samples;
    for (int SampleIndex = 0;
         SampleIndex < SoundBuffer->SampleCount;
         ++SampleIndex)
    {
#if 0
        real32 SineValue = sinf(GameState->tSine);
        int16 SampleValue = (int16)(SineValue * ToneVolume);
#else
        int16 SampleValue = 0;
#endif
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;
        
#if 0
        GameState->tSine += 2.0f*pi32 / (real32)WavePeriod;
        if (GameState->tSine > 2.0f*pi32)
        {
            GameState->tSine -= 2.0f*pi32;
        }
#endif
    }
}

internal void
DrawRectangle(game_offscreen_buffer* Buffer, 
              real32 RealMinX, real32 RealMinY, real32 RealMaxX, real32 RealMaxY,
              real32 R, real32 G, real32 B)
{
    int32 MinX = RoundReal32ToInt32(RealMinX);
    int32 MinY = RoundReal32ToInt32(RealMinY);
    int32 MaxX = RoundReal32ToInt32(RealMaxX);
    int32 MaxY = RoundReal32ToInt32(RealMaxY);
    
    if (MinX < 0)
    {
        MinX = 0;
    }
    if (MinY < 0)
    {
        MinY = 0;
    }
    if (MaxX > Buffer->Width)
    {
        MaxX = Buffer->Width;
    }
    if (MaxY > Buffer->Height)
    {
        MaxY = Buffer->Height;
    }
    
    uint32 Color = ((RoundReal32ToUInt32(R * 255.0f) << 16) |
                    (RoundReal32ToUInt32(G * 255.0f) << 8) |
                    (RoundReal32ToUInt32(B * 255.0f) << 0));
    
    uint8 *Row = ((uint8 *)Buffer->Memory + MinX*Buffer->BytesPerPixel + MinY*Buffer->Pitch);
    for (int Y = MinY; Y < MaxY; ++Y)
    {
        uint32 *Pixel = (uint32 *)Row;
        for (int X = MinX; X < MaxX; ++X)
        {
            *Pixel++ = Color;
        }
        Row += Buffer->Pitch;
    }
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender) 
{
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) == 
           ArrayCount(Input->Controllers[0].Buttons));
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
    
    real32 PlayerHeight = 1.4f;
    real32 PlayerWidth = 0.75f*PlayerHeight;
    
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if (!Memory->IsInitialized)
    {
        GameState->PlayerP.AbsTileX = 2;
        GameState->PlayerP.AbsTileY = 2;
        GameState->PlayerP.TileRelX = 5;
        GameState->PlayerP.TileRelY = 5;
        
        InitializeArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(game_state), 
                        (uint8 *)Memory->PermanentStorage + sizeof(game_state));
        
        GameState->World = PushStruct(&GameState->WorldArena, world);
        world *World = GameState->World;
        World->TileMap = PushStruct(&GameState->WorldArena, tile_map);
        
        tile_map *TileMap = World->TileMap;
        
        TileMap->ChunkShift = 4;
        TileMap->ChunkMask = (1 << TileMap->ChunkShift) - 1;
        TileMap->ChunkDim = (1 << TileMap->ChunkShift);
        TileMap->TileChunkCountX = 128;
        TileMap->TileChunkCountY = 128;
        TileMap->TileChunkCountZ = 2;
        TileMap->TileSideInMeters = 1.4f;
        TileMap->TileChunks = PushArray(&GameState->WorldArena, 
                                        TileMap->TileChunkCountX*TileMap->TileChunkCountY,
                                        tile_chunk);
        
        uint32 RandomNumberIndex = 0;
        
        uint32 TilesPerWidth = 17;
        uint32 TilesPerHeight = 9;
        uint32 ScreenX = 0;
        uint32 ScreenY = 0;
        
        bool32 DoorLeft = false;
        bool32 DoorRight = false;
        bool32 DoorTop = false;
        bool32 DoorBottom = false;
        
        for (uint32 ScreenIndex = 0; ScreenIndex < 100; ++ScreenIndex)
        {
            Assert(RandomNumberIndex < ArrayCount(RandomNumberTable));
            uint32 RandomChoice = RandomNumberTable[RandomNumberIndex++] % 2;
            
            if (RandomChoice == 0)
            {
                DoorRight = true;
            }
            else
            {
                DoorTop = true;
            }
            
            for (uint32 TileY = 0; TileY < TilesPerHeight; ++TileY)
            {
                for (uint32 TileX = 0; TileX < TilesPerWidth; ++TileX)
                {
                    uint32 AbsTileX = ScreenX*TilesPerWidth + TileX;
                    uint32 AbsTileY = ScreenY*TilesPerHeight + TileY;
                    
                    uint32 TileValue = 1;
                    if ((TileX == 0) && (!DoorLeft || (TileY != (TilesPerHeight/2))))
                    {
                        TileValue = 2;
                    }
                    
                    if ((TileX == (TilesPerWidth - 1)) && (!DoorRight || (TileY != (TilesPerHeight/2))))
                    {
                        TileValue = 2;
                    }
                    
                    if ((TileY == 0) && (!DoorBottom || (TileX != (TilesPerWidth/2))))
                    {
                        TileValue = 2;
                        
                    }
                    if ((TileY == (TilesPerHeight - 1)) && (!DoorTop || (TileX != (TilesPerWidth/2))))
                    {
                        TileValue = 2;
                    }
                    
                    SetTileValue(&GameState->WorldArena, World->TileMap, AbsTileX, AbsTileY, 
                                 TileValue);
                }
            }
            
            DoorLeft = DoorRight;
            DoorBottom = DoorTop;
            DoorRight = false;
            DoorTop = false;
            
            if (RandomChoice == 0)
            {
                ++ScreenX;
            }
            else
            {
                ++ScreenY;
            }
        }
        
        Memory->IsInitialized = true;
    }
    
    world *World = GameState->World;
    tile_map *TileMap = World->TileMap;
    
    int32 TileSideInPixels = 60;
    real32 MetersToPixels = TileSideInPixels / (real32)TileMap->TileSideInMeters;
    
    real32 LowerLeftX = -(real32)TileSideInPixels/2;
    real32 LowerLeftY = (real32)Buffer->Height;
    
    for (int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ++ControllerIndex)
    {
        game_controller_input *Controller = GetController(Input, ControllerIndex);
        if (Controller->IsAnalog)
        {
            // NOTE(rob): Use analog tuning.
        }
        else
        {
            // NOTE(rob): Use digital movement tuning
            real32 dPlayerX = 0.f;
            real32 dPlayerY = 0.f;
            
            if (Controller->MoveUp.EndedDown)
            {
                dPlayerY = 1.f;
            }
            if (Controller->MoveDown.EndedDown)
            {
                dPlayerY = -1.f;
            }
            if (Controller->MoveLeft.EndedDown)
            {
                dPlayerX = -1.f;
            }
            if (Controller->MoveRight.EndedDown)
            {
                dPlayerX = 1.f;
            }
            real32 PlayerSpeed = 2.f;
            
            if (Controller->ActionUp.EndedDown)
            {
                PlayerSpeed = 10.f;
            }
            dPlayerX *= PlayerSpeed;
            dPlayerY *= PlayerSpeed;
            
            tile_map_position NewPlayerP = GameState->PlayerP;
            NewPlayerP.TileRelX += Input->dtForFrame*dPlayerX;
            NewPlayerP.TileRelY += Input->dtForFrame*dPlayerY;
            NewPlayerP = RecanonicalizePosition(TileMap, NewPlayerP);
            
            tile_map_position PlayerLeft = NewPlayerP;
            PlayerLeft.TileRelX -= 0.5f*PlayerWidth;
            PlayerLeft = RecanonicalizePosition(TileMap, PlayerLeft);
            
            tile_map_position PlayerRight = NewPlayerP;
            PlayerRight.TileRelX += 0.5f*PlayerWidth;
            PlayerRight = RecanonicalizePosition(TileMap, PlayerRight);
            
            if (IsTileMapPointEmpty(TileMap, NewPlayerP) &&
                IsTileMapPointEmpty(TileMap, PlayerLeft) &&
                IsTileMapPointEmpty(TileMap, PlayerRight))
            {
                GameState->PlayerP = NewPlayerP;
            }
        }
    }
    
    DrawRectangle(Buffer, 0.f, 0.f, (real32)Buffer->Width, (real32)Buffer->Height, 1.f, 0.f, 1.f);
    
    real32 ScreenCenterX = 0.5f*(real32)Buffer->Width;
    real32 ScreenCenterY = 0.5f*(real32)Buffer->Height;
    
    for (int32 RelRow = -10; RelRow < 10; ++RelRow)
    {
        for (int32 RelColumn = -20; RelColumn < 20; ++RelColumn)
        {
            uint32 Column = RelColumn + GameState->PlayerP.AbsTileX;
            uint32 Row = RelRow + GameState->PlayerP.AbsTileY;
            uint32 TileID = GetTileValue(TileMap, Column, Row);
            
            if (TileID > 0)
            {
                real32 Gray = 0.5f;
                if (TileID == 2)
                {
                    Gray = 1.f;
                }
                
                if((Column == GameState->PlayerP.AbsTileX) && (Row == GameState->PlayerP.AbsTileY))
                {
                    Gray = 0.f;
                }
                
                real32 CenX = ScreenCenterX - MetersToPixels*GameState->PlayerP.TileRelX + ((real32)RelColumn)*TileSideInPixels;
                real32 CenY = ScreenCenterY + MetersToPixels*GameState->PlayerP.TileRelY - ((real32)RelRow)*TileSideInPixels;
                real32 MinX = CenX - 0.5f*TileSideInPixels;
                real32 MinY = CenY - 0.5f*TileSideInPixels;
                real32 MaxX = CenX + 0.5f*TileSideInPixels;
                real32 MaxY = CenY + 0.5f*TileSideInPixels;
                DrawRectangle(Buffer, MinX, MinY, MaxX, MaxY, Gray, Gray, Gray);
            }
        }
    }
    
    real32 PlayerR = 1.f;
    real32 PlayerG = 1.f;
    real32 PlayerB = 0.f;
    real32 PlayerLeft = ScreenCenterX - 0.5f*MetersToPixels*PlayerWidth;
    real32 PlayerTop = ScreenCenterY - MetersToPixels*PlayerHeight;
    
    DrawRectangle(Buffer, PlayerLeft, PlayerTop, 
                  PlayerLeft + PlayerWidth*MetersToPixels, 
                  PlayerTop + PlayerHeight*MetersToPixels,
                  PlayerR, PlayerG, PlayerB);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    GameOutputSound(GameState, SoundBuffer, 400);
}

/*
internal void 
RenderWeirdGradient(game_offscreen_buffer* Buffer, int BlueOffset, int GreenOffset)
{
// TODO(rob): Lets see what the optimiser does.

uint8* Row = (uint8*)Buffer->Memory;
for (int y = 0; y < Buffer->Height; ++y)
{
uint32* Pixel = (uint32*)Row;
for (int x = 0; x < Buffer->Width; ++x)
{
uint8 Blue = (uint8)(x + BlueOffset);
uint8 Green = (uint8)(y + GreenOffset);

*Pixel++ = ((Green << 16) | Blue);
}

Row += Buffer->Pitch;
}
}
*/