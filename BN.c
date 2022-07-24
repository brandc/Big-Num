#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>


/*NOTE: All integers are stored in the little endian format.*/


/*Defines*/
#define BN_BITS_PER_BYTE 8
	/*Neither can be greater than 128*/
#define int32_size     sizeof(int32_t)
#define BN_INTSIZE     8

/*Typedef*/
typedef struct {
	uint8_t num[BN_INTSIZE];
}BN_INT;

BN_INT BIG_0   = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
BN_INT BIG_MAX = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

/*Globals*/
bool BN_math_overflow;

/*==============================================================CODE=============================================================*/

/*-1 is less, 0 is equal, and 1 is greater*/
int
BN_compare(BN_INT *left, BN_INT *right)
{
	uint8_t i, ltmp, rtmp;

	for (i = BN_INTSIZE; i != 0;) {
		i--;

		ltmp = left->num[i];
		rtmp = right->num[i];
		/*greater*/
		if (ltmp > rtmp)
			return 1;
		/*less*/
		else if (ltmp < rtmp)
			return -1;
	}

	/*equal*/
	return 0;
}

/*zeros input*/
void
BN_zero_int(BN_INT *p)
{
	bzero(p, BN_INTSIZE);
}

/*takes integer to BN*/
void
uint32_t_to_buf(BN_INT *dest, uint32_t *src)
{
	uint8_t i;
	uint32_t tmp;

	tmp = *src;
	BN_zero_int(dest);
	for (i = 0; i < int32_size; i++)
		dest->num[i] = (tmp >> (i*BN_BITS_PER_BYTE)) & 0xFF;
}

void
BN_buf_to_uint(uint32_t *dest, BN_INT *src)
{
	uint8_t i;
	uint32_t ret;

	ret = 0;
	for (i = 0; i < BN_INTSIZE ;i++)
		ret += ((src->num[i]) << (i*BN_INTSIZE));

	*dest = ret;
}

void
BN_buf_to_buf(BN_INT *dest, BN_INT *src)
{
	memcpy(dest, src, BN_INTSIZE);
}

/*True to*/
bool
BN_equal(BN_INT *left, BN_INT *right)
{
	if (!BN_compare(left, right))
		return true;
	/*else*/
		return false;
}

/*greater than*/
bool
BN_greater(BN_INT *left, BN_INT *right)
{
	if (BN_compare(left, right) == 1)
		return true;
	/*else*/
		return false;
}

/*less than*/
bool
BN_less(BN_INT *left, BN_INT *right)
{
	if (BN_compare(left, right) == -1)
		return true;
	/*else*/
		return false;
}

/*greater than or equal to*/
bool
BN_greater_or_equal(BN_INT *left, BN_INT *right)
{
	return BN_greater(left, right) || BN_equal(left, right);
}

/*less than or equal to*/
bool
BN_less_or_equal(BN_INT *left, BN_INT *right)
{
	return BN_less(left, right) || BN_equal(left, right);
}

void
BN_left_shift(BN_INT *dest, uint8_t amount)
{
	uint8_t i;
	uint8_t tmp;
	uint8_t jump;
	uint8_t carry;
	uint8_t unshift;

	if (!amount)
		return;
/*	jump = amount / BN_BITS_PER_BYTE;*/
	else if ((jump = amount / BN_BITS_PER_BYTE) >= BN_INTSIZE) {
		BN_zero_int(dest);
		return;
	}

	if (jump > 0) {
		for (i = BN_INTSIZE-jump; i != 0;) {
			i--;
			dest->num[i+jump] = dest->num[i];
			dest->num[i]      = 0;
		}

		amount -= (jump*BN_BITS_PER_BYTE);
		if (!amount)
			return;
	}

	carry   = 0;
	unshift = (8-amount);
	for (i = 0; i < BN_INTSIZE; i++) {
		tmp            = dest->num[i];
		tmp          <<= amount;
		tmp           |= carry;
		carry          = dest->num[i] >> unshift;
		dest->num[i]   = tmp;
	}
}

void BN_sub(BN_INT *dest, BN_INT *src);

void
BN_add(BN_INT *dest, BN_INT *src)
{
	/*operator*/
	uint8_t i;
	uint16_t op;
	BN_INT tmp1, tmp2, tmp_max;

	/*You don't have to complain about how slow this, I know.*/
	BN_buf_to_buf(&tmp_max, &BIG_MAX);

	if (BN_greater(dest, src)) {
		BN_buf_to_buf(&tmp1, dest);
		BN_buf_to_buf(&tmp2, src);
	} else {
		BN_buf_to_buf(&tmp1, src);
		BN_buf_to_buf(&tmp2, dest);
	}

	BN_sub(&tmp_max, &tmp1);
	if (BN_greater(&tmp2, &tmp_max)) {
		BN_math_overflow = true;

		BN_sub(&tmp2, &tmp_max);
		BN_buf_to_buf(dest, &tmp2);
		return;
	}
	/*End of the roll over handling*/

	op = 0;
	for (i = 0; i < BN_INTSIZE; i++) {
		op           += src->num[i];
		op           += dest->num[i];
		dest->num[i]  = op & 0xFF;

		op >>= BN_BITS_PER_BYTE;
	}

	if (i == BN_INTSIZE && op != 0)
		BN_math_overflow = true;
}

void
BN_sub(BN_INT *dest, BN_INT *src)
{
	uint16_t op;
	uint8_t i, j;
	uint16_t tmp_op;
	bool got_borrow;
	BN_INT tmp_dest, tmp_src;

	BN_buf_to_buf(&tmp_src, src);
	BN_buf_to_buf(&tmp_dest, dest);

	/* Yes this is slow, and uses recursion, but it's required for
	 * accurate rollover*/
	if (BN_greater(src, dest)) {
		BN_math_overflow = true;
		BN_sub(&tmp_src, &tmp_dest);
		BN_buf_to_buf(&tmp_dest, &BIG_MAX);
	}

	for (i = 0; i < BN_INTSIZE; i++) {
		op = tmp_dest.num[i];

		if (tmp_src.num[i] > tmp_dest.num[i]) {
			if (i == (BN_INTSIZE-1))
				BN_math_overflow = true;

			tmp_op = 0;
			got_borrow = false;
			for (j = (i+1); j < BN_INTSIZE; j++)
				if (tmp_dest.num[j]) {
					tmp_dest.num[j] -= 1;
					tmp_op = 0x100;
					got_borrow = true;
					break;
				}

			if (!got_borrow)
				BN_math_overflow = true;

			while (--j > i)
				tmp_src.num[j] = 0xFF;

			op += tmp_op;
		}

		op           -= tmp_src.num[i];
		dest->num[i]  = op & 0xFF;
	}
}

void
BN_mul(BN_INT *dest, BN_INT *src)
{
	uint16_t op;
	uint8_t i, j;
	BN_INT tmp_buf;
	BN_INT tmp_dest;

	BN_zero_int(&tmp_dest);
	for (i = 0; i < BN_INTSIZE; i++) {
		for (j = 0; j < BN_INTSIZE; j++) {
			op  = src->num[i];
			op *= dest->num[j];

			BN_zero_int(&tmp_buf);
			if ((i+j) < BN_INTSIZE) {
				tmp_buf.num[i+j] = op & 0xFF;
				op >>= 8;

				if ((i+j+1) < BN_INTSIZE)
					tmp_buf.num[i+j+1] = op & 0xFF;
				else if (op != 0)
					BN_math_overflow = true;
			}
			else if (op != 0)
				BN_math_overflow = true;

			BN_add(&tmp_dest, &tmp_buf);
		}
	}

	BN_buf_to_buf(dest, &tmp_dest);
}

void
BN_div_n_mod(BN_INT *dest, BN_INT *src, BN_INT *rem)
{
	uint8_t i;
	uint16_t j;
	/*Operator and tmp variable*/
	BN_INT op, tmp;
	/*the jump variable and incrementing factor*/
	BN_INT jump, inc;
	/*temporary integer and counter*/
	uint32_t tmpint;
	/*Last, current and temp dest*/
	BN_INT tdest;

	if (BN_less(dest, src)) {
		BN_buf_to_buf(rem, dest);
		BN_zero_int(dest);
		return;
	} else if (BN_equal(dest, src)) {
		BN_zero_int(rem);
		BN_zero_int(dest);
		dest->num[0] = 1;
		return;
	}

	BN_zero_int(&op);
	BN_zero_int(&tdest);

	tmpint = 1;
	uint32_t_to_buf(&inc, &tmpint);

	tmpint = BN_BITS_PER_BYTE;
	uint32_t_to_buf(&jump, &tmpint);

	i = BN_INTSIZE;
	do {
		i--;

		BN_left_shift(&op, 8);
		op.num[0] = dest->num[i];

		j = 0;
		while(BN_less(src, &op) || BN_equal(src, &op)) {
			j++;
			BN_sub(&op, src);
		}

		if (j) {
			tmpint = j;
			uint32_t_to_buf(&tmp, &tmpint);
			BN_left_shift(&tmp, i*BN_BITS_PER_BYTE);
			BN_add(&tdest, &tmp);
		}
	} while(i != 0);

	BN_buf_to_buf(rem, &op);
	BN_buf_to_buf(dest, &tdest);
}


void
BN_div(BN_INT *dest, BN_INT *src)
{
	BN_INT rem;

	BN_div_n_mod(dest, src, &rem);
}

#ifndef NNEG
bool
BN_is_neg(BN_INT *src)
{
	return (src->num[BN_INTSIZE-1] >> 7) & 0x1;
}

int
BN_scompare(BN_INT *left, BN_INT *right)
{
	int ret;
	bool lneg, rneg;

	ret = BN_compare(left, right);
	if (!ret)
		return 0;

	lneg = BN_is_neg(left);
	rneg = BN_is_neg(right);

	if (!lneg && rneg)
		return 1;
	else if (lneg && !rneg)
		return -1;

	if (lneg && rneg) {
		if (ret < 0)
			return 1;
		else
			return -1;
	} else /*if (!lneg && !rneg)*/ {
		return ret;
	}
}

void
BN_flips(BN_INT *to_flip)
{
	uint32_t tmp_int;
	BN_INT tmp;
	uint8_t i;
	bool neg_sign;

	if (BN_equal(to_flip, &BIG_0))
		return;

	neg_sign = BN_is_neg(to_flip);
	for (i = 0; i < BN_INTSIZE; i++)
		to_flip->num[i] = 0xFF - to_flip->num[i];

	tmp_int = 1;
	uint32_t_to_buf(&tmp, &tmp_int);

	if (neg_sign)
		BN_add(to_flip, &tmp);
	else /*if (!neg_sign)*/
		BN_sub(to_flip, &tmp);
}


void
BN_sadd(BN_INT *dest, BN_INT *src)
{
	BN_INT tmp;
	bool dneg, sneg;

	sneg = BN_is_neg(src);
	dneg = BN_is_neg(dest);

	if (!dneg && !sneg) {
		BN_add(dest, src);
		if (BN_is_neg(dest))
			BN_math_overflow = true;

	/* currently there is no overflow detection on this code, sorry :( */
	} else /*if ((!dneg && sneg) || (dneg && !sneg) || (dneg && sneg))*/ {
		BN_buf_to_buf(&tmp, src);
		BN_flips(&tmp);
		BN_sub(dest, &tmp);
	}
}

/* No overflow detection written into this function, sorry :( */
void
BN_ssub(BN_INT *dest, BN_INT *src)
{
	BN_INT tmp;
	bool dneg, sneg;

	sneg = BN_is_neg(src);
	dneg = BN_is_neg(dest);

	if (!dneg && !sneg)
		BN_sub(dest, src);
	else if ((!dneg && sneg) || (dneg && sneg)) {
		BN_buf_to_buf(&tmp, src);
		BN_flips(&tmp);
		BN_add(dest, &tmp);
	} else if (dneg && !sneg) {
		BN_buf_to_buf(&tmp, src);
		BN_flips(&tmp);
		BN_sub(dest, &tmp);
	}
}

void
BN_smul(BN_INT *dest, BN_INT *src)
{
	bool dneg, sneg;
	BN_INT tdest, tsrc;

	sneg = BN_is_neg(src);
	dneg = BN_is_neg(dest);
	BN_buf_to_buf(&tsrc,  src);
	BN_buf_to_buf(&tdest, dest);

	if (sneg)
		BN_flips(&tsrc);
	if (dneg)
		BN_flips(&tdest);

	BN_mul(&tdest, &tsrc);
	if (BN_is_neg(&tdest))
		BN_math_overflow = true;
	BN_buf_to_buf(dest, &tdest);

	if ((!sneg && dneg) || (sneg && !dneg))
		BN_flips(dest);
}

void
BN_sdiv(BN_INT *dest, BN_INT *src)
{
	bool dneg, sneg;
	BN_INT tdest, tsrc;

	sneg = BN_is_neg(src);
	dneg = BN_is_neg(dest);
	BN_buf_to_buf(&tsrc,  src);
	BN_buf_to_buf(&tdest, dest);

	if (sneg)
		BN_flips(&tsrc);
	if (dneg)
		BN_flips(&tdest);

	BN_div(&tdest, &tsrc);
	BN_buf_to_buf(dest, &tdest);

	if ((!sneg && dneg) || (sneg && !dneg))
		BN_flips(dest);
}

void
int32_t_to_buf(BN_INT *dest, int32_t *src)
{
	uint8_t i;
	int32_t tmp;

	tmp = *src;
	if (*src < 0)
		dest->num[BN_INTSIZE-1] = 0x80;

	for (i = 0; i < int32_size; i++)
		dest->num[i] = (tmp >> (i*8)) & 0xFF;
}

void
BN_buf_to_int(int32_t *dest, BN_INT *src)
{
	uint8_t i;
	int32_t ret;

	ret = 0;
	for (i = int32_size-1; i != 0;) {
		i--;

		ret += ((int32_t)src->num[i]) << (i*8);
	}

	*dest = ret;
}


#endif /*NNEG*/


/*
#ifndef NDEBUG
#include <stdio.h>

int
main(void)
{
	uint32_t tmp;
	BN_INT *dest, *src, *garbage;*/
/*	BN_INT dest, src, garbage;*/

/*
	if ( (src = calloc(sizeof(BN_INT), 1)) == NULL ) {
		fprintf(stderr, "Failed to allocate memory for src.\n");
		return 1;
	}
	if ( (dest = calloc(sizeof(BN_INT), 1)) == NULL ) {
		fprintf(stderr, "Failed to allocate memory for dest.\n");
		return 1;
	}
	if ( (garbage = calloc(sizeof(BN_INT), 1)) == NULL ) {
		fprintf(stderr, "Failed to allocate memory for garbage.\n");
		return 1;
	}


	tmp = 100;
	uint32_t_to_buf(src, &tmp);
	tmp = 200;
	uint32_t_to_buf(dest, &tmp);


	BN_buf_to_uint(&tmp, src);
	printf("src:  %u\n", tmp);
	BN_buf_to_uint(&tmp, dest);
	printf("dest: %u\n", tmp);


	BN_add(dest, src);
	BN_buf_to_uint(&tmp, dest);
	printf("After addition of dest and src: %u\n", tmp);

	BN_sub(dest, src);
	BN_buf_to_uint(&tmp, dest);
	printf("After subtraction of dest and src: %u\n", tmp);

	BN_mul(dest, src);
	BN_buf_to_uint(&tmp, dest);
	printf("After multiplication of dest and src: %d\n", tmp);

	BN_left_shift(dest, 17);
	BN_buf_to_uint(&tmp, dest);
	printf("dest after left shift: %u\n", tmp);

	BN_div_n_mod(dest, src, garbage);
	BN_buf_to_uint(&tmp, dest);
	printf("After division of dest and src: %u\n", tmp);
	BN_buf_to_uint(&tmp, garbage);
	printf("Remainder: %u\n", tmp);

 	free(src);
 	free(dest);
 	free(garbage);

	return 0;
}
#endif
*/
