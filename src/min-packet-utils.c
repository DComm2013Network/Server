#include "Server.h"

#define TILE_SIZE 40
#define GRANULARITY_VEL 6.35
#define GRANULARITY_POS 2.0
#define FACTOR 128

/**
 * This decapsulates a new packet into an old packet, allowing for
 * easy implementation to existing code.
 *
 * This function fails only if memory can't be allocated and always returns with a valid
 * structure. Note that the actual transmitted data may be wrong.
 *
 * @param pkt Pointer to the PKT_POS_UPDATE packet to be decapsulated. Must be valid.
 * @return Pointer to the decapsulated PKT_OLD_POS_UPDATE. This may contain incorrect
 *         or impossible values (according to game rules). Returns NULL if the packet
 *         couldn't be allocated.
 *
 * @designer Clark Allenby
 * @author   Clark Allenby
 */
void decapsulate_pos_update(PKT_MIN_POS_UPDATE *pkt, PKT_POS_UPDATE* old_pkt) {

	if(!old_pkt)
		return;

	old_pkt->floor = (pkt->data >> 27) & 0x1F;
	old_pkt->playerNumber = (pkt->data >> 22) & 0x1F;
	old_pkt->xPos = (float)(GRANULARITY_POS * ((pkt->data >> 11) & 0x7FF));
	old_pkt->yPos = (float)(GRANULARITY_POS * (pkt->data & 0x7FF));

	old_pkt->xVel = (float)((((pkt->vel >> 8) & 0xFF) - FACTOR) / GRANULARITY_VEL);
	old_pkt->yVel = (float)(((pkt->vel & 0xFF) - FACTOR) / GRANULARITY_VEL);

}

/**
 * This encapsulates the data from an old packet into a new packet, no
 * questions asked.
 *
 * This function returns NULL if memory can't be allocated, but otherwise returns with
 * a successful packet. If there is data that is out of bounds, (for example a position
 * higher than 4096) it will overflow silently. The structure returned can be easily sent
 * over the network.
 *
 * Revisions: <ul>
 *					<li>March 22, 2014 - Shane Spoor - Uses loops instead of hard coding
 *                      each step, and returns a pointer to the packet.</li>
 *            </ul>
 *
 * @param old_pkt Pointer to the PKT_OLD_ALL_POS_UPDATE packet to be encapsulated. Must be valid.
 * @return Pointer to the encapsulated PKT_ALL_POS_UPDATE, or NULL if the memory couldn't be allocated.
 *
 * @designer Clark Allenby
 * @author   Clark Allenby
 */
void encapsulate_all_pos_update(PKT_ALL_POS_UPDATE *old_pkt, PKT_MIN_ALL_POS_UPDATE *pkt) {
	int i, j;
	uint32_t n_xPos[32], n_yPos[32], n_xVel, n_yVel, and_mask;

	if(!pkt)
		return;

    pkt->floor = old_pkt->floor;
	pkt->playersOnFloor = 0;
	for (i = 0; i < 32; i++) {
		n_xVel = (uint32_t)round(old_pkt->xVel[i] * GRANULARITY_VEL + FACTOR);
		n_yVel = (uint32_t)round(old_pkt->yVel[i] * GRANULARITY_VEL + FACTOR);
		n_xPos[i] = (uint32_t)round(old_pkt->xPos[i] / GRANULARITY_POS);
		n_yPos[i] = (uint32_t)round(old_pkt->yPos[i] / GRANULARITY_POS);
		pkt->vel[i] = 0;
		pkt->vel[i] |= n_xVel & 0xFF;
		pkt->vel[i] <<= 8;
		pkt->vel[i] |= n_yVel & 0xFF;
		pkt->playersOnFloor |= (old_pkt->playersOnFloor[i]) << i;
	}

	memset(&pkt->xPos, 0x00, sizeof(uint32_t) * 11);
	memset(&pkt->yPos, 0x00, sizeof(uint32_t) * 11);

	pkt->xPos[0] |= (n_xPos[0] & 0x7FF) << 21;
	pkt->xPos[0] |= (n_xPos[1] & 0x7FF) << 10;
	pkt->xPos[0] |= (n_xPos[2] & 0x7FE) >> 1;

	for(i = 2, j = 1, and_mask = 1; j < 11; and_mask += 1 << j, i+=3, j++)
	{
		pkt->xPos[j] |= (n_xPos[i] & and_mask) << (32 - j);
		pkt->xPos[j] |= (n_xPos[i + 1] & 0x7FF) << (21 - j);
		pkt->xPos[j] |= (n_xPos[i + 2] & 0x7FF) << (10 - j);
		pkt->xPos[j] |= (n_xPos[i + 3] & (0x7FE - (1 << j))) >> (j + 1);
	}

	pkt->yPos[0] |= (n_yPos[0] & 0x7FF) << 21;
	pkt->yPos[0] |= (n_yPos[1] & 0x7FF) << 10;
	pkt->yPos[0] |= (n_yPos[2] & 0x7FE) >> 1;

	for(i = 2, j = 1, and_mask = 1; j < 11; and_mask += 1 << j, i+=3, j++)
	{
		pkt->yPos[j] |= (n_yPos[i] & and_mask) << (32 - j);
		pkt->yPos[j] |= (n_yPos[i + 1] & 0x7FF) << (21 - j);
		pkt->yPos[j] |= (n_yPos[i + 2] & 0x7FF) << (10 - j);
		pkt->yPos[j] |= (n_yPos[i + 3] & (0x7FE - (1 << j))) >> (j + 1);
	}

}
