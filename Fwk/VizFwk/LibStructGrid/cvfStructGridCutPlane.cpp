//##################################################################################################
//
//   Custom Visualization Core library
//   Copyright (C) 2011-2013 Ceetron AS
//
//   This library may be used under the terms of either the GNU General Public License or
//   the GNU Lesser General Public License as follows:
//
//   GNU General Public License Usage
//   This library is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   This library is distributed in the hope that it will be useful, but WITHOUT ANY
//   WARRANTY; without even the implied warranty of MERCHANTABILITY or
//   FITNESS FOR A PARTICULAR PURPOSE.
//
//   See the GNU General Public License at <<http://www.gnu.org/licenses/gpl.html>>
//   for more details.
//
//   GNU Lesser General Public License Usage
//   This library is free software; you can redistribute it and/or modify
//   it under the terms of the GNU Lesser General Public License as published by
//   the Free Software Foundation; either version 2.1 of the License, or
//   (at your option) any later version.
//
//   This library is distributed in the hope that it will be useful, but WITHOUT ANY
//   WARRANTY; without even the implied warranty of MERCHANTABILITY or
//   FITNESS FOR A PARTICULAR PURPOSE.
//
//   See the GNU Lesser General Public License at <<http://www.gnu.org/licenses/lgpl-2.1.html>>
//   for more details.
//
//##################################################################################################


#include "cvfBase.h"
#include "cvfStructGridCutPlane.h"
#include "cvfRectilinearGrid.h"
#include "cvfGeometryBuilderDrawableGeo.h"
#include "cvfPrimitiveSetIndexedUInt.h"
#include "cvfDebugTimer.h"
#include "cvfPlane.h"
#include "cvfScalarMapper.h"
#include "cvfEdgeKey.h"
#include "cvfMeshEdgeExtractor.h"

#include <map>
#include <cstring>

namespace cvf {



//==================================================================================================
///
/// \class cvf::StructGridCutPlane
/// \ingroup StructGrid
///
/// 
///
//==================================================================================================

// Based on description and implementation from Paul Bourke:
//   http://local.wasp.uwa.edu.au/~pbourke/geometry/polygonise/

const uint StructGridCutPlane::sm_edgeTable[256] = 
{
    0x0  , 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c,
    0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
    0x190, 0x99 , 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c,
    0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
    0x230, 0x339, 0x33 , 0x13a, 0x636, 0x73f, 0x435, 0x53c,
    0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
    0x3a0, 0x2a9, 0x1a3, 0xaa , 0x7a6, 0x6af, 0x5a5, 0x4ac,
    0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0,
    0x460, 0x569, 0x663, 0x76a, 0x66 , 0x16f, 0x265, 0x36c,
    0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60,
    0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0xff , 0x3f5, 0x2fc,
    0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0,
    0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x55 , 0x15c,
    0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
    0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0xcc ,
    0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
    0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc,
    0xcc , 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
    0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c,
    0x15c, 0x55 , 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
    0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc,
    0x2fc, 0x3f5, 0xff , 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
    0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c,
    0x36c, 0x265, 0x16f, 0x66 , 0x76a, 0x663, 0x569, 0x460,
    0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac,
    0x4ac, 0x5a5, 0x6af, 0x7a6, 0xaa , 0x1a3, 0x2a9, 0x3a0,
    0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c,
    0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x33 , 0x339, 0x230,
    0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c,
    0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x99 , 0x190,
    0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c,
    0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x0   
};

const int StructGridCutPlane::sm_triTable[256][16] = 
{
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 8, 3, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {9, 2, 10, 0, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {2, 8, 3, 2, 10, 8, 10, 9, 8, -1, -1, -1, -1, -1, -1, -1},
    {3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 11, 2, 8, 11, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 9, 0, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 11, 2, 1, 9, 11, 9, 8, 11, -1, -1, -1, -1, -1, -1, -1},
    {3, 10, 1, 11, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 10, 1, 0, 8, 10, 8, 11, 10, -1, -1, -1, -1, -1, -1, -1},
    {3, 9, 0, 3, 11, 9, 11, 10, 9, -1, -1, -1, -1, -1, -1, -1},
    {9, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {4, 3, 0, 7, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 1, 9, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {4, 1, 9, 4, 7, 1, 7, 3, 1, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 10, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {3, 4, 7, 3, 0, 4, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1},
    {9, 2, 10, 9, 0, 2, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
    {2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1, -1, -1},
    {8, 4, 7, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {11, 4, 7, 11, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1, -1},
    {9, 0, 1, 8, 4, 7, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
    {4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, -1, -1, -1, -1},
    {3, 10, 1, 3, 11, 10, 7, 8, 4, -1, -1, -1, -1, -1, -1, -1},
    {1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, -1, -1, -1, -1},
    {4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3, -1, -1, -1, -1},
    {4, 7, 11, 4, 11, 9, 9, 11, 10, -1, -1, -1, -1, -1, -1, -1},
    {9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {9, 5, 4, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 5, 4, 1, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {8, 5, 4, 8, 3, 5, 3, 1, 5, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 10, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {3, 0, 8, 1, 2, 10, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
    {5, 2, 10, 5, 4, 2, 4, 0, 2, -1, -1, -1, -1, -1, -1, -1},
    {2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, -1, -1, -1, -1},
    {9, 5, 4, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 11, 2, 0, 8, 11, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
    {0, 5, 4, 0, 1, 5, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
    {2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, -1, -1, -1, -1},
    {10, 3, 11, 10, 1, 3, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1},
    {4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10, -1, -1, -1, -1},
    {5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, -1, -1, -1, -1},
    {5, 4, 8, 5, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1},
    {9, 7, 8, 5, 7, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {9, 3, 0, 9, 5, 3, 5, 7, 3, -1, -1, -1, -1, -1, -1, -1},
    {0, 7, 8, 0, 1, 7, 1, 5, 7, -1, -1, -1, -1, -1, -1, -1},
    {1, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {9, 7, 8, 9, 5, 7, 10, 1, 2, -1, -1, -1, -1, -1, -1, -1},
    {10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, -1, -1, -1, -1},
    {8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2, -1, -1, -1, -1},
    {2, 10, 5, 2, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1},
    {7, 9, 5, 7, 8, 9, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1},
    {9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, -1, -1, -1, -1},
    {2, 3, 11, 0, 1, 8, 1, 7, 8, 1, 5, 7, -1, -1, -1, -1},
    {11, 2, 1, 11, 1, 7, 7, 1, 5, -1, -1, -1, -1, -1, -1, -1},
    {9, 5, 8, 8, 5, 7, 10, 1, 3, 10, 3, 11, -1, -1, -1, -1},
    {5, 7, 0, 5, 0, 9, 7, 11, 0, 1, 0, 10, 11, 10, 0, -1},
    {11, 10, 0, 11, 0, 3, 10, 5, 0, 8, 0, 7, 5, 7, 0, -1},
    {11, 10, 5, 7, 11, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 8, 3, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {9, 0, 1, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 8, 3, 1, 9, 8, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
    {1, 6, 5, 2, 6, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 6, 5, 1, 2, 6, 3, 0, 8, -1, -1, -1, -1, -1, -1, -1},
    {9, 6, 5, 9, 0, 6, 0, 2, 6, -1, -1, -1, -1, -1, -1, -1},
    {5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, -1, -1, -1, -1},
    {2, 3, 11, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {11, 0, 8, 11, 2, 0, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
    {0, 1, 9, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
    {5, 10, 6, 1, 9, 2, 9, 11, 2, 9, 8, 11, -1, -1, -1, -1},
    {6, 3, 11, 6, 5, 3, 5, 1, 3, -1, -1, -1, -1, -1, -1, -1},
    {0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, -1, -1, -1, -1},
    {3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, -1, -1, -1, -1},
    {6, 5, 9, 6, 9, 11, 11, 9, 8, -1, -1, -1, -1, -1, -1, -1},
    {5, 10, 6, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {4, 3, 0, 4, 7, 3, 6, 5, 10, -1, -1, -1, -1, -1, -1, -1},
    {1, 9, 0, 5, 10, 6, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
    {10, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4, -1, -1, -1, -1},
    {6, 1, 2, 6, 5, 1, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7, -1, -1, -1, -1},
    {8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6, -1, -1, -1, -1},
    {7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9, -1},
    {3, 11, 2, 7, 8, 4, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
    {5, 10, 6, 4, 7, 2, 4, 2, 0, 2, 7, 11, -1, -1, -1, -1},
    {0, 1, 9, 4, 7, 8, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1},
    {9, 2, 1, 9, 11, 2, 9, 4, 11, 7, 11, 4, 5, 10, 6, -1},
    {8, 4, 7, 3, 11, 5, 3, 5, 1, 5, 11, 6, -1, -1, -1, -1},
    {5, 1, 11, 5, 11, 6, 1, 0, 11, 7, 11, 4, 0, 4, 11, -1},
    {0, 5, 9, 0, 6, 5, 0, 3, 6, 11, 6, 3, 8, 4, 7, -1},
    {6, 5, 9, 6, 9, 11, 4, 7, 9, 7, 11, 9, -1, -1, -1, -1},
    {10, 4, 9, 6, 4, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {4, 10, 6, 4, 9, 10, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1},
    {10, 0, 1, 10, 6, 0, 6, 4, 0, -1, -1, -1, -1, -1, -1, -1},
    {8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 10, -1, -1, -1, -1},
    {1, 4, 9, 1, 2, 4, 2, 6, 4, -1, -1, -1, -1, -1, -1, -1},
    {3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4, -1, -1, -1, -1},
    {0, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {8, 3, 2, 8, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1},
    {10, 4, 9, 10, 6, 4, 11, 2, 3, -1, -1, -1, -1, -1, -1, -1},
    {0, 8, 2, 2, 8, 11, 4, 9, 10, 4, 10, 6, -1, -1, -1, -1},
    {3, 11, 2, 0, 1, 6, 0, 6, 4, 6, 1, 10, -1, -1, -1, -1},
    {6, 4, 1, 6, 1, 10, 4, 8, 1, 2, 1, 11, 8, 11, 1, -1},
    {9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3, -1, -1, -1, -1},
    {8, 11, 1, 8, 1, 0, 11, 6, 1, 9, 1, 4, 6, 4, 1, -1},
    {3, 11, 6, 3, 6, 0, 0, 6, 4, -1, -1, -1, -1, -1, -1, -1},
    {6, 4, 8, 11, 6, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {7, 10, 6, 7, 8, 10, 8, 9, 10, -1, -1, -1, -1, -1, -1, -1},
    {0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10, -1, -1, -1, -1},
    {10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0, -1, -1, -1, -1},
    {10, 6, 7, 10, 7, 1, 1, 7, 3, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, -1, -1, -1, -1},
    {2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9, -1},
    {7, 8, 0, 7, 0, 6, 6, 0, 2, -1, -1, -1, -1, -1, -1, -1},
    {7, 3, 2, 6, 7, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {2, 3, 11, 10, 6, 8, 10, 8, 9, 8, 6, 7, -1, -1, -1, -1},
    {2, 0, 7, 2, 7, 11, 0, 9, 7, 6, 7, 10, 9, 10, 7, -1},
    {1, 8, 0, 1, 7, 8, 1, 10, 7, 6, 7, 10, 2, 3, 11, -1},
    {11, 2, 1, 11, 1, 7, 10, 6, 1, 6, 7, 1, -1, -1, -1, -1},
    {8, 9, 6, 8, 6, 7, 9, 1, 6, 11, 6, 3, 1, 3, 6, -1},
    {0, 9, 1, 11, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {7, 8, 0, 7, 0, 6, 3, 11, 0, 11, 6, 0, -1, -1, -1, -1},
    {7, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {3, 0, 8, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 1, 9, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {8, 1, 9, 8, 3, 1, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
    {10, 1, 2, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 10, 3, 0, 8, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
    {2, 9, 0, 2, 10, 9, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
    {6, 11, 7, 2, 10, 3, 10, 8, 3, 10, 9, 8, -1, -1, -1, -1},
    {7, 2, 3, 6, 2, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {7, 0, 8, 7, 6, 0, 6, 2, 0, -1, -1, -1, -1, -1, -1, -1},
    {2, 7, 6, 2, 3, 7, 0, 1, 9, -1, -1, -1, -1, -1, -1, -1},
    {1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, -1, -1, -1, -1},
    {10, 7, 6, 10, 1, 7, 1, 3, 7, -1, -1, -1, -1, -1, -1, -1},
    {10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8, -1, -1, -1, -1},
    {0, 3, 7, 0, 7, 10, 0, 10, 9, 6, 10, 7, -1, -1, -1, -1},
    {7, 6, 10, 7, 10, 8, 8, 10, 9, -1, -1, -1, -1, -1, -1, -1},
    {6, 8, 4, 11, 8, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {3, 6, 11, 3, 0, 6, 0, 4, 6, -1, -1, -1, -1, -1, -1, -1},
    {8, 6, 11, 8, 4, 6, 9, 0, 1, -1, -1, -1, -1, -1, -1, -1},
    {9, 4, 6, 9, 6, 3, 9, 3, 1, 11, 3, 6, -1, -1, -1, -1},
    {6, 8, 4, 6, 11, 8, 2, 10, 1, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 10, 3, 0, 11, 0, 6, 11, 0, 4, 6, -1, -1, -1, -1},
    {4, 11, 8, 4, 6, 11, 0, 2, 9, 2, 10, 9, -1, -1, -1, -1},
    {10, 9, 3, 10, 3, 2, 9, 4, 3, 11, 3, 6, 4, 6, 3, -1},
    {8, 2, 3, 8, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1},
    {0, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8, -1, -1, -1, -1},
    {1, 9, 4, 1, 4, 2, 2, 4, 6, -1, -1, -1, -1, -1, -1, -1},
    {8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 10, 1, -1, -1, -1, -1},
    {10, 1, 0, 10, 0, 6, 6, 0, 4, -1, -1, -1, -1, -1, -1, -1},
    {4, 6, 3, 4, 3, 8, 6, 10, 3, 0, 3, 9, 10, 9, 3, -1},
    {10, 9, 4, 6, 10, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {4, 9, 5, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 8, 3, 4, 9, 5, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
    {5, 0, 1, 5, 4, 0, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
    {11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5, -1, -1, -1, -1},
    {9, 5, 4, 10, 1, 2, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
    {6, 11, 7, 1, 2, 10, 0, 8, 3, 4, 9, 5, -1, -1, -1, -1},
    {7, 6, 11, 5, 4, 10, 4, 2, 10, 4, 0, 2, -1, -1, -1, -1},
    {3, 4, 8, 3, 5, 4, 3, 2, 5, 10, 5, 2, 11, 7, 6, -1},
    {7, 2, 3, 7, 6, 2, 5, 4, 9, -1, -1, -1, -1, -1, -1, -1},
    {9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7, -1, -1, -1, -1},
    {3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0, -1, -1, -1, -1},
    {6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8, -1},
    {9, 5, 4, 10, 1, 6, 1, 7, 6, 1, 3, 7, -1, -1, -1, -1},
    {1, 6, 10, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4, -1},
    {4, 0, 10, 4, 10, 5, 0, 3, 10, 6, 10, 7, 3, 7, 10, -1},
    {7, 6, 10, 7, 10, 8, 5, 4, 10, 4, 8, 10, -1, -1, -1, -1},
    {6, 9, 5, 6, 11, 9, 11, 8, 9, -1, -1, -1, -1, -1, -1, -1},
    {3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9, 5, -1, -1, -1, -1},
    {0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11, -1, -1, -1, -1},
    {6, 11, 3, 6, 3, 5, 5, 3, 1, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 10, 9, 5, 11, 9, 11, 8, 11, 5, 6, -1, -1, -1, -1},
    {0, 11, 3, 0, 6, 11, 0, 9, 6, 5, 6, 9, 1, 2, 10, -1},
    {11, 8, 5, 11, 5, 6, 8, 0, 5, 10, 5, 2, 0, 2, 5, -1},
    {6, 11, 3, 6, 3, 5, 2, 10, 3, 10, 5, 3, -1, -1, -1, -1},
    {5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, -1, -1, -1, -1},
    {9, 5, 6, 9, 6, 0, 0, 6, 2, -1, -1, -1, -1, -1, -1, -1},
    {1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8, -1},
    {1, 5, 6, 2, 1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 3, 6, 1, 6, 10, 3, 8, 6, 5, 6, 9, 8, 9, 6, -1},
    {10, 1, 0, 10, 0, 6, 9, 5, 0, 5, 6, 0, -1, -1, -1, -1},
    {0, 3, 8, 5, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {10, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {11, 5, 10, 7, 5, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {11, 5, 10, 11, 7, 5, 8, 3, 0, -1, -1, -1, -1, -1, -1, -1},
    {5, 11, 7, 5, 10, 11, 1, 9, 0, -1, -1, -1, -1, -1, -1, -1},
    {10, 7, 5, 10, 11, 7, 9, 8, 1, 8, 3, 1, -1, -1, -1, -1},
    {11, 1, 2, 11, 7, 1, 7, 5, 1, -1, -1, -1, -1, -1, -1, -1},
    {0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 11, -1, -1, -1, -1},
    {9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 11, 7, -1, -1, -1, -1},
    {7, 5, 2, 7, 2, 11, 5, 9, 2, 3, 2, 8, 9, 8, 2, -1},
    {2, 5, 10, 2, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1},
    {8, 2, 0, 8, 5, 2, 8, 7, 5, 10, 2, 5, -1, -1, -1, -1},
    {9, 0, 1, 5, 10, 3, 5, 3, 7, 3, 10, 2, -1, -1, -1, -1},
    {9, 8, 2, 9, 2, 1, 8, 7, 2, 10, 2, 5, 7, 5, 2, -1},
    {1, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 8, 7, 0, 7, 1, 1, 7, 5, -1, -1, -1, -1, -1, -1, -1},
    {9, 0, 3, 9, 3, 5, 5, 3, 7, -1, -1, -1, -1, -1, -1, -1},
    {9, 8, 7, 5, 9, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {5, 8, 4, 5, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1},
    {5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0, -1, -1, -1, -1},
    {0, 1, 9, 8, 4, 10, 8, 10, 11, 10, 4, 5, -1, -1, -1, -1},
    {10, 11, 4, 10, 4, 5, 11, 3, 4, 9, 4, 1, 3, 1, 4, -1},
    {2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8, -1, -1, -1, -1},
    {0, 4, 11, 0, 11, 3, 4, 5, 11, 2, 11, 1, 5, 1, 11, -1},
    {0, 2, 5, 0, 5, 9, 2, 11, 5, 4, 5, 8, 11, 8, 5, -1},
    {9, 4, 5, 2, 11, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4, -1, -1, -1, -1},
    {5, 10, 2, 5, 2, 4, 4, 2, 0, -1, -1, -1, -1, -1, -1, -1},
    {3, 10, 2, 3, 5, 10, 3, 8, 5, 4, 5, 8, 0, 1, 9, -1},
    {5, 10, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2, -1, -1, -1, -1},
    {8, 4, 5, 8, 5, 3, 3, 5, 1, -1, -1, -1, -1, -1, -1, -1},
    {0, 4, 5, 1, 0, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5, -1, -1, -1, -1},
    {9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {4, 11, 7, 4, 9, 11, 9, 10, 11, -1, -1, -1, -1, -1, -1, -1},
    {0, 8, 3, 4, 9, 7, 9, 11, 7, 9, 10, 11, -1, -1, -1, -1},
    {1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11, -1, -1, -1, -1},
    {3, 1, 4, 3, 4, 8, 1, 10, 4, 7, 4, 11, 10, 11, 4, -1},
    {4, 11, 7, 9, 11, 4, 9, 2, 11, 9, 1, 2, -1, -1, -1, -1},
    {9, 7, 4, 9, 11, 7, 9, 1, 11, 2, 11, 1, 0, 8, 3, -1},
    {11, 7, 4, 11, 4, 2, 2, 4, 0, -1, -1, -1, -1, -1, -1, -1},
    {11, 7, 4, 11, 4, 2, 8, 3, 4, 3, 2, 4, -1, -1, -1, -1},
    {2, 9, 10, 2, 7, 9, 2, 3, 7, 7, 4, 9, -1, -1, -1, -1},
    {9, 10, 7, 9, 7, 4, 10, 2, 7, 8, 7, 0, 2, 0, 7, -1},
    {3, 7, 10, 3, 10, 2, 7, 4, 10, 1, 10, 0, 4, 0, 10, -1},
    {1, 10, 2, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {4, 9, 1, 4, 1, 7, 7, 1, 3, -1, -1, -1, -1, -1, -1, -1},
    {4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1, -1, -1, -1, -1},
    {4, 0, 3, 7, 4, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {9, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {3, 0, 9, 3, 9, 11, 11, 9, 10, -1, -1, -1, -1, -1, -1, -1},
    {0, 1, 10, 0, 10, 8, 8, 10, 11, -1, -1, -1, -1, -1, -1, -1},
    {3, 1, 10, 11, 3, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 2, 11, 1, 11, 9, 9, 11, 8, -1, -1, -1, -1, -1, -1, -1},
    {3, 0, 9, 3, 9, 11, 1, 2, 9, 2, 11, 9, -1, -1, -1, -1},
    {0, 2, 11, 8, 0, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {3, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {2, 3, 8, 2, 8, 10, 10, 8, 9, -1, -1, -1, -1, -1, -1, -1},
    {9, 10, 2, 0, 9, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {2, 3, 8, 2, 8, 10, 0, 1, 8, 1, 10, 8, -1, -1, -1, -1},
    {1, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {1, 3, 8, 9, 1, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 9, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}
};


//--------------------------------------------------------------------------------------------------
/// Constructor
//--------------------------------------------------------------------------------------------------
StructGridCutPlane::StructGridCutPlane(const RectilinearGrid* grid)
:   m_grid(grid),
    m_mapScalarSetIndex(UNDEFINED_UINT),
    m_scalarMapper(NULL),
    m_mapNodeAveragedScalars(false),
    m_mustRecompute(true)
{
    CVF_ASSERT(grid);
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
StructGridCutPlane::~StructGridCutPlane()
{
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void StructGridCutPlane::setPlane(const Plane& plane)
{
    m_plane = plane;

    m_mustRecompute = true;
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void StructGridCutPlane::setMapScalar(uint scalarSetIndex, const ScalarMapper* mapper, bool nodeAveragedScalars)
{
    CVF_ASSERT(scalarSetIndex < m_grid->scalarSetCount());
    CVF_ASSERT(mapper);

    m_mapScalarSetIndex = scalarSetIndex;
    m_scalarMapper = mapper;
    m_mapNodeAveragedScalars = nodeAveragedScalars;

    m_mustRecompute = true;
}


//--------------------------------------------------------------------------------------------------
/// Generate cut plane geometry from current configuration
///
/// \return  Reference to created DrawableGeo object. Returns NULL if no cut plane was generated
/// 
/// \todo   Remove duplicate nodes from returned geometry
///         Current implementation is not optimized in any way
///         Should set normal from plane normal instead of relying on caller to compute them
//--------------------------------------------------------------------------------------------------
ref<DrawableGeo> StructGridCutPlane::generateSurface()
{
    if (m_mustRecompute)
    {
        computeCutPlane();
        m_mustRecompute = false;
    }

    size_t numVertices = m_vertices.size();
    size_t numTriangles = m_triangleIndices.size()/3;
    if (numVertices == 0 || numTriangles == 0)
    {
        return NULL;
    }

    bool doMapScalar = false;
    if (m_mapScalarSetIndex != UNDEFINED_UINT && m_scalarMapper.notNull())
    {
        CVF_ASSERT(numVertices == m_vertexScalars.size());
        doMapScalar = true;
    }


    ref<Vec3fArray> vertexArr = new Vec3fArray(m_vertices);

    ref<UIntArray> indices = new UIntArray(m_triangleIndices);
    ref<PrimitiveSetIndexedUInt> primSet = new PrimitiveSetIndexedUInt(PT_TRIANGLES);
    primSet->setIndices(indices.p());

    ref<cvf::DrawableGeo> geo = new cvf::DrawableGeo;;
    geo->setVertexArray(vertexArr.p());
    geo->addPrimitiveSet(primSet.p());

    if (doMapScalar)
    {
        CVF_ASSERT(numVertices == m_vertexScalars.size());

        ref<Color3ubArray> vertexColors = new Color3ubArray;
        ref<Vec2fArray> textureCoords = new Vec2fArray;
        vertexColors->reserve(numVertices);
        textureCoords->reserve(numVertices);
        size_t i;
        for (i = 0; i < numVertices; i++)
        {
            Color3ub clr = m_scalarMapper->mapToColor(m_vertexScalars[i]);
            vertexColors->add(clr);

            Vec2f texCoord = m_scalarMapper->mapToTextureCoord(m_vertexScalars[i]);
            textureCoords->add(texCoord);
        }

        geo->setColorArray(vertexColors.p());
        geo->setTextureCoordArray(textureCoords.p());
    }

    //Trace::show("generateSurface(): Vertices:%d  TriConns:%d  Tris:%d", vertexArr->size(), indices->size(), indices->size()/3);

    return geo;
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
ref<DrawableGeo> StructGridCutPlane::generateMesh()
{
    if (m_mustRecompute)
    {
        computeCutPlane();
        m_mustRecompute = false;
    }

    size_t numVertices = m_vertices.size();
    size_t numLines = m_meshLineIndices.size()/2;
    if (numVertices == 0 || numLines == 0)
    {
        return NULL;
    }

    MeshEdgeExtractor ee;
    ee.addPrimitives(2, &m_meshLineIndices[0], m_meshLineIndices.size());

    ref<UIntArray> indices = ee.lineIndices();
    ref<PrimitiveSetIndexedUInt> primSet = new PrimitiveSetIndexedUInt(PT_LINES);
    primSet->setIndices(indices.p());

    ref<Vec3fArray> vertexArr = new Vec3fArray(m_vertices);

    ref<cvf::DrawableGeo> geo = new cvf::DrawableGeo;;
    geo->setVertexArray(vertexArr.p());
    geo->addPrimitiveSet(primSet.p());

    //Trace::show("generateMesh(): Vertices:%d  LineConns:%d  Lines:%d", vertexArr->size(), indices->size(), indices->size()/2);

    return geo;
}


//--------------------------------------------------------------------------------------------------
/// Generate surface representation of the specified cut plane
/// 
/// \note Will compute normals before returning geometry
//--------------------------------------------------------------------------------------------------
void StructGridCutPlane::computeCutPlane() 
{
    DebugTimer tim("StructGridCutPlane::computeCutPlane", DebugTimer::DISABLED);

    bool doMapScalar = false;
    if (m_mapScalarSetIndex != UNDEFINED_UINT && m_scalarMapper.notNull())
    {
        doMapScalar = true;
    }

    uint cellCountI = m_grid->cellCountI();
    uint cellCountJ = m_grid->cellCountJ();
    uint cellCountK = m_grid->cellCountK();

    // Clear any current data
    m_vertices.clear();
    m_vertexScalars.clear();
    m_triangleIndices.clear();
    m_meshLineIndices.clear();


    // The indexing conventions for vertices and 
    // edges used in the algorithm:
    //                                                                   edg   verts
    //      4-------------5                     *------4------*           0    0 - 1
    //     /|            /|                    /|            /|           1    1 - 2
    //    / |           / |                  7/ |          5/ |           2    2 - 3
    //   /  |          /  |      |z          /  8          /  9           3    3 - 0
    //  7-------------6   |      | /y       *------6------*   |           4    4 - 5
    //  |   |         |   |      |/         |   |         |   |           5    5 - 6
    //  |   0---------|---1      *---x      |   *------0--|---*           6    6 - 7
    //  |  /          |  /                 11  /         10  /            7    7 - 4
    //  | /           | /                   | /3          | /1            8    0 - 4
    //  |/            |/                    |/            |/              9    1 - 5
    //  3-------------2                     *------2------*              10    2 - 6
    //  vertex indices                       edge indices                11    3 - 7
    //                                                                 

    uint k;
    for (k = 0; k < cellCountK; k++)
    {
        uint j;
        for (j = 0; j < cellCountJ; j++)
        {
            uint i;
            for (i = 0; i < cellCountI; i++)
            {
                size_t cellIndex = m_grid->cellIndexFromIJK(i, j, k);

                Vec3d minCoord;
                Vec3d maxCoord;
                m_grid->cellMinMaxCordinates(cellIndex, &minCoord, &maxCoord);

                // Early reject for cells outside clipping box
                if (m_clippingBoundingBox.isValid())
                {
                    BoundingBox cellBB(minCoord, maxCoord);
                    if (!m_clippingBoundingBox.intersects(cellBB))
                    {
                        continue;
                    }
                }

                // Check if plane intersects this cell and skip if it doesn't
                if (!isCellIntersectedByPlane(m_plane, minCoord, maxCoord))
                {
                    continue;
                }

                GridCell cell;

                bool isClipped = false;
                if (m_clippingBoundingBox.isValid())
                {
                    if (!m_clippingBoundingBox.contains(minCoord) || !m_clippingBoundingBox.contains(maxCoord))
                    {
                        isClipped = true;

                        minCoord.x() = CVF_MAX(minCoord.x(), m_clippingBoundingBox.min().x());
                        minCoord.y() = CVF_MAX(minCoord.y(), m_clippingBoundingBox.min().y());
                        minCoord.z() = CVF_MAX(minCoord.z(), m_clippingBoundingBox.min().z());

                        maxCoord.x() = CVF_MIN(maxCoord.x(), m_clippingBoundingBox.max().x());
                        maxCoord.y() = CVF_MIN(maxCoord.y(), m_clippingBoundingBox.max().y());
                        maxCoord.z() = CVF_MIN(maxCoord.z(), m_clippingBoundingBox.max().z());
                    }
                }

                cell.p[0].set(minCoord.x(), maxCoord.y(), minCoord.z());
                cell.p[1].set(maxCoord.x(), maxCoord.y(), minCoord.z());
                cell.p[2].set(maxCoord.x(), minCoord.y(), minCoord.z());
                cell.p[3].set(minCoord.x(), minCoord.y(), minCoord.z());
                cell.p[4].set(minCoord.x(), maxCoord.y(), maxCoord.z());
                cell.p[5].set(maxCoord.x(), maxCoord.y(), maxCoord.z());
                cell.p[6].set(maxCoord.x(), minCoord.y(), maxCoord.z());
                cell.p[7].set(minCoord.x(), minCoord.y(), maxCoord.z());


                // Fetch scalar values
                double cellScalarValue = 0;
                if (doMapScalar)
                {
                    cellScalarValue = m_grid->cellScalar(m_mapScalarSetIndex, i, j, k);

                    // If we're doing node averaging we must populate grid cell with scalar values interpolated to the grid points
                    if (m_mapNodeAveragedScalars)
                    {
                        if (isClipped)
                        {
                            double scalarVal;
                            if (m_grid->pointScalar(m_mapScalarSetIndex, cell.p[0], &scalarVal)) cell.s[0] = scalarVal;
                            if (m_grid->pointScalar(m_mapScalarSetIndex, cell.p[1], &scalarVal)) cell.s[1] = scalarVal;
                            if (m_grid->pointScalar(m_mapScalarSetIndex, cell.p[2], &scalarVal)) cell.s[2] = scalarVal;
                            if (m_grid->pointScalar(m_mapScalarSetIndex, cell.p[3], &scalarVal)) cell.s[3] = scalarVal;
                            if (m_grid->pointScalar(m_mapScalarSetIndex, cell.p[4], &scalarVal)) cell.s[4] = scalarVal;
                            if (m_grid->pointScalar(m_mapScalarSetIndex, cell.p[5], &scalarVal)) cell.s[5] = scalarVal;
                            if (m_grid->pointScalar(m_mapScalarSetIndex, cell.p[6], &scalarVal)) cell.s[6] = scalarVal;
                            if (m_grid->pointScalar(m_mapScalarSetIndex, cell.p[7], &scalarVal)) cell.s[7] = scalarVal;
                        }
                        else
                        {
                            cell.s[0] = m_grid->gridPointScalar(m_mapScalarSetIndex, i,     j + 1, k);
                            cell.s[1] = m_grid->gridPointScalar(m_mapScalarSetIndex, i + 1, j + 1, k);
                            cell.s[2] = m_grid->gridPointScalar(m_mapScalarSetIndex, i + 1, j,     k);
                            cell.s[3] = m_grid->gridPointScalar(m_mapScalarSetIndex, i,     j,     k);
                            cell.s[4] = m_grid->gridPointScalar(m_mapScalarSetIndex, i,     j + 1, k + 1);
                            cell.s[5] = m_grid->gridPointScalar(m_mapScalarSetIndex, i + 1, j + 1, k + 1);
                            cell.s[6] = m_grid->gridPointScalar(m_mapScalarSetIndex, i + 1, j,     k + 1);
                            cell.s[7] = m_grid->gridPointScalar(m_mapScalarSetIndex, i,     j,     k + 1);
                        }
                    }
                }


                Triangles triangles;
                uint numTriangles = polygonise(m_plane, cell, &triangles);
                if (numTriangles > 0)
                {
                    // Add all the referenced vertices
                    // At the same time registering their index in the 'global' vertex list
                    uint globalVertexIndices[12];
                    int iv;
                    for (iv = 0; iv < 12; iv++)
                    {
                        if (triangles.usedVertices[iv])
                        {
                            globalVertexIndices[iv] = static_cast<uint>(m_vertices.size());
                            m_vertices.push_back(Vec3f(triangles.vertices[iv]));

                            if (doMapScalar)
                            {
                                if (m_mapNodeAveragedScalars)
                                {
                                    m_vertexScalars.push_back(triangles.scalars[iv]);
                                }
                                else
                                {
                                    m_vertexScalars.push_back(cellScalarValue);
                                }
                            }
                        }
                        else
                        {
                            globalVertexIndices[iv] = UNDEFINED_UINT;
                        }
                    }

                    // Build triangles from the cell
                    const size_t prevNumTriangleIndices = m_triangleIndices.size();
                    uint t;
                    for (t = 0; t < numTriangles; t++)
                    {
                        m_triangleIndices.push_back(globalVertexIndices[triangles.triangleIndices[3*t]]);
                        m_triangleIndices.push_back(globalVertexIndices[triangles.triangleIndices[3*t + 1]]);
                        m_triangleIndices.push_back(globalVertexIndices[triangles.triangleIndices[3*t + 2]]);
                    }

                    // Add mesh line indices 
                    addMeshLineIndices(&m_triangleIndices[prevNumTriangleIndices], numTriangles);
                }
            }
        }
    }

    //Trace::show("Vertices:%d  TriConns:%d  Tris:%d", m_vertices.size(), m_triangleIndices.size(), m_triangleIndices.size()/3);
    tim.reportTimeMS();
}


//--------------------------------------------------------------------------------------------------
/// Add mesh line indices by analyzing the triangle indices and only adding 'unique' edges
//--------------------------------------------------------------------------------------------------
void StructGridCutPlane::addMeshLineIndices(const uint* triangleIndices, uint triangleCount)
{
    std::vector<int64> edges;
    edges.reserve(3*triangleCount);

    std::vector<int64>::iterator it;

    uint t;
    for (t = 0; t < triangleCount; t++)
    {
        uint i;
        for (i = 0; i < 3; i++)
        {
            const uint vertexIdx1 = triangleIndices[3*t + i];
            const uint vertexIdx2 = (i < 2) ? triangleIndices[3*t + i + 1] : triangleIndices[3*t];

            int64 edgeKeyVal = EdgeKey(vertexIdx1, vertexIdx2).toKeyVal();
            it = find(edges.begin(), edges.end(), edgeKeyVal);
            if (it == edges.end())
            {
                edges.push_back(edgeKeyVal);
            }
            else
            {
                edges.erase(it);
            }
        }
    }

    for (it = edges.begin(); it != edges.end(); ++it)
    {
        EdgeKey ek = EdgeKey::fromkeyVal(*it);
        m_meshLineIndices.push_back(ek.index1());
        m_meshLineIndices.push_back(ek.index2());
    }
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
uint StructGridCutPlane::polygonise(const Plane& plane, const GridCell& cell, Triangles* triangles)
{
    int cubeindex = 0;
    if (plane.distanceSquared(cell.p[0]) < 0) cubeindex |= 1;
    if (plane.distanceSquared(cell.p[1]) < 0) cubeindex |= 2;
    if (plane.distanceSquared(cell.p[2]) < 0) cubeindex |= 4;
    if (plane.distanceSquared(cell.p[3]) < 0) cubeindex |= 8;
    if (plane.distanceSquared(cell.p[4]) < 0) cubeindex |= 16;
    if (plane.distanceSquared(cell.p[5]) < 0) cubeindex |= 32;
    if (plane.distanceSquared(cell.p[6]) < 0) cubeindex |= 64;
    if (plane.distanceSquared(cell.p[7]) < 0) cubeindex |= 128;

    if (sm_edgeTable[cubeindex] == 0)
    {
        return 0;
    }

    // Compute vertex coordinates on the edges where we have intersections
    if (sm_edgeTable[cubeindex] & 1)    triangles->vertices[0] =   planeLineIntersection(plane, cell.p[0], cell.p[1], cell.s[0], cell.s[1], &triangles->scalars[0] );
    if (sm_edgeTable[cubeindex] & 2)    triangles->vertices[1] =   planeLineIntersection(plane, cell.p[1], cell.p[2], cell.s[1], cell.s[2], &triangles->scalars[1] );
    if (sm_edgeTable[cubeindex] & 4)    triangles->vertices[2] =   planeLineIntersection(plane, cell.p[2], cell.p[3], cell.s[2], cell.s[3], &triangles->scalars[2] );
    if (sm_edgeTable[cubeindex] & 8)    triangles->vertices[3] =   planeLineIntersection(plane, cell.p[3], cell.p[0], cell.s[3], cell.s[0], &triangles->scalars[3] );
    if (sm_edgeTable[cubeindex] & 16)   triangles->vertices[4] =   planeLineIntersection(plane, cell.p[4], cell.p[5], cell.s[4], cell.s[5], &triangles->scalars[4] );
    if (sm_edgeTable[cubeindex] & 32)   triangles->vertices[5] =   planeLineIntersection(plane, cell.p[5], cell.p[6], cell.s[5], cell.s[6], &triangles->scalars[5] );
    if (sm_edgeTable[cubeindex] & 64)   triangles->vertices[6] =   planeLineIntersection(plane, cell.p[6], cell.p[7], cell.s[6], cell.s[7], &triangles->scalars[6] );
    if (sm_edgeTable[cubeindex] & 128)  triangles->vertices[7] =   planeLineIntersection(plane, cell.p[7], cell.p[4], cell.s[7], cell.s[4], &triangles->scalars[7] );
    if (sm_edgeTable[cubeindex] & 256)  triangles->vertices[8] =   planeLineIntersection(plane, cell.p[0], cell.p[4], cell.s[0], cell.s[4], &triangles->scalars[8] );
    if (sm_edgeTable[cubeindex] & 512)  triangles->vertices[9] =   planeLineIntersection(plane, cell.p[1], cell.p[5], cell.s[1], cell.s[5], &triangles->scalars[9] );
    if (sm_edgeTable[cubeindex] & 1024) triangles->vertices[10] =  planeLineIntersection(plane, cell.p[2], cell.p[6], cell.s[2], cell.s[6], &triangles->scalars[10]);
    if (sm_edgeTable[cubeindex] & 2048) triangles->vertices[11] =  planeLineIntersection(plane, cell.p[3], cell.p[7], cell.s[3], cell.s[7], &triangles->scalars[11]);


    // Create the triangles
    memset(triangles->usedVertices, 0, sizeof(triangles->usedVertices));
    const int* triConnects = sm_triTable[cubeindex];
    uint n = 0;
    while (triConnects[n] != -1)
    {
        triangles->triangleIndices[n] = triConnects[n];
        triangles->usedVertices[triConnects[n]] = true;
        n++;
    }

    uint numTriangles = n/3;

    return numTriangles;
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
Vec3d StructGridCutPlane::planeLineIntersection(const Plane& plane, const Vec3d& p1, const Vec3d& p2, const double s1, const double s2, double* s)
{
    // From http://local.wasp.uwa.edu.au/~pbourke/geometry/planeline/
    //
    // P1 (x1,y1,z1) and P2 (x2,y2,z2)
    //
    // P = P1 + u (P2 - P1)
    //
    //          A*x1 + B*y1 + C*z1 + D
    // u = ---------------------------------
    //     A*(x1-x2) + B*(y1-y2) + C*(z1-z2)

    CVF_ASSERT(s);

    const Vec3d v = p2 - p1;

    double denominator = -(plane.A()*v.x() + plane.B()*v.y() + plane.C()*v.z());
    if (denominator != 0)
    {
        double u = (plane.A()*p1.x() + plane.B()*p1.y() + plane.C()*p1.z() + plane.D())/denominator;
        if (u > 0.0 && u < 1.0)
        {
            *s = s1 + u*(s2 - s1);
            return (p1 + u*v);
        }
        else
        {
            if (u >= 1.0)
            {
                *s = s2;
                return p2;
            }
            else
            {
                *s = s1;
                return p1;
            }
        }
    }
    else
    {
        *s = s1;
        return p1;
    }
}


//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
bool StructGridCutPlane::isCellIntersectedByPlane(const Plane& plane, const Vec3d& cellMinCoord, const Vec3d& cellMaxCoord)
{
    // See http://zach.in.tu-clausthal.de/teaching/cg_literatur/lighthouse3d_view_frustum_culling/index.html

    // Start by finding the "positive vertex" and the "negative vertex" relative to plane normal
    Vec3d pVertex(cellMinCoord);
    Vec3d nVertex(cellMaxCoord);

    if (plane.A() >= 0)
    {
        pVertex.x() = cellMaxCoord.x();
        nVertex.x() = cellMinCoord.x();
    }

    if (plane.B() >= 0)
    {
        pVertex.y() = cellMaxCoord.y();
        nVertex.y() = cellMinCoord.y();
    }

    if (plane.C() >= 0)
    {
        pVertex.z() = cellMaxCoord.z();
        nVertex.z() = cellMinCoord.z();
    }

    // Chek if both positive and negative vertex are on same side of plane
    if (plane.distanceSquared(pVertex) < 0)
    {
        if (plane.distanceSquared(nVertex) < 0)
        {
            return false;
        }
        else
        {
            return true;
        }
    }
    else
    {
        if (plane.distanceSquared(nVertex) >= 0)
        {
            return false;
        }
        else
        {
            return true;
        }
    }
}

//--------------------------------------------------------------------------------------------------
/// 
//--------------------------------------------------------------------------------------------------
void StructGridCutPlane::setClippingBoundingBox(const BoundingBox& boundingBox)
{
    m_clippingBoundingBox = boundingBox;
}


} // namespace cvf
