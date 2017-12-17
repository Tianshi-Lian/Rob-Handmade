#if !defined(HANDMADE_TILE_H)

struct tile_map_position
{
    // NOTE(ROB): These are fixed point tile locations. The hight bits are the tile chunk index,
    // and the low bits are the tile index in the chunk.
    uint32 AbsTileX;
    uint32 AbsTileY;
    uint32 AbsTileZ;
    
    real32 TileRelX;
    real32 TileRelY;
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
    uint32 *Tiles;
};

struct tile_map
{
    uint32 ChunkShift;
    uint32 ChunkMask;
    uint32 ChunkDim;
    
    real32 TileSideInMeters;
    
    uint32 TileChunkCountX;
    uint32 TileChunkCountY;
    uint32 TileChunkCountZ;
    
    tile_chunk *TileChunks;
};

#define HANDMADE_TILE_H
#endif // HANDMADE_TILE_H