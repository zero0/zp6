
#include "Core/Allocator.h"

constexpr zp::MemoryLabel kSTBMemoryLabel = zp::MemoryLabels::Default;


#define STBI_MALLOC( size )         ZP_MALLOC( kSTBMemoryLabel, size )
#define STBI_REALLOC( ptr, size )   ZP_REALLOC( kSTBMemoryLabel, ptr, size )
#define STBI_FREE( ptr )            ZP_SAFE_FREE( kSTBMemoryLabel, ptr )

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
