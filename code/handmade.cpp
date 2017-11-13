#include "handmade.h"
#include "handmade_intrinsics.h"

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

inline tile_chunk*
GetTileChunk(world *World, int32 TileChunkX, int32 TileChunkY)
{
    tile_chunk *TileChunk = 0;
    
    if ((TileChunkX >= 0) &&(TileChunkX < World->TileChunkCountX) &&
        (TileChunkY >= 0) &&(TileChunkY < World->TileChunkCountY))
    {
        TileChunk = &World->TileChunks[TileChunkY*World->TileChunkCountX + TileChunkX];
    }
    return (TileChunk);
}

inline uint32
GetTileValueUnchecked(world *World, tile_chunk *TileChunk, uint32 TileX, uint32 TileY)
{
    Assert(TileChunk);
    Assert(TileX < World->ChunkDim);
    Assert(TileY < World->ChunkDim);
    
    uint32 TileChunkValue = TileChunk->Tiles[TileY*World->ChunkDim + TileX];
    return (TileChunkValue);
}

inline bool32
GetTileValue(world *World, tile_chunk *TileChunk, uint32 TestX, uint32 TestY)
{
    uint32 TileChunkValue = 0;
    
    if (TileChunk)
    {
        TileChunkValue = GetTileValueUnchecked(World, TileChunk, TestX, TestY);
    }
    
    return (TileChunkValue);
}

inline void
RecanonicalizeCoord(world *World, uint32 *Tile, real32 *TileRel)
{
    int32 Offset = FloorReal32ToInt32(*TileRel / World->TileSideInMeters);
    *Tile += Offset;
    *TileRel -= Offset*World->TileSideInMeters;
    
    Assert(*TileRel >= 0);
    Assert(*TileRel <= World->TileSideInMeters);
}

inline world_position
RecanonicalizePosition(world *World, world_position Pos)
{
    world_position Result = Pos;
    
    RecanonicalizeCoord(World, &Result.AbsTileX, &Result.TileRelX);
    RecanonicalizeCoord(World, &Result.AbsTileY, &Result.TileRelY);
    
    return (Result);
}

inline tile_chunk_position
GetChunkPositionFor(world *World, uint32 AbsTileX, uint32 AbsTileY)
{
    tile_chunk_position Result;
    
    Result.TileChunkX = AbsTileX >> World->ChunkShift;
    Result.TileChunkY = AbsTileY >> World->ChunkShift;
    Result.RelTileX = AbsTileX & World->ChunkMask;
    Result.RelTileY = AbsTileY & World->ChunkMask;
    
    return (Result);
}

internal uint32
GetTileValue(world *World, uint32 AbsTileX, uint32 AbsTileY)
{
    tile_chunk_position ChunkPos = GetChunkPositionFor(World, AbsTileX, AbsTileY);
    tile_chunk *TileChunk = GetTileChunk(World, ChunkPos.TileChunkX, ChunkPos.TileChunkY);
    uint32 TileChunkValue = GetTileValue(World, TileChunk, ChunkPos.RelTileX, ChunkPos.RelTileY);
    
    return (TileChunkValue);
}

internal bool32
IsWorldPointEmpty(world *World, world_position WorldPos)
{
    uint32 TileChunkValue = GetTileValue(World, WorldPos.AbsTileX, WorldPos.AbsTileY);
    bool32 Empty = (TileChunkValue == 0);
    
    return (Empty);
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender) 
{
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) == 
           ArrayCount(Input->Controllers[0].Buttons));
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
    
#define TILE_MAP_COUNT_X 256
#define TILE_MAP_COUNT_Y 256
    
    uint32 TempTiles[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
    {
        {1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, 1},
        {1, 1, 0, 0,  0, 1, 0, 0,  0, 0, 0, 0,  0, 1, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 1, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 1, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 1, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  0, 1, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  1, 0, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 1, 1, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0, 1, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1},
        {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, 1},
        {1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1, 1}
    };
    
    world World = {};
    World.ChunkShift = 8;
    World.ChunkMask = (1 << World.ChunkShift) - 1;
    World.ChunkDim = 256;
    
    real32 LowerLeftX = -(real32)World.TileSideInPixels/2;
    real32 LowerLeftY = (real32)Buffer->Height;
    
    World.TileChunkCountX = 1;
    World.TileChunkCountY = 1;
    
    tile_chunk TileChunk;
    TileChunk.Tiles = (uint32 *)TempTiles;
    World.TileChunks = &TileChunk;
    
    World.TileSideInMeters = 1.4f;
    World.TileSideInPixels = 60;
    World.MetersToPixels = (real32)World.TileSideInPixels / (real32)World.TileSideInMeters;
    
    real32 PlayerHeight = 1.4f;
    real32 PlayerWidth = 0.75f*PlayerHeight;
    
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if (!Memory->IsInitialized)
    {
        GameState->PlayerP.AbsTileX = 3;
        GameState->PlayerP.AbsTileY = 3;
        GameState->PlayerP.TileRelX = 5;
        GameState->PlayerP.TileRelY = 5;
        
        Memory->IsInitialized = true;
    }
    
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
            dPlayerX *= 2.f;
            dPlayerY *= 2.f;
            
            world_position NewPlayerP = GameState->PlayerP;
            NewPlayerP.TileRelX += Input->dtForFrame*dPlayerX;
            NewPlayerP.TileRelY += Input->dtForFrame*dPlayerY;
            NewPlayerP = RecanonicalizePosition(&World, NewPlayerP);
            
            world_position PlayerLeft = NewPlayerP;
            PlayerLeft.TileRelX -= 0.5f*PlayerWidth;
            PlayerLeft = RecanonicalizePosition(&World, PlayerLeft);
            
            world_position PlayerRight = NewPlayerP;
            PlayerRight.TileRelX += 0.5f*PlayerWidth;
            PlayerRight = RecanonicalizePosition(&World, PlayerRight);
            
            if (IsWorldPointEmpty(&World, NewPlayerP) &&
                IsWorldPointEmpty(&World, PlayerLeft) &&
                IsWorldPointEmpty(&World, PlayerRight))
            {
                GameState->PlayerP = NewPlayerP;
            }
        }
    }
    
    DrawRectangle(Buffer, 0.f, 0.f, (real32)Buffer->Width, (real32)Buffer->Height, 1.f, 0.f, 1.f);
    
    real32 CenterX = 0.5f*(real32)Buffer->Width;
    real32 CenterY = 0.5f*(real32)Buffer->Height;
    
    for (int32 RelRow = -10; RelRow < 10; ++RelRow)
    {
        for (int32 RelColumn = -20; RelColumn < 20; ++RelColumn)
        {
            uint32 Column = RelColumn + GameState->PlayerP.AbsTileX;
            uint32 Row = RelRow + GameState->PlayerP.AbsTileY;
            uint32 TileID = GetTileValue(&World, Column, Row);
            real32 Gray = 0.5f;
            if (TileID == 1)
            {
                Gray = 1.f;
            }
            
            if((Column == GameState->PlayerP.AbsTileX) && (Row == GameState->PlayerP.AbsTileY))
            {
                Gray = 0.f;
            }
            
            real32 MinX = CenterX + ((real32)RelColumn)*World.TileSideInPixels;
            real32 MinY = CenterY - ((real32)RelRow)*World.TileSideInPixels;
            real32 MaxX = MinX + World.TileSideInPixels;
            real32 MaxY = MinY - World.TileSideInPixels;
            DrawRectangle(Buffer, MinX, MaxY, MaxX, MinY, Gray, Gray, Gray);
        }
    }
    
    real32 PlayerR = 1.f;
    real32 PlayerG = 1.f;
    real32 PlayerB = 0.f;
    real32 PlayerLeft = CenterX + World.MetersToPixels*GameState->PlayerP.TileRelX - 0.5f*World.MetersToPixels*PlayerWidth;
    real32 PlayerTop = CenterY - World.MetersToPixels*GameState->PlayerP.TileRelY - World.MetersToPixels*PlayerHeight;
    
    DrawRectangle(Buffer, PlayerLeft, PlayerTop, 
                  PlayerLeft + PlayerWidth*World.MetersToPixels, 
                  PlayerTop + PlayerHeight*World.MetersToPixels,
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