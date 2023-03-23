// SPDX-License-Identifier: MIT
/* Copyright 2022 Eileen Yoon <eyn@gmx.com> */

#include "ane.h"
#include "ane_mem.h"
#include "ane_priv.h"

static inline void ane_tile(void *data, void *tile, const uint64_t N,
			    const uint64_t C, const uint64_t H,
			    const uint64_t W, const uint64_t P,
			    const uint64_t R)
{
	const uint64_t new_H = P / R;
	const uint64_t new_W = R / sizeof(uint16_t);
	const uint64_t stride = W * sizeof(uint16_t);

	uint16_t(*data_a)[N][C][H][W] = (uint16_t(*)[N][C][H][W])data;
	uint16_t(*tile_a)[N][C][new_H][new_W] =
		(uint16_t(*)[N][C][new_H][new_W])tile;

	for (uint64_t n = 0; n < N; n++) {
		for (uint64_t c = 0; c < C; c++) {
			for (uint64_t h = 0; h < H; h++) {
				memcpy((*tile_a)[n][c][h], (*data_a)[n][c][h],
				       stride);
			}
		}
	}

	return;
}

static inline void ane_untile(void *data, void *tile, const uint64_t N,
			      const uint64_t C, const uint64_t H,
			      const uint64_t W, const uint64_t P,
			      const uint64_t R)
{
	const uint64_t new_H = P / R;
	const uint64_t new_W = R / sizeof(uint16_t);
	const uint64_t stride = W * sizeof(uint16_t);

	const uint64_t data_size = N * C * H * W * sizeof(uint16_t);
	memset(data, 0, data_size); // TODO; needed?

	uint16_t(*data_a)[N][C][H][W] = (uint16_t(*)[N][C][H][W])data;
	uint16_t(*tile_a)[N][C][new_H][new_W] =
		(uint16_t(*)[N][C][new_H][new_W])tile;

	for (uint64_t n = 0; n < N; n++) { // TODO; early exit on alignment
		for (uint64_t c = 0; c < C; c++) {
			for (uint64_t h = 0; h < H; h++) {
				memcpy((*data_a)[n][c][h], (*tile_a)[n][c][h],
				       stride);
			}
		}
	}

	return;
}

int ane_tiled_send(struct ane_nn *nn, void *from, const int idx)
{
	if (idx >= input_count(nn))
		return -EINVAL;

	const struct ane_model *model = nn->model;
	const int bdx = nn->src_bdx[idx];

	uint16_t tile[tile_size(nn, bdx) / sizeof(uint16_t)];
	memset(tile, 0, tile_size(nn, bdx));

	ane_tile(from, tile, model->nchw[bdx][0], model->nchw[bdx][1],
		 model->nchw[bdx][2], model->nchw[bdx][3], model->nchw[bdx][4],
		 model->nchw[bdx][5]);
	ane_send(nn, tile, idx);

	return 0;
}

int ane_tiled_read(struct ane_nn *nn, void *to, const int idx)
{
	if (idx >= output_count(nn))
		return -EINVAL;

	const struct ane_model *model = nn->model;
	const int bdx = nn->dst_bdx[idx];

	uint16_t tile[tile_size(nn, bdx) / sizeof(uint16_t)];
	// memset(tile, 0, tile_size(nn, bdx));

	ane_read(nn, tile, idx);
	ane_untile(to, tile, model->nchw[bdx][0], model->nchw[bdx][1],
		   model->nchw[bdx][2], model->nchw[bdx][3],
		   model->nchw[bdx][4], model->nchw[bdx][5]);

	return 0;
}
