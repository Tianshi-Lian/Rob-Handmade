/********************************************************************
 *
 * Author:  Rob Graham.
 * Created: Sun Apr 22 18:26:14 2018
 * Notice:  (C) Copyright 2018 by Rob Graham. All Rights Reserved.
 *
********************************************************************/

#if !defined(HANDMADE_WORLD_H)

struct world_difference
{
    v2 dXY;
    real32 dZ;
};

struct world_position
{
    // NOTE(Rob): These are fixed point tile locations. The hight bits are the tile chunk index,
    // and the low bits are the tile index in the chunk.
    int32 AbsTileX;
    int32 AbsTileY;
    int32 AbsTileZ;
    
    v2 Offset_;
};

struct world_entity_block
{
    world_entity_block *Next;
    uint32 EntityCount;
    uint32 LowEntityIndex[16];
};

struct world_chunk
{
    int32 ChunkX;
    int32 ChunkY;
    int32 ChunkZ;
    
    world_entity_block FirstBlock;
    
    world_chunk *NextInHash;
};

struct world
{
    real32 TileSideInMeters;
    
    int32 ChunkShift;
    int32 ChunkMask;
    int32 ChunkDim;
    world_chunk ChunkHash[4096];
};

#define HANDMADE_WORLD_H
#endif