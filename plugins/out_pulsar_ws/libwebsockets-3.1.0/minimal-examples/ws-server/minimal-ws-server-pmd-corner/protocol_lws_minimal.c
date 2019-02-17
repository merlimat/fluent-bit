/*
 * ws protocol handler plugin for "lws-minimal"
 *
 * Copyright (C) 2010-2018 Andy Green <andy@warmcat.com>
 *
 * This file is made available under the Creative Commons CC0 1.0
 * Universal Public Domain Dedication.
 *
 * This version holds a single message at a time, which may be lost if a new
 * message comes.  See the minimal-ws-server-ring sample for the same thing
 * but using an lws_ring ringbuffer to hold up to 8 messages at a time.
 */

#if !defined (LWS_PLUGIN_STATIC)
#define LWS_DLL
#define LWS_INTERNAL
#include <libwebsockets.h>
#endif

#include <string.h>

/*
 * This came from...
 *
 * cat /dev/urandom | hexdump -C -n 1024 | tr -s ' ' | cut -d' ' -f 2-17 | head -n-1 | sed "s/\ /, 0x/g" | sed "s/^/0x/g" | sed "s/\$/,/g"
 *
 * ...then the length tuned by hand to get the ciphertext sizes that we want to
 * confirm are OK.
 *
 * We can only pass in a maximum of one compression buffer of input at a time,
 * which is 1024 by default.
 */

unsigned char uncompressible[] = {
	0xfe, 0xcc, 0x47, 0xcb, 0x10, 0xf4, 0x3c, 0x85,
	0x8e, 0xd4, 0xe2, 0xf6, 0xd1, 0xd1, 0xdb, 0x64,
	0x94, 0x50, 0xf6, 0x14, 0x25, 0x03, 0x09, 0x3a,
	0xb1, 0x47, 0x86, 0xa8, 0x3c, 0x4f, 0x3b, 0x98,
	0x7b, 0x3e, 0x67, 0x3e, 0x22, 0xc5, 0x4c, 0x45,
	0xf4, 0xf7, 0xb5, 0x79, 0xc0, 0x26, 0x6e, 0x5c,
	0xf4, 0x10, 0x04, 0xa9, 0x3c, 0x4f, 0xed, 0xc5,
	0x3d, 0xd4, 0x9f, 0x9f, 0xa3, 0xdb, 0x29, 0xeb,
	0x1e, 0xe1, 0x52, 0xab, 0xb5, 0x75, 0x25, 0x86,
	0x86, 0x02, 0x2c, 0x9d, 0x9c, 0x86, 0x46, 0x92,
	0xe9, 0x04, 0xd8, 0x2c, 0x7d, 0x8a, 0x56, 0xe1,
	0xe1, 0xb6, 0x84, 0x4d, 0x17, 0x30, 0x01, 0x60,
	0xa6, 0xf4, 0xba, 0xc9, 0x5a, 0x29, 0xe3, 0x05,
	0xe1, 0xb4, 0x0b, 0x23, 0x74, 0x93, 0x25, 0x76,
	0xce, 0x15, 0xe4, 0x82, 0x9f, 0xbf, 0xe8, 0x6a,
	0x4a, 0xc5, 0xc2, 0x22, 0x91, 0x80, 0xb5, 0xd7,
	0xb3, 0xce, 0x70, 0x0e, 0xf7, 0xbb, 0x2f, 0xc5,
	0x83, 0x39, 0x86, 0xe5, 0x3e, 0xb7, 0x83, 0x87,
	0xc2, 0xeb, 0xc8, 0xed, 0x59, 0x26, 0xc1, 0xe6,
	0x80, 0x17, 0x3c, 0x29, 0x53, 0x4c, 0x1c, 0x3f,
	0x54, 0xbe, 0x34, 0x26, 0x72, 0xed, 0x38, 0x10,
	0xd1, 0x37, 0x07, 0x2d, 0x12, 0x31, 0x9b, 0xc5,
	0x92, 0x09, 0x13, 0x5d, 0x8e, 0xef, 0xdb, 0x52,
	0x7f, 0x7d, 0x6f, 0x62, 0x1e, 0x17, 0xd2, 0xf9,
	0x72, 0x74, 0xc7, 0xd6, 0x1f, 0x8b, 0x9c, 0x4c,
	0x26, 0xd2, 0x6f, 0x7c, 0x33, 0x06, 0xee, 0xc2,
	0xa3, 0x41, 0x43, 0x4f, 0x40, 0x2a, 0x9c, 0xb3,
	0x4a, 0xb1, 0x88, 0x4e, 0x6f, 0xf2, 0xb7, 0x38,
	0xde, 0x87, 0x0d, 0xdc, 0x15, 0x6a, 0x36, 0x6b,
	0xf3, 0x6c, 0x61, 0xf5, 0x24, 0x8e, 0xb6, 0xcc,
	0x8a, 0x3a, 0xa0, 0xb4, 0x9b, 0xae, 0x85, 0x87,
	0x75, 0xf5, 0xbd, 0x50, 0x1f, 0xb5, 0x0c, 0xdb,
	0x6c, 0x68, 0x59, 0xef, 0x37, 0x5a, 0x2a, 0x85,
	0xf0, 0xce, 0x4d, 0x58, 0xa1, 0xa5, 0xde, 0x73,
	0x9b, 0x1a, 0x3d, 0x8a, 0x00, 0xba, 0x2f, 0xe2,
	0xda, 0xad, 0x3c, 0x63, 0x8a, 0x33, 0x39, 0xc4,
	0x07, 0x29, 0x1d, 0xa7, 0x40, 0x3b, 0xa4, 0xa6,
	0xae, 0xee, 0x37, 0x08, 0x83, 0xd1, 0x72, 0x66,
	0x3d, 0x43, 0xe3, 0x7a, 0x48, 0xfc, 0xf8, 0xd4,
	0xe3, 0xab, 0xd0, 0xe9, 0xb1, 0xf4, 0x4d, 0x3c,
	0x6b, 0x58, 0xde, 0x3c, 0x91, 0x0d, 0x3e, 0xec,
	0x35, 0x6d, 0x53, 0xe6, 0xb6, 0x4b, 0xc0, 0x80,
	0x18, 0xab, 0x96, 0x7f, 0x05, 0xd7, 0xd4, 0x81,
	0x0f, 0x92, 0x2b, 0xaf, 0x72, 0x59, 0xc2, 0x14,
	0xca, 0x62, 0x82, 0xac, 0xe3, 0x17, 0x43, 0x61,
	0x4d, 0x1e, 0xfc, 0x72, 0xaf, 0xfc, 0x55, 0x2a,
	0x2b, 0xb6, 0x8e, 0x6e, 0xe6, 0x86, 0xeb, 0xcc,
	0x26, 0x6c, 0xdf, 0xac, 0x02, 0x58, 0xa1, 0x5d,
	0x1b, 0x07, 0xe2, 0x5d, 0x50, 0xb9, 0xbf, 0x2e,
	0x1f, 0x49, 0x39, 0xe6, 0x7f, 0x2f, 0x0e, 0x9d,
	0x09, 0x42, 0xc7, 0xa1, 0xcc, 0xeb, 0x5b, 0x06,
	0x1c, 0x11, 0x9f, 0xea, 0xc1, 0x96, 0x82, 0xa9,
	0x30, 0x6a, 0xda, 0x98, 0x87, 0x43, 0xfd, 0x25,
	0xe7, 0x27, 0x53, 0x9a, 0xb3, 0x2f, 0x19, 0xa9,
	0x1a, 0xf4, 0xd6, 0xf3, 0x9e, 0xba, 0x9a, 0x91,
	0x52, 0x8f, 0x20, 0x6b, 0x4c, 0x3a, 0x2a, 0x3d,
	0xa0, 0xff, 0x8d, 0x61, 0x04, 0xee, 0x26, 0x55,
	0xdd, 0xd7, 0x67, 0xe4, 0x84, 0x0d, 0xf1, 0x5d,
	0xc7, 0xeb, 0xb3, 0x8c, 0x67, 0xa2, 0xc8, 0x1f,
	0x53, 0x02, 0xc4, 0x8c, 0x89, 0xd5, 0x51, 0xc8,
	0x8b, 0xb7, 0xc8, 0x11, 0xbe, 0x0e, 0xc2, 0xb1,
	0x00, 0x35, 0x81, 0x96, 0xac, 0x90, 0x9c, 0xbc,
	0x09, 0x82, 0x75, 0xc3, 0xe7, 0x66, 0x4e, 0x68,
	0xdc, 0xa1, 0xf0, 0xd0, 0x2d, 0x49, 0x3b, 0x47,
	0xba, 0x19, 0xc8, 0x9b, 0x90, 0x12, 0xc0, 0xdf,
	0xda, 0x32, 0x0f, 0x79, 0x6d, 0x1a, 0x5f, 0x92,
	0x51, 0x70, 0xfc, 0xca, 0x08, 0xd4, 0x7f, 0x1a,
	0x56, 0x04, 0x99, 0x33, 0x89, 0x3d, 0x6f, 0x89,
	0x10, 0x25, 0x81, 0xe2, 0xbd, 0x06, 0xd6, 0xaa,
	0x02, 0x8e, 0x4c, 0xa3, 0x60, 0xfd, 0xaf, 0x9c,
	0x81, 0x75, 0xaf, 0x2f, 0xe1, 0x72, 0xe0, 0x6e,
	0x15, 0xdd, 0xbb, 0x92, 0xd1, 0xbe, 0x8e, 0x9b,
	0xfb, 0x82, 0xb9, 0x47, 0x6f, 0x02, 0x28, 0x2a,
	0x67, 0x50, 0xed, 0x24, 0x9b, 0x4d, 0x69, 0xd7,
	0xa9, 0x66, 0x3e, 0x14, 0x4b, 0x00, 0x2a, 0xe4,
	0x3d, 0x63, 0xb2, 0x10, 0xd4, 0x05, 0x9d, 0xe3,
	0xde, 0xce, 0xd8, 0x04, 0x41, 0x03, 0xb5, 0xda,
	0xb0, 0x6f, 0xca, 0x63, 0x64, 0x04, 0xff, 0x07,
	0x58, 0x5f, 0x96, 0xf7, 0x6c, 0xb7, 0x67, 0x05,
	0xd6, 0x85, 0xf2, 0x1e, 0xc1, 0xdc, 0x76, 0x12,
	0x50, 0x83, 0x78, 0xa2, 0x51, 0x94, 0xe1, 0x2e,
	0xb8, 0x97, 0x5b, 0x08, 0x81, 0xac, 0x59, 0x43,
	0xe9, 0x01, 0x09, 0xa2, 0xed, 0x10, 0x4f, 0xb1,
	0x5b, 0xb8, 0x67, 0xe8, 0x61, 0x8d, 0xc8, 0xd9,
	0xc3, 0x5f, 0x65, 0xd7, 0xaa, 0x30, 0x0e, 0xc9,
	0x43, 0x98, 0x1d, 0xf1, 0xa5, 0x28, 0xd5, 0xa1,
	0x6b, 0x8f, 0x89, 0x76, 0x97, 0xa1, 0x3e, 0x6f,
	0x39, 0xf4, 0xb9, 0x6b, 0xa7, 0xfe, 0x58, 0x24,
	0xcd, 0x75, 0xa8, 0xec, 0x9e, 0x1c, 0x8e, 0x02,
	0x2a, 0xce, 0xe9, 0x0a, 0x24, 0x31, 0x89, 0x5a,
	0xd5, 0xdd, 0x70, 0x8e, 0x5f, 0xee, 0xc1, 0x34,
	0xf8, 0xe2, 0x8a, 0xca, 0xf1, 0xf2, 0x71, 0x4c,
	0x31, 0x56, 0xeb, 0x03, 0xf9, 0x6c, 0x0d, 0xa9,
	0x65, 0x6e, 0x88, 0x4f, 0x8e, 0x80, 0x69, 0xd7,
	0xd4, 0x63, 0x45, 0x9c, 0xab, 0x8c, 0x3d, 0x08,
	0x8b, 0xd9, 0x97, 0xdc, 0x88, 0x59, 0x19, 0x2d,
	0xb2, 0x84, 0xf4, 0x78, 0x3e, 0xce, 0x80, 0xba,
	0xeb, 0x34, 0x5a, 0x9e, 0x8e, 0x98, 0xc4, 0x45,
	0x9d, 0x59, 0xb2, 0x7e, 0xc1, 0x7e, 0x5b, 0x89,
	0xd0, 0x02, 0xcb, 0xa4, 0xf1, 0xf2, 0xa7, 0x3a,
	0x05, 0xc3, 0x7d, 0x43, 0x64, 0x7f, 0xf0, 0xc1,
	0xf8, 0x71, 0x3b, 0x38, 0x39, 0xc7, 0x1b, 0xf4,
	0x2f, 0x5a, 0x5c, 0x43, 0x1b, 0xe3, 0x93, 0xe8,
	0x79, 0xe8, 0x35, 0x63, 0x34, 0x7e, 0x25, 0x41,
	0x6f, 0x08, 0xce, 0x6f, 0x95, 0x2a, 0xc2, 0xdc,
	0x65, 0xe2, 0xa5, 0xc0, 0xfd, 0xf1, 0x78, 0x32,
	0x23, 0x09, 0x75, 0x99, 0x12, 0x7a, 0x83, 0xfd,
	0xae, 0x1e, 0xb2, 0xe9, 0x12, 0x5c, 0x3d, 0x03,
	0x68, 0x12, 0x1e, 0xe3, 0x8f, 0xff, 0x47, 0xe3,
	0xb4, 0x7e, 0x9b, 0x7e, 0x60, 0x2e, 0xf4, 0x06,
	0xba, 0x10, 0x08, 0x6b, 0xf9, 0x25, 0x59, 0xf3,
	0x61, 0x13, 0x2b, 0xd1, 0x2f, 0x04, 0x5f, 0xd6,
	0xd3, 0x42, 0xf6, 0x21, 0x57, 0xf6, 0xd3, 0xb3,
	0xec, 0xec, 0x07, 0x33, 0xbf, 0x69, 0x04, 0xec,
	0x88, 0x8d, 0x06, 0x2b, 0xfa, 0xee, 0xb2, 0x7b,
	0x41, 0x2a, 0x49, 0x0f, 0x30, 0x52, 0x41, 0x29,
	0x70, 0xd0, 0xf6, 0xb6, 0xbf, 0x27, 0x1a, 0x56,
	0x9a, 0x4b, 0x2a, 0x67, 0xfb, 0xc8, 0x16, 0x46,
	0x59, 0xc7, 0xf5, 0x5f, 0x20, 0x10, 0x25, 0x6c,
	0x1e, 0x36, 0x20, 0x0c, 0x3e, 0x7e, 0x15, 0x6c,
	0xa2, 0xbd, 0x22, 0xc4, 0x3d, 0xc9, 0x74, 0x56,
	0xab, 0x31, 0x92, 0xb8, 0x9f, 0xa1, 0x05, 0x2e,
	0xc4, 0xdb, 0x32, 0x91, 0xcb, 0x0f, 0x4a, 0x73,
	0x7f, 0xe1, 0xe6, 0x65, 0x2e, 0x5e, 0xa6, 0xaf,
	0xae, 0xa9, 0x04, 0x14, 0x83, 0xef, 0x19, 0x70,
	0x5e, 0xcb, 0xf5, 0x87, 0xcc, 0x45, 0xf7, 0x60,
	0xd7, 0x9d, 0x1e, 0x2e, /* 1012 */

	/* up to here, this generates a 1022-byte single packet of compressed
	 * data that is well-formed and produces 1012 bytes of plaintext.
	 *
	 * The compressed packet ends
	 *
	 *  03F0: 70 5E CB F5 87 CC 45 F7 60 D7 9D 1E 2E 00
	 */

	0x54, /* 1013 */

	/* up to here, this generates a 1023-byte single packet of compressed
	 * data that is well-formed and produces 1013 bytes of plaintext.
	 *
	 * The compressed packet ends
	 *
	 *  03F0: 70 5E CB F5 87 CC 45 F7 60 D7 9D 1E 2E 54 00
	 */

	0x83, /* 1014 */

	/* up to here, a 1023-byte + 3-byte (1 byte payload) packet
	 * of uncompressed length 1014 */

	0x09, 0x99, 0xf9, 0x71, 0x9f, 0x15, 0x49, 0xda, 0xa8, 0x99, /* 1024 */

	/* up to here, a 1023-byte (1020 payload) + 3-byte (1 payload) packet
	 * of uncompressed length 1019 */

	0xf5, 0xe6, 0xa1, 0x71, 0x64, 0x9a, 0x95, 0xed,


};

/* generates ciphertext:        1022  1023  1023 + 3 1023 + 3 */
static int corner_lengths[] = {
/* bytes plaintext,	ciphertext */
	1012,	/*	1019 */
	1013,	/*	1020 */
	1014,	/*	1021 */
	1019,	/*	1021 */
	1024,	/*	1021*/
};


/* one of these is created for each client connecting to us */

struct per_session_data__minimal {
	int which;
	int last; /* 0 no test, else test number in corner_lengths[] + 1 */
};

static int
callback_minimal(struct lws *wsi, enum lws_callback_reasons reason,
			void *user, void *in, size_t len)
{
	struct per_session_data__minimal *pss =
			(struct per_session_data__minimal *)user;
	unsigned char buf[LWS_PRE + 2048];
	int m;

	switch (reason) {
	case LWS_CALLBACK_ESTABLISHED:
		if (lws_hdr_copy(wsi, (char *)buf, sizeof(buf),
				 WSI_TOKEN_GET_URI) < 0)
			return -1;

		pss->last = atoi((char *)buf + 1);

		if (pss->last > (int)LWS_ARRAY_SIZE(corner_lengths))
			pss->last = 0;
		lws_callback_on_writable(wsi);
		break;

	case LWS_CALLBACK_SERVER_WRITEABLE:
		if (!pss->last)
			break;

		lwsl_err("%s: writable %d, %d\n", __func__, pss->last,
				corner_lengths[pss->last - 1]);

		memcpy(buf + LWS_PRE, uncompressible,
		       corner_lengths[pss->last - 1]);

		/* notice we allowed for LWS_PRE in the payload already */
		m = lws_write(wsi, buf + LWS_PRE, corner_lengths[pss->last - 1],
				LWS_WRITE_BINARY);
		if (m < corner_lengths[pss->last - 1]) {
			lwsl_err("ERROR %d writing to ws socket\n", m);
			return -1;
		}

		pss->last = 0;
		break;

	default:
		break;
	}

	return 0;
}

#define LWS_PLUGIN_PROTOCOL_MINIMAL \
	{ \
		"lws-minimal", \
		callback_minimal, \
		sizeof(struct per_session_data__minimal), \
		2048, \
		0, NULL, 0 \
	}

#if !defined (LWS_PLUGIN_STATIC)

/* boilerplate needed if we are built as a dynamic plugin */

static const struct lws_protocols protocols[] = {
	LWS_PLUGIN_PROTOCOL_MINIMAL
};

LWS_EXTERN LWS_VISIBLE int
init_protocol_minimal(struct lws_context *context,
		      struct lws_plugin_capability *c)
{
	if (c->api_magic != LWS_PLUGIN_API_MAGIC) {
		lwsl_err("Plugin API %d, library API %d", LWS_PLUGIN_API_MAGIC,
			 c->api_magic);
		return 1;
	}

	c->protocols = protocols;
	c->count_protocols = LWS_ARRAY_SIZE(protocols);
	c->extensions = NULL;
	c->count_extensions = 0;

	return 0;
}

LWS_EXTERN LWS_VISIBLE int
destroy_protocol_minimal(struct lws_context *context)
{
	return 0;
}
#endif
