/********************************************************************
 *
 *Author:  Rob Graham.
 *Created: Sun Nov 19 18:57:47 2017
 *Notice:  (C) Copyright 2017-2018 by Rob Graham. All Rights Reserved.
 *
********************************************************************/

#if !defined(HANDMADE_TILE_H)

struct tile_map_difference
{
    v2 dXY;
    real32 dZ;
};

struct tile_map_position
{
    // NOTE(Rob): These are fixed point tile locations. The hight bits are the tile chunk index,
    // and the low bits are the tile index in the chunk.
    uint32 AbsTileX;
    uint32 AbsTileY;
    uint32 AbsTileZ;
    
    v2 Offset_;
};

struct tile_chunk_position
{
    uint32 TileChunkX;
    uint32 TileChunkY;
    uint32 TileChunkZ;
    
    uint32 RelTileX;
    uint32 RelTileY;
};

struct tile_chunk
{
    uint32 TileChunkX;
    uint32 TileChunkY;
    uint32 TileChunkZ;
    
    uint32 *Tiles;
    
    tile_chunk *NextInHash;
};

struct tile_map
{
    uint32 ChunkShift;
    uint32 ChunkMask;
    uint32 ChunkDim;
    
    real32 TileSideInMeters;
    
    tile_chunk *TileChunkHash[4096];
};

#define HANDMADE_TILE_H
#endif // HANDMADE_TILE_H