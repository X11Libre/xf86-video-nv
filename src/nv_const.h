#ifndef __NV_CONST_H__
#define __NV_CONST_H__

#define NV_VERSION 4000
#define NV_VERSION_MAJOR PACKAGE_VERSION_MAJOR
#define NV_VERSION_MINOR PACKAGE_VERSION_MINOR
#define NV_VERSION_PATCH PACKAGE_VERSION_PATCHLEVEL
#define NV_VERSION_SUFFIX PACKAGE_VERSION_SUFFIX
#define NV_NAME "NV"
#define NV_DRIVER_NAME "nv"

/* libpciaccess gratuitous API changes */
 #define MEMBASE(p,n)     (p)->regions[n].base_addr
 #define VENDOR_ID(p)     (p)->vendor_id
 #define DEVICE_ID(p)     (p)->device_id
 #define SUBVENDOR_ID(p)  (p)->subvendor_id
 #define SUBDEVICE_ID(p)  (p)->subdevice_id
 #define CHIP_REVISION(p) (p)->revision

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#endif /* __NV_CONST_H__ */
