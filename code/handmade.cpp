#include "handmade.h"

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

inline uint32
RoundReal32ToUInt32(real32 Real32)
{
    uint32 Result = (int32)(Real32 + 0.5f);
    
    return (Result);
}

inline int32
RoundReal32ToInt32(real32 Real32)
{
    int32 Result = (int32)(Real32 + 0.5f);
    
    return (Result);
}


#include "math.h"
inline int32
FloorReal32ToInt32(real32 Real32)
{
    int32 Result = (int32)floorf(Real32);
    return (Result);
}

inline int32
TruncateReal32ToInt32(real32 Real32)
{
    int32 Result = (int32)Real32;
    return (Result);
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

inline tile_map*
GetTileMap(world *World, int32 TileMapX, int32 TileMapY)
{
    tile_map *TileMap = 0;
    
    if ((TileMapX >= 0) &&(TileMapX < World->TileMapCountX) &&
        (TileMapY >= 0) &&(TileMapY < World->TileMapCountY))
    {
        TileMap = &World->TileMaps[TileMapY*World->TileMapCountX + TileMapX];
    }
    return (TileMap);
}

inline uint32
GetTileValueUnchecked(world *World, tile_map *TileMap, int32 TestX, int32 TestY)
{
    Assert(TileMap);
    Assert((TestX >= 0) && (TestX < World->CountX) && 
           (TestY >= 0) && (TestY < World->CountY));
    
    uint32 TileMapValue = TileMap->Tiles[TestY*World->CountX + TestX];
    return (TileMapValue);
}

inline bool32
IsTileMapPointEmpty(world *World, tile_map *TileMap, int32 TestX, int32 TestY)
{
    bool32 Empty = false;
    
    if (TileMap)
    {
        if ((TestX >= 0) && (TestX < World->CountX) && 
            (TestY >= 0) && (TestY < World->CountY))
        {
            uint32 TileMapeValue = GetTileValueUnchecked(World, TileMap, TestX, TestY);
            Empty = (TileMapeValue == 0);
        }
    }
    
    return (Empty);
}

inline canonical_position
GetCanonicalPosition(world *World, raw_position Pos)
{
    canonical_position Result;
    
    Result.TileMapX = Pos.TileMapX;
    Result.TileMapY = Pos.TileMapY;
    
    real32 X = Pos.X - World->UpperLeftX;
    real32 Y = Pos.Y - World->UpperLeftY;
    Result.TileX = FloorReal32ToInt32((Pos.X - World->UpperLeftX) / World->TileWidth);
    Result.TileY = FloorReal32ToInt32((Pos.Y - World->UpperLeftY) / World->TileHeight);
    
    Result.X = X - Result.TileX*World->TileWidth;
    Result.Y = Y - Result.TileY*World->TileHeight;
    
    Assert(Result.X >= 0);
    Assert(Result.Y >= 0);
    Assert(Result.X < World->TileWidth);
    Assert(Result.Y < World->TileHeight);
    
    if (Result.TileX < 0)
    {
        Result.TileX = World->CountX + Result.TileX;
        --Result.TileMapX;
    }
    if (Result.TileY < 0)
    {
        Result.TileY = World->CountY + Result.TileY;
        --Result.TileMapY;
    }
    
    if (Result.TileX >= World->CountX)
    {
        Result.TileX = Result.TileX - World->CountX;
        ++Result.TileMapX;
    }
    if (Result.TileY >= World->CountY)
    {
        Result.TileY = Result.TileY - World->CountY;
        ++Result.TileMapY;
    }
    
    return (Result);
}

internal bool32
IsWorldPointEmpty(world *World, raw_position TestPos)
{
    bool32 Empty = false;
    
    canonical_position CanPos = GetCanonicalPosition(World, TestPos);
    tile_map *TileMap = GetTileMap(World, CanPos.TileMapX, CanPos.TileMapY);
    Empty = IsTileMapPointEmpty(World, TileMap, CanPos.TileX, CanPos.TileY);
    
    return (Empty);
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender) 
{
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) == 
           ArrayCount(Input->Controllers[0].Buttons));
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
    
#define TILE_MAP_COUNT_X 17
#define TILE_MAP_COUNT_Y 9
    
    uint32 Tiles00[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
    {
        {1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1},
        {1, 1, 0, 0,  0, 1, 0, 0,  0, 0, 0, 0,  0, 1, 0, 0,  1},
        {1, 1, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 1, 0,  1},
        {1, 0, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0,  1},
        {1, 0, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0,  0},
        {1, 1, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  0, 1, 0, 0,  1},
        {1, 0, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  1, 0, 0, 0,  1},
        {1, 1, 1, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0, 1, 0, 0,  1},
        {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1,  1}
    };
    
    uint32 Tiles01[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
    {
        {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1,  1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1},
        {1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1}
    };
    
    uint32 Tiles10[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
    {
        {1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1},
        {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1,  1}
    };
    
    uint32 Tiles11[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
    {
        {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1,  1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1},
        {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1},
        {1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1}
    };
    
    tile_map TileMaps[2][2];
    TileMaps[0][0].Tiles = (uint32 *)Tiles00;
    TileMaps[0][1].Tiles = (uint32 *)Tiles10;
    TileMaps[1][0].Tiles = (uint32 *)Tiles01;
    TileMaps[1][1].Tiles = (uint32 *)Tiles11;
    
    world World;
    World.TileMapCountX = 2;
    World.TileMapCountY = 2;
    World.CountX = TILE_MAP_COUNT_X;
    World.CountY = TILE_MAP_COUNT_Y;
    World.UpperLeftX = -30.f;
    World.UpperLeftY = 0.f;
    World.TileWidth = 60.f;
    World.TileHeight = 60.f;
    
    World.TileMaps = (tile_map *)TileMaps;
    
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if (!Memory->IsInitialized)
    {
        GameState->PlayerX = 140.f;
        GameState->PlayerY = 140.f;
        
        Memory->IsInitialized = true;
    }
    
    tile_map *TileMap = GetTileMap(&World, GameState->PlayerTileMapX, GameState->PlayerTileMapY);
    Assert(TileMap);
    
    real32 PlayerWidth = 0.75f*World.TileWidth;
    real32 PlayerHeight = World.TileHeight;
    
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
                dPlayerY = -1.f;
            }
            if (Controller->MoveDown.EndedDown)
            {
                dPlayerY = 1.f;
            }
            if (Controller->MoveLeft.EndedDown)
            {
                dPlayerX = -1.f;
            }
            if (Controller->MoveRight.EndedDown)
            {
                dPlayerX = 1.f;
            }
            dPlayerX *= 64.f;
            dPlayerY *= 64.f;
            
            real32 NewPlayerX = GameState->PlayerX + Input->dtForFrame*dPlayerX;
            real32 NewPlayerY = GameState->PlayerY + Input->dtForFrame*dPlayerY;
            
            raw_position PlayerPos = {
                GameState->PlayerTileMapX, GameState->PlayerTileMapY,
                NewPlayerX, NewPlayerY};
            raw_position PlayerLeft = PlayerPos;
            PlayerLeft.X -= 0.5f*PlayerWidth;
            raw_position PlayerRight = PlayerPos;
            PlayerRight.X += 0.5f*PlayerWidth;
            
            if (IsWorldPointEmpty(&World, PlayerPos) &&
                IsWorldPointEmpty(&World, PlayerLeft) &&
                IsWorldPointEmpty(&World, PlayerRight))
            {
                canonical_position CanPos = GetCanonicalPosition(&World, PlayerPos);
                
                GameState->PlayerTileMapX = CanPos.TileMapX;
                GameState->PlayerTileMapY = CanPos.TileMapY;
                GameState->PlayerX = World.UpperLeftX + World.TileWidth*CanPos.TileX + CanPos.X;
                GameState->PlayerY = World.UpperLeftY + World.TileHeight*CanPos.TileY + CanPos.Y;
            }
        }
    }
    
    DrawRectangle(Buffer, 0.f, 0.f, (real32)Buffer->Width, (real32)Buffer->Height, 1.f, 0.f, 1.f);
    
    for (int Row = 0; Row < TILE_MAP_COUNT_Y; ++Row)
    {
        for (int Column = 0; Column < TILE_MAP_COUNT_X; ++Column)
        {
            uint32 TileID = GetTileValueUnchecked(&World, TileMap, Column, Row);
            real32 Gray = 0.5f;
            if (TileID == 1)
            {
                Gray = 1.f;
            }
            
            real32 MinX = World.UpperLeftX + ((real32)Column)*World.TileWidth;
            real32 MinY = World.UpperLeftY + ((real32)Row)*World.TileHeight;
            real32 MaxX = MinX + World.TileWidth;
            real32 MaxY = MinY + World.TileHeight;
            DrawRectangle(Buffer, MinX, MinY, MaxX, MaxY, Gray, Gray, Gray);
        }
    }
    
    real32 PlayerR = 1.f;
    real32 PlayerG = 1.f;
    real32 PlayerB = 0.f;
    real32 PlayerLeft = GameState->PlayerX - 0.5f*PlayerWidth;
    real32 PlayerTop = GameState->PlayerY - PlayerHeight;
    
    DrawRectangle(Buffer, PlayerLeft, PlayerTop, PlayerLeft + PlayerWidth, PlayerTop + PlayerHeight, 
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