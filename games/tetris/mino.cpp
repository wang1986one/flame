#include "mino.h"

Vec2i g_mino_coords[MinoTypeCount][3];
Vec4c g_mino_colors[MinoTypeCount];

Vec2i g_mino_LTSZJ_offsets[5][4];
Vec2i g_mino_O_offsets[5][4];
Vec2i g_mino_I_offsets[5][4];

void init_mino()
{
	g_mino_coords[Mino_L][0] = Vec2i(-1, +0);
	g_mino_coords[Mino_L][1] = Vec2i(+1, +0);
	g_mino_coords[Mino_L][2] = Vec2i(-1, -1);

	g_mino_coords[Mino_J][0] = Vec2i(-1, +0);
	g_mino_coords[Mino_J][1] = Vec2i(+1, +0);
	g_mino_coords[Mino_J][2] = Vec2i(+1, -1);

	g_mino_coords[Mino_T][0] = Vec2i(-1, +0);
	g_mino_coords[Mino_T][1] = Vec2i(+1, +0);
	g_mino_coords[Mino_T][2] = Vec2i(+0, -1);

	g_mino_coords[Mino_S][0] = Vec2i(-1, +0);
	g_mino_coords[Mino_S][1] = Vec2i(+0, -1);
	g_mino_coords[Mino_S][2] = Vec2i(+1, -1);

	g_mino_coords[Mino_Z][0] = Vec2i(-1, -1);
	g_mino_coords[Mino_Z][1] = Vec2i(+0, -1);
	g_mino_coords[Mino_Z][2] = Vec2i(+1, +0);

	g_mino_coords[Mino_O][0] = Vec2i(+0, -1);
	g_mino_coords[Mino_O][1] = Vec2i(+1, +0);
	g_mino_coords[Mino_O][2] = Vec2i(+1, -1);

	g_mino_coords[Mino_I][0] = Vec2i(-1, +0);
	g_mino_coords[Mino_I][1] = Vec2i(+1, +0);
	g_mino_coords[Mino_I][2] = Vec2i(+2, +0);

	g_mino_colors[Mino_L] = Vec4c(0, 81, 179, 255);
	g_mino_colors[Mino_T] = Vec4c(169, 0, 225, 255);
	g_mino_colors[Mino_S] = Vec4c(0, 221, 50, 255);
	g_mino_colors[Mino_Z] = Vec4c(193, 0, 0, 255);
	g_mino_colors[Mino_J] = Vec4c(230, 132, 0, 255);
	g_mino_colors[Mino_O] = Vec4c(225, 198, 0, 255);
	g_mino_colors[Mino_I] = Vec4c(0, 184, 217, 255);

	memset(g_mino_LTSZJ_offsets, 0, sizeof(g_mino_LTSZJ_offsets));
	memset(g_mino_O_offsets, 0, sizeof(g_mino_O_offsets));
	memset(g_mino_I_offsets, 0, sizeof(g_mino_I_offsets));

	g_mino_LTSZJ_offsets[0][0] = Vec2i(+0, +0);
	g_mino_LTSZJ_offsets[0][1] = Vec2i(+0, +0);
	g_mino_LTSZJ_offsets[0][2] = Vec2i(+0, +0);
	g_mino_LTSZJ_offsets[0][3] = Vec2i(+0, +0);

	g_mino_LTSZJ_offsets[1][0] = Vec2i(+0, +0);
	g_mino_LTSZJ_offsets[1][1] = Vec2i(+1, +0);
	g_mino_LTSZJ_offsets[1][2] = Vec2i(+0, +0);
	g_mino_LTSZJ_offsets[1][3] = Vec2i(-1, +0);

	g_mino_LTSZJ_offsets[2][0] = Vec2i(+0, +0);
	g_mino_LTSZJ_offsets[2][1] = Vec2i(+1, +1);
	g_mino_LTSZJ_offsets[2][2] = Vec2i(+0, +0);
	g_mino_LTSZJ_offsets[2][3] = Vec2i(-1, +1);

	g_mino_LTSZJ_offsets[3][0] = Vec2i(+0, +0);
	g_mino_LTSZJ_offsets[3][1] = Vec2i(+0, -2);
	g_mino_LTSZJ_offsets[3][2] = Vec2i(+0, +0);
	g_mino_LTSZJ_offsets[3][3] = Vec2i(+0, -2);

	g_mino_LTSZJ_offsets[4][0] = Vec2i(+0, +0);
	g_mino_LTSZJ_offsets[4][1] = Vec2i(+1, -2);
	g_mino_LTSZJ_offsets[4][2] = Vec2i(+0, +0);
	g_mino_LTSZJ_offsets[4][3] = Vec2i(-1, -2);

	g_mino_I_offsets[0][0] = Vec2i(+0, +0);
	g_mino_I_offsets[0][1] = Vec2i(-1, +0);
	g_mino_I_offsets[0][2] = Vec2i(-1, -1);
	g_mino_I_offsets[0][3] = Vec2i(+0, -1);

	g_mino_I_offsets[1][0] = Vec2i(-1, +0);
	g_mino_I_offsets[1][1] = Vec2i(+0, +0);
	g_mino_I_offsets[1][2] = Vec2i(+1, -1);
	g_mino_I_offsets[1][3] = Vec2i(+0, -1);

	g_mino_I_offsets[2][0] = Vec2i(+2, +0);
	g_mino_I_offsets[2][1] = Vec2i(+0, +0);
	g_mino_I_offsets[2][2] = Vec2i(-2, -1);
	g_mino_I_offsets[2][3] = Vec2i(+0, -1);

	g_mino_I_offsets[3][0] = Vec2i(-1, +0);
	g_mino_I_offsets[3][1] = Vec2i(+0, -1);
	g_mino_I_offsets[3][2] = Vec2i(+1, +0);
	g_mino_I_offsets[3][3] = Vec2i(+0, +1);

	g_mino_I_offsets[4][0] = Vec2i(+2, +0);
	g_mino_I_offsets[4][1] = Vec2i(+0, +2);
	g_mino_I_offsets[4][2] = Vec2i(-2, +0);
	g_mino_I_offsets[4][3] = Vec2i(+0, -2);

	g_mino_O_offsets[0][0] = Vec2i(+0, +0);
	g_mino_O_offsets[0][1] = Vec2i(+0, +1);
	g_mino_O_offsets[0][2] = Vec2i(-1, +1);
	g_mino_O_offsets[0][3] = Vec2i(-1, +0);
}
