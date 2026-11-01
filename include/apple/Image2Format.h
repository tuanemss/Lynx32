/*
 * Image2 on-media format.
 *
 * The I2 layout is described by the I2 superblock, which is located at
 * kI2SuperblockOffset from the top of the media.  The superblock is a write-once
 * item; it should only be updated when the bootblock code is rewritten, implying
 * a complete re-initialisation of the media.
 *
 * Following the superblock there is a reserved area for boot code, after which
 * image headers describe individual images.
 *
 *                   0 --------------------
 *
 * kI2SuperblockOffset   Image2Superblock
 *
 *      isImageGranule --------------------
 *                       bootcode
 *   + isBootBlockSize --------------------
 *                       ...
 *   + isImageOffset     Image2Header
 *                       ...
 *
 */

#if !defined (__LIB_IMAGE2FORMAT_H)
#define  __LIB_IMAGE2FORMAT_H 1
#include <sys/types.h>

__BEGIN_DECLS

/*
 * XXX 7561698 isImageOffset is documented inconsistently
 * and not used as described above.
 */
typedef struct {
	u_int32_t	isMagic;
#define kImage2SuperMagic		0x494d4732	/* "IMG2" */
	u_int32_t	isImageGranule;			/* fundamental block size (bytes) */
	u_int32_t	isImageOffset;			/* image header offset within granule (image granules) */
	u_int32_t	isBootBlockSize;		/* size of the bootblock (image granules) */
	u_int32_t	isImageAvail;			/* total granules available for images. */
	u_int32_t	isNvramGranule;			/* size of NVRAM blocks (bytes) */
	u_int32_t	isNvramOffset;			/* offset to first NVRAM block (nvram granules) */
	u_int32_t	isFlags;			/* flags field reserved for future use */
	u_int32_t	isRsvd1;			/* reserved 1 for future use */
	u_int32_t	isRsvd2;			/* reserved 2 for future use */
	u_int32_t	isRsvd3;			/* reserved 3 for future use */
	u_int32_t	isRsvd4;			/* reserved 4 for future use */
	u_int32_t	isCheck;			/* CRC-32 of header fields preceding this one */
} Image2Superblock;


/*
 * Image types
 */
typedef u_int32_t	Image2ImageType;
#define kImage2ImageTypeWildcard	0x2a2a2a2a	/* **** */
#define kImage2ImageTypeIdent		0x49646e74	/* Idnt - ASCII string identifying the image
							 * collection */
#define kImage2ImageTypeDTree		0x64747265	/* dtre */
#define kImage2ImageTypeDiag		0x64696167	/* diag */
#define kImage2ImageTypeLogo		0x6c6f676f	/* logo */
#define kImage2ImageTypeiBoot		0x69626f74	/* "ibot" => iBoot */
#define kImage2ImageTypeLLB		0x6c6c627a	/* "llbz" => LLB */
#define kImage2ImageTypeEOT		0x04040404	/* EOT */

/*
 * On-media image header.
 */
typedef struct {
	u_int32_t	ihMagic;
#define kImage2ImageMagic		0x496d6732	/* "Img2" */
	Image2ImageType	ihType;
	u_int16_t	ihRevision;
	u_int16_t	ihSecurityEpoch;		/* Image is from this security epoch */
	u_int32_t	ihLoadAddress;			/* preferred load address */
	u_int32_t	ihDataSize;			/* payload data size (bytes) */
	u_int32_t	ihEncryptedDataSize;   		/* payload data size after decryption (bytes) */
	u_int32_t	ihAllocationSize;		/* allocation size including header (granules) */
	u_int32_t	ihOptions;
#define kImage2OptionSignatureTypeExternal	(1<<0)	/* signature present elsewhere */
#define kImage2OptionSignatureTypeInternalSHA1	(1<<1)	/* SHA-1 hash in ihSignatureData */
#define kImage2OptionSignatureTypeInternalCRC	(1<<2)	/* CRC-32 in first word of ihSignatureData*/
#define kImage2OptionsTrustedImage		(1<<8)	/* Image was written down with trust */
#define kImage2OptionsEncryptedImage		(1<<9)	/* Image body is encrypted */
#define kImage2OptionsInstalledWithSB		(1<<24) /* Image was written with secure boot support */
#define kImage2OptionsExtensionPresent		(1<<30)	/* Extension header follows this main */
#define kImage2OptionsImmutable			(1<<31)
	u_int32_t	ihSignatureData[16];
	u_int32_t	ihNextSize;			/* Size in bytes of the next extension */
#define kImage2HeaderExtensionMaxSize	128
	u_int32_t	ihCheck;			/* CRC-32 of the preceeding fields */
} Image2Header;
#define kImage2HeaderReservation	1024

typedef u_int32_t	Image2HeaderExtensionType;
#define kImage2HeaderExtensionVersionString	0x76657273	/* "vers" => version string */

typedef struct {
	u_int32_t			iheCheck;	/* CRC-32 of the preceeding fields */
	u_int32_t			iheNextSize;	/* Size in bytes of te next extension */
	Image2HeaderExtensionType	iheType;
	u_int32_t			ihOptions;
/* Extension prese*/
	u_int32_t 			iheData;	/* extension data. This field is not
							   written, instead the actual data is. */
} Image2HeaderExtension;

__END_DECLS

#endif /* __LIB_IMAGE2FORMAT_H*/
