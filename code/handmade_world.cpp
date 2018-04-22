/********************************************************************
 *
 * Author:  Rob Graham.
 * Created: Sun Apr 22 18:27:43 2018
 * Notice:  (C) Copyright 2018 by Rob Graham. All Rights Reserved.
 *
********************************************************************/

#define TILE_CHUNK_SAFE_MARGIN (INT32_MAX/64)
#define TILE_CHUNK_UNINITIALIZED INT32_MAX

inline world_chunk *
GetWorldChunk(world *World, int32 ChunkX, int32 ChunkY, int32 ChunkZ,
              memory_arena *Arena = 0)
{
    Assert(ChunkX > -TILE_CHUNK_SAFE_MARGIN);
    Assert(ChunkY > -TILE_CHUNK_SAFE_MARGIN);
    Assert(ChunkZ > -TILE_CHUNK_SAFE_MARGIN);
    Assert(ChunkX < TILE_CHUNK_SAFE_MARGIN);
    Assert(ChunkY < TILE_CHUNK_SAFE_MARGIN);
    Assert(ChunkZ < TILE_CHUNK_SAFE_MARGIN);
    
    // TODO(casey): Write a better hash function.
    uint32 HashValue = 19*ChunkX + 7*ChunkY + 3*ChunkZ;
    uint32 HashSlot = HashValue & (ArrayCount(World->ChunkHash) - 1);
    Assert(HashSlot < ArrayCount(World->ChunkHash));
    
    world_chunk *Chunk = World->ChunkHash + HashSlot;
    do
    {
        if((ChunkX == Chunk->ChunkX) &&
           (ChunkY == Chunk->ChunkY) &&
           (ChunkZ == Chunk->ChunkZ))
        {            
            break;
        }
        
        if(Arena && (Chunk->ChunkX != TILE_CHUNK_UNINITIALIZED) && (!Chunk->NextInHash))
        {
            Chunk->NextInHash = PushStruct(Arena, world_chunk);
            Chunk = Chunk->NextInHash;
            Chunk->ChunkX = TILE_CHUNK_UNINITIALIZED;
        }
        
        if(Arena && (Chunk->ChunkX == TILE_CHUNK_UNINITIALIZED))
        {
            uint32 TileCount = World->ChunkDim*World->ChunkDim;
            
            Chunk->ChunkX = ChunkX;
            Chunk->ChunkY = ChunkY;
            Chunk->ChunkZ = ChunkZ;
            
            Chunk->NextInHash = 0;
            
            break;
        }
        
        Chunk = Chunk->NextInHash;
    } while(Chunk);
    
    return(Chunk);
}

#if 0
inline world_position
GetChunkPositionFor(world *World, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
    world_position Result;
    
    Result.AbsTileX = AbsTileX >> World->ChunkShift;
    Result.AbsTileY = AbsTileY >> World->ChunkShift;
    Result.AbsTileZ = AbsTileZ;
    Result.RelTileX = AbsTileX & World->ChunkMask;
    Result.RelTileY = AbsTileY & World->ChunkMask;
    
    return(Result);
}
#endif

internal void
InitializeWorld(world *World, real32 TileSideInMeters)
{
    World->ChunkShift = 4;
    World->ChunkMask = (1 << World->ChunkShift) - 1;
    World->ChunkDim = (1 << World->ChunkShift);        
    World->TileSideInMeters = TileSideInMeters;
    
    for(uint32 WorldChunkIndex = 0;
        WorldChunkIndex < ArrayCount(World->ChunkHash);
        ++WorldChunkIndex)
    {
        World->ChunkHash[WorldChunkIndex].ChunkX = TILE_CHUNK_UNINITIALIZED;
    }
}

inline void
RecanonicalizeCoord(world *World, int32 *Tile, real32 *TileRel)
{
    // TODO(Rob): Need to do something that doesn't use the divide/multiply method
    // for recanonicalizing because this can end up rounding back on to the tile
    // you just came from.
    
    // NOTE(Rob): World is assumed to be toroidal topology, if you
    // step off one end you come back on the other!
    int32 Offset = RoundReal32ToInt32(*TileRel / World->TileSideInMeters);
    *Tile += Offset;
    *TileRel -= Offset*World->TileSideInMeters;
    
    // TODO(Rob): Fix floating point math so this can be exact?
    Assert(*TileRel > -0.5f*World->TileSideInMeters);
    Assert(*TileRel < 0.5f*World->TileSideInMeters);
}

inline world_position
MapIntoTileSpace(world *World, world_position BasePos, v2 Offset)
{
    world_position Result = BasePos;
    
    Result.Offset_ += Offset;
    RecanonicalizeCoord(World, &Result.AbsTileX, &Result.Offset_.X);
    RecanonicalizeCoord(World, &Result.AbsTileY, &Result.Offset_.Y);
    
    return(Result);
}

inline bool32
AreOnSameTile(world_position *A, world_position *B)
{
    bool32 Result = ((A->AbsTileX == B->AbsTileX) &&
                     (A->AbsTileY == B->AbsTileY) &&
                     (A->AbsTileZ == B->AbsTileZ));
    
    return(Result);
}

inline world_difference
Subtract(world *World, world_position *A, world_position *B)
{
    world_difference Result;
    
    v2 dTileXY = {(real32)A->AbsTileX - (real32)B->AbsTileX,
        (real32)A->AbsTileY - (real32)B->AbsTileY};
    real32 dTileZ = (real32)A->AbsTileZ - (real32)B->AbsTileZ;
    
    Result.dXY = World->TileSideInMeters*dTileXY + (A->Offset_ - B->Offset_);
    
    // TODO(Rob): Think about what we want to do about Z
    Result.dZ = World->TileSideInMeters*dTileZ;
    
    return(Result);
}

inline world_position
CenteredTilePoint(uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ)
{
    world_position Result = {};
    
    Result.AbsTileX = AbsTileX;
    Result.AbsTileY = AbsTileY;
    Result.AbsTileZ = AbsTileZ;
    
    return(Result);
}