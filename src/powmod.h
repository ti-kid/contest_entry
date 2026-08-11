#ifndef CONTEST_POWMOD_H
#define CONTEST_POWMOD_H

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

void big_mult(
    const uint8_t *a_be,
    const uint8_t *b_be,
    uint8_t *out_be,
	int len
) {
	memset(out_be, 0, len);
	int idx_base = len - 1;

    for (int i = 0; i < len; i++) {
        int carry = 0;

        for (int j = 0; j < (len-i); j++) {

			carry += (uint16_t)a_be[idx_base-i] * (uint16_t)b_be[idx_base-j] + out_be[idx_base-(i+j)];
			out_be[idx_base-(i+j)] = (uint8_t)(carry & 0xFF);
            carry = carry >> 8;
        }
    }
}

static bool ge_be(const uint8_t *a, const uint8_t *b, int len) {
    for (int i = 0; i < len; i++) {
        if (a[i] > b[i]) return true;
        if (a[i] < b[i]) return false;
    }
    return true; // Equal
}

/**
 * Subtracts big-endian array `b` from `a` in-place: a = a - b
 */
static void sub_be(uint8_t *a, const uint8_t *b, int len) {
    uint16_t borrow = 0;
    for (int i = (int)len - 1; i >= 0; i--) {
        uint16_t diff = (uint16_t)a[i] - b[i] - borrow;
        a[i] = (uint8_t)diff;
        borrow = (diff >> 8) & 1; // Extract borrow bit
    }
}

/**
 * Shifts big-endian array `a` left by 1 bit, inserting `in_bit` at the LSB.
 */
static void shift_left_1(uint8_t *a, int len, uint8_t in_bit) {
    uint8_t carry = in_bit & 1;
    for (int i = (int)len - 1; i >= 0; i--) {
        uint8_t next_carry = (a[i] & 0x80) ? 1 : 0;
        a[i] = (uint8_t)((a[i] << 1) | carry);
        carry = next_carry;
    }
}

/**
 * In-place modular reduction: a = a mod m
 * Works for inputs 'a' up to 'len' bytes long.
 *
 * @param a    Input buffer (len bytes), overwritten with (a mod m)
 * @param m    Big-endian modulus buffer (len bytes)
 * @param len  Length of the buffers in bytes
 */
void mod_in_place(uint8_t *a, const uint8_t *m, int len) {
    // Allocate exactly 'len' bytes dynamically to support large sizes (1024, 2048, 4096 bits)
    uint8_t *rem = (uint8_t *)calloc(1, len);
    if (!rem) return; // Allocation safety

    // Process 'a' bit-by-bit from MSB to LSB
    for (int byte_i = 0; byte_i < len; byte_i++) {
        uint8_t current_byte = a[byte_i];

        for (int bit_i = 7; bit_i >= 0; bit_i--) {
            uint8_t bit = (current_byte >> bit_i) & 1;

            // Shift current bit into the running remainder
            shift_left_1(rem, len, bit);

            // Keep remainder < m
            if (ge_be(rem, m, len)) {
                sub_be(rem, m, len);
            }
        }
    }

    // Overwrite 'a' with the final calculated remainder
    memcpy(a, rem, len);

    // Free dynamically allocated buffer to prevent memory leaks
    free(rem);
}

void powmod(
		uint8_t			*result,
		const uint24_t	exp,
		const uint8_t	*base,
		const uint8_t	*mod,
		uint16_t		modulus_len
) {
	uint8_t *workspace, *a, *b, *c, *d;

	workspace = (uint8_t*)malloc(modulus_len*3);
	a = workspace;
	b = workspace + modulus_len;
	c = b + modulus_len;
	d = result;

	memcpy(a, base, modulus_len);
	memcpy(b, base, modulus_len);
	memset(d, 0, modulus_len);
	d[modulus_len - 1] = 0;  //I know seems unnecesarry , this is js to prove a point

	for (int i = 0; i < 24; i++) {
		if (((exp >> i) & 1) == 1) {
			big_mult(a, d, c, modulus_len);
			memcpy(d, c, modulus_len);
		}
		big_mult(a, b, c, modulus_len);
		memcpy(a, c, modulus_len);
		memcpy(b, c, modulus_len);
		
	}
	
	mod_in_place(d, mod, modulus_len);
	free(workspace);
}
#endif /* CONTEST_POWMOD_H */
