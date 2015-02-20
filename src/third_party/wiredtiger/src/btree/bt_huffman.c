/*-
 * Copyright (c) 2014-2015 MongoDB, Inc.
 * Copyright (c) 2008-2014 WiredTiger, Inc.
 *	All rights reserved.
 *
 * See the file LICENSE for redistribution information.
 */

#include "wt_internal.h"

/*
 * 7-bit ASCII, with English language frequencies.
 *
 * Based on "Case-sensitive letter and bigram frequency counts from large-scale
 * English corpora"
 *	Michael N. Jones and D.J.K. Mewhort
 *	Queen's University, Kingston, Ontario, Canada
 * Behavior Research Methods, Instruments, & Computers 2004, 36 (3), 388-396
 *
 * Additionally supports space and tab characters; space is the most common
 * character in text where it occurs, and tab appears about as frequently as
 * 'a' and 'n' in text where it occurs.
 */
struct __wt_huffman_table {
	uint32_t symbol;
	uint32_t frequency;
};
static const struct __wt_huffman_table __wt_huffman_nytenglish[] = {
	/* nul */	{ 0x00,       0 },	/* For an escape character. */
	/*  ht */	{ 0x09, 5263779 },
	/*  sp */	{ 0x20, 8000000 },
	/*  !  */	{ 0x21,    2178 },
	/*  "  */	{ 0x22,  284671 },
	/*  #  */	{ 0x23,      10 },
	/*  $  */	{ 0x24,   51572 },
	/*  %  */	{ 0x25,    1993 },
	/*  &  */	{ 0x26,    6523 },
	/*  '  */	{ 0x27,  204497 },
	/*  (  */	{ 0x28,   53398 },
	/*  )  */	{ 0x29,   53735 },
	/*  *  */	{ 0x2a,   20716 },
	/*  +  */	{ 0x2b,     309 },
	/*  ,  */	{ 0x2c,  984969 },
	/*  -  */	{ 0x2d,  252302 },
	/*  .  */	{ 0x2e,  946136 },
	/*  /  */	{ 0x2f,    8161 },
	/*  0  */	{ 0x30,  546233 },
	/*  1  */	{ 0x31,  460946 },
	/*  2  */	{ 0x32,  333499 },
	/*  3  */	{ 0x33,  187606 },
	/*  4  */	{ 0x34,  192528 },
	/*  5  */	{ 0x35,  374413 },
	/*  6  */	{ 0x36,  153865 },
	/*  7  */	{ 0x37,  120094 },
	/*  8  */	{ 0x38,  182627 },
	/*  9  */	{ 0x39,  282364 },
	/*  :  */	{ 0x3a,   54036 },
	/*  ;  */	{ 0x3b,   36727 },
	/*  <  */	{ 0x3c,      82 },
	/*  =  */	{ 0x3d,      22 },
	/*  >  */	{ 0x3e,      83 },
	/*  ?  */	{ 0x3f,   12357 },
	/*  @  */	{ 0x40,       1 },
	/*  A  */	{ 0x41,  280937 },
	/*  B  */	{ 0x42,  169474 },
	/*  C  */	{ 0x43,  229363 },
	/*  D  */	{ 0x44,  129632 },
	/*  E  */	{ 0x45,  138443 },
	/*  F  */	{ 0x46,  100751 },
	/*  G  */	{ 0x47,   93212 },
	/*  H  */	{ 0x48,  123632 },
	/*  I  */	{ 0x49,  223312 },
	/*  J  */	{ 0x4a,   78706 },
	/*  K  */	{ 0x4b,   46580 },
	/*  L  */	{ 0x4c,  106984 },
	/*  M  */	{ 0x4d,  259474 },
	/*  N  */	{ 0x4e,  205409 },
	/*  O  */	{ 0x4f,  105700 },
	/*  P  */	{ 0x50,  144239 },
	/*  Q  */	{ 0x51,   11659 },
	/*  R  */	{ 0x52,  146448 },
	/*  S  */	{ 0x53,  304971 },
	/*  T  */	{ 0x54,  325462 },
	/*  U  */	{ 0x55,   57488 },
	/*  V  */	{ 0x56,   31053 },
	/*  W  */	{ 0x57,  107195 },
	/*  X  */	{ 0x58,    7578 },
	/*  Y  */	{ 0x59,   94297 },
	/*  Z  */	{ 0x5a,    5610 },
	/*  [  */	{ 0x5b,       1 },
	/*  \  */	{ 0x5c,       1 },
	/*  ]  */	{ 0x5d,       1 },
	/*  ^  */	{ 0x5e,       1 },
	/*  _  */	{ 0x5f,       1 },
	/*  `  */	{ 0x60,       1 },
	/*  a  */	{ 0x61, 5263779 },
	/*  b  */	{ 0x62,  866156 },
	/*  c  */	{ 0x63, 1960412 },
	/*  d  */	{ 0x64, 2369820 },
	/*  e  */	{ 0x65, 7741842 },
	/*  f  */	{ 0x66, 1296925 },
	/*  g  */	{ 0x67, 1206747 },
	/*  h  */	{ 0x68, 2955858 },
	/*  i  */	{ 0x69, 4527332 },
	/*  j  */	{ 0x6a,   65856 },
	/*  k  */	{ 0x6b,  460788 },
	/*  l  */	{ 0x6c, 2553152 },
	/*  m  */	{ 0x6d, 1467376 },
	/*  n  */	{ 0x6e, 4535545 },
	/*  o  */	{ 0x6f, 4729266 },
	/*  p  */	{ 0x70, 1255579 },
	/*  q  */	{ 0x71,   54221 },
	/*  r  */	{ 0x72, 4137949 },
	/*  s  */	{ 0x73, 4186210 },
	/*  t  */	{ 0x74, 5507692 },
	/*  u  */	{ 0x75, 1613323 },
	/*  v  */	{ 0x76,  653370 },
	/*  w  */	{ 0x77, 1015656 },
	/*  x  */	{ 0x78,  123577 },
	/*  y  */	{ 0x79, 1062040 },
	/*  z  */	{ 0x7a,   66423 },
	/*  {  */	{ 0x7b,       1 },
	/*  |  */	{ 0x7c,       1 },
	/*  }  */	{ 0x7d,       1 },
	/*  ~  */	{ 0x7e,       1 }
};

static int __wt_huffman_read(WT_SESSION_IMPL *,
    WT_CONFIG_ITEM *, struct __wt_huffman_table **, u_int *, u_int *);

#define	WT_HUFFMAN_CONFIG_VALID(str, len)				\
	(WT_STRING_CASE_MATCH("english", (str), (len)) ||		\
	    WT_PREFIX_MATCH((str), "utf8") || WT_PREFIX_MATCH((str), "utf16"))

/*
 * __btree_huffman_config --
 *	Verify the key or value strings passed in.
 */
static int
__btree_huffman_config(WT_SESSION_IMPL *session,
    WT_CONFIG_ITEM *key_conf, WT_CONFIG_ITEM *value_conf)
{
	if (key_conf->len != 0 &&
	    !WT_HUFFMAN_CONFIG_VALID(key_conf->str, key_conf->len))
		WT_RET_MSG(
		    session, EINVAL, "illegal Huffman key configuration");
	if (value_conf->len != 0 &&
	    !WT_HUFFMAN_CONFIG_VALID(value_conf->str, value_conf->len))
		WT_RET_MSG(
		    session, EINVAL, "illegal Huffman value configuration");
	return (0);

}

/*
 * __wt_btree_huffman_open --
 *	Configure Huffman encoding for the tree.
 */
int
__wt_btree_huffman_open(WT_SESSION_IMPL *session)
{
	struct __wt_huffman_table *table;
	WT_BTREE *btree;
	WT_CONFIG_ITEM key_conf, value_conf;
	WT_DECL_RET;
	const char **cfg;
	u_int entries, numbytes;

	btree = S2BT(session);
	cfg = btree->dhandle->cfg;

	WT_RET(__wt_config_gets_none(session, cfg, "huffman_key", &key_conf));
	WT_RET(
	    __wt_config_gets_none(session, cfg, "huffman_value", &value_conf));
	if (key_conf.len == 0 && value_conf.len == 0)
		return (0);
	WT_RET(__btree_huffman_config(session, &key_conf, &value_conf));

	switch (btree->type) {		/* Check file type compatibility. */
	case BTREE_COL_FIX:
		WT_RET_MSG(session, EINVAL,
		    "fixed-size column-store files may not be Huffman encoded");
		/* NOTREACHED */
	case BTREE_COL_VAR:
		if (key_conf.len != 0)
			WT_RET_MSG(session, EINVAL,
			    "the keys of variable-length column-store files "
			    "may not be Huffman encoded");
		break;
	case BTREE_ROW:
		break;
	}

	if (key_conf.len == 0) {
		;
	} else if (strncasecmp(key_conf.str, "english", key_conf.len) == 0) {
		struct __wt_huffman_table
		    copy[WT_ELEMENTS(__wt_huffman_nytenglish)];

		memcpy(copy,
		    __wt_huffman_nytenglish, sizeof(__wt_huffman_nytenglish));
		WT_RET(__wt_huffman_open(
		    session, copy, WT_ELEMENTS(__wt_huffman_nytenglish),
		    1, &btree->huffman_key));

		/* Check for a shared key/value table. */
		if (value_conf.len != 0 && strncasecmp(
		    value_conf.str, "english", value_conf.len) == 0) {
			btree->huffman_value = btree->huffman_key;
			return (0);
		}
	} else {
		WT_RET(__wt_huffman_read(
		    session, &key_conf, &table, &entries, &numbytes));
		ret = __wt_huffman_open(
		    session, table, entries, numbytes, &btree->huffman_key);
		__wt_free(session, table);
		if (ret != 0)
			return (ret);

		/* Check for a shared key/value table. */
		if (value_conf.len != 0 && key_conf.len == value_conf.len &&
		    memcmp(key_conf.str, value_conf.str, key_conf.len) == 0) {
			btree->huffman_value = btree->huffman_key;
			return (0);
		}
	}

	if (value_conf.len == 0) {
		;
	} else if (
	    strncasecmp(value_conf.str, "english", value_conf.len) == 0) {
		struct __wt_huffman_table
		    copy[WT_ELEMENTS(__wt_huffman_nytenglish)];

		memcpy(copy,
		    __wt_huffman_nytenglish, sizeof(__wt_huffman_nytenglish));
		WT_RET(__wt_huffman_open(
		    session, copy, WT_ELEMENTS(__wt_huffman_nytenglish),
		    1, &btree->huffman_value));
	} else {
		WT_RET(__wt_huffman_read(
		    session, &value_conf, &table, &entries, &numbytes));
		ret = __wt_huffman_open(
		    session, table, entries, numbytes, &btree->huffman_value);
		__wt_free(session, table);
		if (ret != 0)
			return (ret);
	}

	return (0);
}

/*
 * __wt_huffman_read --
 *	Read a Huffman table from a file.
 */
static int
__wt_huffman_read(WT_SESSION_IMPL *session, WT_CONFIG_ITEM *ip,
    struct __wt_huffman_table **tablep, u_int *entriesp, u_int *numbytesp)
{
	struct __wt_huffman_table *table, *tp;
	FILE *fp;
	WT_DECL_RET;
	uint64_t symbol, frequency;
	u_int entries, lineno;
	char *file;

	*tablep = NULL;
	*entriesp = *numbytesp = 0;

	fp = NULL;
	file = NULL;
	table = NULL;

	/*
	 * UTF-8 table is 256 bytes, with a range of 0-255.
	 * UTF-16 is 128KB (2 * 65536) bytes, with a range of 0-65535.
	 */
	if (strncasecmp(ip->str, "utf8", 4) == 0) {
		entries = UINT8_MAX;
		*numbytesp = 1;
		WT_ERR(__wt_calloc_def(session, entries, &table));

		if (ip->len == 4)
			WT_ERR_MSG(session, EINVAL,
			    "no Huffman table file name specified");
		WT_ERR(__wt_calloc_def(session, ip->len, &file));
		memcpy(file, ip->str + 4, ip->len - 4);
	} else if (strncasecmp(ip->str, "utf16", 5) == 0) {
		entries = UINT16_MAX;
		*numbytesp = 2;
		WT_ERR(__wt_calloc_def(session, entries, &table));

		if (ip->len == 5)
			WT_ERR_MSG(session, EINVAL,
			    "no Huffman table file name specified");
		WT_ERR(__wt_calloc_def(session, ip->len, &file));
		memcpy(file, ip->str + 5, ip->len - 5);
	} else {
		WT_ERR_MSG(session, EINVAL,
		    "unknown Huffman configuration value %.*s",
		    (int)ip->len, ip->str);
	}

	if ((fp = fopen(file, "r")) == NULL)
		WT_ERR_MSG(session, __wt_errno(),
		    "unable to read Huffman table file %.*s",
		    (int)ip->len, ip->str);

	for (tp = table, lineno = 1; (ret =
	    fscanf(fp, "%" SCNu64 " %" SCNu64, &symbol, &frequency)) != EOF;
	    ++tp, ++lineno) {
		if (lineno > entries)
			WT_ERR_MSG(session, EINVAL,
			    "Huffman table file %.*s is corrupted, "
			    "more than %" PRIu32 " entries",
			    (int)ip->len, ip->str, entries);
		if (ret != 2)
			WT_ERR_MSG(session, EINVAL,
			    "line %u of Huffman table file %.*s is corrupted: "
			    "expected two unsigned integral values",
			    lineno, (int)ip->len, ip->str);
		if (symbol > entries)
			WT_ERR_MSG(session, EINVAL,
			    "line %u of Huffman table file %.*s is corrupted: "
			    "symbol larger than maximum value of %u",
			    lineno, (int)ip->len, ip->str, entries);
		if (frequency > UINT32_MAX)
			WT_ERR_MSG(session, EINVAL,
			    "line %u of Huffman table file %.*s is corrupted: "
			    "frequency larger than maximum value of %" PRIu32,
			    lineno, (int)ip->len, ip->str, UINT32_MAX);

		tp->symbol = (uint32_t)symbol;
		tp->frequency = (uint32_t)frequency;
	}

	if (ret == EOF)
		ret = 0;
	*entriesp = lineno - 1;
	*tablep = table;

	if (0) {
err:		__wt_free(session, table);
	}
	if (fp != NULL)
		(void)fclose(fp);
	__wt_free(session, file);
	return (ret);
}

/*
 * __wt_btree_huffman_close --
 *	Close the Huffman tables.
 */
void
__wt_btree_huffman_close(WT_SESSION_IMPL *session)
{
	WT_BTREE *btree;

	btree = S2BT(session);

	if (btree->huffman_key != NULL) {
		/* Key and data may use the same table, only close it once. */
		if (btree->huffman_value == btree->huffman_key)
			btree->huffman_value = NULL;

		__wt_huffman_close(session, btree->huffman_key);
		btree->huffman_key = NULL;
	}
	if (btree->huffman_value != NULL) {
		__wt_huffman_close(session, btree->huffman_value);
		btree->huffman_value = NULL;
	}
}