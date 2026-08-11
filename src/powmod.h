#ifndef CONTEST_POWMOD_H
#define CONTEST_POWMOD_H

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
/*
 * Modular exponentiation: result = base^exp mod n
 *
 * All values are big-endian. base and mod are modulus_len bytes wide.
 * modulus_len may be 1–512 (up to 4096-bit modulus).
 *
 * Special cases your implementation must handle:
 *   base = 0  =>  result = 0
 *   base = 1  =>  result = 1
 *   exp  = 0  =>  result = 1  (by convention)
 *   exp  = 1  =>  result = base mod n
 *
 * Choose ONE of the two forms below and declare it in this file.
 * The test harness will call whichever signature is declared here.
 * Remove or comment out the form you are not implementing.
 *
 * --------------------------------------------------------------------
 * Form 1 — fixed u24 exponent
 *
 * Use this if you only need to handle public exponents (e.g. 65537).
 * Covers RSA encrypt and signature verify.
 *
 * Parameters:
 *   result       — output buffer, modulus_len bytes (caller-allocated)
 *   exp          — exponent as a 24-bit integer (e.g. 65537)
 *   base         — base value, modulus_len bytes, 0 <= base < mod
 *   mod          — modulus, modulus_len bytes, must be odd and > 1
 *   modulus_len  — byte width of base, mod, and result
 * --------------------------------------------------------------------
 */
void mod_in_place(uint8_t *a, const uint8_t *m, int len);
void mul_lsb_trunc_be(
    const uint8_t *a_be,
    const uint8_t *b_be,
    int len,
    uint8_t *out_be   // N bytes, big-endian
);

void powmod(
    uint8_t        *result,
    const uint24_t  exp,
    const uint8_t  *base,
    const uint8_t  *mod,
    uint16_t        modulus_len
) {
	uint24_t curr_exp = exp;
	uint8_t *work_space = (uint8_t*)malloc(modulus_len*3);
	uint8_t *a, *b, *c, *d;
	a = work_space;
	b = work_space + modulus_len;
	c = work_space + modulus_len * 2;
	d = result;
	
	memcpy(a, base, modulus_len);
	memcpy(b, base, modulus_len);
	memset(c, 0, modulus_len);
	c[modulus_len-1] = 0;
	memset(d, 0, modulus_len);
	d[modulus_len-1] = 0;

	for (int i = 0; i < 24; i++) {
		if (((exp >> i) & 1) == 1) {
			mul_lsb_trunc_be(a, d, modulus_len, c);
			memcpy(d, c, modulus_len);
		}
		mul_lsb_trunc_be(a, b, modulus_len, c);
		memcpy(a, c, modulus_len);
		memcpy(b, c, modulus_len);
	}
	
	mod_in_place(result, mod, modulus_len);
	free(work_space);
}

void mul_lsb_trunc_be(
    const uint8_t *a_be,
    const uint8_t *b_be,
    int len,
    uint8_t *out_be
) {
	memset(out_be, 0, len);

    for (int i = 0; i < len; i++) {
        uint16_t carry = 0;
		int max_j = len - i;

        for (int j = 0; j < max_j; j++) {

			int idx_a   = len - 1 - i;
            int idx_b   = len - 1 - j;
            int idx_out = len - 1 - (i + j);
			
			uint16_t prod = (uint16_t)a_be[idx_a] * (uint16_t)b_be[idx_b];
            uint16_t sum  = (uint16_t)out_be[idx_out] + prod + carry;
            
			out_be[idx_out] = (uint8_t)(sum & 0xFF);
            carry         = sum >> 8;
        }
    }
}
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/**
 * Compares two big-endian byte arrays of length `len`.
 * Returns true if `a` >= `b`, false otherwise.
 */
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
/*
 * --------------------------------------------------------------------
 * Form 2 — variable-length exponent buffer (BONUS — +4 points)
 *
 * Use this if you want to support full-width private exponents.
 * Passing all full-width exponent test vectors with this form also
 * covers RSA decrypt and sign operations.
 *
 * Parameters:
 *   result       — output buffer, modulus_len bytes (caller-allocated)
 *   exp          — exponent, exp_len bytes, big-endian
 *   exp_len      — byte width of the exponent (1–512)
 *   base         — base value, modulus_len bytes, 0 <= base < mod
 *   mod          — modulus, modulus_len bytes, must be odd and > 1
 *   modulus_len  — byte width of base, mod, and result
 * --------------------------------------------------------------------
 */
/*
void powmod(
    uint8_t        *result,
    const uint8_t  *exp,
    uint16_t        exp_len,
    const uint8_t  *base,
    const uint8_t  *mod,
    uint16_t        modulus_len
);
*/

#endif /* CONTEST_POWMOD_H */
